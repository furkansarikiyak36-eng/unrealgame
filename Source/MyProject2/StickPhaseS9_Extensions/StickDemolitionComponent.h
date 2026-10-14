#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickDemolitionComponent.generated.h"
class AStickActor;
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickDemolitionComponent:public UActorComponent{GENERATED_BODY() public: UStickDemolitionComponent(); UFUNCTION(BlueprintCallable) bool Demolish(AStickActor* Stick); UFUNCTION(BlueprintCallable) int32 DemolishBundle(const TArray<AStickActor*>& Sticks); UFUNCTION(BlueprintCallable) int32 RecoverFromStructure(AActor* Structure); UPROPERTY(EditAnywhere,BlueprintReadWrite) float RecoveryRate=.9f;};
