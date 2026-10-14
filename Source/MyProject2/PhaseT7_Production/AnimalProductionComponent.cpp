#include "AnimalProductionComponent.h"
#include "AnimalDefinition.h"
UAnimalProductionComponent::UAnimalProductionComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UAnimalProductionComponent::ComputeDailyOutput(float Max,float Age,float Nutrition,float Health,float Season){const float Potential=FMath::Max(0.f,Max)*FMath::Clamp(Age,0.f,1.f)*FMath::Clamp(Nutrition,0.f,1.f)*FMath::Clamp(Health,0.f,1.f)*FMath::Clamp(Season,0.f,1.5f);const float Actual=FMath::Min(Potential,FMath::Max(0.f,MaxStorage-StoredProduct));StoredProduct+=Actual;return Actual;}
