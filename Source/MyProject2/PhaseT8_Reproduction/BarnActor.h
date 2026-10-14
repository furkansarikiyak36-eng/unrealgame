#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarnActor.generated.h"
class UStaticMeshComponent;
UCLASS() class MYPROJECT2_API ABarnActor:public AActor{GENERATED_BODY()
public: ABarnActor(); virtual void BeginPlay()override; virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override; UFUNCTION(BlueprintPure,Category="Barn") int32 GetOccupancy()const{return Occupancy;} UFUNCTION(BlueprintCallable,Category="Barn") bool TryAddAnimal(); UFUNCTION(BlueprintCallable,Category="Barn") void RemoveAnimal(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BarnMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Capacity=10; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) int32 Occupancy=0;};
