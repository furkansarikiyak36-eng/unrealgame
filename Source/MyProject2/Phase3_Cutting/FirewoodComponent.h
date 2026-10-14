#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FirewoodComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Wood),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFirewoodComponent:public UActorComponent
{GENERATED_BODY() public: UFirewoodComponent(); UFUNCTION(BlueprintCallable) TArray<ALogActor*> SplitIntoFirewood(ALogActor* Log,int32 Pieces=4); UPROPERTY(EditAnywhere,BlueprintReadWrite) float BurnTimePerPiece=120.0f; };
