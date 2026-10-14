#include "Phase6_Utility/CraftingBench.h"
#include "Components/StaticMeshComponent.h"
#include "Phase11_Production/ProductionChainComponent.h"
ACraftingBench::ACraftingBench(){PrimaryActorTick.bCanEverTick=false;BenchMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bench"));RootComponent=BenchMesh;ProductionChain=CreateDefaultSubobject<UProductionChainComponent>(TEXT("ProductionChain"));}
bool ACraftingBench::Craft(FName N){if(N.IsNone())return false;if(ProductionChain&&ProductionChain->QueueRecipeByID(N,1))return true;for(const FBenchRecipe& R:Recipes)if(R.OutputItem==N&&R.CraftTime>=0&&!R.Inputs.IsEmpty())return true;return false;}
