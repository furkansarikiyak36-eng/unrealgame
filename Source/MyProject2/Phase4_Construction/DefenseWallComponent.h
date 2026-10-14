#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DefenseWallComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UDefenseWallComponent:public UActorComponent
{GENERATED_BODY() public: UDefenseWallComponent(); UFUNCTION(BlueprintCallable) bool SharpenTop(ALogActor* TopLog); UFUNCTION(BlueprintCallable) bool AddBrace(ALogActor* Wall,ALogActor* Brace); UPROPERTY(EditAnywhere,BlueprintReadWrite) float DamageResistanceBonus=0.5f; };
