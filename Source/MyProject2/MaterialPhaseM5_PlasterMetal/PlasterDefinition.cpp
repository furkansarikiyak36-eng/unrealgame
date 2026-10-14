// PlasterDefinition.cpp
#include "PlasterDefinition.h"
#include "MaterialPhaseM1_Definition/MaterialPropertyMapper.h"

UPlasterDefinition::UPlasterDefinition()
{
    MaterialID = TEXT("Plaster_Default");
    DisplayName = FText::FromString(TEXT("Alçı"));
    MaterialClass = EMaterialClass::Plaster;
    Properties = UMaterialPropertyMapper::MakePlasterProfile();
}
