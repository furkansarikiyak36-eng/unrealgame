// MaterialRegistry.cpp
#include "MaterialRegistry.h"

void UMaterialRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMaterialRegistry::RegisterMaterial(UMaterialDefinition* Definition)
{
    if (!Definition || Definition->MaterialID.IsNone()) return;
    Materials.Add(Definition->MaterialID, Definition);
}

UMaterialDefinition* UMaterialRegistry::FindMaterial(FName MaterialID) const
{
    if (const TObjectPtr<UMaterialDefinition>* Found = Materials.Find(MaterialID))
        return Found->Get();
    return nullptr;
}

TArray<UMaterialDefinition*> UMaterialRegistry::GetAllMaterials() const
{
    TArray<UMaterialDefinition*> Result;
    for (const auto& Pair : Materials)
        if (Pair.Value) Result.Add(Pair.Value.Get());
    return Result;
}

TArray<UMaterialDefinition*> UMaterialRegistry::FindMaterialsByClass(EMaterialClass Class) const
{
    TArray<UMaterialDefinition*> Result;
    for (const auto& Pair : Materials)
        if (Pair.Value && Pair.Value->MaterialClass == Class)
            Result.Add(Pair.Value.Get());
    return Result;
}
