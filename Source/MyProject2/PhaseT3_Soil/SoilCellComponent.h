#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimulationTypes.h"
#include "SoilCellComponent.generated.h"
UCLASS(ClassGroup=(Soil),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API USoilCellComponent:public UActorComponent{GENERATED_BODY()
public: USoilCellComponent(); UFUNCTION(BlueprintPure,Category="Soil") FSoilState GetSoilState()const{return SoilState;} // Native mutable accessor; intentionally not Blueprint-reflected.
 FSoilState& GetMutableSoilState(){return SoilState;} UFUNCTION(BlueprintCallable,Category="Soil") void UpdateDaily(float Rainfall,float Irrigation,float Evaporation,float Transpiration,float Runoff,float Drainage); UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="State") FSoilState SoilState; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Config") float AreaHectares=.01f;
};
