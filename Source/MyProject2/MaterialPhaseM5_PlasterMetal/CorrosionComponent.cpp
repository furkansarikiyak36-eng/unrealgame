// CorrosionComponent.cpp
#include "CorrosionComponent.h"
#include "MetalDefinition.h"

UCorrosionComponent::UCorrosionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCorrosionComponent::InitializeFromDefinition(UMetalDefinition* Def)
{
    Definition = Def;
    CorrosionLevel = 0.0f;
    ProtectionLevel = 0.0f;
}

void UCorrosionComponent::UpdateCorrosion(float Moisture, float DeltaDays)
{
    if (!Definition) return;

    // Koruma seviyesi düşürür
    const float EffectiveRate = FMath::Max(0.0f, Definition->CorrosionRate) * (1.0f - ProtectionLevel);
    const float SafeMoisture = FMath::Clamp(Moisture, 0.0f, 1.0f);
    CorrosionLevel = FMath::Clamp(CorrosionLevel + EffectiveRate * SafeMoisture * FMath::Max(0.0f, DeltaDays), 0.0f, 1.0f);

    // Koruma zamanla azalır
    ProtectionLevel = FMath::Max(0.0f, ProtectionLevel - 0.001f * DeltaDays);
}

void UCorrosionComponent::ApplyAntiCorrosionTreatment(float Amount)
{
    ProtectionLevel = FMath::Clamp(ProtectionLevel + Amount, 0.0f, 1.0f);
}
