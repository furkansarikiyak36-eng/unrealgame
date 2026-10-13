#include "WoodTypes.h"

float FWoodPieceData::GetModuleLengthCm() const
{
	return FullLogLengthCm / 6.f;
}

float FWoodPieceData::GetLengthCm() const
{
	return GetModuleLengthCm() * static_cast<float>(FMath::Clamp(LengthModules, 1, 6));
}

float FWoodPieceData::GetVolumeLiters() const
{
	const float Radius = DiameterCm * 0.5f;
	float Area = PI * Radius * Radius;

	switch (CrossSection)
	{
	case EWoodCrossSection::Half:    Area *= 0.5f;  break;
	case EWoodCrossSection::Quarter: Area *= 0.25f; break;
	case EWoodCrossSection::Plank:   Area *= 0.3f;  break;
	default: break;
	}

	if (bHollow)
	{
		Area *= 0.4f;
	}

	return (Area * GetLengthCm()) / 1000.f;
}

float FWoodPieceData::GetLateralCm() const
{
	return CrossSection == EWoodCrossSection::Round ? DiameterCm : DiameterCm * 0.5f;
}

float FWoodPieceData::GetVerticalCm() const
{
	switch (CrossSection)
	{
	case EWoodCrossSection::Quarter: return DiameterCm * 0.5f;
	case EWoodCrossSection::Plank:   return DiameterCm * 0.47f;
	default:                         return DiameterCm;
	}
}
