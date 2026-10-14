#include "HordeGroupResolver.h"
#include "HordePlanAsset.h"
TArray<FWeightedEntry> UHordeGroupResolver::ResolveGroup(FName ID,const UEnemyGroupLibrary* L){if(L)if(const FEnemyGroupDefinition* G=L->FindGroup(ID))return G->Members;return {};}
