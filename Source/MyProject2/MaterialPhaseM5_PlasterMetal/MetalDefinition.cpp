// MetalDefinition.cpp
#include "MetalDefinition.h"
#include "MaterialPhaseM1_Definition/MaterialPropertyMapper.h"

UMetalDefinition::UMetalDefinition()
{
    MaterialID = TEXT("Metal_Default");
    DisplayName = FText::FromString(TEXT("Demir"));
    MaterialClass = EMaterialClass::Metal;
    Properties = UMaterialPropertyMapper::MakeMetalProfile();
}
