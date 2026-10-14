#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WallShelf.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AWallShelf:public AActor{GENERATED_BODY() public: AWallShelf(); UFUNCTION(BlueprintCallable) bool AttachToWall(AActor* Wall); UFUNCTION(BlueprintCallable) bool PlaceItem(FName ItemName); UFUNCTION(BlueprintCallable) FName TakeItem(int32 Slot); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ShelfMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxSlots=4; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<FName> Items; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<AActor> AttachedWall=nullptr; };
