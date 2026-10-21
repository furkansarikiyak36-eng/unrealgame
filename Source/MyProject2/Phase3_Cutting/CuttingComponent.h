#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogTypes.h"
#include "CuttingComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Cut),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UCuttingComponent:public UActorComponent
{GENERATED_BODY() public: UCuttingComponent(); UFUNCTION(BlueprintCallable) bool CutCrosswise(ALogActor* Log,int32 ModuleIndex); UFUNCTION(BlueprintCallable) bool SplitLengthwise(ALogActor* Log); UFUNCTION(BlueprintCallable) bool SplitQuarter(ALogActor* Log); UFUNCTION(BlueprintCallable) bool FlattenEnd(ALogActor* Log); UFUNCTION(BlueprintCallable) bool TaperEnd(ALogActor* Log); UFUNCTION(BlueprintCallable) bool ChamferEdge(ALogActor* Log); UPROPERTY(EditAnywhere,BlueprintReadWrite) float ModuleLength=75.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 HitsPerCut=4; };
