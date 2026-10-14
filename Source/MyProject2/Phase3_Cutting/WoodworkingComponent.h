#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WoodworkingComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Wood),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UWoodworkingComponent:public UActorComponent
{GENERATED_BODY() public: UWoodworkingComponent(); UFUNCTION(BlueprintCallable) bool MakePlank(ALogActor* Log); UFUNCTION(BlueprintCallable) bool CarveNotch(ALogActor* Log,FName NotchType); UFUNCTION(BlueprintCallable) bool CarveGroove(ALogActor* Log); UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 PlankHitsRequired=4; UPROPERTY(EditAnywhere,BlueprintReadWrite) float PlankThickness=4.0f; };
