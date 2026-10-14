// MaterialPersistenceIntegration.cpp
#include "MaterialPersistenceIntegration.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

namespace
{
    constexpr uint32 MaterialStateMagic = 0x4D535431; // MST1
    constexpr uint8 MaterialStateVersion = 1;
    constexpr int32 MaxSerializedReactions = 64;
}

TArray<uint8> UMaterialPersistenceIntegration::SerializeInstanceState(const FMaterialInstanceState& State)
{
    FBufferArchive Archive;
    uint32 Magic = MaterialStateMagic;
    uint8 Version = MaterialStateVersion;
    Archive << Magic;
    Archive << Version;
    FString MaterialID = State.MaterialID.ToString();
    Archive << MaterialID;
    float CurrentMoisture = State.CurrentMoisture;
    float CurrentTemperature = State.CurrentTemperature;
    float CurrentDamage = State.CurrentDamage;
    float StructuralHealth = State.StructuralHealth;
    float BurnProgress = State.BurnProgress;
    bool bIsBurning = State.bIsBurning;
    float DecayProgress = State.DecayProgress;
    float WeatheringLevel = State.WeatheringLevel;
    float LastUpdateDay = State.LastUpdateDay;
    Archive << CurrentMoisture;
    Archive << CurrentTemperature;
    Archive << CurrentDamage;
    Archive << StructuralHealth;
    Archive << BurnProgress;
    Archive << bIsBurning;
    Archive << DecayProgress;
    Archive << WeatheringLevel;
    Archive << LastUpdateDay;
    int32 ReactionCount = FMath::Min(State.ActiveReactions.Num(), MaxSerializedReactions);
    Archive << ReactionCount;
    for (int32 i = 0; i < ReactionCount; ++i)
    {
        uint8 Value = (uint8)State.ActiveReactions[i];
        Archive << Value;
    }
    TArray<uint8> Result;
    Result.Append(Archive.GetData(), Archive.Num());
    return Result;
}

FMaterialInstanceState UMaterialPersistenceIntegration::DeserializeInstanceState(const TArray<uint8>& Bytes)
{
    FMaterialInstanceState State;
    if (Bytes.Num() < (int32)(sizeof(uint32) + sizeof(uint8))) return State;

    FMemoryReader Reader(Bytes, true);
    uint32 Magic = 0;
    uint8 Version = 0;
    Reader << Magic;
    Reader << Version;
    if (Reader.IsError() || Magic != MaterialStateMagic || Version != MaterialStateVersion) return State;

    FString MaterialID;
    Reader << MaterialID;
    State.MaterialID = FName(*MaterialID);
    Reader << State.CurrentMoisture;
    Reader << State.CurrentTemperature;
    Reader << State.CurrentDamage;
    Reader << State.StructuralHealth;
    Reader << State.BurnProgress;
    Reader << State.bIsBurning;
    Reader << State.DecayProgress;
    Reader << State.WeatheringLevel;
    Reader << State.LastUpdateDay;
    if (Reader.IsError()) return FMaterialInstanceState();

    int32 ReactionCount = 0;
    Reader << ReactionCount;
    if (Reader.IsError() || ReactionCount < 0 || ReactionCount > MaxSerializedReactions ||
        ReactionCount > Reader.TotalSize() - Reader.Tell()) return FMaterialInstanceState();
    for (int32 i = 0; i < ReactionCount; ++i)
    {
        uint8 Value = 0;
        Reader << Value;
        if (Reader.IsError() || Value > (uint8)EMaterialReactionType::Melt) return FMaterialInstanceState();
        State.ActiveReactions.Add((EMaterialReactionType)Value);
    }
    State.CurrentMoisture = FMath::Clamp(State.CurrentMoisture, 0.0f, 1.0f);
    State.CurrentDamage = FMath::Clamp(State.CurrentDamage, 0.0f, 1.0f);
    State.StructuralHealth = FMath::Clamp(State.StructuralHealth, 0.0f, 1.0f);
    State.BurnProgress = FMath::Clamp(State.BurnProgress, 0.0f, 1.0f);
    State.DecayProgress = FMath::Clamp(State.DecayProgress, 0.0f, 1.0f);
    State.WeatheringLevel = FMath::Clamp(State.WeatheringLevel, 0.0f, 1.0f);
    State.LastUpdateDay = FMath::Max(1.0f, State.LastUpdateDay);
    return State;
}
