// Copyright Epic Games, Inc. All Rights Reserved.

#include "FarmKit.h"
#include "ForestCharacter.h"
#include "ForestTypes.h"
#include "DayLoopSubsystem.h"
#include "SurvivalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhaseT2_Plant/CropFieldActor.h"
#include "PhaseT2_Plant/PlantDefinition.h"
#include "PhaseT2_Plant/PlantGrowthComponent.h"
#include "PhaseT5_Harvest/HarvestComponent.h"
#include "PhaseT6_Animal/AnimalActor.h"
#include "PhaseT6_Animal/AnimalDefinition.h"

namespace
{
	void Tint(UStaticMeshComponent* Part, UObject* Outer, const FLinearColor& Color)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Outer);
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Material);
		}
	}
}

UPlantDefinition* FarmKit::WildGrain(UObject* Outer)
{
	UPlantDefinition* Grain = NewObject<UPlantDefinition>(Outer);
	Grain->PlantID = TEXT("WildGrain");
	Grain->DisplayName = FText::FromString(TEXT("Yabani tahıl"));
	Grain->BaseTemperature = 4.f;
	Grain->MaxTemperature = 34.f;
	Grain->HarvestIndex = 0.4f;
	Grain->MaxBiomass = 2000.f;
	const TPair<EGrowthStage, float> Stages[] = {
		{ EGrowthStage::Germination, 6.f }, { EGrowthStage::Seedling, 14.f }, { EGrowthStage::Vegetative, 28.f },
		{ EGrowthStage::Flowering, 44.f }, { EGrowthStage::Fruiting, 56.f }, { EGrowthStage::Maturity, 66.f } };
	for (const TPair<EGrowthStage, float>& Stage : Stages)
	{
		FGDDStageThreshold& Threshold = Grain->StageThresholds.AddDefaulted_GetRef();
		Threshold.Stage = Stage.Key;
		Threshold.GDDRequired = Stage.Value;
	}
	return Grain;
}

UAnimalDefinition* FarmKit::AnimalOf(UObject* Outer, EAnimalSpecies Species)
{
	UAnimalDefinition* Animal = NewObject<UAnimalDefinition>(Outer);
	Animal->Species = Species;
	const bool bChicken = Species == EAnimalSpecies::Chicken;
	Animal->AnimalID = bChicken ? TEXT("Chicken") : TEXT("Sheep");
	Animal->DisplayName = FText::FromString(bChicken ? TEXT("Tavuk") : TEXT("Koyun"));
	Animal->W_infinity = bChicken ? 3.f : 70.f;
	Animal->AgeMaturityDays = bChicken ? 150.f : 300.f;
	Animal->MaxMilkPerDay = 0.f;
	Animal->MaxEggsPerDay = bChicken ? 1.f : 0.f;
	// Greybox pace: a sheep grows enough wool for a bundle in a few days.
	Animal->MaxWoolPerDay = bChicken ? 0.f : 0.35f;
	return Animal;
}

ACropFieldActor* FarmKit::MakeField(UWorld* World, const FVector& Ground)
{
	ACropFieldActor* Field = World ? World->SpawnActor<ACropFieldActor>(ACropFieldActor::StaticClass(), FTransform(Ground + FVector(0.f, 0.f, 4.f))) : nullptr;
	if (Field && Field->FieldMesh)
	{
		Field->FieldMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Field->FieldMesh->SetWorldScale3D(FVector(2.f, 2.f, 0.08f));
		Field->FieldMesh->SetCollisionProfileName(TEXT("BlockAll"));
		Tint(Field->FieldMesh, Field, FLinearColor(0.22f, 0.13f, 0.06f));
	}
	return Field;
}

ACropFieldActor* FarmKit::MakeWildPatch(UWorld* World, const FVector& Ground)
{
	ACropFieldActor* Patch = MakeField(World, Ground);
	if (!Patch || !Patch->GrowthComponent)
	{
		return Patch;
	}
	Patch->PlantCrop(WildGrain(Patch), SeedsPerField);
	FPlantState& Plant = Patch->GrowthComponent->PlantState;
	Plant.AccumulatedGDD = 66.f;
	Plant.Stage = EGrowthStage::Maturity;
	Plant.Biomass = 150.f;
	Plant.bIsHarvestable = true;
	if (Patch->FieldMesh)
	{
		Tint(Patch->FieldMesh, Patch, FLinearColor(0.55f, 0.48f, 0.12f));
	}
	return Patch;
}

bool FarmKit::IsSown(const ACropFieldActor* Field)
{
	return Field && Field->bCropPlanted;
}

bool FarmKit::IsRipe(const ACropFieldActor* Field)
{
	return Field && Field->bCropPlanted && Field->HarvestComponent && Field->HarvestComponent->CanHarvest();
}

bool FarmKit::Sow(ACropFieldActor* Field, AForestCharacter* Player)
{
	if (!Field || !Player || Field->bCropPlanted || Player->GetItemCount(EForestItem::Seed) < SeedsPerField)
	{
		return false;
	}
	Field->PlantCrop(WildGrain(Field), SeedsPerField);
	if (!Field->bCropPlanted)
	{
		return false;
	}
	Player->TestRemoveItem(EForestItem::Seed, SeedsPerField);
	if (Field->FieldMesh)
	{
		Tint(Field->FieldMesh, Field, FLinearColor(0.2f, 0.32f, 0.08f));
	}
	return true;
}

int32 FarmKit::Harvest(ACropFieldActor* Field, AForestCharacter* Player)
{
	if (!IsRipe(Field) || !Player)
	{
		return 0;
	}
	const float Yield = Field->HarvestComponent->Harvest();
	Field->bCropPlanted = false;
	// A poor field (dry, hungry soil) still gives a little grain.
	const int32 Grain = FMath::Clamp(3 + FMath::RoundToInt(Yield / 10.f), 3, MaxGrainPerField);
	Player->TestGiveItem(EForestItem::Grain, Grain);
	Player->TestGiveItem(EForestItem::Seed, SeedsPerField + 2);
	if (Field->FieldMesh)
	{
		Tint(Field->FieldMesh, Field, FLinearColor(0.22f, 0.13f, 0.06f));
	}
	return Grain;
}

AAnimalActor* FarmKit::SpawnAnimal(UWorld* World, const FVector& Location, EAnimalSpecies Species)
{
	AAnimalActor* Animal = World ? World->SpawnActor<AAnimalActor>(AAnimalActor::StaticClass(), FTransform(Location)) : nullptr;
	if (!Animal)
	{
		return nullptr;
	}
	UAnimalDefinition* Definition = AnimalOf(Animal, Species);
	Animal->Initialize(Definition, EAnimalSex::Female, Definition->AgeMaturityDays + 30.f);
	if (Animal->AnimalMesh)
	{
		const bool bChicken = Species == EAnimalSpecies::Chicken;
		Animal->AnimalMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, bChicken ? TEXT("/Engine/BasicShapes/Sphere.Sphere") : TEXT("/Engine/BasicShapes/Cube.Cube")));
		Animal->AnimalMesh->SetWorldScale3D(bChicken ? FVector(0.35f, 0.3f, 0.35f) : FVector(1.1f, 0.55f, 0.7f));
		Animal->AnimalMesh->SetCollisionProfileName(TEXT("BlockAll"));
		Tint(Animal->AnimalMesh, Animal, bChicken ? FLinearColor(0.9f, 0.8f, 0.55f) : FLinearColor(0.85f, 0.85f, 0.8f));
	}
	return Animal;
}

int32 FarmKit::CollectProducts(AAnimalActor* Animal, AForestCharacter* Player)
{
	UInventoryComponent* Pack = Player ? Player->GetInventoryComponent() : nullptr;
	if (!Animal || !Pack)
	{
		return 0;
	}
	return Animal->CollectProductsToInventory(Pack, ForestItems::GetInventoryID(EForestItem::Milk),
		ForestItems::GetInventoryID(EForestItem::Egg), ForestItems::GetInventoryID(EForestItem::Wool));
}

bool FarmKit::Butcher(AAnimalActor* Animal, AForestCharacter* Player)
{
	if (!Animal || !Player || !Animal->Definition)
	{
		return false;
	}
	const bool bChicken = Animal->Definition->Species == EAnimalSpecies::Chicken;
	Player->TestGiveItem(EForestItem::Meat, bChicken ? 1 : 4);
	if (!bChicken)
	{
		Player->TestGiveItem(EForestItem::Hide, 2);
		Player->TestGiveItem(EForestItem::Wool, 1);
	}
	Animal->Destroy();
	return true;
}

FString FarmKit::Eat(AForestCharacter* Player)
{
	USurvivalComponent* Body = Player ? Player->GetSurvivalForTest() : nullptr;
	if (!Body)
	{
		return FString();
	}
	// Meat by a fire is cooked and fills most; then eggs, grain, raw meat, milk.
	const UDayLoopSubsystem* Loop = UDayLoopSubsystem::Get(Player->GetWorld());
	const bool bByFire = Loop && Loop->GetTemperatureAt(Player->GetActorLocation()).Fire > 5.f;
	struct FFood { EForestItem Item; float Hunger; float Thirst; const TCHAR* Name; };
	const FFood Menu[] = {
		{ EForestItem::Meat, bByFire ? 45.f : 15.f, 0.f, bByFire ? TEXT("Pişmiş et") : TEXT("Çiğ et") },
		{ EForestItem::Egg, 15.f, 0.f, TEXT("Yumurta") },
		{ EForestItem::Grain, 12.f, 0.f, TEXT("Tahıl") },
		{ EForestItem::Milk, 8.f, 25.f, TEXT("Süt") },
	};
	for (const FFood& Food : Menu)
	{
		if (Player->GetItemCount(Food.Item) > 0 && Player->TestRemoveItem(Food.Item, 1))
		{
			Body->Eat(Food.Hunger);
			Body->Drink(Food.Thirst);
			return Food.Name;
		}
	}
	return FString();
}

bool FarmKit::MakeHideGarment(AForestCharacter* Player)
{
	USurvivalComponent* Body = Player ? Player->GetSurvivalForTest() : nullptr;
	if (!Body || Body->Insulation >= MaxInsulation - 0.01f || Player->GetItemCount(EForestItem::Hide) < HidesPerGarment)
	{
		return false;
	}
	Player->TestRemoveItem(EForestItem::Hide, HidesPerGarment);
	Body->Insulation = FMath::Min(MaxInsulation, Body->Insulation + InsulationPerGarment);
	return true;
}
