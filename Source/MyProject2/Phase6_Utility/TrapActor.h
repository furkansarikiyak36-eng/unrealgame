#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrapActor.generated.h"
class UBoxComponent;
UENUM(BlueprintType) enum class ETrapType:uint8{RollingLog,FallingLog,SpikePit};
UCLASS() class MYPROJECT2_API ATrapActor:public AActor
{GENERATED_BODY() public: ATrapActor(); UFUNCTION(BlueprintCallable) void Trigger(AActor* Victim); UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> TriggerVolume; UPROPERTY(EditAnywhere,BlueprintReadWrite) ETrapType TrapType=ETrapType::RollingLog; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Damage=50.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Cooldown=5.0f; protected: float LastTriggerTime=-100.0f; };
