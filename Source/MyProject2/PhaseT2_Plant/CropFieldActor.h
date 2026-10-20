#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimulationTypes.h"
#include "CropFieldActor.generated.h"

class UStaticMeshComponent;
class UPlantGrowthComponent;
class UPlantStageComponent;
class UPlantVisualComponent;
class USoilCellComponent;
class UPlantDefinition;

UCLASS()
class MYPROJECT2_API ACropFieldActor : public AActor
{
    GENERATED_BODY()
public:
    ACropFieldActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="Field") void PlantCrop(UPlantDefinition* Def, int32 SeedCount);
    UFUNCTION(BlueprintCallable, Category="Field") void UpdateDaily();
    UFUNCTION(BlueprintPure, Category="Field") UPlantGrowthComponent* GetGrowth() const { return GrowthComponent; }
    UFUNCTION(BlueprintPure, Category="Field") USoilCellComponent* GetSoil() const { return SoilComponent; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> FieldMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPlantGrowthComponent> GrowthComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPlantStageComponent> StageComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPlantVisualComponent> VisualComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USoilCellComponent> SoilComponent;

protected:
    UFUNCTION() void HandleSimulationDay(int32 DayNumber);
    UFUNCTION() void HandleStageChanged(EGrowthStage OldStage, EGrowthStage NewStage);
};