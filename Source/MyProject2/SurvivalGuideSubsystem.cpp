// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurvivalGuideSubsystem.h"
#include "Campfire.h"
#include "DayLoopSubsystem.h"
#include "FarmKit.h"
#include "ForestCharacter.h"
#include "ForestTypes.h"
#include "ScenarioSubsystem.h"
#include "StickStructure.h"
#include "StructureLink.h"
#include "SurvivalComponent.h"
#include "WorldFeatures.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "PhaseT2_Plant/CropFieldActor.h"

namespace
{
	struct FGoalDef
	{
		const TCHAR* Text;
		const TCHAR* Hint;
	};

	const FGoalDef Goals[] = {
		{ TEXT("Çubuk ve taş topla (4 çubuk, 7 taş)"), TEXT("Ağaç devir: dalları çubuk olur. Kayalara vur (Kayalık, batı): küçük taşlar [E] ile çantaya, yapı taşı ele") },
		{ TEXT("Ateş yeri kur"), TEXT("Düz yere bak, [C] ile 7 taşı tek tek koy: halka tamamlanınca ateş yeri olur") },
		{ TEXT("Ateşi yak"), TEXT("Bir parça odunu omza al ([E]), ateş yerine bak, [Sol tık]; sonra [F] basılı tut") },
		{ TEXT("Ateşin yanında ısın"), TEXT("Gece soğuk: ateşin 2-3 m yakınında dur, vücut ısısı dolsun") },
		{ TEXT("Yabani tahıldan tohum al"), TEXT("Nehrin öte yakasındaki sarı tarlalara bak, [E]") },
		{ TEXT("Tarla aç ve tohum ek"), TEXT("[I] ile Tarla seç (2 çubuk), yere [Sol tık]; tarlaya bak, [E] ile 4 tohum ek") },
		{ TEXT("Hasat al"), TEXT("Ekin günlerle büyür (admin [9] zamanı ileri alır); olgun tarlaya [E]") },
		{ TEXT("Hayvandan yiyecek ya da deri al"), TEXT("Tavuktan yumurta [E]; koyunu baltayla kes: et ve deri") },
		{ TEXT("Deri giysi giy"), TEXT("[I] ile Deri giysi seç (3 deri), [Sol tık]: soğukta ısı daha yavaş düşer") },
		{ TEXT("İp bük ve ip hattı ger"), TEXT("[I] İp (2 yün ya da 1 deri); sonra [I] İp hattı: yarın dibine ve tepesine tıkla") },
		{ TEXT("İple yara tırman"), TEXT("İpin ucunda [E]: tepeye çıkarsın; tepede iyi taş ve bir not var") },
		{ TEXT("Nehre köprü kur"), TEXT("Kıyıların ikisine de direk dik, üstlerine nehri aşan bir kiriş koy") },
		{ TEXT("Savunma kur: diken ya da çit"), TEXT("[I] ile Çubuk diken ya da Çubuk çit; ya da sivri uçlu kazıklardan duvar") },
		{ TEXT("Önceki yerleşimcilerin 3 notunu oku"), TEXT("Kazıklara asılı kâğıtlar: başlangıçta, nehir kıyısında, yarın dibinde, yanık kulübede") },
		{ TEXT("İlk baskın gecesini atlat (3. gece)"), TEXT("22:00'de gelirler; ateşi besle, kazık ve çit kur, şafağı bekle (admin [Z] hemen başlatır)") },
	};
	constexpr int32 NumGoals = UE_ARRAY_COUNT(Goals);
}

USurvivalGuideSubsystem* USurvivalGuideSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<USurvivalGuideSubsystem>() : nullptr;
}

bool USurvivalGuideSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 USurvivalGuideSubsystem::GetGoalCount()
{
	return NumGoals;
}

bool USurvivalGuideSubsystem::IsActive() const
{
	const UScenarioSubsystem* Scenario = UScenarioSubsystem::Get(GetWorld());
	return Scenario && Scenario->IsLaidOut();
}

int32 USurvivalGuideSubsystem::GetDoneCount() const
{
	int32 Count = 0;
	for (const bool bDone : Done)
	{
		Count += bDone ? 1 : 0;
	}
	return Count;
}

void USurvivalGuideSubsystem::GetLines(TArray<FSurvivalGoalLine>& OutLines, int32& OutCurrent) const
{
	OutLines.Reset();
	OutCurrent = INDEX_NONE;
	for (int32 Index = 0; Index < NumGoals; ++Index)
	{
		FSurvivalGoalLine& Line = OutLines.AddDefaulted_GetRef();
		Line.Text = Goals[Index].Text;
		Line.Hint = Goals[Index].Hint;
		Line.bDone = Done.IsValidIndex(Index) && Done[Index];
		if (!Line.bDone && OutCurrent == INDEX_NONE)
		{
			OutCurrent = Index;
		}
	}
}

void USurvivalGuideSubsystem::Tick(float DeltaSeconds)
{
	PollTimer -= DeltaSeconds;
	if (const AForestCharacter* Player = Cast<AForestCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)); Player && Player->IsRiding())
	{
		bRodeRope = true;
	}
	if (PollTimer <= 0.f)
	{
		PollTimer = 0.5f;
		Poll();
	}
}

void USurvivalGuideSubsystem::Poll()
{
	UWorld* World = GetWorld();
	AForestCharacter* Player = Cast<AForestCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (!World || !Player)
	{
		return;
	}
	if (Done.Num() != NumGoals)
	{
		Done.Init(false, NumGoals);
	}
	auto Mark = [this](int32 Index, bool bCondition)
	{
		Done[Index] = Done[Index] || bCondition;
	};
	auto Has = [Player](EForestItem Item, int32 Count) { return Player->GetItemCount(Item) >= Count; };

	bool bPit = false;
	bool bFire = false;
	for (TActorIterator<ACampfire> It(World); It; ++It)
	{
		bPit = true;
		bFire |= It->IsBurning();
	}
	const USurvivalComponent* Body = Player->GetSurvival();
	const UDayLoopSubsystem* Loop = UDayLoopSubsystem::Get(World);
	const bool bWarmByFire = Body && Loop && Body->GetBodyHeat() >= Body->GetMax() - 2.f && Loop->GetTemperatureAt(Player->GetActorLocation()).Fire > 5.f;

	bool bSown = false;
	for (TActorIterator<ACropFieldActor> It(World); It; ++It)
	{
		// The wild patches are ripe from the start; a field the player sowed is not.
		bSown |= FarmKit::IsSown(*It) && !FarmKit::IsRipe(*It);
	}
	bool bBridge = false;
	bool bDefenceWall = false;
	for (const StructureLink::FFeatureInfo& Feature : StructureLink::Features(World))
	{
		bBridge |= Feature.Kind == EWoodFeatureKind::Bridge;
		bDefenceWall |= Feature.Kind == EWoodFeatureKind::StakeWall || Feature.Kind == EWoodFeatureKind::DefenseWall;
	}
	bool bStickDefence = false;
	for (TActorIterator<AStickStructure> It(World); It; ++It)
	{
		bStickDefence |= It->GetKind() != EStickBuild::Torch;
	}
	const UScenarioSubsystem* Scenario = UScenarioSubsystem::Get(World);

	Mark(0, Has(EForestItem::Stick, 4) && Has(EForestItem::Stone, 7));
	Mark(1, bPit);
	Mark(2, bFire);
	Mark(3, bWarmByFire);
	Mark(4, Has(EForestItem::Seed, 4));
	Mark(5, bSown);
	Mark(6, Has(EForestItem::Grain, 1));
	Mark(7, Has(EForestItem::Meat, 1) || Has(EForestItem::Hide, 1) || Has(EForestItem::Egg, 1));
	Mark(8, Body && Body->Insulation > 0.f);
	Mark(9, ARopeLine::CountAll(World) > 0);
	Mark(10, bRodeRope);
	Mark(11, bBridge);
	Mark(12, bStickDefence || bDefenceWall);
	Mark(13, Scenario && Scenario->GetJournal().Num() >= 3);
	Mark(14, Loop && Loop->GetDay() >= 4 && Body && Body->GetHealth() > 0.f);
}
