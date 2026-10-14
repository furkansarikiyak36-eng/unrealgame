// MaterialChunkIntegration.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialPhaseM3_Instance/MaterialInstanceState.h"
#include "WorldCore/ChunkState.h"
#include "MaterialChunkIntegration.generated.h"

class UChunkState;
class UMaterialInteractionSubsystem;

UCLASS()
class MYPROJECT2_API UMaterialChunkIntegration : public UObject
{
    GENERATED_BODY()

public:
    // Chunk hücrelerinde malzeme durumunu güncelle
    UFUNCTION(BlueprintCallable, Category = "Integration")
    static void UpdateChunkMaterials(UChunkState* Chunk, float DeltaDays, UMaterialInteractionSubsystem* InteractionSys);

    // Chunk'tan malzeme durumu çıkar
    UFUNCTION(BlueprintCallable, Category = "Integration")
    static FMaterialInstanceState ExtractStateFromCell(const FChunkCell& Cell);

    // Chunk hücresine malzeme durumu yaz
    UFUNCTION(BlueprintCallable, Category = "Integration")
    static void WriteStateToCell(FChunkCell& Cell, const FMaterialInstanceState& State);

    UFUNCTION(BlueprintCallable, Category = "Integration|Persistence")
    static bool SetChunkCellMaterialState(UChunkState* Chunk, int32 CellIndex, const FMaterialInstanceState& State);

    UFUNCTION(BlueprintCallable, Category = "Integration|Persistence")
    static bool GetChunkCellMaterialState(const UChunkState* Chunk, int32 CellIndex, FMaterialInstanceState& OutState);
};
