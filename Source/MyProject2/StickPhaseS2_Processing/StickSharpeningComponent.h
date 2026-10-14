#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickSharpeningComponent.generated.h"
class AStickActor;
UENUM(BlueprintType) enum class ESharpeningMode:uint8{SingleEnd,DoubleEnd};
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickSharpeningComponent:public UActorComponent{GENERATED_BODY() public: UStickSharpeningComponent(); UFUNCTION(BlueprintCallable) bool SharpenStick(AStickActor* Stick,ESharpeningMode Mode=ESharpeningMode::SingleEnd); UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 HitsRequired=3;};
