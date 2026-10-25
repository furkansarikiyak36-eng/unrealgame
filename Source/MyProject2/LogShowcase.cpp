// Copyright Epic Games, Inc. All Rights Reserved.

#include "LogShowcase.h"
#include "StructureLink.h"
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
	if (!bBuildOnPlay)
	{
		return;
	}
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
	// The ground round the whole station (things dropped there must land on it).
	if (UExcavationTerrainChunkSubsystem* Terrain = GetWorld()->GetSubsystem<UExcavationTerrainChunkSubsystem>())
	{
		for (int32 X = -1; X <= 1; ++X)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				const FVector At = O + F * (X * 800.f) + R * (Y * 800.f);
				Terrain->EnsureGroundAt(FVector(At.X, At.Y, 0.f));
			}
		}
	}
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
	StationModuleOpening();
	StationFaceSockets();
	StationUnderSockets();
	StationLogHouse();
	StationPostBeams();
	StationLengthMatrix();

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

FString ALogShowcase::BuildStation(int32 Index)
{
	Player = Cast<AForestCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player.IsValid())
	{
		return TEXT("no ForestCharacter");
	}
	if (Index < 0 || Index >= StationCount())
	{
		return TEXT("no such station");
	}
	const FVector PlayerStart = Player->GetActorLocation();
	const FRotator PlayerFacing = Player->GetActorRotation();
	F = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	R = FVector::CrossProduct(FVector::UpVector, F);
	FYaw = F.Rotation().Yaw;
	Report = FString::Printf(TEXT("Kütük vitrini, istasyon %d — her parça oyuncunun yerleştirmesiyle\n"), Index + 1);
	Placed = 0;
	Failed = 0;
	switch (Index)
	{
	case 0: StationCutChain(); break;
	case 1: StationWall(); break;
	case 2: StationCabin(); break;
	case 3: StationPlatform(); break;
	case 4: StationRoof(); break;
	case 5: StationPalisade(); break;
	case 6: StationDiagonal(); break;
	case 7: StationSteps(); break;
	case 8: StationFurniture(); break;
	case 9: StationHauling(); break;
	case 10: StationModuleOpening(); break;
	case 11: StationFaceSockets(); break;
	case 12: StationUnderSockets(); break;
	case 13: StationLogHouse(); break;
	case 14: StationPostBeams(); break;
	default: StationLengthMatrix(); break;
	}
	Report += FString::Printf(TEXT("Toplam: %d parça istendiği gibi, %d parça olmadı\n"), Placed, Failed);
	Player->SetActorLocationAndRotation(PlayerStart, PlayerFacing, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerController* Controller = Cast<APlayerController>(Player->GetController()))
	{
		Controller->SetControlRotation(PlayerFacing);
	}
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
		LayAt(Mesh(Piece.Length, Piece.Profile), O + R * (Index * 38.f - 220.f), FYaw, Piece.Name);
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
			const float X = IsValid(Log) ? Log->GetActorTransform().InverseTransformPosition(O + R * WorldAlongR + FVector(0.f, 0.f, Log->GetActorLocation().Z - O.Z)).X : 0.f;
			if (IsValid(Log) && X > Log->GetMinX() && X < Log->GetMaxX())
			{
				Into = Log;
				LocalX = X;
			}
		}
		ACarvableLog* Other = Into ? Into->CutOpening(LocalX, 2, O - F * 400.f) : nullptr;
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
	BeginStation(2, TEXT("Kütük kulübe: dört duvar, köşe geçmeli (6 sıra, 2.25 m oda)"));
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
			Note(*FString::Printf(TEXT("sıra %d köşe %d"), Course + 1, End + 1), Got, P(EPlace::Corner));
			Next.Add(Piece);
			if (Piece)
			{
				// Where it went, in the room's own frame (F forward, R right of the first base log): a square room puts
				// the logs of one course on two lines 225 apart.
				const FVector Local = Piece->GetActorLocation() - O;
				const float Yaw = FMath::Fmod(FMath::Abs(FRotator::NormalizeAxis(Piece->GetActorRotation().Yaw - FYaw)), 180.f);
				Report += FString::Printf(TEXT("      %s  F=%.0f R=%.0f yükseklik=%.0f\n"), Yaw < 45.f || Yaw > 135.f ? TEXT("boyuna (F)") : TEXT("enine (R)"),
					FVector::DotProduct(Local, F), FVector::DotProduct(Local, R), Local.Z);
			}
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
	// The two beams under the two ends of a 300 cm board (the user's layout).
	const float Span = 285.f;
	ACarvableLog* P00 = StandAt(Mesh(150), O, FYaw, TEXT("direk 1"));
	ACarvableLog* P10 = StandAt(Mesh(150), O + F * Span, FYaw, TEXT("direk 2"));
	ACarvableLog* P01 = StandAt(Mesh(150), O + R * Span, FYaw, TEXT("direk 3"));
	ACarvableLog* P11 = StandAt(Mesh(150), O + F * Span + R * Span, FYaw, TEXT("direk 4"));
	ACarvableLog* BeamA = OnPostTop(Mesh(300), P00, FYaw, TEXT("kiriş 1"), P(EPlace::Beam));
	ACarvableLog* BeamB = OnPostTop(Mesh(300), P01, FYaw, TEXT("kiriş 2"), P(EPlace::Beam));
	OnPostTop(Mesh(150), P00, FYaw, TEXT("payanda 1"), P(EPlace::Brace));
	OnPostTop(Mesh(150), P01, FYaw, TEXT("payanda 2"), P(EPlace::Brace));
	for (int32 Board = 0; Board < 9 && BeamA; ++Board)
	{
		// Boards 30 cm centre to centre along the beam (the user's spacing).
		OnLog(Mesh(300, TEXT("Plank")), BeamA, -120.f + Board * 30.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f,
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
	// A log held slanted and aimed at a standing post leans on it: lower end on the ground, upper end on the post top.
	// It must also stay there (not be released as unsupported) after the structure settles.
	if (ACarvableLog* Post = StandAt(Mesh(150), O - R * 250.f, FYaw, TEXT("yaslama direği")))
	{
		FVector C, A;
		float Radius, Half;
		ACarvableLog* Lean = ACarvableLog::GetLogAxis(Post, C, A, Radius, Half) ? Spawn(Mesh(300)) : nullptr;
		if (Lean)
		{
			const FVector Top = C + FVector(0.f, 0.f, Half);
			Player->SetActorLocation(Top - F * 250.f + FVector(0.f, 0.f, -60.f), false, nullptr, ETeleportType::TeleportPhysics);
			const uint8 Got = P(Player->PlaceForShowcase(Lean, Post, Top, FVector::UpVector, FYaw, EHold::Slope, 45.f));
			Note(TEXT("direğe yaslanan kütük"), Got, P(EPlace::Slope));
			Note(TEXT("yaslanan kütük yerinde kaldı"), Lean->IsAnchored() ? P(EPlace::Slope) : P(EPlace::None), P(EPlace::Slope));
			Note(TEXT("yaslama direği yerinde kaldı"), Post->IsAnchored() ? P(EPlace::Stand) : P(EPlace::None), P(EPlace::Stand));
		}
	}
	// The other order: a log laid slanted on the ground first (its raised end in the air), then a post stood under that end.
	{
		ACarvableLog* Slant = Spawn(Mesh(300));
		AActor* GroundHit = nullptr;
		const FVector Foot = Ground(O - R * 450.f, GroundHit);
		if (Slant)
		{
			Player->SetActorLocation(Foot - F * 250.f + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			Note(TEXT("yere eğik kütük"), P(Player->PlaceForShowcase(Slant, GroundHit, Foot, FVector::UpVector, FYaw, EHold::Slope, 30.f)), P(EPlace::Slope));
			Note(TEXT("yere eğik kütük yerinde kaldı"), Slant->IsAnchored() ? P(EPlace::Slope) : P(EPlace::None), P(EPlace::Slope));
			FVector C, A;
			float Radius, Half;
			if (Slant->IsAnchored() && ACarvableLog::GetLogAxis(Slant, C, A, Radius, Half))
			{
				const FVector RaisedEnd = C + A * (A.Z > 0.f ? Half : -Half) - A * 40.f;
				ACarvableLog* Under = StandAt(Mesh(75), FVector(RaisedEnd.X, RaisedEnd.Y, O.Z), FYaw, TEXT("eğik kütüğün altına direk"));
				Note(TEXT("altına konan direk yerinde kaldı"), Under && Under->IsAnchored() ? P(EPlace::Stand) : P(EPlace::None), P(EPlace::Stand));
				Note(TEXT("eğik kütük direkten sonra yerinde kaldı"), Slant->IsAnchored() ? P(EPlace::Slope) : P(EPlace::None), P(EPlace::Slope));
			}
		}
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

// 11. The 75 cm module at work: a wall of four 300 cm courses, a 2 x 2 module opening cut out of its middle; the
// four modules that come out drop as four 75 cm logs, ready to be used again.
void ALogShowcase::StationModuleOpening()
{
	BeginStation(10, TEXT("Modül: 4 x 300 duvar, ortadan 2 x 2 açıklık -> 4 adet 75 cm kütük düşer"));
	TArray<ACarvableLog*> Courses;
	ACarvableLog* Top = LayAt(Mesh(300), O, FYaw + 90.f, TEXT("sıra 1"));
	Courses.Add(Top);
	for (int32 Course = 2; Course <= 4 && Top; ++Course)
	{
		Top = OnLog(Mesh(300), Top, 0.f, FYaw, static_cast<uint8>(EHold::Lay), 0.f, *FString::Printf(TEXT("sıra %d"), Course), P(EPlace::Stack));
		Courses.Add(Top);
	}
	int32 Offcuts = 0;
	for (int32 Course = 1; Course <= 2 && Course < Courses.Num(); ++Course)
	{
		ACarvableLog* Log = Courses[Course];
		ACarvableLog* Right = Log ? Log->CutOpening(Log->GetCenterX(), 2, O - F * 400.f) : nullptr;
		const TCHAR* What = Course == 1 ? TEXT("açıklık, sıra 2") : TEXT("açıklık, sıra 3");
		Report += FString::Printf(TEXT("    %s %s\n"), Right ? TEXT("OK ") : TEXT("XX "), What);
		Right ? ++Placed : ++Failed;
		if (!Right)
		{
			StationNotes += FString::Printf(TEXT("\nolmadı: %s"), What);
			continue;
		}
		Report += FString::Printf(TEXT("      kalan: sol %.0f cm, sağ %.0f cm\n"), Log->GetLength(), Right->GetLength());
		for (const TWeakObjectPtr<ACarvableLog>& Piece : Log->GetLastOpeningOffcuts())
		{
			if (Piece.IsValid())
			{
				++Offcuts;
				Report += FString::Printf(TEXT("      düşen parça: %.0f cm\n"), Piece->GetLength());
			}
		}
	}
	const bool bFour = Offcuts == 4;
	Report += FString::Printf(TEXT("    %s düşen 75 cm kütük: %d (beklenen 4)\n"), bFour ? TEXT("OK ") : TEXT("XX "), Offcuts);
	bFour ? ++Placed : ++Failed;
	StationNotes += FString::Printf(TEXT("\n%d adet 75 cm kütük düştü"), Offcuts);
	EndStation();
}


// 12. Snap points on every face of a log: front, back and both ends (the top is the wall station's), each at a 75 cm
// line, with the piece parallel, square or at 45 degrees as the view turns.
void ALogShowcase::StationFaceSockets()
{
	BeginStation(11, TEXT("Yüz snap noktaları: ön, arka, iki uç (her 75 cm'de)"));
	ACarvableLog* Base = LayAt(Mesh(300), O, FYaw + 90.f, TEXT("hedef kütük"));
	FVector C, A;
	float Radius, Half;
	if (!Base || !ACarvableLog::GetLogAxis(Base, C, A, Radius, Half))
	{
		EndStation();
		return;
	}
	A = FVector(A.X, A.Y, 0.f).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, A);
	const float AxisYaw = A.Rotation().Yaw;
	struct FCase { const TCHAR* Name; FVector Normal; float Along; float PieceAngle; uint8 Hold; int32 Length; };
	// PieceAngle: the carried piece's direction relative to the target's axis (the view is 90 degrees off it).
	const FCase Cases[] = {
		{ TEXT("ön yüz, yan yana"),        Side,  -75.f,   0.f, static_cast<uint8>(EHold::Lay),   300 },
		{ TEXT("arka yüz, dik (T)"),      -Side,    0.f,  90.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("arka yüz, 45°"),          -Side,   75.f,  45.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("uç B, uca uç uzatma"),     A,       0.f,   0.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("uç A, ucuna dik (T)"),    -A,       0.f,  90.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("ön yüz, yanına direk"),    Side,   75.f,   0.f, static_cast<uint8>(EHold::Stand), 150 },
		{ TEXT("ön yüz, dik (T)"),         Side,  -75.f,  90.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("ön yüz, 45°"),             Side,    0.f, -45.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("uç B, 45°"),               A,       0.f,  45.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("uç A, 45°"),              -A,       0.f, -45.f, static_cast<uint8>(EHold::Lay),   150 },
		{ TEXT("uç B, ucuna direk"),       A,       0.f,   0.f, static_cast<uint8>(EHold::Stand), 150 },
	};
	for (const FCase& Case : Cases)
	{
		const bool bEnd = FMath::Abs(FVector::DotProduct(Case.Normal, A)) > 0.7f;
		const FVector Aim = bEnd ? C + Case.Normal * Half : C + A * Case.Along + Case.Normal * Radius;
		const float ViewYaw = AxisYaw + Case.PieceAngle - 90.f;
		const FVector View = FRotator(0.f, ViewYaw, 0.f).Vector();
		Player->SetActorLocation(Aim + Case.Normal * 200.f + FVector(0.f, 0.f, 80.f), false, nullptr, ETeleportType::TeleportPhysics);
		ACarvableLog* Piece = Spawn(Mesh(Case.Length));
		const uint8 Got = Piece ? P(Player->PlaceForShowcase(Piece, Base, Aim, Case.Normal, ViewYaw, static_cast<EHold>(Case.Hold))) : P(EPlace::None);
		Note(Case.Name, Got, P(EPlace::Socket));
		if (Piece && Got == P(EPlace::Socket))
		{
			Report += FString::Printf(TEXT("      %s  açı %.0f°  merkez F=%.0f R=%.0f z=%.0f\n"), *Player->GetSocketLabel(),
				FMath::Fmod(FMath::Abs(FRotator::NormalizeAxis(Piece->GetActorRotation().Yaw - AxisYaw)), 180.f),
				FVector::DotProduct(Piece->GetActorLocation() - O, F), FVector::DotProduct(Piece->GetActorLocation() - O, R),
				Piece->GetActorLocation().Z - O.Z);
		}
		(void)View;
	}
	EndStation();
}


// 13. The bottom face: a log raised on two posts as a beam, a log laid under it and a support post under it.
void ALogShowcase::StationUnderSockets()
{
	BeginStation(12, TEXT("Alt yüz: kirişin altına paralel kütük ve destek direği"));
	ACarvableLog* PostA = StandAt(Mesh(150), O - R * 112.f, FYaw, TEXT("direk 1"));
	StandAt(Mesh(150), O + R * 113.f, FYaw, TEXT("direk 2"));
	ACarvableLog* Beam = OnPostTop(Mesh(300), PostA, FYaw + 90.f, TEXT("kiriş"), P(EPlace::Beam));
	FVector C, A;
	float Radius, Half;
	if (!Beam || !ACarvableLog::GetLogAxis(Beam, C, A, Radius, Half))
	{
		EndStation();
		return;
	}
	A = FVector(A.X, A.Y, 0.f).GetSafeNormal();
	const float AxisYaw = A.Rotation().Yaw;
	auto Under = [&](const TCHAR* What, float Along, uint8 Hold, int32 Length)
	{
		const FVector Aim = C + A * Along - FVector::UpVector * Radius;
		Player->SetActorLocation(Aim - FVector::UpVector * 100.f - FRotator(0.f, AxisYaw - 90.f, 0.f).Vector() * 200.f, false, nullptr, ETeleportType::TeleportPhysics);
		ACarvableLog* Piece = Spawn(Mesh(Length));
		const uint8 Got = Piece ? P(Player->PlaceForShowcase(Piece, Beam, Aim, -FVector::UpVector, AxisYaw - 90.f, static_cast<EHold>(Hold))) : P(EPlace::None);
		Note(What, Got, P(EPlace::Socket));
	};
	Under(TEXT("alt yüz, altına paralel"), 0.f, static_cast<uint8>(EHold::Lay), 150);
	Under(TEXT("alt yüz, destek direği"), 75.f, static_cast<uint8>(EHold::Stand), 150);
	EndStation();
}


// 14. A log house: four walls of 600 cm logs joined at the corners (12 courses, a 5.25 m square room), a door through the
// front wall, a window in a side wall, and a ridge roof frame (gable posts, ridge beam, rafters from both side walls).
void ALogShowcase::StationLogHouse()
{
	BeginStation(13, TEXT("Kütük ev: 600'lük kütükler, 12 sıra köşe geçmeli, kapı, pencere, çatı iskeleti"));
	const float Span = 525.f;     // 600 cm logs joined half a module in from each end
	const int32 Courses = 12;
	TArray<TArray<ACarvableLog*>> Walls;
	Walls.Add({ LayAt(Mesh(600), O, FYaw, TEXT("eşik kütüğü 1")), LayAt(Mesh(600), O + R * Span, FYaw, TEXT("eşik kütüğü 2")) });
	for (int32 Course = 1; Course < Courses; ++Course)
	{
		const TArray<ACarvableLog*>& Lower = Walls.Last();
		if (Lower.Num() != 2 || !Lower[0] || !Lower[1])
		{
			break;
		}
		TArray<ACarvableLog*> Next;
		for (int32 End = 0; End < 2; ++End)
		{
			FVector C, Axis;
			float Radius, Half;
			ACarvableLog::GetLogAxis(Lower[0], C, Axis, Radius, Half);
			const FVector Aim = C + Axis * ((End == 0 ? -1.f : 1.f) * (Half - 35.f)) + FVector(0.f, 0.f, Radius);
			const FVector Inward = (Lower[1]->GetActorLocation() - Lower[0]->GetActorLocation()).GetSafeNormal2D();
			Player->SetActorLocation(Aim - Inward * 220.f + FVector(0.f, 0.f, 60.f), false, nullptr, ETeleportType::TeleportPhysics);
			ACarvableLog* Piece = Spawn(Mesh(600));
			const uint8 Got = Piece ? P(Player->PlaceForShowcase(Piece, Lower[0], Aim, FVector::UpVector,
				FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal().Rotation().Yaw, EHold::Lay)) : P(EPlace::None);
			Note(*FString::Printf(TEXT("sıra %d köşe %d"), Course + 1, End + 1), Got, P(EPlace::Corner));
			Next.Add(Got == P(EPlace::Corner) ? Piece : nullptr);
			FVector PC, PA;
			float PR, PH;
			if (Piece && ACarvableLog::GetLogAxis(Piece, PC, PA, PR, PH))
			{
				Report += FString::Printf(TEXT("      merkez F=%.0f R=%.0f z=%.0f boy=%.0f  (alttaki boy=%.0f)\n"), FVector::DotProduct(PC - O, F),
					FVector::DotProduct(PC - O, R), PC.Z - O.Z, PH * 2.f, Half * 2.f);
			}
		}
		Walls.Add(Next);
	}

	// Which log of a course is where: along F or along R, and on which side.
	auto AlongF = [this](const ACarvableLog* Log)
	{
		const float Yaw = FMath::Fmod(FMath::Abs(FRotator::NormalizeAxis(Log->GetActorRotation().Yaw - FYaw)), 180.f);
		return Yaw < 45.f || Yaw > 135.f;
	};
	auto Local = [this](const ACarvableLog* Log)
	{
		FVector C, A;
		float Radius, Half;
		return ACarvableLog::GetLogAxis(Log, C, A, Radius, Half) ? C - O : Log->GetActorLocation() - O;
	};
	auto Cut = [this](ACarvableLog* Log, const TCHAR* What)
	{
		ACarvableLog* Right = Log ? Log->CutOpening(Log->GetCenterX(), 2, O - F * 600.f + R * 262.f) : nullptr;
		Report += FString::Printf(TEXT("    %s %s\n"), Right ? TEXT("OK ") : TEXT("XX "), What);
		Right ? ++Placed : ++Failed;
		if (!Right)
		{
			StationNotes += FString::Printf(TEXT("\nolmadı: %s"), What);
		}
	};
	// Door: the front gable wall (logs along R, the one at the smaller F), its lowest five courses.
	int32 DoorCourses = 0;
	// Window: the side wall at the larger R (logs along F), two courses about a metre up.
	int32 WindowCourses = 0;
	for (int32 Course = 0; Course < Walls.Num(); ++Course)
	{
		for (ACarvableLog* Log : Walls[Course])
		{
			if (!Log)
			{
				continue;
			}
			if (!AlongF(Log) && FVector::DotProduct(Local(Log), F) < 0.f && DoorCourses < 5)
			{
				++DoorCourses;
				Cut(Log, *FString::Printf(TEXT("kapı, sıra %d"), Course + 1));
			}
			else if (AlongF(Log) && FVector::DotProduct(Local(Log), R) > Span * 0.5f && (Course == 4 || Course == 6) && WindowCourses < 2)
			{
				++WindowCourses;
				Cut(Log, *FString::Printf(TEXT("pencere, sıra %d"), Course + 1));
			}
		}
	}

	// Roof frame. The top course runs along R (the gables); the course under it along F (the side walls).
	ACarvableLog* GableFront = nullptr;
	ACarvableLog* GableBack = nullptr;
	ACarvableLog* SideNear = nullptr;
	ACarvableLog* SideFar = nullptr;
	for (int32 Course = Walls.Num() - 1; Course >= 0; --Course)
	{
		for (ACarvableLog* Log : Walls[Course])
		{
			if (!IsValid(Log))
			{
				continue;
			}
			if (!AlongF(Log))
			{
				if (FVector::DotProduct(Local(Log), F) < 0.f) { GableFront = GableFront ? GableFront : Log; }
				else { GableBack = GableBack ? GableBack : Log; }
			}
			else
			{
				if (FVector::DotProduct(Local(Log), R) < Span * 0.5f) { SideNear = SideNear ? SideNear : Log; }
				else { SideFar = SideFar ? SideFar : Log; }
			}
		}
	}
	// Gable posts on the middle of the top gable logs, the ridge beam from one to the other.
	auto MiddleAlong = [this, Span](ACarvableLog* Log)
	{
		FVector C, A;
		float Radius, Half;
		ACarvableLog::GetLogAxis(Log, C, A, Radius, Half);
		return FVector::DotProduct(O + R * (Span * 0.5f) - C, A);
	};
	ACarvableLog* PostFront = GableFront ? OnLog(Mesh(75), GableFront, MiddleAlong(GableFront), FYaw, static_cast<uint8>(EHold::Stand), 0.f, TEXT("mahya direği ön"), P(EPlace::Stand)) : nullptr;
	if (GableBack)
	{
		OnLog(Mesh(75), GableBack, MiddleAlong(GableBack), FYaw, static_cast<uint8>(EHold::Stand), 0.f, TEXT("mahya direği arka"), P(EPlace::Stand));
	}
	if (PostFront)
	{
			OnPostTop(Mesh(600), PostFront, FYaw, TEXT("mahya kirişi"), P(EPlace::Beam));
	}
	// Rafters every 75 cm along both side walls, leaning in to the ridge.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		ACarvableLog* Wall = Side == 0 ? SideNear : SideFar;
		if (!Wall)
		{
			continue;
		}
		for (int32 Index = 0; Index < 7; ++Index)
		{
			OnLog(Mesh(300), Wall, -225.f + Index * 75.f, FYaw + (Side == 0 ? 90.f : -90.f), static_cast<uint8>(EHold::Slope), 30.f,
				*FString::Printf(TEXT("çatı kirişi %s %d"), Side == 0 ? TEXT("sol") : TEXT("sağ"), Index + 1), P(EPlace::Slope));
		}
	}
	EndStation();
}

// 15. What a post holds: a beam on its top, a horizontal log on its side, and that they stay (one firm end, the other free).
void ALogShowcase::StationPostBeams()
{
	BeginStation(14, TEXT("Direk ve kiriş: tek direkte kiriş, direğin ortasına yatay log, sabit kalma"));
	StructureLink::bLogFalls = true;   // the structure graph at the moment a piece falls goes to the log
	auto Stays = [this](const TCHAR* What, const ACarvableLog* Piece, uint8 Kind)
	{
		Note(What, Piece && Piece->IsAnchored() ? Kind : P(EPlace::None), Kind);
	};
	// A. One 300 post, a 600 beam on its top: the beam holds on the post, its far end hangs free.
	ACarvableLog* PostA = StandAt(Mesh(300), O - R * 300.f, FYaw, TEXT("direk A"));
	ACarvableLog* BeamA = PostA ? OnPostTop(Mesh(600), PostA, FYaw, TEXT("kiriş tek direğin tepesinde"), P(EPlace::Beam)) : nullptr;
	// B. One 300 post, a 300 horizontal log laid on the side of its middle.
	ACarvableLog* PostB = StandAt(Mesh(300), O, FYaw, TEXT("direk B"));
	ACarvableLog* SideB = nullptr;
	if (PostB && Player.IsValid())
	{
		FVector C, A;
		float Radius, Half;
		if (ACarvableLog::GetLogAxis(PostB, C, A, Radius, Half))
		{
			SideB = Spawn(Mesh(300));
			const FVector Aim = C - F * Radius;
			Player->SetActorLocation(Aim - F * 220.f + FVector(0.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			Note(TEXT("direğin ortasına yatay log"), P(Player->PlaceForShowcase(SideB, PostB, Aim, -F, FYaw, EHold::Lay)), P(EPlace::Socket));
			Report += FString::Printf(TEXT("      mesaj: %s\n"), *Player->TestLastMessage());
		}
	}
	// C. A third piece elsewhere (a structure settling must not shake the first two).
	LayAt(Mesh(150), O + R * 300.f, FYaw, TEXT("üçüncü parça, yeniden değerlendirme"));
	Stays(TEXT("direk A yerinde kaldı"), PostA, P(EPlace::Stand));
	Stays(TEXT("kiriş yerinde kaldı"), BeamA, P(EPlace::Beam));
	{
		FVector BC, BA;
		float BR, BH;
		const bool bGot = BeamA && ACarvableLog::GetLogAxis(BeamA, BC, BA, BR, BH);
		const float LowEnd = bGot ? FMath::Min(BC.Z + BA.Z * BH, BC.Z - BA.Z * BH) - O.Z : 999.f;
		Report += FString::Printf(TEXT("      kirişin boşta ucu: yerden %.0f cm (yarıçap %.0f)\n"), LowEnd, bGot ? BR : 0.f);
		Note(TEXT("kirişin boşta ucu yere indi"), bGot && LowEnd < BR + 12.f ? P(EPlace::Beam) : P(EPlace::None), P(EPlace::Beam));
	}
	Stays(TEXT("direk B yerinde kaldı"), PostB, P(EPlace::Stand));
	Stays(TEXT("yan log yerinde kaldı"), SideB, P(EPlace::Socket));
	// D. Taking the post away: what it held falls, what it did not hold stays.
	if (PostA && BeamA && PostB && SideB && Player.IsValid() && Player->TestPickUp(PostA))
	{
		Note(TEXT("direği alınca kiriş düştü (desteği gitti)"), !BeamA->IsAnchored() ? P(EPlace::Beam) : P(EPlace::None), P(EPlace::Beam));
		Note(TEXT("başka direğin yan logu etkilenmedi"), SideB->IsAnchored() ? P(EPlace::Socket) : P(EPlace::None), P(EPlace::Socket));
	}
	StructureLink::bLogFalls = false;
	EndStation();
}

// 16. The same mechanics with every length (75, 150, 300, 600): a mechanic works with any log, not with one chosen length.
void ALogShowcase::StationLengthMatrix()
{
	BeginStation(15, TEXT("Boy matrisi: her mekanik 75, 150, 300 ve 600 cm ile"));
	const int32 Lengths[] = { 75, 150, 300, 600 };
	auto Stays = [this](const FString& What, const ACarvableLog* Piece, uint8 Kind)
	{
		Note(*What, Piece && Piece->IsAnchored() ? Kind : P(EPlace::None), Kind);
	};
	for (int32 K = 0; K < 4; ++K)
	{
		const int32 L = Lengths[K];
		const FVector P0 = O + F * (K * 1500.f);
		// A. Beside, across, diagonal and end to end of a 600 base.
		ACarvableLog* Base = LayAt(Mesh(600), P0, FYaw + 90.f, *FString::Printf(TEXT("%d: hedef 600"), L));
		FVector C, A;
		float Radius, Half;
		if (Base && ACarvableLog::GetLogAxis(Base, C, A, Radius, Half))
		{
			A = FVector(A.X, A.Y, 0.f).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, A);
			const float AxisYaw = A.Rotation().Yaw;
			struct FCase { const TCHAR* Name; FVector Normal; float Along; float PieceAngle; EHold Hold; };
			const FCase Cases[] = {
				{ TEXT("ön yüz, yan yana"),   Side, -150.f,   0.f, EHold::Lay },
				{ TEXT("arka yüz, dik (T)"), -Side,    0.f,  90.f, EHold::Lay },
				{ TEXT("arka yüz, 45°"),     -Side,  150.f,  45.f, EHold::Lay },
				{ TEXT("uç B, uca uç"),       A,       0.f,   0.f, EHold::Lay },
				{ TEXT("uç A, ucuna dik (T)"), -A,     0.f,  90.f, EHold::Lay },
				{ TEXT("uç B, 45°"),          A,       0.f,  45.f, EHold::Lay },
				{ TEXT("ön yüz, yanına direk"), Side,  225.f,  0.f, EHold::Stand },
			};
			for (const FCase& Case : Cases)
			{
				const bool bEnd = FMath::Abs(FVector::DotProduct(Case.Normal, A)) > 0.7f;
				const FVector Aim = bEnd ? C + Case.Normal * Half : C + A * Case.Along + Case.Normal * Radius;
				const float ViewYaw = AxisYaw + Case.PieceAngle - 90.f;
				Player->SetActorLocation(Aim + Case.Normal * 200.f + FVector(0.f, 0.f, 80.f), false, nullptr, ETeleportType::TeleportPhysics);
				ACarvableLog* Piece = Spawn(Mesh(L));
				const uint8 Got = Piece ? P(Player->PlaceForShowcase(Piece, Base, Aim, Case.Normal, ViewYaw, Case.Hold)) : P(EPlace::None);
				Note(*FString::Printf(TEXT("%d: %s"), L, Case.Name), Got, P(EPlace::Socket));
				Stays(FString::Printf(TEXT("%d: %s, yerinde kaldı"), L, Case.Name), Piece, P(EPlace::Socket));
			}
		}
		// B. Stacking: a course of the same length on top.
		ACarvableLog* Low = LayAt(Mesh(L), P0 - R * 1000.f, FYaw, *FString::Printf(TEXT("%d: alt sıra"), L));
		ACarvableLog* Up = Low ? OnLog(Mesh(L), Low, 0.f, FYaw + 90.f, static_cast<uint8>(EHold::Lay), 0.f, *FString::Printf(TEXT("%d: üst sıra"), L), P(EPlace::Stack)) : nullptr;
		Stays(FString::Printf(TEXT("%d: üst sıra yerinde kaldı"), L), Up, P(EPlace::Stack));
		// C. A post, a beam of this length on it, a log on its side (T and 45 degrees), a leaning log.
		const FVector PostAt = P0 + R * 1000.f;
		ACarvableLog* Post = StandAt(Mesh(300), PostAt, FYaw, *FString::Printf(TEXT("%d: direk (300)"), L));
		ACarvableLog* BeamOn = Post ? OnPostTop(Mesh(L), Post, FYaw, *FString::Printf(TEXT("%d: direk üstüne kiriş"), L), P(EPlace::Beam)) : nullptr;
		ACarvableLog* PostB = StandAt(Mesh(300), PostAt + F * 700.f, FYaw, *FString::Printf(TEXT("%d: direk B (300)"), L));
		ACarvableLog* SideT = nullptr;
		ACarvableLog* Side45 = nullptr;
		if (PostB && ACarvableLog::GetLogAxis(PostB, C, A, Radius, Half))
		{
			const FVector AimFront = C - F * Radius;
			Player->SetActorLocation(AimFront - F * 220.f, false, nullptr, ETeleportType::TeleportPhysics);
			SideT = Spawn(Mesh(L));
			Note(*FString::Printf(TEXT("%d: direğe yatay log, dik"), L), SideT ? P(Player->PlaceForShowcase(SideT, PostB, AimFront, -F, FYaw, EHold::Lay)) : P(EPlace::None), P(EPlace::Socket));
			Player->SetActorLocation(AimFront - F * 220.f + FVector(0.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			Side45 = Spawn(Mesh(L));
			Note(*FString::Printf(TEXT("%d: direğe yatay log, 45°"), L), Side45 ? P(Player->PlaceForShowcase(Side45, PostB, C + R * Radius, R, FYaw + 45.f, EHold::Lay)) : P(EPlace::None), P(EPlace::Socket));
		}
		// A log can only lean on a post lower than itself: the post is half its length (nothing shorter than 75 exists).
		ACarvableLog* PostC = L >= 150 ? StandAt(Mesh(L / 2), PostAt + F * 1400.f, FYaw, *FString::Printf(TEXT("%d: direk C (%d)"), L, L / 2)) : nullptr;
		ACarvableLog* Lean = nullptr;
		if (PostC && ACarvableLog::GetLogAxis(PostC, C, A, Radius, Half))
		{
			const FVector Top = C + FVector(0.f, 0.f, Half);
			Lean = Spawn(Mesh(L));
			Player->SetActorLocation(Top - F * 250.f + FVector(0.f, 0.f, -60.f), false, nullptr, ETeleportType::TeleportPhysics);
			Note(*FString::Printf(TEXT("%d: direğe yaslanan log"), L), Lean ? P(Player->PlaceForShowcase(Lean, PostC, Top, FVector::UpVector, FYaw, EHold::Slope, 45.f)) : P(EPlace::None), P(EPlace::Slope));
		}
		Stays(FString::Printf(TEXT("%d: direk üstü kiriş yerinde kaldı"), L), BeamOn, P(EPlace::Beam));
		Stays(FString::Printf(TEXT("%d: direğe dik log yerinde kaldı"), L), SideT, P(EPlace::Socket));
		Stays(FString::Printf(TEXT("%d: direğe 45° log yerinde kaldı"), L), Side45, P(EPlace::Socket));
		if (L >= 150)
		{
			Stays(FString::Printf(TEXT("%d: yaslanan log yerinde kaldı"), L), Lean, P(EPlace::Slope));
		}
	}
	EndStation();
}
