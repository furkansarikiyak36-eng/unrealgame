#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GerminationComponent.generated.h"
class UPlantDefinition;
UCLASS(ClassGroup=(Plant),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UGerminationComponent:public UActorComponent{GENERATED_BODY()
public: UGerminationComponent(); UFUNCTION(BlueprintCallable,Category="Germination") float ComputeGerminationProbability(float Temperature,float SoilMoisture,float SeedQuality)const; UFUNCTION(BlueprintCallable,Category="Germination") int32 RollGermination(int32 SeedCount,float Probability,int32 Seed)const; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Config") TObjectPtr<UPlantDefinition> Definition=nullptr;
protected: float TempSuitability(float T)const; float MoistureSuitability(float W)const;};
