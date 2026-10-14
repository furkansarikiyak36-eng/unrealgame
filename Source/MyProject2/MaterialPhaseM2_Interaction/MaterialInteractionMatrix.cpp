// MaterialInteractionMatrix.cpp
#include "MaterialInteractionMatrix.h"

TArray<UInteractionRule*> UMaterialInteractionMatrix::GetRulesFor(EMaterialClass Class) const
{
    TArray<UInteractionRule*> Result;
    for (UInteractionRule* Rule : Rules)
        if (Rule && Rule->TargetClass == Class)
            Result.Add(Rule);
    return Result;
}
