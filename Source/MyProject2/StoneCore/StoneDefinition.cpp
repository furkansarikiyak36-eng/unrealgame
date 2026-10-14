#include "StoneDefinition.h"

bool UStoneDefinition::ValidateDefinition(FString& OutError) const
{
    if (StoneName.IsNone()) { OutError = TEXT("StoneName boş"); return false; }
    if (Dimensions.X <= 0 || Dimensions.Y <= 0 || Dimensions.Z <= 0)
    { OutError = TEXT("Dimensions <= 0"); return false; }
    if (Mass <= 0.0f) { OutError = TEXT("Mass <= 0"); return false; }
    if (CompressiveStrength <= 0.0f) { OutError = TEXT("CompressiveStrength <= 0"); return false; }
    const float Expected = GetVolumeFromDimensions();
    if (FMath::Abs(Expected - Volume) > Expected * 0.2f)
    {
        OutError = FString::Printf(TEXT("Volume tutarsız: beklenen %.4f, tanımlı %.4f"), Expected, Volume);
        return false;
    }
    OutError.Reset();
    return true;
}

float UStoneDefinition::GetVolumeFromDimensions() const
{
    return (Dimensions.X * Dimensions.Y * Dimensions.Z) / 1000000.0f;
}

bool UStoneDefinition::CanStackOn(const UStoneDefinition* Below) const
{
    if (!Below) return false;
    const float AreaBelow = Below->Dimensions.X * Below->Dimensions.Y;
    const float AreaMine = Dimensions.X * Dimensions.Y;
    return AreaMine <= AreaBelow * 1.2f && Mass <= Below->CompressiveStrength;
}
