#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimulationTypes.h"
#include "AnimalMarketComponent.generated.h"
class AAnimalActor; class UAnimalDefinition;
UCLASS(ClassGroup=(Market),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalMarketComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalMarketComponent(); UFUNCTION(BlueprintCallable,Category="Market") AAnimalActor* PurchaseAnimal(UAnimalDefinition* Def,EAnimalSex Sex,float AgeDays); UFUNCTION(BlueprintCallable,Category="Market") float SellAnimal(AAnimalActor* Animal); UFUNCTION(BlueprintPure,Category="Market") float ComputeAnimalValue(AAnimalActor* Animal)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<AAnimalActor> AnimalClass;};
