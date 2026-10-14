#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogTypes.h"
#include "ConstructionComponent.generated.h"
class ALogActor;
class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UConstructionComponent:public UActorComponent
{GENERATED_BODY() public: UConstructionComponent(); UFUNCTION(BlueprintCallable) ALogActor* PlacePillar(const FVector& Location); UFUNCTION(BlueprintCallable) ALogActor* PlaceBeam(const FVector& Start,const FVector& End); UFUNCTION(BlueprintCallable) ALogActor* PlaceBrace(const FVector& Start,const FVector& End); UFUNCTION(BlueprintCallable) ALogActor* PlaceFloor(const FVector& Location,const FVector& Normal); UFUNCTION(BlueprintCallable) ALogActor* PlaceWall(const FVector& Location,const FVector& Direction,bool bVertical); UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; };
