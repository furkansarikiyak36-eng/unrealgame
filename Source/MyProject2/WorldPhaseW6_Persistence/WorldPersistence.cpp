#include "WorldPersistence.h"
#include "WorldCore/ChunkStateStore.h"
#include "WorldCore/ChunkState.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Compression.h"
#include "HAL/FileManager.h"

namespace
{
    constexpr uint32 SaveMagic = 0x4D325753; // "SW2M" in little-endian stream
    constexpr int32 SaveVersion = 4;
    constexpr int32 MaxChunks = 1000000;
    constexpr int32 MaxCellsPerChunk = 65536;
    constexpr int32 MaxPayloadBytes = 1024 * 1024 * 1024;
    constexpr int32 MaxExtensionEntriesPerChunk = 65536;
    constexpr int32 MaxExtensionBytesPerCell = 4096;

    void SerializeName(FArchive& Ar, FName& Name)
    {
        FString Text = Ar.IsSaving() ? Name.ToString() : FString();
        Ar << Text;
        if (Ar.IsLoading()) Name = FName(*Text);
    }

    void SerializeCell(FArchive& Ar, FSerializedCell& C)
    {
        Ar << C.TerrainHeight << C.TerrainOriginalHeight << C.TerrainDeformation << C.TerrainPermeability;
        SerializeName(Ar, C.TerrainMaterialID);
        Ar << C.WaterVolume << C.WaterLevel << C.WaterFlowRate;
        Ar << C.WaterFlowDirection.X << C.WaterFlowDirection.Y << C.bWaterSource << C.bWaterFlowing;
        Ar << C.SoilMoisture << C.SoilN << C.SoilP << C.SoilK << C.SoilPH;
        Ar << C.Toxin << C.Radioactivity << C.ChemistryPH << C.Pollution;
        Ar << C.Biomass << C.BiomassHealth << C.BiomassMoisture << C.BiomassDecayRate;
        SerializeName(Ar, C.SpeciesID);
        Ar << C.bBurning;
    }

    void SerializeChunk(FArchive& Ar, FSerializedChunk& C, bool bHasExtensionData)
    {
        Ar << C.Coordinate.X << C.Coordinate.Y << C.CellsPerAxis;
        Ar << C.LastUpdateDay << C.LastWeatherUpdateDay << C.LastSimulationTime << C.bIsInitialized;
        uint8 Mode = (uint8)C.ActivationMode;
        Ar << Mode;
        if (Ar.IsLoading())
        {
            if (C.CellsPerAxis <= 0 || C.CellsPerAxis > 256 || Mode > (uint8)EChunkActivationMode::Continuous)
            {
                Ar.SetError();
                return;
            }
            C.ActivationMode = (EChunkActivationMode)Mode;
        }

        int32 CellCount = Ar.IsSaving() ? C.Cells.Num() : 0;
        Ar << CellCount;
        if (Ar.IsLoading())
        {
            if (CellCount < 0 || CellCount > MaxCellsPerChunk || CellCount > C.CellsPerAxis * C.CellsPerAxis)
            {
                Ar.SetError();
                return;
            }
            C.Cells.Empty();
        }

        if (Ar.IsSaving())
        {
            TArray<int32> Keys;
            C.Cells.GetKeys(Keys);
            Keys.Sort();
            for (int32 Key : Keys)
            {
                Ar << Key;
                FSerializedCell Cell = C.Cells.FindChecked(Key);
                SerializeCell(Ar, Cell);
            }
        }
        else
        {
            for (int32 Index = 0; Index < CellCount; ++Index)
            {
                int32 Key = INDEX_NONE;
                FSerializedCell Cell;
                Ar << Key;
                SerializeCell(Ar, Cell);
                if (Ar.IsError() || Key < 0 || Key >= C.CellsPerAxis * C.CellsPerAxis || C.Cells.Contains(Key))
                {
                    Ar.SetError();
                    return;
                }
                C.Cells.Add(Key, Cell);
            }
        }

        if (!bHasExtensionData) return;

        int32 ExtensionCount = Ar.IsSaving() ? C.ExtensionCellPayloads.Num() : 0;
        Ar << ExtensionCount;
        if (Ar.IsError() || ExtensionCount < 0 || ExtensionCount > MaxExtensionEntriesPerChunk)
        {
            Ar.SetError();
            return;
        }
        if (Ar.IsSaving())
        {
            TArray<int32> ExtensionKeys;
            C.ExtensionCellPayloads.GetKeys(ExtensionKeys);
            ExtensionKeys.Sort();
            for (int32 Key : ExtensionKeys)
            {
                const TArray<uint8>& Payload = C.ExtensionCellPayloads.FindChecked(Key);
                int32 PayloadSize = Payload.Num();
                if (Key < 0 || Key >= C.CellsPerAxis * C.CellsPerAxis || PayloadSize <= 0 || PayloadSize > MaxExtensionBytesPerCell)
                {
                    Ar.SetError();
                    return;
                }
                Ar << Key << PayloadSize;
                Ar.Serialize(const_cast<uint8*>(Payload.GetData()), PayloadSize);
                if (Ar.IsError()) return;
            }
        }
        else
        {
            C.ExtensionCellPayloads.Empty();
            for (int32 Index = 0; Index < ExtensionCount; ++Index)
            {
                int32 Key = INDEX_NONE;
                int32 PayloadSize = 0;
                Ar << Key << PayloadSize;
                if (Ar.IsError() || Key < 0 || Key >= C.CellsPerAxis * C.CellsPerAxis ||
                    C.ExtensionCellPayloads.Contains(Key) || PayloadSize <= 0 || PayloadSize > MaxExtensionBytesPerCell ||
                    PayloadSize > Ar.TotalSize() - Ar.Tell())
                {
                    Ar.SetError();
                    return;
                }
                TArray<uint8> Payload;
                Payload.SetNumUninitialized(PayloadSize);
                Ar.Serialize(Payload.GetData(), PayloadSize);
                if (Ar.IsError()) return;
                C.ExtensionCellPayloads.Add(Key, MoveTemp(Payload));
            }
        }
    }

    void SerializePayload(FArchive& Ar, UChunkStateStore* Store, FWorldTime& Time,
        float& Temp, float& TempMin, float& TempMax, float& Rain, float& Wind,
        bool& bRaining, bool& bSnowing, int32& Seed, bool bHasExtensionData = true)
    {
        Ar << Time.Day << Time.Hour << Time.TimeOfDay;
        Ar << Temp << TempMin << TempMax << Rain << Wind << bRaining << bSnowing << Seed;
        if (Ar.IsLoading())
        {
            if (Time.Day < 1 || Time.Hour < 0 || Time.Hour > 23 || !FMath::IsFinite(Time.TimeOfDay) ||
                Time.TimeOfDay < 0.0f || Time.TimeOfDay >= 24.0f || !FMath::IsFinite(Temp) ||
                !FMath::IsFinite(TempMin) || !FMath::IsFinite(TempMax) || !FMath::IsFinite(Rain) ||
                !FMath::IsFinite(Wind) || Rain < 0.0f || Wind < 0.0f)
            {
                Ar.SetError();
                return;
            }
        }

        TArray<UChunkState*> Chunks;
        int32 Count = 0;
        if (Ar.IsSaving())
        {
            if (!Store) { Ar.SetError(); return; }
            Chunks = Store->GetAllChunks();
            Chunks.RemoveAll([](const UChunkState* C) { return C == nullptr; });
            Chunks.Sort([](const UChunkState& A, const UChunkState& B)
            {
                return A.Coordinate.X == B.Coordinate.X ? A.Coordinate.Y < B.Coordinate.Y : A.Coordinate.X < B.Coordinate.X;
            });
            Count = Chunks.Num();
        }
        Ar << Count;
        if (Count < 0 || Count > MaxChunks) { Ar.SetError(); return; }

        if (Ar.IsSaving())
        {
            for (UChunkState* Chunk : Chunks)
            {
                FSerializedChunk Data = UChunkSerializer::SerializeChunk(Chunk);
                SerializeChunk(Ar, Data, bHasExtensionData);
                if (Ar.IsError()) return;
            }
            return;
        }

        // Transactional load: parse/validate every chunk and the full payload before replacing live state.
        TArray<FSerializedChunk> PendingChunks;
        PendingChunks.Reserve(Count);
        TSet<FIntPoint> SeenCoordinates;
        for (int32 I = 0; I < Count; ++I)
        {
            FSerializedChunk Data;
            SerializeChunk(Ar, Data, bHasExtensionData);
            if (Ar.IsError()) return;
            if (SeenCoordinates.Contains(Data.Coordinate)) { Ar.SetError(); return; }
            SeenCoordinates.Add(Data.Coordinate);
            PendingChunks.Add(MoveTemp(Data));
        }
        if (Ar.IsError() || Ar.Tell() != Ar.TotalSize() || !Store) { Ar.SetError(); return; }
        Store->ClearAll();
        for (const FSerializedChunk& Data : PendingChunks)
        {
            UChunkState* Chunk = UChunkSerializer::DeserializeChunk(Store, Data);
            if (!Chunk) { Ar.SetError(); return; }
            Store->AddOrReplaceChunk(Chunk);
        }
    }
}

TArray<uint8> UWorldPersistence::WrapPayload(const TArray<uint8>& Payload) const
{
    TArray<uint8> Stored = Payload;
    bool bCompressed = false;
    if (bCompressSave && Payload.Num() > 128)
    {
        const int32 Bound = FCompression::CompressMemoryBound(NAME_Zlib, Payload.Num());
        TArray<uint8> Candidate;
        Candidate.SetNumUninitialized(Bound);
        int32 CompressedSize = Bound;
        if (FCompression::CompressMemory(NAME_Zlib, Candidate.GetData(), CompressedSize, Payload.GetData(), Payload.Num()) && CompressedSize < Payload.Num())
        {
            Candidate.SetNum(CompressedSize);
            Stored = MoveTemp(Candidate);
            bCompressed = true;
        }
    }

    FBufferArchive Ar;
    uint32 Magic = SaveMagic;
    int32 Version = SaveVersion;
    uint8 CompressionFlag = bCompressed ? 1 : 0;
    int32 RawSize = Payload.Num();
    int32 StoredSize = Stored.Num();
    Ar << Magic << Version << CompressionFlag << RawSize << StoredSize;
    if (StoredSize > 0) Ar.Serialize(Stored.GetData(), StoredSize);
    TArray<uint8> Out;
    Out.Append(Ar.GetData(), Ar.Num());
    return Out;
}

bool UWorldPersistence::UnwrapPayload(const TArray<uint8>& Bytes, TArray<uint8>& OutPayload, int32* OutVersion) const
{
    if (Bytes.Num() < (int32)(sizeof(uint32) + sizeof(int32) + sizeof(uint8) + 2 * sizeof(int32))) return false;
    FMemoryReader Ar(Bytes, true);
    uint32 Magic = 0;
    int32 Version = 0;
    uint8 Flag = 0;
    int32 RawSize = 0;
    int32 StoredSize = 0;
    Ar << Magic << Version << Flag << RawSize << StoredSize;
    if (Ar.IsError() || Magic != SaveMagic || (Version != 3 && Version != SaveVersion) || RawSize < 0 || RawSize > MaxPayloadBytes ||
        StoredSize < 0 || StoredSize > Ar.TotalSize() - Ar.Tell()) return false;
    if (OutVersion) *OutVersion = Version;

    TArray<uint8> Input;
    Input.SetNumUninitialized(StoredSize);
    if (StoredSize > 0) Ar.Serialize(Input.GetData(), StoredSize);
    if (Ar.IsError() || Ar.Tell() != Ar.TotalSize()) return false;

    if (Flag == 0)
    {
        if (RawSize != StoredSize) return false;
        OutPayload = MoveTemp(Input);
        return true;
    }
    if (Flag != 1 || RawSize == 0) return false;
    OutPayload.SetNumUninitialized(RawSize);
    return FCompression::UncompressMemory(NAME_Zlib, OutPayload.GetData(), RawSize, Input.GetData(), StoredSize);
}

TArray<uint8> UWorldPersistence::SerializeWorldToBytes(UWorldStateSubsystem* State)
{
    if (!State || !State->StateStore) return {};
    FBufferArchive Ar;
    FWorldTime Time = State->WorldTime;
    float Temp = State->TemperatureC, Tmin = State->TempMinC, Tmax = State->TempMaxC;
    float Rain = State->RainfallMm, Wind = State->WindSpeed;
    bool Raining = State->bIsRaining, Snowing = State->bIsSnowing;
    int32 Seed = State->RandomSeed;
    SerializePayload(Ar, State->StateStore, Time, Temp, Tmin, Tmax, Rain, Wind, Raining, Snowing, Seed);
    if (Ar.IsError()) return {};
    TArray<uint8> Raw;
    Raw.Append(Ar.GetData(), Ar.Num());
    return WrapPayload(Raw);
}

bool UWorldPersistence::DeserializeWorldFromBytes(const TArray<uint8>& Bytes, UWorldStateSubsystem* State)
{
    if (!State || !State->StateStore) return false;
    TArray<uint8> Raw;
    int32 FormatVersion = 0;
    if (!UnwrapPayload(Bytes, Raw, &FormatVersion)) return false;
    FMemoryReader Ar(Raw, true);
    FWorldTime Time;
    float Temp = State->TemperatureC, Tmin = State->TempMinC, Tmax = State->TempMaxC;
    float Rain = State->RainfallMm, Wind = State->WindSpeed;
    bool Raining = State->bIsRaining, Snowing = State->bIsSnowing;
    int32 Seed = State->RandomSeed;
    SerializePayload(Ar, State->StateStore, Time, Temp, Tmin, Tmax, Rain, Wind, Raining, Snowing, Seed, FormatVersion >= 4);
    if (Ar.IsError() || Ar.Tell() != Ar.TotalSize()) return false;
    State->WorldTime = Time; State->TemperatureC = Temp; State->TempMinC = Tmin; State->TempMaxC = Tmax;
    State->RainfallMm = Rain; State->WindSpeed = Wind; State->bIsRaining = Raining; State->bIsSnowing = Snowing; State->RandomSeed = Seed;
    State->SynchronizeAfterLoad();
    return true;
}

bool UWorldPersistence::SaveToFile(const FString& FilePath, UChunkStateStore* Store)
{
    if (!Store) return false;
    FBufferArchive Ar;
    FWorldTime Time; float Temp=20.0f,Tmin=15.0f,Tmax=25.0f,Rain=0.0f,Wind=0.0f; bool Raining=false,Snowing=false; int32 Seed=12345;
    SerializePayload(Ar, Store, Time, Temp, Tmin, Tmax, Rain, Wind, Raining, Snowing, Seed);
    if (Ar.IsError()) return false;
    TArray<uint8> Raw; Raw.Append(Ar.GetData(), Ar.Num());
    const TArray<uint8> Bytes = WrapPayload(Raw);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
    return FFileHelper::SaveArrayToFile(Bytes, *FilePath);
}

bool UWorldPersistence::LoadFromFile(const FString& FilePath, UChunkStateStore* Store)
{
    if (!Store) return false;
    TArray<uint8> Bytes, Raw;
    int32 FormatVersion = 0;
    if (!FFileHelper::LoadFileToArray(Bytes, *FilePath) || !UnwrapPayload(Bytes, Raw, &FormatVersion)) return false;
    FMemoryReader Ar(Raw, true);
    FWorldTime Time; float Temp=20.0f,Tmin=15.0f,Tmax=25.0f,Rain=0.0f,Wind=0.0f; bool Raining=false,Snowing=false; int32 Seed=12345;
    SerializePayload(Ar, Store, Time, Temp, Tmin, Tmax, Rain, Wind, Raining, Snowing, Seed, FormatVersion >= 4);
    return !Ar.IsError() && Ar.Tell() == Ar.TotalSize();
}

bool UWorldPersistence::SaveWorldToFile(const FString& FilePath, UWorldStateSubsystem* State)
{
    const TArray<uint8> Bytes = SerializeWorldToBytes(State);
    if (Bytes.Num() == 0) return false;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
    return FFileHelper::SaveArrayToFile(Bytes, *FilePath);
}

bool UWorldPersistence::LoadWorldFromFile(const FString& FilePath, UWorldStateSubsystem* State)
{
    if (!State) return false;
    TArray<uint8> Bytes;
    return FFileHelper::LoadFileToArray(Bytes, *FilePath) && DeserializeWorldFromBytes(Bytes, State);
}

TArray<uint8> UWorldPersistence::SerializeToBytes(UChunkStateStore* Store)
{
    if (!Store) return {};
    FBufferArchive Ar;
    FWorldTime Time; float Temp=20.0f,Tmin=15.0f,Tmax=25.0f,Rain=0.0f,Wind=0.0f; bool Raining=false,Snowing=false; int32 Seed=12345;
    SerializePayload(Ar, Store, Time, Temp, Tmin, Tmax, Rain, Wind, Raining, Snowing, Seed);
    if (Ar.IsError()) return {};
    TArray<uint8> Raw; Raw.Append(Ar.GetData(), Ar.Num());
    return WrapPayload(Raw);
}

bool UWorldPersistence::DeserializeFromBytes(const TArray<uint8>& Bytes, UChunkStateStore* Store)
{
    if (!Store) return false;
    TArray<uint8> Raw;
    int32 FormatVersion = 0;
    if (!UnwrapPayload(Bytes, Raw, &FormatVersion)) return false;
    FMemoryReader Ar(Raw, true);
    FWorldTime Time; float Temp=20.0f,Tmin=15.0f,Tmax=25.0f,Rain=0.0f,Wind=0.0f; bool Raining=false,Snowing=false; int32 Seed=12345;
    SerializePayload(Ar, Store, Time, Temp, Tmin, Tmax, Rain, Wind, Raining, Snowing, Seed, FormatVersion >= 4);
    return !Ar.IsError() && Ar.Tell() == Ar.TotalSize();
}
