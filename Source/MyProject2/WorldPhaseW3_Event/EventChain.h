#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "EventChain.generated.h"
UCLASS(BlueprintType)
class MYPROJECT2_API UEventChain : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadWrite) FWorldEvent RootEvent;
 UPROPERTY(BlueprintReadWrite) TArray<FWorldEvent> ChildEvents;
 UPROPERTY(BlueprintReadWrite) int32 Depth=0;
 UPROPERTY(BlueprintReadWrite) bool bIsResolved=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxDepth=5;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float TimeoutSeconds=30.0f;
 UFUNCTION(BlueprintPure,Category="Chain") bool CanAddChild() const{return Depth<MaxDepth&&!bIsResolved;}
 UFUNCTION(BlueprintCallable,Category="Chain") void AddChild(const FWorldEvent& Event){if(CanAddChild()){ChildEvents.Add(Event);++Depth;}}
 UFUNCTION(BlueprintCallable,Category="Chain") void Resolve(){bIsResolved=true;}
};
