// MaterialDefinitionLibrary.cpp
#include "MaterialDefinitionLibrary.h"
#include "MaterialCore/MaterialRegistry.h"

void UMaterialDefinitionLibrary::LoadAllMaterials(UMaterialRegistry* Registry) const
{
    if (!Registry) return;
    for (const TSoftObjectPtr<UMaterialDefinition>& SoftMat : MaterialDefinitions)
    {
        if (UMaterialDefinition* Mat = SoftMat.LoadSynchronous())
            Registry->RegisterMaterial(Mat);
    }
}

bool UMaterialDefinitionLibrary::ValidateLibrary(FString& OutError) const
{
    OutError.Reset();
    if (MaterialDefinitions.Num() == 0) { OutError = TEXT("MaterialDefinitions boş"); return false; }

    TSet<FName> SeenIDs;
    for (const TSoftObjectPtr<UMaterialDefinition>& SoftMat : MaterialDefinitions)
    {
        UMaterialDefinition* Mat = SoftMat.LoadSynchronous();
        if (!Mat) { OutError = TEXT("MaterialDefinitions içinde yüklenemeyen referans var"); return false; }
        if (SeenIDs.Contains(Mat->MaterialID))
        { OutError = FString::Printf(TEXT("Duplicate MaterialID: %s"), *Mat->MaterialID.ToString()); return false; }
        SeenIDs.Add(Mat->MaterialID);

        FString DefError;
        if (!Mat->ValidateDefinition(DefError))
        { OutError = FString::Printf(TEXT("%s: %s"), *Mat->MaterialID.ToString(), *DefError); return false; }
    }
    return true;
}
