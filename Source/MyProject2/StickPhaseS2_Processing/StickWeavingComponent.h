#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickWeavingComponent.generated.h"
class AStickActor;
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickWeavingComponent:public UActorComponent{GENERATED_BODY() public: UStickWeavingComponent(); UFUNCTION(BlueprintCallable) bool WeaveHorizontal(AStickActor*Arg1,AStickActor*Arg2); UFUNCTION(BlueprintCallable) bool WeaveCross(AStickActor*Arg1,AStickActor*Arg2); UFUNCTION(BlueprintCallable) bool CreateWattlePanel(const TArray<AStickActor*>&Arg1,const TArray<AStickActor*>&Arg2); UPROPERTY(EditAnywhere,BlueprintReadWrite) float WeaveTension=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 RowsPerPanel=6;};
