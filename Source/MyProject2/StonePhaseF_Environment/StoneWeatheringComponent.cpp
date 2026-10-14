#include "StoneWeatheringComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
UStoneWeatheringComponent::UStoneWeatheringComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
}
void UStoneWeatheringComponent::ApplyRain(float Intensity, float DeltaTime)
{ Wetness = FMath::Clamp(Wetness + FMath::Max(0.0f, Intensity) * FMath::Max(0.0f, DeltaTime) * 0.1f, 0.0f, 1.0f); }
void UStoneWeatheringComponent::ApplySnow(float Intensity, float DeltaTime)
{ SnowCoverage = FMath::Clamp(SnowCoverage + FMath::Max(0.0f, Intensity) * FMath::Max(0.0f, DeltaTime) * 0.1f, 0.0f, 1.0f); }
void UStoneWeatheringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Wetness = FMath::Max(0.0f, Wetness - FMath::Max(0.0f, DryRate) * DeltaTime);
    SnowCoverage = FMath::Max(0.0f, SnowCoverage - FMath::Max(0.0f, DryRate) * DeltaTime * 0.5f);
    UPrimitiveComponent* Prim = GetOwner() ? Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()) : nullptr;
    if (!Prim) return;
    while (DynamicMaterials.Num() < Prim->GetNumMaterials())
    {
        const int32 Idx = DynamicMaterials.Num();
        UMaterialInstanceDynamic* MID = Prim->CreateAndSetMaterialInstanceDynamic(Idx);
        DynamicMaterials.Add(MID);
    }
    for (UMaterialInstanceDynamic* MID : DynamicMaterials)
    {
        if (!MID) continue;
        MID->SetScalarParameterValue(TEXT("Wetness"), Wetness);
        MID->SetScalarParameterValue(TEXT("Snow"), SnowCoverage);
    }
}
