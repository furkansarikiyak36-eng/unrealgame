// Copyright Epic Games, Inc. All Rights Reserved.

#include "MechanicsDemo.h"
#include "CarvableLog.h"
#include "ForestCharacter.h"
#include "StructureLink.h"
#include "Campfire.h"
#include "ForestTypes.h"
#include "HarvestableActor.h"
#include "PickupItem.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMechanicsDemo, Log, All);

namespace
{
	/** Same damage as one player axe swing (AForestCharacter::HitDamage). */
	constexpr float AxeDamage = 20.f;
	/** Seconds between demo axe swings, slow enough to watch. */
	constexpr float SwingInterval = 0.5f;
	/** A step that takes longer than this has failed. */
	constexpr float StepTimeout = 20.f;
	constexpr float SearchRadius = 3000.f;
}

AMechanicsDemo::AMechanicsDemo()
{
	PrimaryActorTick.bCanEverTick = true;

	CarveSpot = CreateDefaultSubobject<UArrowComponent>(TEXT("CarveSpot"));
	CarveSpot->ArrowSize = 3.f;
	CarveSpot->ArrowColor = FColor(80, 160, 255);
	CarveSpot->SetHiddenInGame(true);
	RootComponent = CarveSpot;
}

void AMechanicsDemo::BeginPlay()
{
	Super::BeginPlay();

	bPlaced = HasAnyFlags(RF_WasLoaded);
	if (!bPlaced)
	{
		return;
	}
	if (bAutoStart)
	{
		Begin(bAutoPlay ? StartDelay : 0.f);
		if (bAutoPlay)
		{
			Info(FString::Printf(TEXT("Mekanik demosu %.0f sn sonra kendi kendine oynuyor"), StartDelay));
		}
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

void AMechanicsDemo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ACarvableLog::OnLogCarved.Remove(CarvedHandle);
	ACarvableLog::OnLogPlacedAcross.Remove(PlacedHandle);
	ACarvableLog::OnLogStoodUp.Remove(StoodHandle);
	ACarvableLog::OnLogRepaired.Remove(RepairedHandle);
	Super::EndPlay(EndPlayReason);
}

void AMechanicsDemo::Begin(float Delay)
{
	Step = EStep::PickTree;
	StepTime = 0.f;
	Hits = Passed = Failed = CarveIndex = 0;
	Tree = nullptr;
	Trunk = nullptr;
	BigLogs.Reset();
	ExistingBefore.Reset();

	bGuideActive = !bAutoPlay;
	bGuideReady = false;
	bTracking = false;
	PollTime = 0.f;
	FMemory::Memzero(GoalDone, sizeof(GoalDone));
	SeenTrunks.Reset();
	SeenBigLogs.Reset();
	SeenLogs.Reset();
	ACarvableLog::OnLogCarved.Remove(CarvedHandle);
	CarvedHandle = ACarvableLog::OnLogCarved.AddUObject(this, &AMechanicsDemo::HandleLogCarved);
	ACarvableLog::OnLogPlacedAcross.Remove(PlacedHandle);
	PlacedHandle = ACarvableLog::OnLogPlacedAcross.AddUObject(this, &AMechanicsDemo::HandleLogPlacedAcross);
	ACarvableLog::OnLogStoodUp.Remove(StoodHandle);
	StoodHandle = ACarvableLog::OnLogStoodUp.AddUObject(this, &AMechanicsDemo::HandleLogStoodUp);
	ACarvableLog::OnLogRepaired.Remove(RepairedHandle);
	RepairedHandle = ACarvableLog::OnLogRepaired.AddUObject(this, &AMechanicsDemo::HandleLogRepaired);

	Wait(Delay);
	SetActorTickEnabled(true);
}

void AMechanicsDemo::Start(UWorld* World, bool bAuto, bool bQuitWhenDone)
{
	if (!World)
	{
		return;
	}
	for (TActorIterator<AMechanicsDemo> It(World); It; ++It)
	{
		// Restart from the first step: a finished guide, a guide switched to auto play (tests), or a fresh request.
		It->bQuit = It->bQuit || bQuitWhenDone;
		It->bAutoPlay = bAuto;
		if (bAuto)
		{
			// Auto play lays its own raw log; drop the one the guide put down.
			if (ACarvableLog* Old = It->Carvable.Get())
			{
				Old->Destroy();
			}
		}
		It->Begin(0.5f);
		It->Info(bAuto ? TEXT("Mekanik demosu kendi kendine oynuyor") : TEXT("Mekanik demosu baştan başladı"));
		return;
	}
	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;
	if (AMechanicsDemo* Demo = World->SpawnActor<AMechanicsDemo>(AMechanicsDemo::StaticClass(), FTransform::Identity, Params))
	{
		Demo->bQuit = bQuitWhenDone;
		Demo->bAutoPlay = bAuto;
		Demo->FinishSpawning(FTransform::Identity);
		Demo->Begin(0.5f);
		Demo->Info(bAuto ? TEXT("Mekanik demosu kendi kendine oynuyor") : TEXT("Mekanik demosu başladı"));
	}
}

void AMechanicsDemo::Info(const FString& Text)
{
	UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo] %s"), *Text);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor(150, 200, 255), Text);
	}
}

void AMechanicsDemo::Report(bool bPassed, const FString& Text)
{
	bPassed ? ++Passed : ++Failed;
	const FString Line = FString::Printf(TEXT("%s  %s"), bPassed ? TEXT("GEÇTİ") : TEXT("KALDI"), *Text);
	UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo] %s"), *Line);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.f, bPassed ? FColor::Green : FColor::Red, Line);
	}
}

void AMechanicsDemo::Finish()
{
	Step = EStep::Done;
	const FString Summary = FString::Printf(TEXT("SONUÇ: %d geçti, %d kaldı"), Passed, Failed);
	UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo] %s"), *Summary);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.f, Failed == 0 ? FColor::Green : FColor::Orange, Summary);
	}
	if (bQuit)
	{
		FPlatformMisc::RequestExit(false);
	}
	if (bPlaced)
	{
		Info(TEXT("Baştan başlatmak için: konsolda MechanicsDemo, ya da P (admin) sonra L"));
		SetActorTickEnabled(false);
	}
	else
	{
		SetLifeSpan(1.f);
	}
}

void AMechanicsDemo::LookAt(const FVector& Target, float Distance)
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player)
	{
		return;
	}
	FVector Away = Player->GetActorLocation() - Target;
	Away.Z = 0.f;
	if (!Away.Normalize())
	{
		Away = -FVector::ForwardVector;
	}
	FVector Stand = Target + Away * Distance;

	// Stand on the ground, never inside a tree canopy or a log.
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DemoStand), false, Player);
	if (GetWorld()->LineTraceSingleByObjectType(Ground, Stand + FVector(0, 0, 2000.f), Stand - FVector(0, 0, 2000.f),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Stand.Z = Ground.ImpactPoint.Z;
	}
	Stand.Z += Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
	Player->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);

	if (AController* Controller = Player->GetController())
	{
		const FVector Eye = Stand + FVector(0, 0, Player->BaseEyeHeight);
		Controller->SetControlRotation((Target - Eye).Rotation());
	}
}

bool AMechanicsDemo::Hit(AHarvestableActor* Target)
{
	const FBoxSphereBounds Bounds = Target->GetRootComponent()->Bounds;
	// Low on a standing tree, mid-height on a log lying down.
	const float Height = Target->IsTree() ? 100.f : Bounds.BoxExtent.Z;
	FHitResult HitResult;
	HitResult.ImpactPoint = FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Bounds.BoxExtent.Z + Height) - ChopDirection * 20.f;
	HitResult.ImpactNormal = -ChopDirection;
	++Hits;
	return Target->ApplyHit(AxeDamage, HitResult, ChopDirection);
}

void AMechanicsDemo::FindBigLogs()
{
	BigLogs.Reset();
	for (TActorIterator<AHarvestableActor> It(GetWorld()); It; ++It)
	{
		if (It->IsTrunk() && It->GetLogItem() == EForestItem::BigLog && !ExistingBefore.Contains(*It)
			&& FVector::Dist2D(It->GetActorLocation(), TreeBase) < SearchRadius)
		{
			BigLogs.Add(*It);
		}
	}
}

ACarvableLog* AMechanicsDemo::SpawnCarvable(ACharacter* Player)
{
	const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();
	FVector Spot = bPlaced ? GetActorLocation() : Player->GetActorLocation() + Forward * 250.f;
	const float Yaw = bPlaced ? GetActorRotation().Yaw : Forward.Rotation().Yaw + 90.f;
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DemoCarve), false, Player);
	if (GetWorld()->LineTraceSingleByChannel(Ground, Spot + FVector(0, 0, 500.f), Spot - FVector(0, 0, 1500.f), ECC_Visibility, Params))
	{
		Spot.Z = Ground.ImpactPoint.Z + 40.f;
	}
	UStaticMesh* Raw = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Greybox/Meshes/SM_GB_RawLog_375.SM_GB_RawLog_375"));
	return ACarvableLog::SpawnFromStaticMesh(GetWorld(), Raw, FTransform(FRotator(0.f, Yaw, 0.f), Spot));
}

namespace
{
	struct FGoalText
	{
		const TCHAR* Text;
		const TCHAR* Hint;
	};

	const FGoalText GoalTexts[] = {
		{ TEXT("Bir ağacı kes"), TEXT("Ağaca bak, sol tıkla balta vur  (çam 6, geniş ağaç 8 vuruş)") },
		{ TEXT("Yerdeki gövdeyi ikiye böl"), TEXT("Gövdeye bak ve vur  (4 vuruş)") },
		{ TEXT("Büyük kütükleri kes: 4 kütük"), TEXT("Her büyük kütüğe vur  (3'er vuruş)") },
		{ TEXT("Oyma: çentik"), TEXT("Bir kütüğe bak, [R] ile Çentik seç, aynı yere 2 vuruş") },
		{ TEXT("Oyma: oval çentik"), TEXT("[R] Oval çentik, kütük ucuna bak  |  [Tekerlek] derinlik  |  2 vuruş") },
		{ TEXT("Oyma: düz çentik"), TEXT("[R] Düz çentik: köşe, orta ya da tam uç (uç geçmesi)  |  [Tekerlek] derinlik  |  2 vuruş") },
		{ TEXT("Oyma: enine kesim"), TEXT("[R] Enine kesim: 65 cm'lik modül çizgilerinden birine bak  |  2 vuruş") },
		{ TEXT("Oyma: boyuna yarma"), TEXT("[R] Boyuna yarma  |  [Tekerlek] dikey/yatay  |  2 vuruş") },
		{ TEXT("Uç: sivriltme (kazık)"), TEXT("[R] Sivri uç, kütüğün ucuna bak, 2 vuruş") },
		{ TEXT("Uç: pah kırma"), TEXT("[R] Pah, diğer ucuna bak, 2 vuruş") },
		{ TEXT("Uç: düzeltme"), TEXT("[R] Uç düzeltme, sivri ya da pahlı uca bak, 2 vuruş (sivri uç geri kesilir)") },
		{ TEXT("Kırlangıç kuyruğu aç"), TEXT("[R] Kırlangıç, kütüğün bir geçme noktasına bak, 2 vuruş") },
		{ TEXT("Köşe: kütüğü çaprazlama yerleştir"), TEXT("Bir kütüğü omzuna al (E), yerdeki bir kütüğe bak, [Sol tık]: üstüne dik oturur, çentik kendiliğinden açılır") },
		{ TEXT("Direk: bir parçayı dik koy"), TEXT("Bir parçayı omzuna al (E), [R] ile Dik seç, yere bak, [Sol tık]") },
		{ TEXT("Payanda: direk ile kirişin arasına"), TEXT("Direğe kiriş koy (2 direk), 2-4 modül boyunda bir parçayla direğe bak, [Sol tık]: 45° payanda") },
		{ TEXT("Platform kur"), TEXT("4 direk, 2 kiriş, üstüne yarım kütük (boyuna yarılmış) döşeme tahtaları: yarım kütükle kirişe bak, [Sol tık]") },
		{ TEXT("Kapı kasası: iki duvar arası boşluk + lento"), TEXT("Üçer direkli iki duvar, arada 100 cm; üstüne uzun kütük koy") },
		{ TEXT("Sivri kazık duvarı"), TEXT("Direklerin ucunu sivriltip 3-4 tanesini yan yana dik") },
		{ TEXT("Ateş yak"), TEXT("Küçük bir parçayı omzuna al, [R] ile Ateş modu, yere bak, [Sol tık]; ateşe bakıp daha çok odun at") },
		{ TEXT("Hasarlı parçayı onar"), TEXT("Yapıdaki yanık parçaya (ateşin yanında ya da [Y] admin) aynı boyda sağlam parçayla bak, [Sol tık]") },
	};
	static_assert(UE_ARRAY_COUNT(GoalTexts) == 20, "one text per goal");
}

void AMechanicsDemo::GetGuide(TArray<FDemoGoalLine>& OutLines, FString& OutHint) const
{
	OutLines.Reset();
	OutHint.Reset();
	bool bCurrentFound = false;
	for (int32 Goal = 0; Goal < GoalCount; ++Goal)
	{
		FDemoGoalLine& Line = OutLines.AddDefaulted_GetRef();
		Line.Text = GoalTexts[Goal].Text;
		if (Goal == GoalSplitTrunk)
		{
			Line.Text += FString::Printf(TEXT("  (%d/2)"), FMath::Min(SeenBigLogs.Num(), 2));
		}
		else if (Goal == GoalSplitBigLogs)
		{
			Line.Text += FString::Printf(TEXT("  (%d/4)"), FMath::Min(SeenLogs.Num(), 4));
		}
		Line.bDone = GoalDone[Goal];
		if (!Line.bDone && !bCurrentFound)
		{
			Line.bCurrent = true;
			bCurrentFound = true;
			OutHint = GoalTexts[Goal].Hint;
		}
	}
	if (!bCurrentFound)
	{
		OutHint = TEXT("Hepsi tamam!  Baştan başlatmak için: P (admin) sonra L, ya da konsolda MechanicsDemo");
	}
}

void AMechanicsDemo::CompleteGoal(int32 Goal)
{
	if (GoalDone[Goal])
	{
		return;
	}
	GoalDone[Goal] = true;
	UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo] Görev tamam: %s"), GoalTexts[Goal].Text);

	for (const bool bDone : GoalDone)
	{
		if (!bDone)
		{
			return;
		}
	}
	UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo] Bütün görevler tamam"));
}

void AMechanicsDemo::HandleLogPlacedAcross(ACarvableLog* Upper)
{
	CompleteGoal(GoalPlaceAcross);
}

void AMechanicsDemo::HandleLogRepaired(ACarvableLog* Piece)
{
	CompleteGoal(GoalRepair);
}

void AMechanicsDemo::HandleLogStoodUp(ACarvableLog* Post)
{
	CompleteGoal(GoalStand);
}

TArray<ACarvableLog*> AMechanicsDemo::FreshLogs() const
{
	TArray<ACarvableLog*> Logs;
	for (TActorIterator<ACarvableLog> It(GetWorld()); It; ++It)
	{
		if (It->IsFullLog() && It->GetCutSpans().Num() == 0 && !It->IsAnchored() && !It->IsCarried() && !ExistingBefore.Contains(*It))
		{
			Logs.Add(*It);
		}
	}
	return Logs;
}

void AMechanicsDemo::HandleLogCarved(ACarvableLog* Log, ECarveTool Tool)
{
	// The saddle notch of a corner is cut by laying the log, not by the player's axe: it does not count as carving.
	if (Log && Log->IsAnchored())
	{
		return;
	}
	switch (Tool)
	{
	case ECarveTool::Notch:      CompleteGoal(GoalNotch); break;
	case ECarveTool::NotchRound: CompleteGoal(GoalNotchRound); break;
	case ECarveTool::NotchFlat:  CompleteGoal(GoalNotchFlat); break;
	case ECarveTool::CrossCut:   CompleteGoal(GoalCrossCut); break;
	case ECarveTool::Split:      CompleteGoal(GoalSplit); break;
	case ECarveTool::Point:      CompleteGoal(GoalPoint); break;
	case ECarveTool::Chamfer:    CompleteGoal(GoalChamfer); break;
	case ECarveTool::Trim:       CompleteGoal(GoalTrim); break;
	case ECarveTool::Dovetail:   CompleteGoal(GoalDovetail); break;
	default: break;
	}
}

void AMechanicsDemo::TickGuided(float DeltaSeconds)
{
	if (WaitTime > 0.f)
	{
		WaitTime -= DeltaSeconds;
		return;
	}
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player)
	{
		return;
	}
	UWorld* World = GetWorld();

	if (!bGuideReady)
	{
		// Whatever already lies around does not count; only what the player makes from here on.
		for (TActorIterator<AHarvestableActor> It(World); It; ++It)
		{
			ExistingBefore.Add(*It);
		}
		for (TActorIterator<APickupItem> It(World); It; ++It)
		{
			ExistingBefore.Add(*It);
		}
		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			ExistingBefore.Add(*It);
		}
		// A restart brings a fresh raw log; cut pieces of the old one stay where they are.
		if (ACarvableLog* Old = Carvable.Get())
		{
			Old->Destroy();
		}
		Carvable = SpawnCarvable(Player);
		bGuideReady = true;
		bTracking = true;
		return;
	}
	TrackGoals(DeltaSeconds);
}

void AMechanicsDemo::TrackGoals(float DeltaSeconds)
{
	if (!bTracking)
	{
		return;
	}
	UWorld* World = GetWorld();

	// The tree and log steps are read from the world a few times a second.
	PollTime -= DeltaSeconds;
	if (PollTime > 0.f)
	{
		return;
	}
	PollTime = 0.25f;
	for (TActorIterator<AHarvestableActor> It(World); It; ++It)
	{
		if (It->IsTrunk() && !ExistingBefore.Contains(*It))
		{
			(It->GetLogItem() == EForestItem::TrunkLog ? SeenTrunks : SeenBigLogs).Add(*It);
		}
	}
	for (TActorIterator<ACarvableLog> It(World); It; ++It)
	{
		if (It->IsFullLog() && !ExistingBefore.Contains(*It))
		{
			SeenLogs.Add(*It);
		}
	}
	if (SeenTrunks.Num() > 0)
	{
		CompleteGoal(GoalFell);
	}
	if (SeenBigLogs.Num() >= 2)
	{
		CompleteGoal(GoalSplitTrunk);
	}
	if (SeenLogs.Num() >= 4)
	{
		CompleteGoal(GoalSplitBigLogs);
	}

	// What the player has built or lit: structures read by the Wood rules, braces, fires.
	for (const StructureLink::FFeatureInfo& Info : StructureLink::Features(World))
	{
		bool bNew = false;
		for (const TWeakObjectPtr<ACarvableLog>& Part : Info.Pieces)
		{
			bNew |= Part.IsValid() && !ExistingBefore.Contains(Part.Get());
		}
		if (!bNew)
		{
			continue;
		}
		switch (Info.Kind)
		{
		case EWoodFeatureKind::Platform:  CompleteGoal(GoalPlatform); break;
		case EWoodFeatureKind::DoorFrame: CompleteGoal(GoalDoor); break;
		case EWoodFeatureKind::StakeWall:
		case EWoodFeatureKind::DefenseWall: CompleteGoal(GoalStakeWall); break;
		default: break;
		}
	}
	for (TActorIterator<ACarvableLog> It(World); It; ++It)
	{
		if (It->IsAnchored() && !ExistingBefore.Contains(*It))
		{
			const float Tilt = FMath::RadiansToDegrees(FMath::Asin(FMath::Abs(It->GetActorTransform().GetUnitAxis(EAxis::X).Z)));
			if (Tilt > 35.f && Tilt < 55.f)
			{
				CompleteGoal(GoalBrace);
			}
		}
	}
	for (TActorIterator<ACampfire> It(World); It; ++It)
	{
		if (It->IsBurning())
		{
			CompleteGoal(GoalFire);
		}
	}
}

void AMechanicsDemo::CheckGuide(const ACarvableLog* Log, ECarveTool Tool, float X, float Angle, float Width, float Depth, const TCHAR* Name)
{
	TArray<TArray<FVector>> Lines;
	TArray<bool> Closed;
	Log->GetCutGuide(Tool, X, 0.f, Angle, Width, Depth, Lines, Closed);
	int32 Points = 0;
	bool bOnBark = true;
	float MinAlong = TNumericLimits<float>::Max();
	float MaxAlong = TNumericLimits<float>::Lowest();
	for (const TArray<FVector>& Line : Lines)
	{
		for (const FVector& Point : Line)
		{
			++Points;
			MinAlong = FMath::Min(MinAlong, Point.X);
			MaxAlong = FMath::Max(MaxAlong, Point.X);
			// Bark lines sit just outside the radius; the split also crosses the end faces, inside it.
			const float FromAxis = FVector2D(Point.Y, Point.Z).Size();
			bOnBark &= FromAxis <= Log->GetRadius() + 2.f && (Tool == ECarveTool::Split || FromAxis >= Log->GetRadius() - 0.5f);
		}
	}
	Report(Lines.Num() > 0 && bOnBark, FString::Printf(TEXT("Kesim çizgisi (%s): %d çizgi, %d nokta, kütük boyunca %.0f cm%s"),
		Name, Lines.Num(), Points, Points > 0 ? MaxAlong - MinAlong : 0.f, bOnBark ? TEXT("") : TEXT(", yüzeyde değil")));
}

bool AMechanicsDemo::RunCarveStep(int32 Index)
{
	ACarvableLog* Log = Carvable.Get();
	if (!Log)
	{
		Report(false, TEXT("Oyma: kütük kayboldu"));
		return false;
	}
	const float C = Log->GetCenterX();
	const float R = Log->GetRadius();
	const int32 TrisBefore = Log->GetTriangleCount();
	const float LengthBefore = Log->GetLength();

	// Joints are as long as the log is thick and a sixth of it deep, as the player starts; every cut goes on a snap point.
	const float W = Log->GetDiameter();
	const float D = Log->GetDiameter() * AForestCharacter::GetCarveDepthFraction(0);
	auto PointsText = [Log, W](ECarveTool Tool)
	{
		TArray<float> Points;
		Log->GetSnapPoints(Tool, W, Points);
		FString Text;
		for (const float X : Points)
		{
			Text += FString::Printf(TEXT(" %.0f"), X - Log->GetCenterX());
		}
		return Text.IsEmpty() ? FString(TEXT(" yok")) : Text;
	};
	float X = 0.f;

	switch (Index)
	{
	case 0:
		Info(FString::Printf(TEXT("Snap noktaları (merkeze göre cm)  çentik:%s  |  enine:%s"),
			*PointsText(ECarveTool::Notch), *PointsText(ECarveTool::CrossCut)));
		if (!Log->FindSnapPoint(ECarveTool::Notch, Log->GetMinX(), W, X))
		{
			Report(false, TEXT("Çentik için snap noktası yok"));
			return true;
		}
		CheckGuide(Log, ECarveTool::Notch, X, 90.f, 0.f, 0.f, TEXT("çentik, beklenen ~çap"));
		Log->Notch(X, 0.f, 90.f, R, R);
		Report(Log->GetTriangleCount() > TrisBefore, FString::Printf(TEXT("Çentik uçtaki köşe noktasında (%.0f) açıldı  -  üçgen %d -> %d"),
			X - C, TrisBefore, Log->GetTriangleCount()));
		return true;
	case 1:
		if (!Log->FindSnapPoint(ECarveTool::NotchRound, C, W, X))
		{
			Report(false, TEXT("Oval çentik için snap noktası yok"));
			return true;
		}
		CheckGuide(Log, ECarveTool::NotchRound, X, 0.f, W, D, TEXT("oval, beklenen < genişlik"));
		Log->CutProfile(X, 0.f, W, D, true);
		Report(Log->GetTriangleCount() > TrisBefore, FString::Printf(TEXT("Oval çentik ortada (%.0f), %.0f cm x %.1f cm  -  üçgen %d -> %d"),
			X - C, W, D, TrisBefore, Log->GetTriangleCount()));
		return true;
	case 2:
	{
		Info(FString::Printf(TEXT("Düz çentik snap noktaları:%s"), *PointsText(ECarveTool::NotchFlat)));
		if (!Log->FindSnapPoint(ECarveTool::NotchFlat, Log->GetMaxX(), W, X))
		{
			Report(false, TEXT("Düz çentik için snap noktası yok"));
			return true;
		}
		CheckGuide(Log, ECarveTool::NotchFlat, X, 0.f, W, D, TEXT("düz, beklenen = genişlik"));
		Log->CutProfile(X, 0.f, W, D, false);
		// The point nearest the end is the end lap itself: the cut runs out through the end, the log keeps its length.
		const bool bEndLap = FMath::IsNearlyEqual(X + W * 0.5f, Log->GetMaxX(), 1.f);
		Report(bEndLap && Log->GetTriangleCount() > TrisBefore && FMath::IsNearlyEqual(Log->GetLength(), LengthBefore, 1.f),
			FString::Printf(TEXT("Düz çentik uçta, uç geçmesi (%.0f), %.0f cm x %.1f cm, boy %.0f cm  -  üçgen %d -> %d"),
			X - C, W, D, Log->GetLength(), TrisBefore, Log->GetTriangleCount()));
		// All three joint places are cut now: no fourth joint, and no cross cut inside a joint.
		TArray<float> Left;
		Log->GetSnapPoints(ECarveTool::Notch, W, Left);
		Report(Left.Num() == 0, FString::Printf(TEXT("Dolu yerlere yeni çentik açılamıyor (kalan çentik noktası: %d)"), Left.Num()));
		return true;
	}
	case 3:
	{
		// Two cross cuts on the module grid: the second one on the cut-off piece must land on the same grid.
		const FString CrossPoints = PointsText(ECarveTool::CrossCut);
		const float M = ACarvableLog::ModuleLength;
		if (!Log->FindSnapPoint(ECarveTool::CrossCut, Log->GetMinX() + 2.f * M, W, X))
		{
			Report(false, TEXT("Enine kesim için modül çizgisi yok"));
			return true;
		}
		CheckGuide(Log, ECarveTool::CrossCut, X, 0.f, 0.f, 0.f, TEXT("enine, beklenen 0"));
		const float Origin = Log->GetMinX();
		ACarvableLog* Piece = Log->CrossCut(X);
		auto Modules = [M](float Length) { return Length / M; };
		auto OnGrid = [M](float Length) { return FMath::Abs(Length / M - FMath::RoundToFloat(Length / M)) < 0.03f; };
		bool bOk = Piece && OnGrid(Log->GetLength()) && OnGrid(Piece->GetLength());
		FString Text = FString::Printf(TEXT("Enine kesim modül çizgisinde (açık çizgiler:%s): %.0f -> %.0f + %.0f cm (%.1fM + %.1fM)"),
			*CrossPoints, LengthBefore, Log->GetLength(), Piece ? Piece->GetLength() : 0.f,
			Modules(Log->GetLength()), Piece ? Modules(Piece->GetLength()) : 0.f);
		// The piece is cut again: its grid lines are the log's, counted from the original end, not from its own.
		float Second = 0.f;
		if (Piece && Piece->FindSnapPoint(ECarveTool::CrossCut, Piece->GetCenterX(), W, Second))
		{
			const bool bSameGrid = OnGrid(Second - Origin);
			const float PieceBefore = Piece->GetLength();
			const ACarvableLog* Third = Piece->CrossCut(Second);
			bOk &= bSameGrid && Third && OnGrid(Piece->GetLength()) && OnGrid(Third->GetLength())
				&& FMath::IsNearlyEqual(Piece->GetLength() + Third->GetLength(), PieceBefore, 1.f);
			Text += FString::Printf(TEXT(", parça tekrar: %.0f -> %.0f + %.0f cm, çizgi uçtan %.1fM"),
				PieceBefore, Piece->GetLength(), Third ? Third->GetLength() : 0.f, (Second - Origin) / M);
		}
		else
		{
			bOk = false;
			Text += TEXT(", parçada kesim çizgisi yok");
		}
		Report(bOk, Text);
		return true;
	}
	case 4:
	case 5:
	{
		// The split goes through the player's own aim and two-hit logic, a second and a half apart while the log settles.
		AForestCharacter* Character = Cast<AForestCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		if (!Character)
		{
			Report(false, TEXT("Oyuncu karakteri yok"));
			return true;
		}
		if (Index == 4)
		{
			CheckGuide(Log, ECarveTool::Split, 0.f, 0.f, 0.f, 0.f, TEXT("yarma, beklenen tüm boy"));
		}
		const FVector Top = Log->GetActorTransform().TransformPosition(FVector(C, 0.f, R));
		int32 Pieces = 0;
		for (TActorIterator<ACarvableLog> It(GetWorld()); It; ++It)
		{
			++Pieces;
		}
		const bool bCut = Character->TestCarveHit(Log, ECarveTool::Split, Top, FVector::UpVector);
		UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo]   vuruş %d: kütük %s, dönüş %s"), Index - 3,
			Log->GetRootComponent()->IsSimulatingPhysics() ? TEXT("hareket ediyor") : TEXT("sabit"), *Log->GetActorRotation().ToCompactString());
		int32 PiecesAfter = 0;
		for (TActorIterator<ACarvableLog> It(GetWorld()); It; ++It)
		{
			++PiecesAfter;
		}
		const bool bSplit = bCut || PiecesAfter > Pieces;
		if (Index == 4)
		{
			Report(!bSplit, TEXT("Oyuncu vuruşu 1: kesim izi açıldı, henüz kesmedi"));
		}
		else
		{
			Report(bSplit, FString::Printf(TEXT("Oyuncu vuruşu 2 (1.5 sn sonra, kütük fizikle oturuyorken): boyuna yarıldı  -  parça %d -> %d"), Pieces, PiecesAfter));
		}
		return true;
	}
	default:
		return false;
	}
}

namespace
{
	/** Clear ground of the demo level for the structure test, away from trees and the carving spot. */
	const FVector FrameArea(-1600.f, 1400.f, 0.f);

	FHitResult GroundAt(UWorld* World, const FVector& Where)
	{
		FHitResult Hit;
		World->LineTraceSingleByObjectType(Hit, Where + FVector(0, 0, 1000.f), Where - FVector(0, 0, 2000.f),
			FCollisionObjectQueryParams(ECC_WorldStatic));
		return Hit;
	}

	/** As if the player aimed at the top end of a post. */
	FHitResult AimAtTop(ACarvableLog* Post)
	{
		FVector Centre, Axis;
		float Radius, Half;
		ACarvableLog::GetLogAxis(Post, Centre, Axis, Radius, Half);
		FHitResult Hit;
		Hit.HitObjectHandle = FActorInstanceHandle(Post);
		Hit.ImpactPoint = Centre + Axis * (Axis.Z > 0.f ? Half : -Half);
		Hit.ImpactNormal = FVector::UpVector;
		return Hit;
	}
}

bool AMechanicsDemo::RunFrameStage(AForestCharacter* Character)
{
	UWorld* World = GetWorld();
	const float M = ACarvableLog::ModuleLength;
	// The player faces +X from the west of the area for every stage: placement directions follow the view.
	Character->SetActorLocation(FrameArea + FVector(-500.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
	Character->SetActorRotation(FRotator::ZeroRotator);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(FRotator::ZeroRotator);
	}
	auto Valid = [this]() { for (const TWeakObjectPtr<ACarvableLog>& Log : FrameLogs) { if (!Log.IsValid()) { return false; } } return true; };

	switch (FrameStage)
	{
	case 0:
	{
		// Five whole logs on clear ground, lying north-south, to settle before the test.
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FVector Spot = FrameArea + FVector(0.f, 900.f + Index * 120.f, 60.f);
			FrameLogs.Add(ACarvableLog::SpawnLog(World, FTransform(FRotator(0.f, 90.f, 0.f), Spot)));
		}
		Wait(3.f);
		return true;
	}
	case 1:
	{
		if (!Valid())
		{
			Report(false, TEXT("Yapı testi: kütükler oluşmadı"));
			return false;
		}
		// Corner: two lower logs, parallel, four modules apart; a third laid square across both.
		ACarvableLog* LowerA = FrameLogs[0].Get();
		ACarvableLog* LowerB = FrameLogs[1].Get();
		LowerA->PlaceAt(LowerA->GetLayTransform(GroundAt(World, FrameArea).ImpactPoint, 90.f), false);
		LowerB->PlaceAt(LowerB->GetLayTransform(GroundAt(World, FrameArea + FVector(4.f * M, 0.f, 0.f)).ImpactPoint, 90.f), false);
		Wait(2.5f);
		return true;
	}
	case 2:
	{
		ACarvableLog* LowerA = FrameLogs[0].Get();
		ACarvableLog* Upper = FrameLogs[2].Get();
		// The lower logs have to lie still first (a log still rolling is not something to build on).
		if ((LowerA->GetRootComponent()->IsSimulatingPhysics() || FrameLogs[1]->GetRootComponent()->IsSimulatingPhysics()) && StepTime < 10.f)
		{
			bRepeatFrameStage = true;
			Wait(0.5f);
			return true;
		}
		Info(FString::Printf(TEXT("Köşe alt kütükleri bırakıldıkları yerden kaydı: A %.0f cm, B %.0f cm (aralarında %.0f cm, hedef %.0f)"),
			FVector::Dist2D(LowerA->GetActorLocation(), FrameArea), FVector::Dist2D(FrameLogs[1]->GetActorLocation(), FrameArea + FVector(4.f * ACarvableLog::ModuleLength, 0.f, 0.f)),
			FVector::Dist2D(LowerA->GetActorLocation(), FrameLogs[1]->GetActorLocation()), 4.f * ACarvableLog::ModuleLength));
		FVector Centre, Axis;
		float Radius, Half;
		ACarvableLog::GetLogAxis(LowerA, Centre, Axis, Radius, Half);
		FHitResult Hit;
		Hit.HitObjectHandle = FActorInstanceHandle(LowerA);
		Hit.ImpactPoint = Centre + FVector(0.f, 0.f, Radius);
		Hit.ImpactNormal = FVector::UpVector;
		const bool bCarried = Character->TestPickUp(Upper);
		const bool bCornerPlaced = bCarried && Character->TestPlace(false, Hit);
		const int32 Notches = Upper->GetCutSpans().Num();
		Report(bCornerPlaced && Notches == 2 && Upper->IsAnchored() && FrameLogs[1]->IsAnchored(),
			FString::Printf(TEXT("Köşe: iki alt kütüğe çaprazlama kondu, %d çentik açıldı, kütük %s"), Notches,
				Upper->IsAnchored() ? TEXT("duruyor (iki destek)") : TEXT("düştü")));
		CornerUpper = Upper;
		CornerUpperStart = Upper->GetActorLocation();
		Wait(2.f);
		return true;
	}
	case 3:
	{
		Report(CornerUpper.IsValid() && CornerUpper->IsAnchored() && FVector::Dist(CornerUpper->GetActorLocation(), CornerUpperStart) < 1.f,
			TEXT("Köşe 2 sn sonra yerinde"));
		// Posts: one whole log cut into three two-module pieces on the grid.
		ACarvableLog* Log = FrameLogs[3].Get();
		float X = 0.f;
		Log->FindSnapPoint(ECarveTool::CrossCut, Log->GetMinX() + 2.f * M, Log->GetDiameter(), X);
		ACarvableLog* Rest = Log->CrossCut(X);
		float Y = 0.f;
		ACarvableLog* Third = Rest && Rest->FindSnapPoint(ECarveTool::CrossCut, Rest->GetMinX() + 2.f * M, Rest->GetDiameter(), Y) ? Rest->CrossCut(Y) : nullptr;
		FramePostA = Log;
		FramePostB = Rest;
		Report(Rest && Third && FMath::IsNearlyEqual(Log->GetLength(), 2.f * M, 1.f) && FMath::IsNearlyEqual(Rest->GetLength(), 2.f * M, 1.f),
			FString::Printf(TEXT("Direkler için kütük 2M parçalara kesildi: %.0f, %.0f, %.0f cm"), Log->GetLength(),
				Rest ? Rest->GetLength() : 0.f, Third ? Third->GetLength() : 0.f));
		Wait(2.5f);
		return true;
	}
	case 4:
	{
		// One post, a beam laid on its top: nothing holds the other end, it has to fall.
		ACarvableLog* PostA = FramePostA.Get();
		ACarvableLog* Beam = FrameLogs[4].Get();
		const FVector SpotA = FrameArea + FVector(0.f, -900.f, 0.f);
		const bool bPost = Character->TestPickUp(PostA) && Character->TestPlace(true, GroundAt(World, SpotA));
		const bool bBeam = bPost && Character->TestPickUp(Beam) && Character->TestPlace(false, AimAtTop(PostA));
		FrameBeam = Beam;
		FrameBeamZ = Beam->GetActorLocation().Z;
		Report(bPost && bBeam && PostA->IsAnchored() && !Beam->IsAnchored(),
			FString::Printf(TEXT("Tek direk + kiriş: direk %s, kiriş %s"), PostA->IsAnchored() ? TEXT("duruyor") : TEXT("düştü"),
				Beam->IsAnchored() ? TEXT("duruyor (yanlış)") : TEXT("desteksiz, fiziğe bırakıldı")));
		Wait(3.f);
		return true;
	}
	case 5:
	{
		ACarvableLog* Beam = FrameBeam.Get();
		Report(Beam && Beam->GetActorLocation().Z < FrameBeamZ - 30.f,
			FString::Printf(TEXT("Kiriş yere düştü (%.0f cm aşağı)"), Beam ? FrameBeamZ - Beam->GetActorLocation().Z : 0.f));
		// Second post under the far end of the beam's reach; the beam goes back up and spans both.
		ACarvableLog* PostB = FramePostB.Get();
		const float Reach = 240.f;
		const FVector SpotB = FrameArea + FVector(Reach, -900.f, 0.f);
		const bool bPost = Character->TestPickUp(PostB) && Character->TestPlace(true, GroundAt(World, SpotB));
		const bool bBeam = bPost && Character->TestPickUp(Beam) && Character->TestPlace(false, AimAtTop(FramePostA.Get()));
		FrameBeamZ = Beam->GetActorLocation().Z;
		Report(bPost && bBeam && Beam->IsAnchored(),
			FString::Printf(TEXT("İki direk + kiriş: kiriş %s"), Beam->IsAnchored() ? TEXT("duruyor (iki destek)") : TEXT("düştü (yanlış)")));
		Wait(2.f);
		return true;
	}
	case 6:
	{
		ACarvableLog* Beam = FrameBeam.Get();
		Report(Beam && Beam->IsAnchored() && FMath::IsNearlyEqual(Beam->GetActorLocation().Z, FrameBeamZ, 1.f), TEXT("Kiriş 2 sn sonra yerinde"));
		// Taking the far post out: the beam has one support left and comes down.
		const bool bTaken = Character->TestPickUp(FramePostB.Get());
		Report(bTaken && Beam && !Beam->IsAnchored(),
			FString::Printf(TEXT("Direk söküldü (omuzda): kiriş %s"), Beam && !Beam->IsAnchored() ? TEXT("desteksiz kaldı, düşüyor") : TEXT("hâlâ duruyor (yanlış)")));
		Wait(3.f);
		return true;
	}
	case 7:
	{
		ACarvableLog* Beam = FrameBeam.Get();
		Report(Beam && Beam->GetActorLocation().Z < FrameBeamZ - 30.f, TEXT("Kiriş yere indi"));
		// Put the post down again so the shoulder is free for the next steps.
		Character->TestPlace(false, GroundAt(World, FrameArea + FVector(0.f, -1400.f, 0.f)));
		return false;
	}
	default:
		return false;
	}
}

void AMechanicsDemo::RunShaping(AForestCharacter* Character)
{
	UWorld* World = GetWorld();
	const FVector Area(-2400.f, -2200.f, 0.f);
	int32 Count = 0;
	// A whole log held still in the air (this tests shapes, not physics).
	auto Fresh = [&]()
	{
		ACarvableLog* Log = ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(0.f, 220.f * Count++, 80.f)));
		Log->Anchor();
		return Log;
	};
	auto Aim = [](ACarvableLog* Log, float LocalX) { return Log->GetActorTransform().TransformPosition(FVector(LocalX, 0.f, Log->GetRadius())); };
	// Wood taken off, as a share of the volume of a log of that radius and length.
	auto Disc = [](float R, float L) { return PI * R * R * L; };
	auto Frustum = [](float R0, float R1, float H) { return PI * H / 3.f * (R0 * R0 + R0 * R1 + R1 * R1); };

	if (!Character)
	{
		Report(false, TEXT("Şekillendirme testi için karakter yok"));
		return;
	}

	// Stake: two hits through the player's own aim and two-hit logic.
	ACarvableLog* Stake = Fresh();
	const float R = Stake->GetRadius();
	const float L = Stake->GetLength();
	const float V0 = Stake->GetVolume();
	Character->TestCarveHit(Stake, ECarveTool::Point, Aim(Stake, Stake->GetMinX() + 10.f), FVector::UpVector);
	const bool bStaked = Character->TestCarveHit(Stake, ECarveTool::Point, Aim(Stake, Stake->GetMinX() + 10.f), FVector::UpVector);
	const float Removed = V0 - Stake->GetVolume();
	const float Expected = Disc(R, ACarvableLog::PointLength) - Frustum(ACarvableLog::PointTipRadius, R, ACarvableLog::PointLength);
	Report(bStaked && Stake->GetEnd(false) == ELogEnd::Pointed && Stake->GetEnd(true) == ELogEnd::Flat
			&& FMath::IsNearlyEqual(Stake->GetLength(), L, 0.5f) && FMath::Abs(Removed - Expected) < 0.08f * Expected,
		FString::Printf(TEXT("Sivri uç: kazık oluştu, boy %.0f cm aynı, giden odun %.0f cm3 (beklenen %.0f)"), Stake->GetLength(), Removed, Expected));

	// Chamfer on the other end.
	const float VBefore = Stake->GetVolume();
	Character->TestCarveHit(Stake, ECarveTool::Chamfer, Aim(Stake, Stake->GetMaxX() - 10.f), FVector::UpVector);
	const bool bChamfered = Character->TestCarveHit(Stake, ECarveTool::Chamfer, Aim(Stake, Stake->GetMaxX() - 10.f), FVector::UpVector);
	const float Bevel = VBefore - Stake->GetVolume();
	const float BevelExpected = Disc(R, ACarvableLog::ChamferSize) - Frustum(R - ACarvableLog::ChamferSize, R, ACarvableLog::ChamferSize);
	Report(bChamfered && Stake->GetEnd(true) == ELogEnd::Chamfered && Stake->GetEnd(false) == ELogEnd::Pointed
			&& FMath::Abs(Bevel - BevelExpected) < 0.25f * BevelExpected,
		FString::Printf(TEXT("Pah: uç pahlandı, giden odun %.0f cm3 (beklenen %.0f)"), Bevel, BevelExpected));

	// A shaped end cannot be shaped again, and a flat one cannot be trimmed.
	TArray<float> Again;
	Stake->GetSnapPoints(ECarveTool::Point, 0.f, Again);
	TArray<float> TrimPoints;
	Stake->GetSnapPoints(ECarveTool::Trim, 0.f, TrimPoints);
	Report(Again.Num() == 0 && TrimPoints.Num() == 2, FString::Printf(TEXT("Şekilli uca yeni şekil verilemiyor (%d nokta), düzeltme iki uçta açık (%d nokta)"), Again.Num(), TrimPoints.Num()));

	// Trim the stake's point: cut back to a flat end exactly one module in, on the grid.
	const float LengthBeforeTrim = Stake->GetLength();
	Character->TestCarveHit(Stake, ECarveTool::Trim, Aim(Stake, Stake->GetMinX() + 10.f), FVector::UpVector);
	const bool bTrimmed = Character->TestCarveHit(Stake, ECarveTool::Trim, Aim(Stake, Stake->GetMinX() + 10.f), FVector::UpVector);
	const float OnGrid = (Stake->GetMinX() - Stake->GetGridOriginX()) / ACarvableLog::ModuleLength;
	Report(bTrimmed && Stake->GetEnd(false) == ELogEnd::Flat
			&& FMath::IsNearlyEqual(LengthBeforeTrim - Stake->GetLength(), ACarvableLog::PointLength, 1.f) && FMath::Abs(OnGrid - FMath::RoundToFloat(OnGrid)) < 0.03f,
		FString::Printf(TEXT("Uç düzeltme: sivri uç geri kesildi (%.0f cm kısaldı), uç yine modül çizgisinde (%.2fM), düz"),
			LengthBeforeTrim - Stake->GetLength(), OnGrid));

	// Ends survive a cross cut: the near piece keeps its point, the cut end is flat.
	ACarvableLog* Pointed = Fresh();
	Pointed->ShapeEnd(false, ECarveTool::Point);
	Pointed->ShapeEnd(true, ECarveTool::Chamfer);
	float CutX = 0.f;
	Pointed->FindSnapPoint(ECarveTool::CrossCut, Pointed->GetMinX() + 2.f * ACarvableLog::ModuleLength, Pointed->GetDiameter(), CutX);
	ACarvableLog* FarPiece = Pointed->CrossCut(CutX);
	Report(FarPiece && Pointed->GetEnd(false) == ELogEnd::Pointed && Pointed->GetEnd(true) == ELogEnd::Flat
			&& FarPiece->GetEnd(false) == ELogEnd::Flat && FarPiece->GetEnd(true) == ELogEnd::Chamfered,
		TEXT("Enine kesimde uçlar korundu: kazık ucu bir parçada, pah diğerinde, kesilen uçlar düz"));

	// Dovetail against flat notches of the same depth: it takes out more wood than the narrow one, less than the wide one.
	const float W = 2.f * R;
	const float Depth = 0.4f * W * 0.5f;
	ACarvableLog* Tail = Fresh();
	ACarvableLog* Narrow = Fresh();
	ACarvableLog* Wide = Fresh();
	const float DovetailV0 = Tail->GetVolume();
	float TailX = 0.f;
	Tail->FindSnapPoint(ECarveTool::Dovetail, Tail->GetCenterX(), W, TailX);
	Character->TestCarveHit(Tail, ECarveTool::Dovetail, Aim(Tail, TailX), FVector::UpVector);
	const bool bTail = Character->TestCarveHit(Tail, ECarveTool::Dovetail, Aim(Tail, TailX), FVector::UpVector);
	const float TailRemoved = DovetailV0 - Tail->GetVolume();
	Narrow->CutProfile(Narrow->GetCenterX(), 0.f, W * 0.7f, Tail->GetRadius() * 0.f + Character->GetCarveDepth(), false);
	Wide->CutProfile(Wide->GetCenterX(), 0.f, W, Character->GetCarveDepth(), false);
	const float NarrowRemoved = DovetailV0 - Narrow->GetVolume();
	const float WideRemoved = DovetailV0 - Wide->GetVolume();
	Report(bTail && Tail->GetCutSpans().Num() == 1 && TailRemoved > NarrowRemoved && TailRemoved < WideRemoved,
		FString::Printf(TEXT("Kırlangıç: giden odun %.0f cm3, aynı derinlikte dar düz çentik %.0f, geniş %.0f; arası, yani dipte geniş yüzeyde dar"),
			TailRemoved, NarrowRemoved, WideRemoved));

	// Split twice: a half, then quarters. Each cut halves the wood.
	ACarvableLog* Half = Fresh();
	const float WholeV = Half->GetVolume();
	ACarvableLog* OtherHalf = Half->Split(0.f);
	ACarvableLog* QuarterA = Half->Split(90.f);
	ACarvableLog* QuarterB = OtherHalf ? OtherHalf->Split(90.f) : nullptr;
	const bool bQuarters = OtherHalf && QuarterA && QuarterB;
	const float Share = bQuarters ? Half->GetVolume() / WholeV : 0.f;
	Report(bQuarters && FMath::Abs(Share - 0.25f) < 0.015f && Half->GetSplitCount() == 2
			&& FMath::Abs(FMath::FindDeltaAngleDegrees(QuarterA->GetFlatAngle(), Half->GetFlatAngle())) > 170.f,
		FString::Printf(TEXT("Yarma tekrar: yarım (%.0f%%) sonra çeyrek (her biri %.1f%%), yarma sayısı %d; aynı yarmadan çıkan iki parçanın düz yüzleri ters (%.0f° ve %.0f°)"),
			bQuarters ? 50.f : 0.f, Share * 100.f, Half->GetSplitCount(), Half->GetFlatAngle(), QuarterA ? QuarterA->GetFlatAngle() : 0.f));
}

ACarvableLog* AMechanicsDemo::Piece(FName Name) const
{
	const TWeakObjectPtr<ACarvableLog>* Found = Named.Find(Name);
	return Found ? Found->Get() : nullptr;
}

namespace
{
	/** A hit as if the player aimed at WorldPoint on Log. */
	FHitResult AimAt(ACarvableLog* Log, const FVector& WorldPoint)
	{
		FHitResult Hit;
		Hit.HitObjectHandle = FActorInstanceHandle(Log);
		Hit.ImpactPoint = WorldPoint;
		Hit.ImpactNormal = FVector::UpVector;
		return Hit;
	}

	/** The player stands at Where (ground level) looking along Yaw. */
	void Face(ACharacter* Player, UWorld* World, const FVector& Where, float Yaw)
	{
		FHitResult Ground;
		World->LineTraceSingleByObjectType(Ground, Where + FVector(0, 0, 1000.f), Where - FVector(0, 0, 2000.f), FCollisionObjectQueryParams(ECC_WorldStatic));
		Player->SetActorLocation(FVector(Where.X, Where.Y, Ground.ImpactPoint.Z + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetActorRotation(FRotator(0.f, Yaw, 0.f));
		if (AController* Controller = Player->GetController())
		{
			Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f));
		}
	}

	/** Angle of a piece's axis above the horizontal, in degrees. */
	float ElevationOf(const ACarvableLog* Log)
	{
		return FMath::RadiansToDegrees(FMath::Asin(FMath::Abs(Log->GetActorTransform().GetUnitAxis(EAxis::X).Z)));
	}
}

void AMechanicsDemo::AddRecognitionScript(AForestCharacter* Character)
{
	const float M = ACarvableLog::ModuleLength;
	// Does a piece belong to a structure of this kind?
	auto Made = [this](EWoodFeatureKind Kind, FName Name)
	{
		const ACarvableLog* Target = Piece(Name);
		for (const StructureLink::FFeatureInfo& Info : StructureLink::Features(GetWorld()))
		{
			if (Info.Kind != Kind)
			{
				continue;
			}
			for (const TWeakObjectPtr<ACarvableLog>& Part : Info.Pieces)
			{
				if (Part.Get() == Target)
				{
					return true;
				}
			}
		}
		return false;
	};
	// A log cut into three two-module pieces, named First, Second, Third.
	auto ThreeFrom = [this, M](FName Base, const FVector& Where, FName First, FName Second, FName Third)
	{
		ACarvableLog* Log = ACarvableLog::SpawnLog(GetWorld(), FTransform(FRotator::ZeroRotator, Where));
		Named.Add(First, Log);
		ACarvableLog* Rest = Log->CrossCut(Log->GetMinX() + 2.f * M);
		Named.Add(Second, Rest);
		Named.Add(Third, Rest->CrossCut(Rest->GetMinX() + 2.f * M));
	};

	// The two structures built earlier are read by the same rules.
	Script.Add([this, Made]()
	{
		Report(Made(EWoodFeatureKind::Platform, TEXT("Board1")) && Made(EWoodFeatureKind::Platform, TEXT("BeamA")) && Made(EWoodFeatureKind::Platform, TEXT("Pd")),
			TEXT("Tanıma: dört direk + iki kiriş + döşeme tahtası = Platform"));
		bool bCabin = true;
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F") })
		{
			bCabin &= Made(EWoodFeatureKind::HorizontalWall, Name);
		}
		Report(bCabin, TEXT("Tanıma: üç katlı kabin = Yatay kütük duvarı (6 kütük)"));
		Report(!Made(EWoodFeatureKind::Platform, TEXT("A")) && !Made(EWoodFeatureKind::HorizontalWall, TEXT("Pa")), TEXT("Tanıma: platformun parçaları duvar sayılmıyor, kabininkiler platform değil"));
		return 0.3f;
	});

	// Pointed posts side by side: a stake wall (and not a plain vertical wall).
	const FVector StakeArea(-2200.f, 2400.f, 0.f);
	Script.Add([this, ThreeFrom, StakeArea, M]()
	{
		ThreeFrom(TEXT("SA"), StakeArea + FVector(1500.f, 0.f, 70.f), TEXT("SW1"), TEXT("SW2"), TEXT("SW3"));
		ACarvableLog* Log = ACarvableLog::SpawnLog(GetWorld(), FTransform(FRotator::ZeroRotator, StakeArea + FVector(1500.f, 200.f, 70.f)));
		Named.Add(TEXT("SW4"), Log);
		Log->CrossCut(Log->GetMinX() + 2.f * M);
		for (const TCHAR* Name : { TEXT("SW1"), TEXT("SW2"), TEXT("SW3"), TEXT("SW4") })
		{
			Piece(Name)->ShapeEnd(true, ECarveTool::Point);
		}
		return 4.f;
	});
	Script.Add([this, Character, Made, StakeArea]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, StakeArea + FVector(-400.f, 0.f, 0.f), 0.f);
		bool bPlaced = true;
		int32 Index = 0;
		for (const TCHAR* Name : { TEXT("SW1"), TEXT("SW2"), TEXT("SW3"), TEXT("SW4") })
		{
			bPlaced &= Character->TestPickUp(Piece(Name)) && Character->TestPlace(true, GroundAt(World, StakeArea + FVector(75.f * Index++, 0.f, 0.f)));
		}
		Report(bPlaced && Made(EWoodFeatureKind::StakeWall, TEXT("SW1")) && Made(EWoodFeatureKind::StakeWall, TEXT("SW4")) && !Made(EWoodFeatureKind::VerticalWall, TEXT("SW1")),
			TEXT("Tanıma: yan yana dört sivri uçlu direk = Sivri kazık duvarı"));
		return 0.3f;
	});

	// Two walls of three posts with a gap of 100 cm between them and a log across the tops: a door frame.
	const FVector DoorArea(-2200.f, 1700.f, 0.f);
	Script.Add([this, ThreeFrom, DoorArea]()
	{
		ThreeFrom(TEXT("DA"), DoorArea + FVector(1500.f, 0.f, 70.f), TEXT("DW1"), TEXT("DW2"), TEXT("DW3"));
		ThreeFrom(TEXT("DB"), DoorArea + FVector(1500.f, 200.f, 70.f), TEXT("DW4"), TEXT("DW5"), TEXT("DW6"));
		Named.Add(TEXT("Lintel"), ACarvableLog::SpawnLog(GetWorld(), FTransform(FRotator::ZeroRotator, DoorArea + FVector(1500.f, 400.f, 70.f))));
		return 4.f;
	});
	Script.Add([this, Character, Made, DoorArea]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, DoorArea + FVector(-400.f, 0.f, 0.f), 0.f);
		const float Spots[6] = { 0.f, 75.f, 150.f, 320.f, 395.f, 470.f };
		bool bPlaced = true;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			bPlaced &= Character->TestPickUp(Piece(*FString::Printf(TEXT("DW%d"), Index + 1))) && Character->TestPlace(true, GroundAt(World, DoorArea + FVector(Spots[Index], 0.f, 0.f)));
		}
		const bool bOpening = Made(EWoodFeatureKind::Opening, TEXT("DW3")) && !Made(EWoodFeatureKind::DoorFrame, TEXT("DW3"));
		const bool bLintel = Character->TestPickUp(Piece(TEXT("Lintel"))) && Character->TestPlace(false, AimAtTop(Piece(TEXT("DW3"))));
		Report(bPlaced && bOpening, TEXT("Tanıma: iki duvar arasında 100 cm boşluk = Geçit (lentosuz)"));
		Report(bLintel && Piece(TEXT("Lintel"))->IsAnchored() && Made(EWoodFeatureKind::DoorFrame, TEXT("DW3")) && Made(EWoodFeatureKind::DoorFrame, TEXT("DW4")) && Made(EWoodFeatureKind::DoorFrame, TEXT("Lintel")),
			TEXT("Tanıma: boşluğun üstüne kütük konunca = Kapı kasası (iki yan direk + lento)"));
		return 0.3f;
	});

	// A long log on two posts over a gap with nothing under its middle: a bridge. Marked as scaffolding it is a scaffold.
	const FVector BridgeArea(-2200.f, 3100.f, 0.f);
	Script.Add([this, BridgeArea, M]()
	{
		ACarvableLog* Log = ACarvableLog::SpawnLog(GetWorld(), FTransform(FRotator::ZeroRotator, BridgeArea + FVector(1500.f, 0.f, 70.f)));
		Named.Add(TEXT("BR1"), Log);
		ACarvableLog* Rest = Log->CrossCut(Log->GetMinX() + 2.f * M);
		Named.Add(TEXT("BR2"), Rest);
		Rest->CrossCut(Rest->GetMinX() + 2.f * M); // the second post is cut to two modules as well
		Named.Add(TEXT("BrBeam"), ACarvableLog::SpawnLog(GetWorld(), FTransform(FRotator::ZeroRotator, BridgeArea + FVector(1500.f, 200.f, 70.f))));
		return 4.f;
	});
	Script.Add([this, Character, Made, BridgeArea]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, BridgeArea + FVector(-400.f, 0.f, 0.f), 0.f);
		bool bPlaced = Character->TestPickUp(Piece(TEXT("BR1"))) && Character->TestPlace(true, GroundAt(World, BridgeArea));
		bPlaced &= Character->TestPickUp(Piece(TEXT("BR2"))) && Character->TestPlace(true, GroundAt(World, BridgeArea + FVector(260.f, 0.f, 0.f)));
		bPlaced &= Character->TestPickUp(Piece(TEXT("BrBeam"))) && Character->TestPlace(false, AimAtTop(Piece(TEXT("BR1"))));
		Report(bPlaced && Piece(TEXT("BrBeam"))->IsAnchored() && Made(EWoodFeatureKind::Bridge, TEXT("BrBeam")),
			FString::Printf(TEXT("Tanıma: iki direk 260 cm arayla + üstünde uzun kütük, ortası boş = Köprü%s%s"), bPlaced ? TEXT("") : TEXT(" [yerleştirme başarısız]"),
				Piece(TEXT("BrBeam"))->IsAnchored() ? TEXT("") : TEXT(" [kiriş sabit değil]")));
		for (const TCHAR* Name : { TEXT("BR1"), TEXT("BR2"), TEXT("BrBeam") })
		{
			Piece(Name)->SetTemporary(true);
		}
		StructureLink::MarkChanged();
		Report(Made(EWoodFeatureKind::Scaffold, TEXT("BrBeam")) && Made(EWoodFeatureKind::Scaffold, TEXT("BR1")), TEXT("Tanıma: geçici işaretli parçalar = İskele"));
		return 0.3f;
	});
}

void AMechanicsDemo::AddFireScript(AForestCharacter* Character)
{
	const FVector Area(-2200.f, 0.f, 0.f);
	const float M = ACarvableLog::ModuleLength;

	// Wood for fire: a whole log cut into four quarters, three more whole logs.
	Script.Add([this, Area]()
	{
		UWorld* World = GetWorld();
		ACarvableLog* Whole = ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(1500.f, 0.f, 70.f)));
		Named.Add(TEXT("Q1"), Whole);
		const float WholeSeconds = ACampfire::FuelSecondsOf(Whole);
		ACarvableLog* Q3 = Whole->Split(0.f);
		const float HalfSeconds = ACampfire::FuelSecondsOf(Whole);
		Named.Add(TEXT("Q2"), Whole->Split(90.f));
		Named.Add(TEXT("Q3"), Q3);
		Named.Add(TEXT("Q4"), Q3->Split(90.f));
		const float QuarterSeconds = ACampfire::FuelSecondsOf(Whole);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Named.Add(*FString::Printf(TEXT("W%d"), Index + 1), ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(1500.f, 200.f * (Index + 1), 70.f))));
		}
		Report(FMath::Abs(WholeSeconds - 600.f) < 24.f && FMath::Abs(HalfSeconds - 300.f) < 14.f && FMath::Abs(QuarterSeconds - 150.f) < 8.f,
			FString::Printf(TEXT("Yakıt odunun hacmiyle orantılı: tam kütük %.0f sn, yarım %.0f sn, çeyrek %.0f sn (10, 5, 2.5 dk)"), WholeSeconds, HalfSeconds, QuarterSeconds));
		return 4.f;
	});

	// A quarter lights the fire (through the player's own aim and click); more wood goes on it.
	Script.Add([this, Character, Area]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, Area + FVector(-400.f, 0.f, 0.f), 0.f);
		const bool bLit = Character->TestPickUp(Piece(TEXT("Q1"))) && Character->TestPlaceFire(GroundAt(World, Area));
		ACampfire* Fire = nullptr;
		for (TActorIterator<ACampfire> It(World); It; ++It)
		{
			Fire = *It;
		}
		Report(bLit && Fire && Fire->IsBurning() && !Piece(TEXT("Q1")) && FMath::Abs(Fire->GetFuelSeconds() - 148.f) < 8.f,
			FString::Printf(TEXT("Ateş yakıldı: çeyrek kütük yakıt oldu (%.0f sn), kütük harcandı, ateş yanıyor"), Fire ? Fire->GetFuelSeconds() : 0.f));

		FHitResult OnFire;
		OnFire.HitObjectHandle = FActorInstanceHandle(Fire);
		OnFire.ImpactPoint = Fire->GetActorLocation() + FVector(0.f, 0.f, 30.f);
		OnFire.ImpactNormal = FVector::UpVector;
		const float Before = Fire->GetFuelSeconds();
		bool bFed = true;
		for (const TCHAR* Name : { TEXT("Q2"), TEXT("Q3"), TEXT("Q4") })
		{
			bFed &= Character->TestPickUp(Piece(Name)) && Character->TestPlace(false, OnFire);
		}
		Report(bFed && FMath::Abs(Fire->GetFuelSeconds() - 4.f * 148.f) < 30.f,
			FString::Printf(TEXT("Ateşe üç çeyrek daha atıldı: yakıt %.0f -> %.0f sn"), Before, Fire->GetFuelSeconds()));
		return 0.3f;
	});

	// It burns down and goes out; a whole log relights it; the fire holds no more than 15 minutes.
	Script.Add([this, Character]()
	{
		UWorld* World = GetWorld();
		ACampfire* Fire = nullptr;
		for (TActorIterator<ACampfire> It(World); It; ++It)
		{
			Fire = *It;
		}
		const float Start = Fire->GetFuelSeconds();
		Fire->Burn(100.f);
		const float After100 = Fire->GetFuelSeconds();
		Fire->Burn(1000.f);
		Report(FMath::Abs((Start - After100) - 100.f) < 0.5f && !Fire->IsBurning() && Fire->GetFuelSeconds() == 0.f,
			FString::Printf(TEXT("Ateş yandıkça yakıt azaldı (100 sn: %.0f -> %.0f) ve söndü"), Start, After100));

		FHitResult OnFire;
		OnFire.HitObjectHandle = FActorInstanceHandle(Fire);
		OnFire.ImpactPoint = Fire->GetActorLocation() + FVector(0.f, 0.f, 30.f);
		OnFire.ImpactNormal = FVector::UpVector;
		Character->TestPickUp(Piece(TEXT("W1"))) && Character->TestPlace(false, OnFire);
		const bool bRelit = Fire->IsBurning();
		const float Once = Fire->GetFuelSeconds();
		Character->TestPickUp(Piece(TEXT("W2"))) && Character->TestPlace(false, OnFire);
		const float Twice = Fire->GetFuelSeconds();
		const bool bTaken = Character->TestPickUp(Piece(TEXT("W3"))) && Character->TestPlace(false, OnFire);
		Report(bRelit && FMath::Abs(Once - 593.f) < 24.f && FMath::Abs(Twice - ACampfire::MaxFuelSeconds) < 1.f && Character->GetCarriedCount() == 1,
			FString::Printf(TEXT("Tam kütük söneni yeniden yaktı (%.0f sn); ikincisiyle dolu (%.0f sn = üst sınır); üçüncüsü atılmadı, omuzda kaldı"), Once, Twice));
		(void)bTaken;
		return 0.3f;
	});
	Script.Add([this, Character, Area]()
	{
		// The shoulder is free again.
		Character->TestPlace(false, GroundAt(GetWorld(), Area + FVector(-900.f, 300.f, 0.f)));
		return 0.2f;
	});
}

void AMechanicsDemo::AddDamageScript(AForestCharacter* Character)
{
	const float M = ACarvableLog::ModuleLength;
	const FVector X(-2200.f, -700.f, 0.f);
	const FVector Y(-2200.f, -1500.f, 0.f);

	// Two small structures (three posts 160 cm apart and a beam each) and spare wood.
	Script.Add([this, M, X, Y]()
	{
		UWorld* World = GetWorld();
		auto Three = [&](const FVector& Where, FName A, FName B, FName C)
		{
			ACarvableLog* Log = ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Where));
			Named.Add(A, Log);
			ACarvableLog* Rest = Log->CrossCut(Log->GetMinX() + 2.f * M);
			Named.Add(B, Rest);
			Named.Add(C, Rest->CrossCut(Rest->GetMinX() + 2.f * M));
		};
		Three(X + FVector(1500.f, 0.f, 70.f), TEXT("XP1"), TEXT("XP2"), TEXT("XP3"));
		Three(Y + FVector(1500.f, 0.f, 70.f), TEXT("YP1"), TEXT("YP2"), TEXT("YP3"));
		Three(Y + FVector(1500.f, 400.f, 70.f), TEXT("RP1"), TEXT("RP2"), TEXT("RP3"));
		Named.Add(TEXT("XBeam"), ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, X + FVector(1500.f, 200.f, 70.f))));
		Named.Add(TEXT("YBeam"), ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Y + FVector(1500.f, 200.f, 70.f))));
		Named.Add(TEXT("YFuel"), ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Y + FVector(1500.f, 600.f, 70.f))));
		return 4.5f;
	});

	auto Build = [this, Character](const FVector& Where, FName P1, FName P2, FName P3, FName BeamName)
	{
		UWorld* World = GetWorld();
		Face(Character, World, Where + FVector(-400.f, 0.f, 0.f), 0.f);
		bool bOk = true;
		int32 Index = 0;
		for (const FName Name : { P1, P2, P3 })
		{
			bOk &= Character->TestPickUp(Piece(Name)) && Character->TestPlace(true, GroundAt(World, Where + FVector(160.f * Index++, 0.f, 0.f)));
		}
		bOk &= Character->TestPickUp(Piece(BeamName)) && Character->TestPlace(false, AimAtTop(Piece(P1)));
		return bOk && Piece(BeamName)->IsAnchored();
	};

	// Structure X: a defender's hit. Past 0.6 the joints of a piece let go, past 0.85 it holds nothing.
	Script.Add([this, Build, X]()
	{
		Report(Build(X, TEXT("XP1"), TEXT("XP2"), TEXT("XP3"), TEXT("XBeam")), TEXT("Hasar testi: üç direk + kiriş duruyor"));
		return 1.5f;
	});
	Script.Add([this]()
	{
		ACarvableLog* Middle = Piece(TEXT("XP2"));
		Middle->AddDamage(0.5f);
		const bool bHeld = Piece(TEXT("XBeam"))->IsAnchored() && Middle->IsAnchored();
		Middle->AddDamage(0.15f);
		// The post still stands on the ground, but its joints have let go: it no longer holds the beam, and the other two are too far apart.
		Report(bHeld && Middle->IsAnchored() && !Piece(TEXT("XBeam"))->IsAnchored(),
			FString::Printf(TEXT("Saldırı: orta direk %%%.0f hasarlı. %%50'de kiriş tutuyordu, %%60'ı geçince eklemi gevşedi: direk ayakta, kiriş kırıldı"), Middle->GetDamage() * 100.f));
		return 2.f;
	});
	Script.Add([this]()
	{
		ACarvableLog* Last = Piece(TEXT("XP3"));
		Last->AddDamage(0.9f);
		Report(!Last->IsAnchored() && Last->GetDamage() > 0.85f, FString::Printf(TEXT("Saldırı: son direk %%%.0f hasarlı, %%85'i geçince hiçbir şey taşıyamadı: yerinden çıktı"), Last->GetDamage() * 100.f));
		return 0.3f;
	});

	// Structure Y: a fire beside the middle post scorches only what is in its heat; the burnt post is swapped for a sound one.
	Script.Add([this, Build, Y]()
	{
		Report(Build(Y, TEXT("YP1"), TEXT("YP2"), TEXT("YP3"), TEXT("YBeam")), TEXT("Yangın testi: üç direk + kiriş duruyor"));
		return 1.5f;
	});
	Script.Add([this, Y]()
	{
		UWorld* World = GetWorld();
		ACampfire* Fire = ACampfire::Build(World, GroundAt(World, Y + FVector(160.f, 160.f, 0.f)).ImpactPoint);
		Fire->AddFuel(Piece(TEXT("YFuel")));
		const float Before = Piece(TEXT("YP2"))->GetDamage();
		Fire->Burn(20.f);
		const bool bOnlyNear = Piece(TEXT("YP1"))->GetDamage() < 0.001f && Piece(TEXT("YP3"))->GetDamage() < 0.001f && Piece(TEXT("YBeam"))->GetDamage() < 0.001f;
		Report(Before == 0.f && FMath::Abs(Piece(TEXT("YP2"))->GetDamage() - 0.4f) < 0.03f && bOnlyNear && Piece(TEXT("YBeam"))->IsAnchored(),
			FString::Printf(TEXT("Yangın: 20 sn yanınca yalnızca ateşin yanındaki direk yandı (%%%.0f hasar), diğerleri sağlam, kiriş yerinde"), Piece(TEXT("YP2"))->GetDamage() * 100.f));
		return 0.3f;
	});
	Script.Add([this, Character, Y]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, Y + FVector(-400.f, 0.f, 0.f), 0.f);
		ACarvableLog* Burnt = Piece(TEXT("YP2"));
		const FVector Where = Burnt->GetActorLocation();
		ACarvableLog* Spare = Piece(TEXT("RP2"));
		const bool bSwapped = Character->TestPickUp(Spare) && Character->TestPlace(false, AimAt(Burnt, Where));
		const bool bBurntCarried = Burnt->IsCarried() && Burnt->GetDamage() > 0.35f;
		// The spare took the burnt post's place exactly: its box and a standing post's end on the ground.
		FVector SpareCentre, SpareAxis;
		float R1, H1;
		ACarvableLog::GetLogAxis(Spare, SpareCentre, SpareAxis, R1, H1);
		const float Offset = FVector::Dist(SpareCentre, Where);
		Report(bSwapped && Spare->IsAnchored() && Spare->GetDamage() == 0.f && bBurntCarried && Piece(TEXT("YBeam"))->IsAnchored() && Offset < 100.f,
			FString::Printf(TEXT("Onarım: yanan direk sağlam parçayla değiştirildi (yeni parça yerinde, hasar 0), yanan parça omza geldi (%%%.0f hasarlı), kiriş duruyor"),
				Burnt->GetDamage() * 100.f));
		Named.Add(TEXT("YP2New"), Spare);
		return 0.3f;
	});
	Script.Add([this, Character]()
	{
		// The fire is still there: the new post burns too, and past 0.6 its joints let go.
		ACampfire* Fire = nullptr;
		for (TActorIterator<ACampfire> It(GetWorld()); It; ++It)
		{
			if (FVector::Dist2D(It->GetActorLocation(), Piece(TEXT("YP2New"))->GetActorLocation()) < 300.f)
			{
				Fire = *It;
			}
		}
		if (Fire)
		{
			Fire->Burn(5.f);
		}
		const bool bStill = Piece(TEXT("YBeam"))->IsAnchored();
		// The wood that was just swapped out goes on the fire as fuel (it carries its damage but burns like any wood).
		FHitResult OnFire;
		OnFire.HitObjectHandle = FActorInstanceHandle(Fire);
		OnFire.ImpactPoint = Fire->GetActorLocation() + FVector(0.f, 0.f, 30.f);
		OnFire.ImpactNormal = FVector::UpVector;
		const float FuelBefore = Fire->GetFuelSeconds();
		const bool bBurntUp = Character->TestPlace(false, OnFire);
		Report(bStill && bBurntUp && Fire->GetFuelSeconds() > FuelBefore - 6.f,
			FString::Printf(TEXT("Değiştirilen yanık parça ateşe yakıt olarak atıldı (yakıt %.0f -> %.0f sn)"), FuelBefore, Fire->GetFuelSeconds()));
		Fire->Burn(30.f);
		Report(!Piece(TEXT("YBeam"))->IsAnchored(), TEXT("Ateş yeni direği de yaktı: eklemleri gevşeyince kiriş yine kırıldı"));
		return 1.f;
	});
}

void AMechanicsDemo::AddCollapseScript(AForestCharacter* Character)
{
	// Take the first log of the cabin out from under the rest: what rested on it only through it falls too.
	Script.Add([this, Character]()
	{
		Character->TestPickUp(Piece(TEXT("A")));
		int32 Fallen = 0;
		for (const TCHAR* Name : { TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F") })
		{
			Fallen += Piece(Name)->IsAnchored() ? 0 : 1;
		}
		Report(Fallen >= 4 && Piece(TEXT("B"))->IsAnchored(),
			FString::Printf(TEXT("Çöküş: kabinin alt kütüğü sökülünce üstündeki %d parça desteksiz kalıp düştü; öbür alt kütük yerinde"), Fallen));
		return 2.f;
	});
}

void AMechanicsDemo::BuildScript(AForestCharacter* Character)
{
	Script.Reset();
	ScriptIndex = 0;
	Named.Reset();
	StructureLink::bLogFalls = true;
	AddBraceScript(Character);
	AddPlatformScript(Character);
	AddCabinScript(Character);
	AddRecognitionScript(Character);
	AddFireScript(Character);
	AddDamageScript(Character);
	AddCollapseScript(Character);
}

void AMechanicsDemo::AddBraceScript(AForestCharacter* Character)
{
	const FVector Area(-2800.f, -800.f, 0.f);
	const float M = ACarvableLog::ModuleLength;

	// 1. Wood: a beam (a whole log), three posts and three braces (two logs, each cut into 2-module pieces).
	Script.Add([this, Area, M]()
	{
		UWorld* World = GetWorld();
		auto Spawn = [&](FName Name, float Row)
		{
			ACarvableLog* Log = ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(0.f, 700.f + Row * 170.f, 70.f)));
			Named.Add(Name, Log);
			return Log;
		};
		Spawn(TEXT("Beam"), 0.f);
		for (const TPair<FName, float>& Source : { TPair<FName, float>(TEXT("P1"), 1.f), TPair<FName, float>(TEXT("B1"), 2.f) })
		{
			ACarvableLog* First = Spawn(Source.Key, Source.Value);
			ACarvableLog* Rest = First->CrossCut(First->GetMinX() + 2.f * M);
			ACarvableLog* Last = Rest->CrossCut(Rest->GetMinX() + 2.f * M);
			const FString Prefix = Source.Key == TEXT("P1") ? TEXT("P") : TEXT("B");
			Named.Add(FName(*(Prefix + TEXT("2"))), Rest);
			Named.Add(FName(*(Prefix + TEXT("3"))), Last);
		}
		return 4.f;
	});

	// 2. Three posts 160 cm apart and a beam on the first: held by three, a control for what follows.
	Script.Add([this, Character, Area]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, Area + FVector(-400.f, 0.f, 0.f), 0.f);
		bool bPosts = true;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			ACarvableLog* Post = Piece(*FString::Printf(TEXT("P%d"), Index + 1));
			bPosts &= Character->TestPickUp(Post) && Character->TestPlace(true, GroundAt(World, Area + FVector(160.f * Index, 0.f, 0.f)));
		}
		ACarvableLog* Beam = Piece(TEXT("Beam"));
		const bool bBeam = bPosts && Character->TestPickUp(Beam) && Character->TestPlace(false, AimAtTop(Piece(TEXT("P1"))));
		FrameBeamZ = Beam->GetActorLocation().Z;
		Report(bBeam && Beam->IsAnchored(),
			FString::Printf(TEXT("Üç direk (160 cm arayla) + kiriş: kiriş %s"), Beam->IsAnchored() ? TEXT("duruyor") : TEXT("düştü")));
		return 2.5f;
	});

	// 3. The middle post comes out: the supports left are 320 cm apart, more than the beam can span.
	Script.Add([this, Character]()
	{
		ACarvableLog* Beam = Piece(TEXT("Beam"));
		const bool bStill = Beam->IsAnchored() && FMath::IsNearlyEqual(Beam->GetActorLocation().Z, FrameBeamZ, 1.f);
		Report(bStill, TEXT("Kiriş 2 sn sonra yerinde"));
		Character->TestPickUp(Piece(TEXT("P2")));
		Report(!Beam->IsAnchored(), FString::Printf(TEXT("Orta direk sökülünce (kalan direkler 320 cm arayla, sınır %.0f cm) kiriş ortadan kırıldı"),
			StructureLink::MaxSupportGapModules * ACarvableLog::ModuleLength));
		return 3.5f;
	});

	// 4. Put it all back, then braces at the two outer posts.
	Script.Add([this, Character, Area]()
	{
		UWorld* World = GetWorld();
		ACarvableLog* Beam = Piece(TEXT("Beam"));
		Report(Beam->GetActorLocation().Z < FrameBeamZ - 30.f, FString::Printf(TEXT("Kırılan kiriş yere indi (%.0f cm aşağı)"), FrameBeamZ - Beam->GetActorLocation().Z));
		// A new whole beam for the rebuild; the broken one stays where it fell.
		Named.Add(TEXT("Beam"), ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(0.f, 700.f, 70.f))));
		Beam = Piece(TEXT("Beam"));
		Face(Character, World, Area + FVector(-400.f, 0.f, 0.f), 0.f);
		const bool bPost = Character->TestPlace(true, GroundAt(World, Area + FVector(160.f, 0.f, 0.f)));
		const bool bBeam = bPost && Character->TestPickUp(Beam) && Character->TestPlace(false, AimAtTop(Piece(TEXT("P1"))));
		Report(bBeam && Beam->IsAnchored(), TEXT("Direk geri kondu, kiriş yine üç direkte duruyor"));

		// A brace on the first post (running towards the middle of the beam) and one on the last (the other way).
		auto Brace = [&](FName Name, FName PostName)
		{
			ACarvableLog* BracePiece = Piece(Name);
			const bool bPlaced = Character->TestPickUp(BracePiece) && Character->TestPlace(false, AimAtTop(Piece(PostName)));
			const float Elevation = ElevationOf(BracePiece);
			FVector Centre, Axis;
			float Radius, Half;
			ACarvableLog::GetLogAxis(BracePiece, Centre, Axis, Radius, Half);
			const FVector Low = Axis.Z > 0.f ? Centre - Axis * Half : Centre + Axis * Half;
			const FVector High = Axis.Z > 0.f ? Centre + Axis * Half : Centre - Axis * Half;
			// The low end is at the post, the high end under the beam, towards its middle.
			const FVector PostXY = Piece(PostName)->GetActorLocation();
			Report(bPlaced && BracePiece->IsAnchored() && FMath::Abs(Elevation - 45.f) < 3.f && High.Z > Low.Z + 40.f,
				FString::Printf(TEXT("Payanda %s direğine kondu: %.0f° eğimli, alt ucu %.0f cm yüksekte, %s"), *PostName.ToString(), Elevation,
					Low.Z - GroundAt(World, Low).ImpactPoint.Z, BracePiece->IsAnchored() ? TEXT("duruyor") : TEXT("düştü")));
		};
		Brace(TEXT("B1"), TEXT("P1"));
		Brace(TEXT("B2"), TEXT("P3"));
		return 1.5f;
	});

	// 5. With braces at both ends the middle post is not needed (the supports are at most ~190 cm apart).
	Script.Add([this, Character]()
	{
		ACarvableLog* Beam = Piece(TEXT("Beam"));
		FrameBeamZ = Beam->GetActorLocation().Z;
		Character->TestPickUp(Piece(TEXT("P2")));
		Report(Beam->IsAnchored(), TEXT("Payandalar varken ara direk söküldü: kiriş düşmedi"));
		// The post goes down on the ground: the shoulder is free again for the next test.
		Character->TestPlace(false, GroundAt(GetWorld(), FVector(-3200.f, 0.f, 0.f)));
		return 3.f;
	});
	Script.Add([this]()
	{
		ACarvableLog* Beam = Piece(TEXT("Beam"));
		Report(Beam->IsAnchored() && FMath::IsNearlyEqual(Beam->GetActorLocation().Z, FrameBeamZ, 1.f) && Piece(TEXT("B1"))->IsAnchored() && Piece(TEXT("B2"))->IsAnchored(),
			TEXT("3 sn sonra: kiriş ve payandalar yerinde (ara direksiz)"));
		return 0.5f;
	});
}

void AMechanicsDemo::AddPlatformScript(AForestCharacter* Character)
{
	const FVector Area(-2800.f, 900.f, 0.f);
	const float M = ACarvableLog::ModuleLength;

	// Wood: four posts (two logs), two beams, a log split into two floor boards.
	Script.Add([this, Area, M]()
	{
		UWorld* World = GetWorld();
		auto Spawn = [&](FName Name, float Row)
		{
			ACarvableLog* Log = ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(0.f, 700.f + Row * 170.f, 70.f)));
			Named.Add(Name, Log);
			return Log;
		};
		Spawn(TEXT("BeamA"), 0.f);
		Spawn(TEXT("BeamB"), 1.f);
		for (const TPair<FName, float>& Source : { TPair<FName, float>(TEXT("Pa"), 2.f), TPair<FName, float>(TEXT("Pc"), 3.f) })
		{
			ACarvableLog* First = Spawn(Source.Key, Source.Value);
			ACarvableLog* Rest = First->CrossCut(First->GetMinX() + 2.f * M);
			ACarvableLog* Last = Rest->CrossCut(Rest->GetMinX() + 2.f * M);
			Named.Add(Source.Key == TEXT("Pa") ? TEXT("Pb") : TEXT("Pd"), Rest);
			(void)Last;
		}
		ACarvableLog* BoardLog = Spawn(TEXT("Board1"), 4.f);
		Named.Add(TEXT("Board2"), BoardLog->Split(0.f));
		return 4.f;
	});

	// Posts 240 cm apart along the beams, the two beams 195 cm apart; beams on top, then boards across them.
	Script.Add([this, Character, Area]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, Area + FVector(-400.f, 0.f, 0.f), 0.f);
		const FVector Spots[4] = { Area, Area + FVector(240.f, 0.f, 0.f), Area + FVector(0.f, 195.f, 0.f), Area + FVector(240.f, 195.f, 0.f) };
		const TCHAR* Names[4] = { TEXT("Pa"), TEXT("Pb"), TEXT("Pc"), TEXT("Pd") };
		bool bPosts = true;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			bPosts &= Character->TestPickUp(Piece(Names[Index])) && Character->TestPlace(true, GroundAt(World, Spots[Index]));
		}
		const bool bBeamA = bPosts && Character->TestPickUp(Piece(TEXT("BeamA"))) && Character->TestPlace(false, AimAtTop(Piece(TEXT("Pa"))));
		const bool bBeamB = bPosts && Character->TestPickUp(Piece(TEXT("BeamB"))) && Character->TestPlace(false, AimAtTop(Piece(TEXT("Pc"))));
		FVector CentreA, AxisA, CentreB, AxisB;
		float Radius, Half;
		ACarvableLog::GetLogAxis(Piece(TEXT("BeamA")), CentreA, AxisA, Radius, Half);
		ACarvableLog::GetLogAxis(Piece(TEXT("BeamB")), CentreB, AxisB, Radius, Half);
		Report(bBeamA && bBeamB && Piece(TEXT("BeamA"))->IsAnchored() && Piece(TEXT("BeamB"))->IsAnchored()
				&& FMath::Abs(AxisA.Y) < 0.05f && FMath::Abs(AxisB.Y) < 0.05f && FMath::IsNearlyEqual(CentreA.Y, Area.Y, 3.f) && FMath::IsNearlyEqual(CentreB.Y, Area.Y + 195.f, 3.f),
			FString::Printf(TEXT("Dört direk + iki kiriş: iki kiriş de paralel (+X boyunca) yerinde, %.0f cm arayla"), CentreB.Y - CentreA.Y));
		return 2.f;
	});

	Script.Add([this, Character, Area]()
	{
		UWorld* World = GetWorld();
		// From the south, looking north: the boards run +Y across both beams.
		Face(Character, World, Area + FVector(100.f, -500.f, 0.f), 90.f);
		FVector Centre, Axis;
		float Radius, Half;
		ACarvableLog* BeamA = Piece(TEXT("BeamA"));
		ACarvableLog::GetLogAxis(BeamA, Centre, Axis, Radius, Half);
		bool bBoards = true;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			ACarvableLog* Board = Piece(Index == 0 ? TEXT("Board1") : TEXT("Board2"));
			const FVector Aim(Area.X + 60.f + 80.f * Index, Centre.Y, Centre.Z + Radius);
			bBoards &= Character->TestPickUp(Board) && Character->TestPlace(false, AimAt(BeamA, Aim));
		}
		Report(bBoards && Piece(TEXT("Board1"))->IsAnchored() && Piece(TEXT("Board2"))->IsAnchored(), TEXT("İki döşeme tahtası iki kirişin üstüne kondu, ikisi de duruyor"));
		return 2.5f;
	});

	Script.Add([this]()
	{
		bool bStill = true;
		float Worst = 1.f;
		for (const TCHAR* Name : { TEXT("Board1"), TEXT("Board2") })
		{
			ACarvableLog* Board = Piece(Name);
			bStill &= Board->IsAnchored();
			// The board's flat face looks out of the wood at FlatAngle (as Split's angle): it has to point up.
			const float Angle = FMath::DegreesToRadians(Board->GetFlatAngle());
			const FVector Normal = Board->GetActorTransform().TransformVectorNoScale(FVector(0.f, FMath::Cos(Angle), -FMath::Sin(Angle)));
			Worst = FMath::Min(Worst, Normal.Z);
		}
		Report(bStill && Worst > 0.98f, FString::Printf(TEXT("Döşeme 2.5 sn sonra yerinde, düz yüzler yukarı bakıyor (en kötü %.3f)"), Worst));
		// A board needs two beams under it: taking one beam's post out lets the beam, and with it half the floor, go.
		return 0.5f;
	});
}

void AMechanicsDemo::AddCabinScript(AForestCharacter* Character)
{
	const FVector Area(-2800.f, -2300.f, 0.f);

	// Layer one: two logs on the ground along Y, 260 cm apart. Four more logs to come.
	Script.Add([this, Area]()
	{
		UWorld* World = GetWorld();
		const TCHAR* Names[6] = { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F") };
		for (int32 Index = 0; Index < 6; ++Index)
		{
			ACarvableLog* Log = ACarvableLog::SpawnLog(World, FTransform(FRotator::ZeroRotator, Area + FVector(1200.f, 170.f * Index, 70.f)));
			Named.Add(Names[Index], Log);
		}
		for (int32 Index = 0; Index < 2; ++Index)
		{
			ACarvableLog* Log = Piece(Names[Index]);
			// The foundation logs are set down and held exactly 260 cm apart: a log left to roll can end up skewed, and what is
			// laid across a skewed pair reaches differently past its second support (the beam rules would then rightly snap it).
			Log->PlaceAt(Log->GetLayTransform(GroundAt(World, Area + FVector(260.f * Index, 0.f, 0.f)).ImpactPoint, 90.f), true);
		}
		return 3.5f;
	});

	// Layer two: two logs across them, each notched into both. The frame stands on its own.
	Script.Add([this, Character, Area, Tries = 0]() mutable
	{
		// The two ground logs have to lie still first: a log that is still rolling cannot be built on.
		if ((Piece(TEXT("A"))->GetRootComponent()->IsSimulatingPhysics() || Piece(TEXT("B"))->GetRootComponent()->IsSimulatingPhysics()) && ++Tries < 20)
		{
			--ScriptIndex;
			return 0.5f;
		}
		UWorld* World = GetWorld();
		Face(Character, World, Area + FVector(-500.f, 0.f, 0.f), 0.f);
		FVector Centre, Axis;
		float Radius, Half;
		ACarvableLog* A = Piece(TEXT("A"));
		ACarvableLog::GetLogAxis(A, Centre, Axis, Radius, Half);
		bool bPlaced = true;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			ACarvableLog* Upper = Piece(Index == 0 ? TEXT("C") : TEXT("D"));
			const FVector Aim(Centre.X, Centre.Y + (Index == 0 ? -130.f : 130.f), Centre.Z + Radius);
			bPlaced &= Character->TestPickUp(Upper) && Character->TestPlace(false, AimAt(A, Aim));
		}
		bool bAll = true;
		int32 Notches = 0;
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D") })
		{
			bAll &= Piece(Name)->IsAnchored();
		}
		Notches = Piece(TEXT("C"))->GetCutSpans().Num() + Piece(TEXT("D"))->GetCutSpans().Num();
		// What the structure graph sees if the loose log were held in place too (to read why it was not).
		auto DescribeWith = [this](ACarvableLog* Loose)
		{
			Loose->Anchor();
			const FString Text = StructureLink::Describe(GetWorld());
			Loose->LetFall();
			return Text;
		};
		FString State;
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D") })
		{
			State += FString::Printf(TEXT(" %s:%s z=%.0f"), Name, Piece(Name)->IsAnchored() ? TEXT("sabit") : TEXT("serbest"), Piece(Name)->GetActorLocation().Z);
		}
		Report(bPlaced && bAll && Notches == 4, FString::Printf(TEXT("Temel çerçevesi: 2 kütük yerde + 2 kütük çaprazlama, %d çentik; durum:%s%s"), Notches, *State,
			bAll ? TEXT("") : *DescribeWith(Piece(TEXT("D")))));
		return 2.f;
	});

	// Layer three: two more logs back across the second layer, running along Y again. A cabin corner.
	Script.Add([this, Character, Area]()
	{
		UWorld* World = GetWorld();
		Face(Character, World, Area + FVector(0.f, -700.f, 0.f), 90.f);
		FVector Centre, Axis;
		float Radius, Half;
		ACarvableLog* C = Piece(TEXT("C"));
		ACarvableLog::GetLogAxis(C, Centre, Axis, Radius, Half);
		bool bPlaced = true;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			ACarvableLog* Upper = Piece(Index == 0 ? TEXT("E") : TEXT("F"));
			// Over the first layer's logs: at the x of A and of B.
			const FVector Aim(Area.X + 260.f * Index, Centre.Y, Centre.Z + Radius);
			bPlaced &= Character->TestPickUp(Upper) && Character->TestPlace(false, AimAt(C, Aim));
		}
		bool bAll = true;
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F") })
		{
			bAll &= Piece(Name)->IsAnchored();
		}
		FVector EC, EA, AC, AA;
		float R1, H1;
		ACarvableLog::GetLogAxis(Piece(TEXT("E")), EC, EA, R1, H1);
		ACarvableLog::GetLogAxis(Piece(TEXT("A")), AC, AA, R1, H1);
		Report(bPlaced && bAll, FString::Printf(TEXT("Kabin: üç kat, 6 kütük, hepsi duruyor; üst kat %.0f cm yukarıda"), EC.Z - AC.Z));
		return 2.5f;
	});
	Script.Add([this]()
	{
		bool bAll = true;
		for (const TCHAR* Name : { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D"), TEXT("E"), TEXT("F") })
		{
			bAll &= Piece(Name)->IsAnchored() && Piece(Name)->GetActorLocation().Z > 0.f;
		}
		Report(bAll, TEXT("Kabin 2.5 sn sonra yerinde"));
		return 0.5f;
	});
}

void AMechanicsDemo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAutoPlay)
	{
		TickGuided(DeltaSeconds);
		return;
	}
	TrackGoals(DeltaSeconds);

	StepTime += DeltaSeconds;
	if (WaitTime > 0.f)
	{
		WaitTime -= DeltaSeconds;
		return;
	}

	UWorld* World = GetWorld();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player)
	{
		return;
	}

	auto NextStep = [this](EStep Next, float Delay)
	{
		Step = Next;
		StepTime = 0.f;
		Wait(Delay);
	};

	if (StepTime > StepTimeout && Step != EStep::Done)
	{
		Report(false, FString::Printf(TEXT("Adım %d zaman aşımına uğradı"), static_cast<int32>(Step)));
		Finish();
		return;
	}

	switch (Step)
	{
	case EStep::PickTree:
	{
		AHarvestableActor* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<AHarvestableActor> It(World); It; ++It)
		{
			ExistingBefore.Add(*It);
			const float Dist = FVector::Dist2D(It->GetActorLocation(), Player->GetActorLocation());
			if (It->IsTree() && It->IsAlive() && Dist < BestDist)
			{
				BestDist = Dist;
				Best = *It;
			}
		}
		for (TActorIterator<APickupItem> It(World); It; ++It)
		{
			ExistingBefore.Add(*It);
		}
		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			ExistingBefore.Add(*It);
		}
		bTracking = true;
		if (!Best)
		{
			Report(false, TEXT("Haritada kesilebilir ağaç yok (SM_GB_Tree_* dönüştürülmemiş)"));
			Finish();
			return;
		}
		Tree = Best;
		TreeBase = Best->GetActorLocation();
		LookAt(TreeBase + FVector(0, 0, 150.f), 220.f);
		ChopDirection = (TreeBase - Player->GetActorLocation()).GetSafeNormal2D();
		Info(FString::Printf(TEXT("Hedef ağaç: %s (%.0f m uzakta)"), *Best->GetActorNameOrLabel(), BestDist / 100.f));
		Hits = 0;
		NextStep(EStep::Chop, 1.f);
		return;
	}
	case EStep::Chop:
	{
		AHarvestableActor* Target = Tree.Get();
		if (!Target)
		{
			Report(false, TEXT("Ağaç kesilmeden yok oldu"));
			Finish();
			return;
		}
		const bool bFelled = Hit(Target);
		if (!bFelled)
		{
			Wait(SwingInterval);
			return;
		}
		Report(Hits == 6 || Hits == 8, FString::Printf(TEXT("Ağaç %d balta vuruşunda devrildi (beklenen: çam 6, geniş 8)"), Hits));
		NextStep(EStep::WaitFall, 3.f);
		return;
	}
	case EStep::WaitFall:
	{
		for (TActorIterator<AHarvestableActor> It(World); It; ++It)
		{
			if (It->IsTrunk() && It->GetLogItem() == EForestItem::TrunkLog && !ExistingBefore.Contains(*It)
				&& FVector::Dist2D(It->GetActorLocation(), TreeBase) < SearchRadius)
			{
				Trunk = *It;
			}
		}
		if (!Trunk.IsValid())
		{
			Wait(0.25f);
			return;
		}
		const float Length = Trunk->GetRootComponent()->Bounds.BoxExtent.Size2D() * 2.f;
		Report(!Tree.IsValid(), FString::Printf(TEXT("Ağaç yıkıldı, yerde gövde kaldı (~%.1f m)"), Length / 100.f));
		LookAt(Trunk->GetActorLocation(), 450.f);
		Hits = 0;
		NextStep(EStep::SplitTrunk, 1.f);
		return;
	}
	case EStep::SplitTrunk:
	{
		AHarvestableActor* Target = Trunk.Get();
		if (!Target)
		{
			Report(false, TEXT("Gövde bölünmeden yok oldu"));
			Finish();
			return;
		}
		if (!Hit(Target))
		{
			Wait(SwingInterval);
			return;
		}
		Info(FString::Printf(TEXT("Gövde %d vuruşta kesildi"), Hits));
		NextStep(EStep::WaitBigLogs, 0.5f);
		return;
	}
	case EStep::WaitBigLogs:
		FindBigLogs();
		Report(BigLogs.Num() == 2, FString::Printf(TEXT("Gövde ikiye bölündü: %d büyük kütük (beklenen 2)"), BigLogs.Num()));
		if (BigLogs.Num() == 0)
		{
			Finish();
			return;
		}
		Hits = 0;
		NextStep(EStep::SplitBigLogs, 1.f);
		return;
	case EStep::SplitBigLogs:
	{
		BigLogs.RemoveAll([](const TWeakObjectPtr<AHarvestableActor>& Log) { return !Log.IsValid(); });
		if (BigLogs.Num() == 0)
		{
			Info(FString::Printf(TEXT("Büyük kütükler %d vuruşta kesildi"), Hits));
			NextStep(EStep::WaitLogs, 2.f);
			return;
		}
		AHarvestableActor* Target = BigLogs[0].Get();
		if (Hits == 0 || StepTime < DeltaSeconds * 2.f)
		{
			LookAt(Target->GetActorLocation(), 400.f);
		}
		if (Hit(Target))
		{
			BigLogs.RemoveAt(0);
			if (BigLogs.Num() > 0 && BigLogs[0].IsValid())
			{
				LookAt(BigLogs[0]->GetActorLocation(), 400.f);
			}
		}
		Wait(SwingInterval);
		return;
	}
	case EStep::WaitLogs:
	{
		int32 Logs = 0;
		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			if (It->IsFullLog() && !ExistingBefore.Contains(*It))
			{
				const FVector Where = It->GetActorLocation();
				const bool bNear = FVector::Dist2D(Where, TreeBase) < SearchRadius && Where.Z > TreeBase.Z - 100.f;
				Logs += bNear ? 1 : 0;
				UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo]   kütük %s: %.0f m uzakta, yükseklik %.0f cm%s"), *It->GetName(),
					FVector::Dist2D(Where, TreeBase) / 100.f, Where.Z - TreeBase.Z, bNear ? TEXT("") : TEXT("  <- yerinde değil"));
			}
		}
		for (const TWeakObjectPtr<AActor>& Seen : SeenLogs)
		{
			if (!Seen.IsValid())
			{
				UE_LOG(LogMechanicsDemo, Display, TEXT("[MechanicsDemo]   bir kütük yok oldu (Destroy edildi)"));
			}
		}
		Report(Logs == 4, FString::Printf(TEXT("Büyük kütükler bölündü: %d taşınabilir kütük düştü (beklenen 4)"), Logs));
		NextStep(EStep::PlaceAcross, 0.5f);
		return;
	}
	case EStep::PlaceAcross:
	{
		// Two of the tree's logs: one stays on the ground, the other is laid square across it, as the player does with E.
		TArray<ACarvableLog*> TreeLogs = FreshLogs();
		if (TreeLogs.Num() < 2)
		{
			Report(false, TEXT("Çaprazlama için iki kütük yok"));
			NextStep(EStep::SpawnCarvable, 0.5f);
			return;
		}
		ACarvableLog* Lower = TreeLogs[0];
		ACarvableLog* UpperLog = TreeLogs[1];
		FVector Centre, Axis;
		float LowerRadius, HalfLength;
		ACarvableLog::GetLogAxis(Lower, Centre, Axis, LowerRadius, HalfLength);
		const FVector Viewer = Centre - FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal() * 300.f;
		FCrossPlacement Placement;
		if (!ACarvableLog::ComputeCrossPlacement(Lower, Centre, Viewer, UpperLog, Placement))
		{
			Report(false, TEXT("Çaprazlama yerleşimi hesaplanamadı"));
			NextStep(EStep::SpawnCarvable, 0.5f);
			return;
		}
		ACarvableLog* Upper = ACarvableLog::PlaceAcross(Lower, Placement, UpperLog);
		CornerUpper = Upper;
		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			if (*It != Upper && It->IsAnchored())
			{
				CornerLower = *It;
			}
		}
		if (!Upper || !CornerLower.IsValid())
		{
			Report(false, TEXT("Çaprazlama: kütükler oluşmadı"));
			NextStep(EStep::SpawnCarvable, 0.5f);
			return;
		}
		FVector UpperCentre, UpperAxis, LowerCentre, LowerAxis;
		float UpperRadius, LowerR, UpperHalf, LowerHalf;
		ACarvableLog::GetLogAxis(Upper, UpperCentre, UpperAxis, UpperRadius, UpperHalf);
		ACarvableLog::GetLogAxis(CornerLower.Get(), LowerCentre, LowerAxis, LowerR, LowerHalf);
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(FVector::DotProduct(UpperAxis, LowerAxis))));
		// Height of the upper axis over the lower one, measured where they cross.
		const FVector OnUpper = Upper->GetActorTransform().TransformPosition(FVector(Placement.NotchX, 0.f, 0.f));
		const float Gap = OnUpper.Z - LowerCentre.Z;
		const float Expected = Placement.UpperRadius + Placement.LowerRadius * 0.02f;
		Report(Upper->GetTriangleCount() > 128 && FMath::IsNearlyEqual(Angle, 90.f, 2.f) && FMath::IsNearlyEqual(Gap, Expected, 2.f),
			FString::Printf(TEXT("Kütük çaprazlama oturdu: açı %.0f°, eksenler arası %.1f cm (beklenen %.1f), üstteki kütüğün altına eyer çentiği  -  üçgen %d"),
				Angle, Gap, Expected, Upper->GetTriangleCount()));
		CornerUpperStart = Upper->GetActorLocation();
		// One lower log only, crossed near the upper log's end: the upper one has a single support and tips over.
		StructureLink::Settle(World);
		NextStep(EStep::CheckAcross, 2.5f);
		return;
	}
	case EStep::CheckAcross:
	{
		// Tipped over: its long axis is no longer level (the far end went down to the ground).
		const float Tilt = CornerUpper.IsValid()
			? FMath::RadiansToDegrees(FMath::Asin(FMath::Abs(CornerUpper->GetActorTransform().GetUnitAxis(EAxis::X).Z))) : 0.f;
		const bool bTipped = CornerUpper.IsValid() && CornerLower.IsValid() && !CornerUpper->IsAnchored() && CornerLower->IsAnchored()
			&& Tilt > 3.f;
		Report(bTipped, FString::Printf(TEXT("Tek alt kütük: uçtan geçen kütük tek destekte durmadı, %.0f° devrildi; alttaki yerinde"), Tilt));
		NextStep(EStep::CarryAndStand, 0.5f);
		return;
	}
	case EStep::CarryAndStand:
	{
		// Through the player's own E and left click: one whole log fits on the shoulder, a second does not;
		// the carried one is stood up as a post.
		AForestCharacter* Character = Cast<AForestCharacter>(Player);
		TArray<ACarvableLog*> TreeLogs = FreshLogs();
		if (!Character || TreeLogs.Num() < 2)
		{
			Report(false, TEXT("Taşıma testi için karakter ya da iki kütük yok"));
			NextStep(EStep::SpawnCarvable, 0.5f);
			return;
		}
		ACarvableLog* First = TreeLogs[0];
		const bool bFirst = Character->TestPickUp(First);
		const float Load = Character->GetCarryLoad();
		const bool bSecond = Character->TestPickUp(TreeLogs[1]);
		Report(bFirst && !bSecond && First->IsCarried() && Load > 0.9f && Load <= 1.f,
			FString::Printf(TEXT("Omuz hacme göre: tam kütük alındı (yük %%%.0f), ikinci tam kütük sığmadı"), Load * 100.f));

		// A post on clear ground (aimed at the ground itself, so nothing lying about turns it into a crossing).
		FHitResult Ground;
		const FVector Spot(-1600.f, -1200.f, 0.f);
		const bool bGround = World->LineTraceSingleByObjectType(Ground, Spot + FVector(0, 0, 1000.f), Spot - FVector(0, 0, 2000.f),
			FCollisionObjectQueryParams(ECC_WorldStatic));
		const bool bPostPlaced = bGround && Character->TestPlace(/*bStanding*/ true, Ground);
		const FVector Up = First->GetActorTransform().GetUnitAxis(EAxis::X);
		const FVector Base = First->GetActorTransform().TransformPosition(FVector(First->GetLocalBox().Min.X, 0.f, 0.f));
		const bool bUpright = Up.Z > 0.99f;
		const bool bOnGround = bGround && FMath::Abs(Base.Z - Ground.ImpactPoint.Z) < 3.f;
		Report(bPostPlaced && bUpright && bOnGround && First->IsAnchored() && !First->IsCarried() && Character->GetCarriedCount() == 0,
			FString::Printf(TEXT("Direk dikildi: dikeylik %.3f, alt uç zeminden %.1f cm, sabit %s, omuz boş"),
				Up.Z, bGround ? Base.Z - Ground.ImpactPoint.Z : -1.f, First->IsAnchored() ? TEXT("evet") : TEXT("hayır")));
		FrameStage = 0;
		NextStep(EStep::Frame, 0.5f);
		return;
	}
	case EStep::Frame:
		if (!RunFrameStage(Cast<AForestCharacter>(Player)))
		{
			NextStep(EStep::Shaping, 0.5f);
			return;
		}
		// A stage still waiting for something to settle runs again, on its own clock.
		if (bRepeatFrameStage)
		{
			bRepeatFrameStage = false;
			return;
		}
		++FrameStage;
		StepTime = 0.f;
		return;
	case EStep::Shaping:
		RunShaping(Cast<AForestCharacter>(Player));
		BuildScript(Cast<AForestCharacter>(Player));
		NextStep(EStep::Script, 0.5f);
		return;
	case EStep::Script:
	{
		if (ScriptIndex >= Script.Num())
		{
			NextStep(EStep::SpawnCarvable, 0.5f);
			return;
		}
		const float Delay = Script[ScriptIndex++]();
		StepTime = 0.f;
		Wait(FMath::Max(Delay, 0.1f));
		return;
	}
	case EStep::SpawnCarvable:
	{
		// Carve one of the logs the tree gave, right where it lies.
		TArray<ACarvableLog*> TreeLogs = FreshLogs();
		Carvable = TreeLogs.Num() > 0 ? TreeLogs[0] : nullptr;
		Report(Carvable.IsValid(), Carvable.IsValid()
			? FString::Printf(TEXT("Ağaçtan çıkan kütük doğrudan oyulabilir (%.0f cm, çap %.0f cm)"), Carvable->GetLength(), Carvable->GetRadius() * 2.f)
			: FString(TEXT("Oymak için ağaçtan çıkan kütük kalmadı")));
		if (!Carvable.IsValid())
		{
			Finish();
			return;
		}
		LookAt(Carvable->GetActorLocation(), 300.f);
		CarveIndex = 0;
		NextStep(EStep::Carve, 2.f);
		return;
	}
	case EStep::Carve:
		if (!RunCarveStep(CarveIndex))
		{
			Finish();
			return;
		}
		++CarveIndex;
		if (CarveIndex >= 6)
		{
			NextStep(EStep::Done, 1.5f);
		}
		else
		{
			Wait(1.5f);
		}
		return;
	case EStep::Done:
		Finish();
		return;
	}
}
