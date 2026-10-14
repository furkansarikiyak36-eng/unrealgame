// MaterialChunkIntegration.cpp
#include "MaterialChunkIntegration.h"
#include "WorldCore/ChunkState.h"
#include "WorldPhaseW2_Lazy/ExponentialDecayModel.h"
#include "WorldPhaseW3_Event/WorldEventSubsystem.h"
#include "MaterialPhaseM2_Interaction/MaterialInteractionSubsystem.h"
#include "MaterialCore/MaterialRegistry.h"
#include "MaterialCore/MaterialDefinition.h"
#include "MaterialPhaseM1_Definition/MaterialPropertyMapper.h"
#include "MaterialPhaseM6_Integration/MaterialPersistenceIntegration.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
    UMaterialDefinition* FindCellMaterial(const UChunkState* Chunk, UMaterialInteractionSubsystem* InteractionSys, FName CandidateID)
    {
        UWorld* World = InteractionSys ? InteractionSys->GetWorld() : nullptr;
        UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
        UMaterialRegistry* Registry = GI ? GI->GetSubsystem<UMaterialRegistry>() : nullptr;
        if (Registry)
        {
            if (!CandidateID.IsNone())
                if (UMaterialDefinition* Existing = Registry->FindMaterial(CandidateID)) return Existing;
            if (UMaterialDefinition* Wood = Registry->FindMaterial(TEXT("Wood"))) return Wood;
        }
        UObject* Outer = const_cast<UChunkState*>(Chunk);
        UMaterialDefinition* Fallback = NewObject<UMaterialDefinition>(Outer);
        if (!Fallback) return nullptr;
        Fallback->MaterialID = FName(TEXT("Wood"));
        Fallback->MaterialClass = EMaterialClass::Wood;
        Fallback->Properties = UMaterialPropertyMapper::MakeWoodProfile();
        return Fallback;
    }
}

void UMaterialChunkIntegration::UpdateChunkMaterials(UChunkState* Chunk, float DeltaDays, UMaterialInteractionSubsystem* InteractionSys)
{
    if (!Chunk || !InteractionSys || DeltaDays <= 0.0f) return;
    const float DurationHours = DeltaDays * 24.0f;
    for (FChunkCell& Cell : Chunk->Cells)
    {
        FBiomassCell& Biomass = Cell.Biomass;
        if (Biomass.Biomass <= 0.0f) continue;
        UMaterialDefinition* Material = FindCellMaterial(Chunk, InteractionSys, Biomass.SpeciesID);
        if (!Material) continue;

        FMaterialInstanceState InstanceState;
        const int32 CellIndex = static_cast<int32>(&Cell - Chunk->Cells.GetData());
        if (!GetChunkCellMaterialState(Chunk, CellIndex, InstanceState))
        {
            InstanceState = ExtractStateFromCell(Cell);
            InstanceState.LastUpdateDay = Chunk->Metadata.LastUpdateDay;
        }

        FMaterialInteractionContext Context;
        Context.Moisture = FMath::Clamp(Cell.SoilMoisture, 0.0f, 1.0f);
        Context.DurationHours = DurationHours;
        Context.Magnitude = Biomass.Biomass;
        Context.AmbientTemperature = Biomass.bIsBurning ? 400.0f : 20.0f;

        const EInteractionType Type = Biomass.bIsBurning ? EInteractionType::Fire : EInteractionType::Time;
        const float DamageExponent = InteractionSys->ApplyInteraction(nullptr, Material, Type, Context);
        if (DamageExponent <= 0.0f) continue;

        const float OldBiomass = Biomass.Biomass;
        Biomass.Biomass = OldBiomass * UExponentialDecayModel::ComputeDecay(1.0f, DamageExponent, 1.0f);
        const float FractionLost = FMath::Clamp(1.0f - Biomass.Biomass / FMath::Max(OldBiomass, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
        Biomass.Health = FMath::Clamp(Biomass.Health - FractionLost, 0.0f, 1.0f);
        InstanceState.MaterialID = Material->MaterialID;
        InstanceState.CurrentMoisture = Context.Moisture;
        InstanceState.CurrentTemperature = Context.AmbientTemperature;
        if (Biomass.Biomass < 0.1f)
        {
            Biomass.Biomass = 0.0f;
            Biomass.bIsBurning = false;
            Cell.bHasFire = false;
        }
        InstanceState.bIsBurning = Biomass.bIsBurning;
        InstanceState.BurnProgress = Type == EInteractionType::Fire
            ? FMath::Clamp(InstanceState.BurnProgress + FractionLost, 0.0f, 1.0f)
            : InstanceState.BurnProgress;
        InstanceState.CurrentDamage = FMath::Clamp(1.0f - Biomass.Health, 0.0f, 1.0f);
        InstanceState.StructuralHealth = 1.0f - InstanceState.CurrentDamage;
        InstanceState.DecayProgress = Type == EInteractionType::Time
            ? FMath::Clamp(InstanceState.DecayProgress + DamageExponent, 0.0f, 1.0f)
            : InstanceState.DecayProgress;
        InstanceState.LastUpdateDay += DeltaDays;
        InstanceState.ActiveReactions.Remove(EMaterialReactionType::Burn);
        InstanceState.ActiveReactions.Remove(EMaterialReactionType::Rot);
        if (Biomass.bIsBurning) InstanceState.ActiveReactions.AddUnique(EMaterialReactionType::Burn);
        else if (Biomass.Biomass > 0.0f && InstanceState.DecayProgress > 0.0f) InstanceState.ActiveReactions.AddUnique(EMaterialReactionType::Rot);
        Cell.bIsDirty = true;
        SetChunkCellMaterialState(Chunk, CellIndex, InstanceState);
    }
}

FMaterialInstanceState UMaterialChunkIntegration::ExtractStateFromCell(const FChunkCell& Cell)
{
    FMaterialInstanceState State;
    State.MaterialID = FName(TEXT("Wood"));
    State.CurrentMoisture = FMath::Clamp(Cell.SoilMoisture, 0.0f, 1.0f);
    State.CurrentTemperature = 20.0f;
    State.bIsBurning = Cell.Biomass.bIsBurning;
    State.CurrentDamage = FMath::Clamp(1.0f - Cell.Biomass.Health, 0.0f, 1.0f);
    State.StructuralHealth = 1.0f - State.CurrentDamage;
    State.DecayProgress = 0.0f;
    State.LastUpdateDay = 1.0f;
    if (Cell.Biomass.bIsBurning) State.ActiveReactions.Add(EMaterialReactionType::Burn);
    else if (Cell.Biomass.Biomass > 0.0f && Cell.Biomass.Health < 1.0f) State.ActiveReactions.Add(EMaterialReactionType::Rot);
    return State;
}

void UMaterialChunkIntegration::WriteStateToCell(FChunkCell& Cell, const FMaterialInstanceState& State)
{
    Cell.SoilMoisture = FMath::Clamp(State.CurrentMoisture, 0.0f, 1.0f);
    Cell.Biomass.bIsBurning = State.bIsBurning;
    Cell.bHasFire = State.bIsBurning;
    Cell.Biomass.Health = FMath::Clamp(1.0f - State.CurrentDamage, 0.0f, 1.0f);
    Cell.bIsDirty = true;
}


bool UMaterialChunkIntegration::SetChunkCellMaterialState(UChunkState* Chunk, int32 CellIndex, const FMaterialInstanceState& State)
{
    if (!Chunk || !Chunk->Cells.IsValidIndex(CellIndex) || State.MaterialID.IsNone()) return false;
    const TArray<uint8> Bytes = UMaterialPersistenceIntegration::SerializeInstanceState(State);
    if (Bytes.Num() <= 0) return false;
    Chunk->ExtensionCellPayloads.Add(CellIndex, Bytes);
    Chunk->MarkDirty();
    return true;
}

bool UMaterialChunkIntegration::GetChunkCellMaterialState(const UChunkState* Chunk, int32 CellIndex, FMaterialInstanceState& OutState)
{
    if (!Chunk || !Chunk->Cells.IsValidIndex(CellIndex)) return false;
    const TArray<uint8>* Bytes = Chunk->ExtensionCellPayloads.Find(CellIndex);
    if (!Bytes || Bytes->Num() <= 0) return false;
    OutState = UMaterialPersistenceIntegration::DeserializeInstanceState(*Bytes);
    return !OutState.MaterialID.IsNone();
}
