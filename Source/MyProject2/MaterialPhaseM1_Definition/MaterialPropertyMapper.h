// MaterialPropertyMapper.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialProperties.h"
#include "MaterialPropertyMapper.generated.h"

class ULogDefinition;
class UStoneDefinition;
class UStickDefinition;

UCLASS()
class MYPROJECT2_API UMaterialPropertyMapper : public UObject
{
    GENERATED_BODY()

public:
    // Ahşap (kütük) → Malzeme profili
    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties FromLogDefinition(const ULogDefinition* LogDef);

    // Taş → Malzeme profili
    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties FromStoneDefinition(const UStoneDefinition* StoneDef);

    // Çubuk → Malzeme profili
    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties FromStickDefinition(const UStickDefinition* StickDef);

    // Ortak profil oluşturucu (5 temel malzeme)
    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties MakeStoneProfile();

    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties MakeWoodProfile();

    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties MakePlasterProfile();

    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties MakeSoilProfile();

    UFUNCTION(BlueprintCallable, Category = "Mapper")
    static FMaterialProperties MakeMetalProfile();
};
