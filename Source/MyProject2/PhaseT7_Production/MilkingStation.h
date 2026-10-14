#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MilkingStation.generated.h"
class UStaticMeshComponent; class AAnimalActor;
UCLASS() class MYPROJECT2_API AMilkingStation:public AActor{GENERATED_BODY()
public: AMilkingStation(); UFUNCTION(BlueprintCallable,Category="Milking") bool MilkAnimal(AAnimalActor* Animal); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StationMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MilkStorage=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MilkStorageMax=200.f;};
