#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DovetailJointComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Wood),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UDovetailJointComponent:public UActorComponent
{GENERATED_BODY() public: UDovetailJointComponent(); UFUNCTION(BlueprintCallable) bool PrepareDovetail(ALogActor* Log); UFUNCTION(BlueprintCallable) bool LockDovetail(ALogActor* A,ALogActor* B); UPROPERTY(EditAnywhere,BlueprintReadWrite) float StrengthMultiplier=2.0f; };
