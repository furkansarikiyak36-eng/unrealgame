#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorActor.generated.h"
class UStaticMeshComponent; class USceneComponent; class UPhysicsConstraintComponent;
UCLASS() class MYPROJECT2_API ADoorActor:public AActor
{GENERATED_BODY() public: ADoorActor(); UFUNCTION(BlueprintCallable) void ToggleDoor(); UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Pivot; UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> DoorMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> Hinge; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bIsOpen=false; UPROPERTY(EditAnywhere,BlueprintReadWrite) float OpenYaw=90.0f; };
