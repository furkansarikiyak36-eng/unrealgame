// MaterialValidation.cpp
#include "MaterialValidation.h"

bool UMaterialValidation::ValidateDefinition(UMaterialDefinition* Def, FString& OutError)
{
    if (!Def) { OutError = TEXT("Def null"); return false; }
    return Def->ValidateDefinition(OutError);
}

bool UMaterialValidation::ValidateAgainstTemplate(UMaterialDefinition* Def, EMaterialClass Template, FString& OutError)
{
    if (!Def) { OutError = TEXT("Def null"); return false; }
    if (Def->MaterialClass != Template)
    { OutError = FString::Printf(TEXT("Beklenen sınıf: %d, gelen: %d"), (int32)Template, (int32)Def->MaterialClass); return false; }
    return Def->ValidateDefinition(OutError);
}
