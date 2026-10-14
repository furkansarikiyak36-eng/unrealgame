#include "Phase6_Utility/CraftingBench.h"
#include "Components/StaticMeshComponent.h"
ACraftingBench::ACraftingBench(){PrimaryActorTick.bCanEverTick=false;BenchMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bench"));RootComponent=BenchMesh;}
bool ACraftingBench::Craft(FName N){if(N.IsNone())return false;for(const FBenchRecipe& R:Recipes)if(R.OutputItem==N&&R.CraftTime>=0&&!R.Inputs.IsEmpty())return true;return false;}
