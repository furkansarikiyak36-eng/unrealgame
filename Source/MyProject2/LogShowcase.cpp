// Copyright Epic Games, Inc. All Rights Reserved.

#include "LogShowcase.h"
#include "CarvableLog.h"
#include "ForestCharacter.h"
#include "HaulCart.h"
#include "WorldCore/ExcavationTerrainChunkSubsystem.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

namespace
{
	using EPlace = AForestCharacter::ECarryPlace;
	using EHold = AForestCharacter::EShowcaseHold;

	uint8 P(EPlace Place) { return static_cast<uint8>(Place); }
}

ALogShowcase::ALogShowcase()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ALogShowcase::BeginPlay()
{
	Super::BeginPlay();
	// After the player is spawned and the ground is in.
	FTimerHandle Start;
	GetWorldTimerManager().SetTimer(Start, [this]() { BuildAll(); }, 1.5f, false);
}

UStaticMesh* ALogShowcase::Mesh(int32 Length, const TCHAR* Profile) const
{
	const FString Name = Profile ? FString::Printf(TEXT("SM_RawLog_%d_%s"), Length, Profile) : FString::Printf(TEXT("SM_RawLog_%d"), Length);
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/UnrealExport/%s.%s"), *Name, *Name));
}

ACarvableLog* ALogShowcase::Spawn(UStaticMesh* LogMesh) const
{
	return LogMesh ? ACarvableLog::SpawnFromStaticMesh(GetWorld(), LogMesh, FTransform(O + FVector(0.f, 0.f, 1500.f))) : nullptr;
}

FVector ALogShowcase::Ground(const FVector& Point, AActor*& OutActor) const
{
	// The diggable ground streams round the player; build it here first.
	if (UExcavationTerrainChunkSubsystem* Terrain = GetWorld()->GetSubsystem<UExcavationTerrainChunkSubsystem>())
	{
		Terrain->EnsureGroundAt(FVector(Point.X, Point.Y, 0.f));
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShowcaseGround), false);
	if (Player.IsValid())
	{
		Params.AddIgnoredActor(Player.Get());
	}
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Point + FVector(0.f, 0.f, 1000.f), Point - FVector(0.f, 0.f, 1000.f),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutActor = Hit.GetActor();
		return Hit.ImpactPoint;
	}
	OutActor = nullptr;
	return FVector(Point.X, Point.Y, 0.f);
}

bool ALogShowcase::Note(const TCHAR* What, uint8 Chosen, uint8 Wanted)
{
	const EPlace Got = static_cast<EPlace>(Chosen);
	const bool bOk = Chosen == Wanted;
	Report += FString::Printf(TEXT("    %s %s: %s%s\n"), bOk ? TEXT("OK ") : TEXT("XX "), What, AForestCharacter::CarryPlaceName(Got),
		bOk ? TEXT("") : *FString::Printf(TEXT("  (istenen: %s)"), AForestCharacter::CarryPlaceName(static_cast<EPlace>(Wanted))));
	if (bOk)
	{
		++Placed;
	}
	else
	{
		++Failed;
		StationNotes += FString::Printf(TEXT("\nolmadı: %s"), What);
	}
	return bOk;
}

ACarvableLog* ALogShowcase::LayAt(UStaticMesh* LogMesh, const FVector& Point, float PieceYaw, const TCHAR* What)
{
	ACarvableLog* Piece = Spawn(LogMesh);
	if (!Piece || !Player.IsValid())
	{
		Note(What, P(EPlace::None), P(EPlace::Lay));
		return nullptr;
	}
	AActor* Hit = nullptr;
	const FVector Spot = Ground(Point, Hit);
	// The piece lies across the view.
	const float ViewYaw = PieceYaw - 90.f;
	const FVector View = FRotator(0.f, ViewYaw, 0.f).Vector();
	Player->SetActorLocation(Spot - View * 250.f + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	Note(What, P(Player->PlaceForShowcase(Piece, Hit, Spot, FVector::UpVector, ViewYaw, EHold::Lay)), P(EPlace::Lay));
	return Piece;
}

ACarvableLog* ALogShowcase::StandAt(UStaticMesh* LogMesh, const FVector& Point, float Yaw, const TCHAR* What)
{
	ACarvableLog* Piece = Spawn(LogMesh);
	if (!Piece || !Player.IsValid())
	{
		Note(What, P(EPlace::None), P(EPlace::Stand));
		return nullptr;
	}
	AActor* Hit = nullptr;
	const FVector Spot = Ground(Point, Hit);
	const FVector View = FRotator(0.f, Yaw, 0.f).Vector();
	Player->SetActorLocation(Spot - View * 250.f + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	Note(What, P(Player->PlaceForShowcase(Piece, Hit, Spot, FVector::UpVector, Yaw, EHold::Stand)), P(EPlace::Stand));
	return Piece;
}

ACarvableLog* ALogShowcase::OnLog(UStaticMesh* LogMesh, ACarvableLog* Lower, float Along, float ViewYaw, uint8 Hold, float Slope,
	const TCHAR* What, uint8 Wanted)
{
	FVector C, A;
	float Radius, Half;
	ACarvableLog* Piece = Lower ? Spawn(LogMesh) : nullptr;
	if (!Piece || !Player.IsValid() || !ACarvableLog::GetLogAxis(Lower, C, A, Radius, Half))
	{
		if (Piece)
		{
			Piece->Destroy();
		}
		Note(What, P(EPlace::None), Wanted);
		return nullptr;
	}
	const FVector Aim = C + A * Along + FVector(0.f, 0.f, Radius);
	const FVector View = FRotator(0.f, ViewYaw, 0.f).Vector();
	Player->SetActorLocation(Aim - View * 250.f + FVector(0.f, 0.f, 60.f), false, nullptr, ETeleportType::TeleportPhysics);
	const uint8 Got = P(Player->PlaceForShowcase(Piece, Lower, Aim, FVector::UpVector, ViewYaw, static_cast<EHold>(Hold), Slope));
	Note(What, Got, Wanted);
	return Piece;
}

ACarvableLog* ALogShowcase::OnPostTop(UStaticMesh* LogMesh, ACarvableLog* Post, float ViewYaw, const TCHAR* What, uint8 Wanted)
{
	FVector C, A;
	float Radius, Half;
	ACarvableLog* Piece = Post ? Spawn(LogMesh) : nullptr;
	if (!Piece || !Player.IsValid() || !ACarvableLog::GetLogAxis(Post, C, A, Radius, Half))
	{
		if (Piece)
		{
			Piece->Destroy();
		}
		Note(What, P(EPlace::None), Wanted);
		return nullptr;
	}
	const FVector Top = C + A * (A.Z > 0.f ? Half : -Half);
	const FVector View = FRotator(0.f, ViewYaw, 0.f).Vector();
	Player->SetActorLocation(Top - View * 250.f, false, nullptr, ETeleportType::TeleportPhysics);
	Note(What, P(Player->PlaceForShowcase(Piece, Post, Top, FVector::UpVector, ViewYaw, EHold::Lay)), Wanted);
	return Piece;
}

void ALogShowcase::BeginStation(int32 Index, const FString& Title)
{
	O = GetActorLocation() + F * (StationSpacingCm * Index);
	StationTitle = FString::Printf(TEXT("%d. %s"), Index + 1, *Title);
	StationNotes.Reset();
	StationSignSpot = O - F * 450.f;
	Report += StationTitle + TEXT("\n");
}

void ALogShowcase::EndStation()
{
	Sign(StationTitle + StationNotes);
}

void ALogShowcase::Sign(const FString& Title)
{
	AActor* GroundActor = nullptr;
	const FVector Spot = Ground(StationSignSpot, GroundActor) + FVector(0.f, 0.f, 280.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATextRenderActor* Text = GetWorld()->SpawnActor<ATextRenderActor>(Spot, FRotator(0.f, FYaw + 180.f, 0.f), Params))
	{
		UTextRenderComponent* Render = Text->GetTextRender();
		Render->SetText(FText::FromString(Title));
		Render->SetHorizontalAlignment(EHTA_Center);
		Render->SetVerticalAlignment(EVRTA_TextBottom);
		Render->SetWorldSize(32.f);
		Render->SetTextRenderColor(FColor(255, 235, 190));
	}
}

FString ALogShowcase::BuildAll()
{
	if (bBuilt)
	{
		return Report;
	}
	Player = Cast<AForestCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player.IsValid())
	{
		return TEXT("no ForestCharacter");
	}
	bBuilt = true;
	const FVector PlayerStart = Player->GetActorLocation();
	const FRotator PlayerFacing = Player->GetActorRotation();
	F = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	R = FVector::CrossProduct(FVector::UpVector, F);
	FYaw = F.Rotation().Yaw;
	Report = TEXT("Kütük vitrini — her parça oyuncunun yerleştirmesiyle\n");

	StationCutChain();
	StationWall();
	StationCabin();
	StationPlatform();
	StationRoof();
	StationPalisade();
	StationDiagonal();
	StationSteps();
	StationFurniture();
	StationHauling();

	Report += FString::Printf(TEXT("Toplam: %d parça istendiği gibi, %d parça olmadı\n"), Placed, Failed);
	Player->SetActorLocationAndRotation(PlayerStart, PlayerFacing, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerController* Controller = Cast<APlayerController>(Player->GetController()))
	{
		Controller->SetControlRotation(PlayerFacing);
	}
	FFileHelper::SaveStringToFile(Report, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("LogShowcase.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
	UE_LOG(LogTemp, Display, TEXT("%s"), *Report);
	return Report;
}

// 1. Every length and every shape a raw log is worked into, side by side.
void ALogShowcase::StationCutChain()
{
	BeginStation(0, TEXT("Kesim zinciri: 600 300 150 75, yarım, çeyrek, düzlenmiş, kalas, tahta, oluk, yalak"));
	struct FPiece { int32 Length; const TCHAR* Profile; const TCHAR* Name; };
	const FPiece Pieces[] = {
		{ 600, nullptr, TEXT("600") }, { 300, nullptr, TEXT("300") }, { 150, nullptr, TEXT("150") }, { 75, nullptr, TEXT("75") },
		{ 300, TEXT("Half"), TEXT("yarım") }, { 300, TEXT("Quarter_04"), TEXT("çeyrek") }, { 300, TEXT("Hewn1"), TEXT("tek yüz düz") },
		{ 300, TEXT("Hewn2"), TEXT("iki yüz düz") }, { 300, TEXT("Kalas"), TEXT("kalas") }, { 300, TEXT("Plank"), TEXT("tahta") },
		{ 300, TEXT("Gutter"), TEXT("oluk") }, { 300, TEXT("Trough"), TEXT("yalak") },
	};
	int32 Index = 0;
	for (const FPiece& Piece : Pieces)
	{
		LayAt(Mesh(Piece.Length, Piece.Profile), O + R * (Index * 75.f - 400.f), FYaw, Piece.Name);
		++Index;
	}
	EndStation();
}

// 2. A horizontal log wall six courses high, with a door opening and a window opening cut into it.
void ALogShowcase::StationWall()
{
	BeginStation(1, TEXT("Yatay kütük duvar (6 sıra), kapı ve pencere açıklığı"));
	TArray<ACarvableLog*> Courses;
	ACarvableLog* Top = LayAt(Mesh(600), O, FYaw + 90.f, TEXT("taban"));
	Courses.Add(Top);
	for (int32 Course = 1; Course < 6 && Top; ++Course)
	{
		Top = OnLog(Mesh(600), Top, 0.f, FYaw, static_cast<uint8>(EHold::Lay), 0.f, *FString::Printf(TEXT("sıra %d"), Course + 1), P(EPlace::Stack));
		if (Top)
		{
			Courses.Add(Top);
		}
	}
	// Door: two modules wide through the lower four courses; window: two modules in courses four and five. An opening
	// splits a log in two, so each cut goes into whichever piece of that course holds the spot.
	TArray<TArray<ACarvableLog*>> Pieces;
	for (ACarvableLog* Log : Courses)
	{
		Pieces.Add({ Log });
	}
	auto Cut = [this, &Pieces](int32 Course, float WorldAlongR, const TCHAR* What)
	{
		ACarvableLog* Into = nullptr;
		float LocalX = 0.f;
		for (ACarvableLog* Log : Pieces[Course])
		{
			const float X = Log ? Log->GetActorTransform().InverseTransformPosition(O + R * WorldAlongR + FVector(0.f, 0.f, Log->GetActorLocation().Z - O.Z)).X : 0.f;
			if (Log && X > Log->GetMinX() && X < Log->GetMaxX())
			{
				Into = Log;
				LocalX = X;
			}
		}
		ACarvableLog* Other = Into ? Into->CutOpening(LocalX, 2) : nullptr;
		Report += FString::Printf(TEXT("    %s %s\n"), Other ? TEXT("OK ") : TEXT("XX "), What);
		Other ? ++Placed : ++Failed;
		if (Other)
		{
			Pieces[Course].Add(Other);
		}
		else
		{
			StationNotes += FString::Printf(TEXT("\nolmadı: %s"), What);
		}
	};
	for (int32 Course = 0; Course < FMath::Min(4, Courses.Num()); ++Course)
	{
		Cut(Course, -150.f, *FString::Printf(TEXT("kapı açıklığı, sıra %d"), Course + 1));
	}
	for (int32 Course = 3; Course < FMath::Min(5, Courses.Num()); ++Course)
	{
		Cut(Course, 150.f, *FString::Printf(TEXT("pencere açıklığı, sıra %d"), Course + 1));
	}
	EndStation();
}

// 3. A small cabin: four walls of 300 cm logs joined at the corners, courses alternating direction.
void ALogShowcase::StationCabin()
{
	BeginStation(2, TEXT("Kütük kulübe: dört duvar, köşe geçmeli (6 sıra)"));
	const float Span = 225.f;
	ACarvableLog* A = LayAt(Mesh(300), O, FYaw, TEXT("taban sol"));
	ACarvableLog* B = LayAt(Mesh(300), O + R * Span, FYaw, TEXT("taban sağ"));
	TArray<ACarvableLog*> Lower = { A, B };
	bool bAlongF = true;
	for (int32 Course = 1; Course < 6 && Lower.Num() == 2 && Lower[0] && Lower[1]; ++Course)
	{
		// The next course runs the other way, crossing both logs of this one near their ends.
		TArray<ACarvableLog*> Next;
		for (int32 End = 0; End < 2; ++End)
		{
			ACarvableLog* On = Lower[0];
			FVector C, Axis;
			float Radius, Half;
			ACarvableLog::GetLogAxis(On, C, Axis, Radius, Half);
			const float Along = (End == 0 ? -1.f : 1.f) * (Half - 35.f);
			// Facing along the lower course makes the carried piece lie square across it; standing on the outer side
			// lets it run inwards over the other log of the course.
			const FVector LowerDir = FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal();
			const float ViewYaw = LowerDir.Rotation().Yaw;
			FVector Aim = C + Axis * Along + FVector(0.f, 0.f, Radius);
			const FVector Inward = (Lower[1]->GetActorLocation() - Lower[0]->GetActorLocation()).GetSafeNormal2D();
			Player->SetActorLocation(Aim - Inward * 220.f + FVector(0.f, 0.f, 60.f), false, nullptr, ETeleportType::TeleportPhysics);
			ACarvableLog* Piece = Spawn(Mesh(300));
			const uint8 Got = Piece ? P(Player->PlaceForShowcase(Piece, On, Aim, FVector::UpVector, ViewYaw, EHold::Lay)) : P(EPlace::None);
			Note(*FString::Printf(TEXT("sıra %d köşe %d"), Course + 1, End + 1), Got, P(EPlace::Across));
			Next.Add(Piece);
		}
		Lower = Next;
		bAlongF = !bAlongF;
	}
	EndStation();
}

// 4. A raised platform: posts, beams, braces, floor boards; a ramp up to it and a second storey on it.
void ALogShowcase::StationPlatform()
{
	BeginStation(3, TEXT("Platform: direk, kiriş, payanda, döşeme; rampa ve ikinci kat"));
	const float Span = 225.f;
	ACarvableLog* P00 = StandAt(Mesh(150), O, FYaw, TEXT("direk 1"));
	ACarvableLog* P10 = StandAt(Mesh(150), O + F * Span, FYaw, TEXT("direk 2"));
	ACarvableLog* P01 = StandAt(Mesh(150), O + R * Span, FYaw, TEXT("direk 3"));
	ACarvableLog* P11 = StandAt(Mesh(150), O + F * Span + R * Span, FYaw, TEXT("direk 4"));
	ACarvableLog* BeamA = OnPostTop(Mesh(300), P00, FYaw, TEXT("kiriş 1"), P(EPlace::Beam));
	ACarvableLog* BeamB = OnPostTop(Mesh(300), P01, FYaw, TEXT("kiriş 2"), P(EPlace::Beam));
	OnPostTop(Mesh(150), P00, FYaw, TEXT("payanda 1"), P(EPlace::Brace));
	OnPostTop(Mesh(150), P01, FYaw, TEXT("payanda 2"), P(EPlace::Brace));
	for (int32 Board = 0; Board < 4 && BeamA; ++Board)
	{
		OnLog(Mesh(300, TEXT("Plank")), BeamA, -90.f + Board * 60.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f,
			*FString::Printf(TEXT("döşeme tahtası %d"), Board + 1), P(EPlace::Floor));
	}
	// Ramp: a log from the ground up to the platform's front edge.
	{
		ACarvableLog* Ramp = Spawn(Mesh(300));
		AActor* Hit = nullptr;
		const FVector Low = Ground(O - F * 255.f + R * (Span * 0.5f), Hit);
		Player->SetActorLocation(Low - F * 250.f + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
		Note(TEXT("rampa"), Ramp ? P(Player->PlaceForShowcase(Ramp, Hit, Low, FVector::UpVector, FYaw, EHold::Slope, 30.f)) : P(EPlace::None), P(EPlace::Slope));
	}
	// Second storey: posts standing on the beams, beams on them.
	ACarvableLog* Q0 = OnLog(Mesh(150), BeamA, -100.f, FYaw, static_cast<uint8>(EHold::Stand), 0.f, TEXT("2. kat direk 1"), P(EPlace::Stand));
	ACarvableLog* Q1 = OnLog(Mesh(150), BeamB, -100.f, FYaw, static_cast<uint8>(EHold::Stand), 0.f, TEXT("2. kat direk 2"), P(EPlace::Stand));
	OnLog(Mesh(150), BeamA, 100.f, FYaw, static_cast<uint8>(EHold::Stand), 0.f, TEXT("2. kat direk 3"), P(EPlace::Stand));
	OnLog(Mesh(150), BeamB, 100.f, FYaw, static_cast<uint8>(EHold::Stand), 0.f, TEXT("2. kat direk 4"), P(EPlace::Stand));
	OnPostTop(Mesh(300), Q0, FYaw, TEXT("2. kat kiriş 1"), P(EPlace::Beam));
	OnPostTop(Mesh(300), Q1, FYaw, TEXT("2. kat kiriş 2"), P(EPlace::Beam));
	(void)P10;
	(void)P11;
	EndStation();
}

// 5. A lean-to roof frame: rafters from a low wall up over a higher one.
void ALogShowcase::StationRoof()
{
	BeginStation(4, TEXT("Eğik çatı: alçak ve yüksek duvar, üstünde çatı kirişleri"));
	ACarvableLog* Low = LayAt(Mesh(300), O, FYaw, TEXT("alçak duvar"));
	ACarvableLog* High = LayAt(Mesh(300), O + R * 225.f, FYaw, TEXT("yüksek duvar 1"));
	for (int32 Course = 2; Course <= 3 && High; ++Course)
	{
		High = OnLog(Mesh(300), High, 0.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f, *FString::Printf(TEXT("yüksek duvar %d"), Course), P(EPlace::Stack));
	}
	for (int32 Rafter = 0; Rafter < 3 && Low; ++Rafter)
	{
		OnLog(Mesh(300), Low, -100.f + Rafter * 100.f, FYaw + 90.f, static_cast<uint8>(EHold::Slope), 18.f,
			*FString::Printf(TEXT("çatı kirişi %d"), Rafter + 1), P(EPlace::Slope));
	}
	EndStation();
}

// 6. A palisade: logs stood side by side.
void ALogShowcase::StationPalisade()
{
	BeginStation(5, TEXT("Savunma duvarı (palisat): yan yana dikili kütükler"));
	for (int32 Index = 0; Index < 8; ++Index)
	{
		StandAt(Mesh(300), O + R * (Index * 40.f - 140.f), FYaw, *FString::Printf(TEXT("kazık kütük %d"), Index + 1));
	}
	EndStation();
}

// 7. Courses at +45 and -45 degrees.
void ALogShowcase::StationDiagonal()
{
	BeginStation(6, TEXT("45° çapraz sıralar: +45, üstüne 4 sıra, en üstte -45"));
	ACarvableLog* Base = LayAt(Mesh(300), O, FYaw + 90.f, TEXT("taban"));
	ACarvableLog* Top = OnLog(Mesh(300), Base, 0.f, FYaw + 45.f, static_cast<uint8>(EHold::Lay), 0.f, TEXT("+45"), P(EPlace::Diagonal));
	for (int32 Course = 0; Course < 4 && Top; ++Course)
	{
		FVector C, A;
		float Radius, Half;
		ACarvableLog::GetLogAxis(Top, C, A, Radius, Half);
		Top = OnLog(Mesh(300), Top, 0.f, FVector(A.X, A.Y, 0.f).Rotation().Yaw - 90.f, static_cast<uint8>(EHold::Lay), 0.f,
			*FString::Printf(TEXT("+45 sıra %d"), Course + 2), P(EPlace::Stack));
	}
	if (Top)
	{
		FVector C, A;
		float Radius, Half;
		ACarvableLog::GetLogAxis(Top, C, A, Radius, Half);
		OnLog(Mesh(300), Top, 60.f, FVector(A.X, A.Y, 0.f).Rotation().Yaw, static_cast<uint8>(EHold::Lay), 0.f, TEXT("-45 (dik çapraz)"), P(EPlace::Across));
	}
	EndStation();
}

// 8. Steps: shorter logs stacked toward one end.
void ALogShowcase::StationSteps()
{
	BeginStation(7, TEXT("Basamak (yaklaşık merdiven): 300, 150, 75 üst üste"));
	ACarvableLog* Base = LayAt(Mesh(300), O, FYaw + 90.f, TEXT("basamak 1"));
	ACarvableLog* Mid = OnLog(Mesh(150), Base, 75.f, FYaw, static_cast<uint8>(EHold::Lay), 0.f, TEXT("basamak 2"), P(EPlace::Stack));
	OnLog(Mesh(75), Mid, 37.f, FYaw, static_cast<uint8>(EHold::Lay), 0.f, TEXT("basamak 3"), P(EPlace::Stack));
	EndStation();
}

// 9. A bench (two short logs, a plank across) and a table (legs three logs high, a plank across).
void ALogShowcase::StationFurniture()
{
	BeginStation(8, TEXT("Bank ve masa: kısa kütük ayaklar, üstünde tahta"));
	ACarvableLog* BenchA = LayAt(Mesh(75), O, FYaw, TEXT("bank ayağı 1"));
	LayAt(Mesh(75), O + R * 75.f, FYaw, TEXT("bank ayağı 2"));
	OnLog(Mesh(150, TEXT("Plank")), BenchA, 0.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f, TEXT("bank oturağı"), P(EPlace::Floor));
	ACarvableLog* LegA = LayAt(Mesh(75), O + F * 300.f, FYaw, TEXT("masa ayağı 1"));
	ACarvableLog* LegB = LayAt(Mesh(75), O + F * 300.f + R * 75.f, FYaw, TEXT("masa ayağı 2"));
	for (int32 Course = 2; Course <= 3; ++Course)
	{
		LegA = LegA ? OnLog(Mesh(75), LegA, 0.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f, *FString::Printf(TEXT("masa ayağı 1 sıra %d"), Course), P(EPlace::Stack)) : nullptr;
		LegB = LegB ? OnLog(Mesh(75), LegB, 0.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f, *FString::Printf(TEXT("masa ayağı 2 sıra %d"), Course), P(EPlace::Stack)) : nullptr;
	}
	OnLog(Mesh(150, TEXT("Plank")), LegA, 0.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f, TEXT("masa tablası"), P(EPlace::Floor));
	EndStation();
}

// 10. Hauling and storage: a cart, a sled and a rack, loaded by hand.
void ALogShowcase::StationHauling()
{
	BeginStation(9, TEXT("Taşıma ve depolama: el arabası, kızak, kütük rafı"));
	const EHaulKind Kinds[3] = { EHaulKind::Cart, EHaulKind::Sled, EHaulKind::Rack };
	const int32 Loads[3] = { 2, 4, 8 };
	for (int32 Kind = 0; Kind < 3; ++Kind)
	{
		AActor* Hit = nullptr;
		const FVector Spot = Ground(O + R * (Kind * 350.f - 350.f), Hit);
		AHaulCart* Cart = AHaulCart::Spawn(GetWorld(), Spot, FYaw, Kinds[Kind]);
		for (int32 Index = 0; Index < Loads[Kind] && Cart; ++Index)
		{
			ACarvableLog* Piece = Spawn(Mesh(150));
			Player->SetActorLocation(Spot - F * 250.f + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			const uint8 Got = Piece ? P(Player->PlaceForShowcase(Piece, Cart, Cart->GetActorLocation() + FVector(0.f, 0.f, 50.f), FVector::UpVector, FYaw, EHold::Lay)) : P(EPlace::None);
			Note(*FString::Printf(TEXT("%s yük %d"), *AHaulCart::KindName(Kinds[Kind]), Index + 1), Got, P(EPlace::Load));
		}
	}
	EndStation();
}
