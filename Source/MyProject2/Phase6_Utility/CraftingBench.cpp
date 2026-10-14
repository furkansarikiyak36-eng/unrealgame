#include "Phase6_Utility/CraftingBench.h"
#include "Components/StaticMeshComponent.h"
#include "Phase11_Production/ProductionChainComponent.h"
#include "Phase11_Production/ProductionChainTypes.h"
ACraftingBench::ACraftingBench(){PrimaryActorTick.bCanEverTick=false;BenchMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bench"));RootComponent=BenchMesh;ProductionChain=CreateDefaultSubobject<UProductionChainComponent>(TEXT("ProductionChain"));}
bool ACraftingBench::Craft(FName N)
{
    if(N.IsNone()||!ProductionChain)return false;
    if(ProductionChain->QueueRecipeByID(N,1))return true;
    for(const FBenchRecipe& R:Recipes)
    {
        if(R.OutputItem!=N||!FMath::IsFinite(R.CraftTime)||R.CraftTime<0.01f||R.Inputs.IsEmpty())continue;
        TArray<FProductionIngredient> Inputs;
        for(const TPair<FName,int32>& Pair:R.Inputs)
        {
            FProductionIngredient Input;
            Input.ItemID=Pair.Key;
            Input.Count=Pair.Value;
            Inputs.Add(Input);
        }
        TArray<FProductionOutput> Outputs;
        FProductionOutput Output;
        Output.ItemID=R.OutputItem;
        Output.Count=1;
        Outputs.Add(Output);
        return ProductionChain->QueueConfiguredRecipe(N,Inputs,Outputs,R.CraftTime);
    }
    return false;
}
