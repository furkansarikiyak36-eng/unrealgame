#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimulationTypes.h"
#include "PlantStageComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStageChanged,EGrowthStage,OldStage,EGrowthStage,NewStage);
UCLASS(ClassGroup=(Plant),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UPlantStageComponent:public UActorComponent{GENERATED_BODY()
public: UPlantStageComponent(); UFUNCTION(BlueprintCallable,Category="Stage") void CheckStageTransition(EGrowthStage NewStage); UPROPERTY(BlueprintAssignable,Category="Events") FOnStageChanged OnStageChanged;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="State") EGrowthStage LastStage=EGrowthStage::Seed;};
