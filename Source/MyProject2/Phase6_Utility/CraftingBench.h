#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CraftingBench.generated.h"
class UStaticMeshComponent;
class UProductionChainComponent;
USTRUCT(BlueprintType) struct FBenchRecipe{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadOnly) FName OutputItem=NAME_None; UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,int32> Inputs; UPROPERTY(EditAnywhere,BlueprintReadOnly) float CraftTime=2.0f;};
UCLASS() class MYPROJECT2_API ACraftingBench:public AActor
{GENERATED_BODY() public: ACraftingBench(); UFUNCTION(BlueprintCallable) bool Craft(FName RecipeName); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BenchMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FBenchRecipe> Recipes; UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Production") TObjectPtr<UProductionChainComponent> ProductionChain; };
