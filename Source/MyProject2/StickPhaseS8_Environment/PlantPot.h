#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class UStaticMeshComponent;
#include "PlantPot.generated.h"
UENUM(BlueprintType) enum class EPlantType:uint8{Berry,Herb,Flower,Root}; UCLASS() class MYPROJECT2_API APlantPot:public AActor{GENERATED_BODY() public: APlantPot(); UFUNCTION(BlueprintCallable) bool Plant(EPlantType Type); UFUNCTION(BlueprintCallable) bool Water(float Amount); UFUNCTION(BlueprintCallable) int32 Harvest(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PotMesh; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) EPlantType PlantedType=EPlantType::Berry; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float GrowthProgress=0; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float WaterLevel=50; UPROPERTY(EditAnywhere,BlueprintReadWrite) float GrowthTime=300; protected: virtual void Tick(float DeltaTime)override;};
