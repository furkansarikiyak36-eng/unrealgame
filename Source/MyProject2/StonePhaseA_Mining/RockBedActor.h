#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoneTypes.h"
#include "RockBedActor.generated.h"

class UStaticMeshComponent;
class UStoneDefinition;
class AStoneActor;

UCLASS()
class MYPROJECT2_API ARockBedActor : public AActor
{
    GENERATED_BODY()
public:
    ARockBedActor();
    UFUNCTION(BlueprintCallable) int32 MineChunk(float Strength, AActor* InstigatorActor);
    UFUNCTION(BlueprintPure) bool IsDepleted() const { return RemainingChunks <= 0; }
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> RockMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EStoneType StoneType = EStoneType::Granite;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TotalChunks = 8;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 RemainingChunks = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> SpawnStoneClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStoneDefinition> StoneDefinition;
};
