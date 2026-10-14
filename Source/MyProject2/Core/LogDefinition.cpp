#include "Core/LogDefinition.h"

bool ULogDefinition::ValidateDefinition(FString& OutError) const
{
    OutError.Reset();
    if (LogType.IsNone()) { OutError = TEXT("LogType is required"); return false; }
    if (Length <= 0.0f || Diameter <= 0.0f || Mass <= 0.0f || BurnTime <= 0.0f) { OutError = TEXT("Length, diameter, mass and burn time must be positive"); return false; }
    if (Flammability < 0.0f || Flammability > 1.0f || Buoyancy < 0.0f || Buoyancy > 1.0f) { OutError = TEXT("Normalized values must be in [0,1]"); return false; }
    if (RollingResistance < 0.0f || DragResistance < 0.0f || StructuralStrength < 0.0f || MaxCantilever < 0.0f) { OutError = TEXT("Physics and structural values cannot be negative"); return false; }
    const float Expected = GetVolumeFromDimensions();
    if (Volume <= 0.0f || FMath::Abs(Expected - Volume) > FMath::Max(Expected * 0.15f, 0.0001f))
    { OutError = FString::Printf(TEXT("Volume mismatch: expected %.4f m^3, configured %.4f m^3"), Expected, Volume); return false; }
    return true;
}

float ULogDefinition::GetVolumeFromDimensions() const
{
    const float RadiusM = (Diameter * 0.5f) / 100.0f;
    const float LengthM = Length / 100.0f;
    return PI * RadiusM * RadiusM * LengthM;
}
