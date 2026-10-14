#include "StickDefinition.h"
bool UStickDefinition::ValidateDefinition(FString& OutError) const
{
    if (StickName.IsNone()) { OutError = TEXT("StickName boş"); return false; }
    if (Length <= 0.f || Diameter <= 0.f) { OutError = TEXT("Boyutlar sıfırdan büyük olmalı"); return false; }
    if (Mass <= 0.f) { OutError = TEXT("Mass <= 0"); return false; }
    if (Flammability < 0.f || Flammability > 1.f) { OutError = TEXT("Flammability 0-1 aralığında olmalı"); return false; }
    if (WetnessResistance < 0.f || WetnessResistance > 1.f) { OutError = TEXT("WetnessResistance 0-1 aralığında olmalı"); return false; }
    const float Expected = GetVolumeFromDimensions();
    if (FMath::Abs(Expected - Volume) > FMath::Max(Expected * 0.2f, KINDA_SMALL_NUMBER))
    { OutError = FString::Printf(TEXT("Volume tutarsız: beklenen %.6f, tanımlı %.6f"), Expected, Volume); return false; }
    OutError.Reset(); return true;
}
float UStickDefinition::GetVolumeFromDimensions() const
{
    const float R = (Diameter * 0.5f) / 100.f;
    return PI * R * R * (Length / 100.f);
}
float UStickDefinition::GetEffectiveBurnTime() const
{
    // Süre artışı, ıslak malzemenin daha zor tutuşmasını tek başına modellemez; oyun dengelemesi için basit katsayı.
    if (Shape == EStickShape::Wet) return BurnTime * (1.f + WetnessResistance);
    if (Shape == EStickShape::Dry) return BurnTime * 1.4f;
    if (Shape == EStickShape::Charred) return BurnTime * 0.15f;
    return BurnTime;
}
