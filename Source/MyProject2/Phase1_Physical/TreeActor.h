#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogTypes.h"
#include "TreeActor.generated.h"
class UStaticMeshComponent; class ULogDefinition;
USTRUCT(BlueprintType) struct FTreeProfile
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EWoodSpecies Species = EWoodSpecies::Pine;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0")) float TrunkHeight = 800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0")) float TrunkDiameter = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 HitsToFell = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0")) float ModuleLength = 65.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BranchCount = 3;
};
UCLASS() class MYPROJECT2_API ATreeActor : public AActor
{
    GENERATED_BODY()
public:
    ATreeActor();
    UFUNCTION(BlueprintCallable) void ApplyAxeHit(float Strength, AActor* InstigatorActor);
    UFUNCTION(BlueprintCallable) void FellTree(const FVector& FallDirection);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTreeProfile Profile;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> TrunkMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Harvest") TSoftObjectPtr<ULogDefinition> HarvestLogDefinition;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 CurrentHits = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) bool bFelled = false;
};
