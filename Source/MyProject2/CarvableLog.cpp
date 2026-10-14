// Copyright Epic Games, Inc. All Rights Reserved.

#include "CarvableLog.h"
#include "Wood/WoodStructureGeometry.h"
#include "Wood/WoodStructureLogic.h"
#include "StructureLink.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "ForestTypes.h"
#include "PickupItem.h"
#include "Campfire.h"
#include "BurnSubsystem.h"
#include "MaterialCore/MaterialDefinition.h"
#include "RaidEnemy.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GeometryScript/CollisionFunctions.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "Materials/MaterialInterface.h"
#include "UDynamicMesh.h"

namespace
{
	const TCHAR* CutWoodPath = TEXT("/Game/Building/Materials/M_CutWood.M_CutWood");
	/** ~500 kg/m3, in kg per cubic centimetre. */
	constexpr float WoodDensity = 0.0005f;
	/** A cup is cut a hair wider than the log that sits in it. */
	constexpr float CupFit = 1.02f;
	constexpr float BigBox = 4000.f;
	/** A dovetail's opening at the surface, as a share of its width at the floor. */
	constexpr float DovetailNeck = 0.7f;
}

FOnLogCarved ACarvableLog::OnLogCarved;
FOnLogPlacedAcross ACarvableLog::OnLogPlacedAcross;
FOnLogStoodUp ACarvableLog::OnLogStoodUp;
FOnLogRepaired ACarvableLog::OnLogRepaired;

float ACarvableLog::GetShoulderCapacity()
{
	// One whole greybox log, 70 cm thick and six modules long, with a little slack for rounding.
	return 1.03f * PI * 35.f * 35.f * 6.f * ModuleLength;
}

ACarvableLog* ACarvableLog::SpawnLog(UWorld* World, const FTransform& Transform)
{
	return SpawnFromStaticMesh(World, ForestItems::GetMesh(EForestItem::Log), Transform);
}

void ACarvableLog::BeginCarry(USceneComponent* AttachTo, const FTransform& Relative)
{
	bCarried = true;
	MeshComponent->SetSimulatePhysics(false);
	SetActorTickEnabled(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(false);
	AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeTransform(Relative);
}

void ACarvableLog::PlaceAt(const FTransform& Transform, bool bAnchorIt)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bCarried = false;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCastShadow(true);
	SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	if (bAnchorIt)
	{
		Anchor();
	}
	else
	{
		Resettle();
	}
}

FTransform ACarvableLog::GetLayTransform(const FVector& Ground, float Yaw) const
{
	const FQuat Rotation = FRotator(0.f, Yaw, 0.f).Quaternion();
	return FTransform(Rotation, Ground + FVector(0.f, 0.f, Radius + 1.f) - Rotation.RotateVector(FVector(GetCenterX(), 0.f, 0.f)));
}

FTransform ACarvableLog::GetStandTransform(const FVector& Ground, float Yaw) const
{
	// Pitch 90 turns the log's length straight up; its lower end (MinX) goes on the ground.
	const FQuat Rotation = FRotator(90.f, Yaw, 0.f).Quaternion();
	return FTransform(Rotation, Ground + FVector(0.f, 0.f, 0.5f) - Rotation.RotateVector(FVector(MinX, 0.f, 0.f)));
}

ACarvableLog::ACarvableLog()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	MeshComponent->SetComplexAsSimpleCollisionEnabled(false, false);
	// A round log on flat ground rolls at the slightest push; damping lets it come to rest quickly.
	MeshComponent->SetAngularDamping(3.f);
	MeshComponent->SetLinearDamping(0.3f);
	MeshComponent->SetNotifyRigidBodyCollision(true);
	RootComponent = MeshComponent;
}

void ACarvableLog::Resettle()
{
	if (bAnchored || bCarried)
	{
		return;
	}
	MeshComponent->SetSimulatePhysics(true);
	// Start from rest: a body frozen while still moving would otherwise get its old speed back and roll away.
	MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	MeshComponent->WakeAllRigidBodies();
	SettleTime = 0.f;
	StillTime = 0.f;
	SetActorTickEnabled(true);
}

void ACarvableLog::Anchor()
{
	bAnchored = true;
	MeshComponent->SetSimulatePhysics(false);
	SetActorTickEnabled(false);
}

void ACarvableLog::UpdateDamageLook()
{
	if (Damage <= 0.f || !BarkMaterial)
	{
		return;
	}
	if (!DamageMaterial)
	{
		DamageMaterial = UMaterialInstanceDynamic::Create(BarkMaterial, this);
		MeshComponent->SetMaterial(0, DamageMaterial);
	}
	// Scorched wood goes from its bark colour to charcoal.
	FLinearColor Bark(0.2f, 0.12f, 0.06f);
	BarkMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Bark);
	DamageMaterial->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(Bark, FLinearColor(0.015f, 0.014f, 0.013f), FMath::Clamp(Damage * 1.2f, 0.f, 1.f)));
}

void ACarvableLog::AddDamage(float Amount)
{
	SetDamage(Damage + Amount);
}

void ACarvableLog::SetDamage(float Value)
{
	if (!FMath::IsFinite(Value))
	{
		return;
	}
	const float Before = Damage;
	Damage = FMath::Clamp(Value, 0.f, 1.f);
	if (FMath::IsNearlyEqual(Before, Damage, 0.0001f))
	{
		return;
	}
	UpdateDamageLook();
	// The existing StructureLink graph owns load-bearing effects. Re-evaluate only
	// when the same thresholds its evaluator consumes have been crossed.
	const auto Crossed = [Before, this](float Threshold) { return (Before < Threshold) != (Damage < Threshold); };
	if (bAnchored && (Crossed(WoodStructure::JointLooseDamage) || Crossed(WoodStructure::FailureDamage)))
	{
		StructureLink::Settle(GetWorld());
	}
}

void ACarvableLog::SimulateMaterialEnvironment(
	float ElapsedDays, float AmbientTemperatureC, bool bRainExposed, float RainfallMm)
{
	if (!FMath::IsFinite(ElapsedDays) || ElapsedDays <= 0.0f ||
		!FMath::IsFinite(AmbientTemperatureC) || !FMath::IsFinite(RainfallMm))
	{
		return;
	}

	ElapsedDays = FMath::Min(ElapsedDays, 365.0f);
	MaterialAgeDays = FMath::Min(MaterialAgeDays + ElapsedDays, 1000000.0f);

	const FMaterialProperties* Properties = MaterialDefinition ? &MaterialDefinition->Properties : nullptr;
	const float WaterAbsorption = Properties
		? FMath::Clamp(Properties->WaterAbsorption, 0.0f, 1.0f)
		: 0.35f;
	const float WaterDamageRate = Properties
		? FMath::Clamp(Properties->WaterDamageRate, 0.0f, 0.1f)
		: 0.02f;
	const float DecayRate = Properties
		? FMath::Clamp(Properties->DecayRate, 0.0f, 0.1f)
		: 0.0015f;

	if (bRainExposed && RainfallMm > 0.0f && WaterAbsorption > 0.0f)
	{
		// Rainfall is a per-world-day total. Applying at most one day's total avoids
		// multiplying today's weather by a long offline catch-up interval.
		const float DailyRainFactor = FMath::Min(ElapsedDays, 1.0f);
		const float AddedMoisture = (RainfallMm / 100.0f) * WaterAbsorption * DailyRainFactor;
		MoistureContent = FMath::Clamp(MoistureContent + AddedMoisture, 0.0f, 1.0f);
	}
	else
	{
		const float DryingRate = AmbientTemperatureC < 2.0f ? 0.02f : 0.08f;
		MoistureContent = FMath::Max(0.0f, MoistureContent - DryingRate * ElapsedDays);
	}

	// Burning/charred state belongs exclusively to UBurnSubsystem/EBurnState.
	// Wetness changes damage and burn resistance without creating another fire state.
	if (BurnState == EBurnState::Charred || Damage >= 1.0f)
	{
		return;
	}

	const float ThermalFactor = FMath::Clamp((AmbientTemperatureC + 5.0f) / 25.0f, 0.0f, 1.5f);
	const float MoistureFactor = FMath::Clamp(MoistureContent, 0.0f, 1.0f);
	const float EnvironmentalDamage = ElapsedDays * ThermalFactor * MoistureFactor *
		(DecayRate + WaterDamageRate * 0.01f);
	if (EnvironmentalDamage > 0.0f && FMath::IsFinite(EnvironmentalDamage))
	{
		AddDamage(EnvironmentalDamage);
	}
}

float ACarvableLog::GetIgniteSeconds() const
{
	// Wet timber takes longer to catch; UBurnSubsystem still owns the ignition/warm lists.
	return FMath::Clamp(Radius * 0.4f * (1.0f + 2.0f * MoistureContent), 2.f, 60.f);
}

float ACarvableLog::GetHeatShare() const
{
	return BurnState == EBurnState::Burning || BurnState == EBurnState::Charred ? 1.f : FMath::Clamp(HeatSeconds / GetIgniteSeconds(), 0.f, 1.f);
}

bool ACarvableLog::ApplyHeat(float Seconds)
{
	if (BurnState == EBurnState::Burning)
	{
		return true;
	}
	if (BurnState == EBurnState::Charred || Seconds <= 0.f)
	{
		return false;
	}
	HeatSeconds += Seconds;
	bHeatedSinceStep = true;
	BurnState = EBurnState::Igniting;
	if (UBurnSubsystem* Burn = UBurnSubsystem::Get(GetWorld()))
	{
		Burn->Warm(this);
	}
	return HeatSeconds >= GetIgniteSeconds() && Ignite();
}

bool ACarvableLog::Ignite()
{
	UBurnSubsystem* Burn = UBurnSubsystem::Get(GetWorld());
	if (!Burn || BurnState == EBurnState::Charred || (BurnState != EBurnState::Burning && Burn->GetBurningCount() >= MaxBurning))
	{
		return false;
	}
	if (BurnState != EBurnState::Burning)
	{
		BurnState = EBurnState::Burning;
		HeatSeconds = GetIgniteSeconds();
		// Moisture lowers the effective burn rate without creating a second fire-state owner.
		BurnTotal = FMath::Max(ACampfire::FuelSecondsOf(this), 1.f) * (1.0f + 1.5f * MoistureContent);
		BurnLeft = BurnTotal;
		Burn->Light(this);
		ShowFlames(true, 1.f);
	}
	return true;
}

bool ACarvableLog::CoolStep(float Seconds)
{
	if (BurnState != EBurnState::Igniting)
	{
		return false;
	}
	// Heated during this step: no cooling. Otherwise it loses heat at half the rate it gains it.
	if (bHeatedSinceStep)
	{
		bHeatedSinceStep = false;
		return true;
	}
	HeatSeconds = FMath::Max(0.f, HeatSeconds - Seconds * 0.5f);
	if (HeatSeconds <= 0.f)
	{
		BurnState = EBurnState::Intact;
		return false;
	}
	return true;
}

void ACarvableLog::StepBurn(float Seconds)
{
	if (BurnState != EBurnState::Burning)
	{
		return;
	}
	const float Burned = FMath::Min(Seconds, BurnLeft);
	BurnLeft -= Burned;
	// The piece is burnt through exactly when its fuel is used up.
	AddDamage(Burned / BurnTotal);
	if (BurnLeft <= 0.f)
	{
		EndBurn();
		return;
	}
	ShowFlames(true, FMath::Clamp(0.4f + 0.6f * BurnLeft / BurnTotal, 0.4f, 1.f));
}

void ACarvableLog::EndBurn()
{
	BurnState = EBurnState::Charred;
	BurnLeft = 0.f;
	SetDamage(FMath::Max(Damage, 1.f));
	ShowFlames(false, 0.f);
}

void ACarvableLog::ShowFlames(bool bShow, float Strength)
{
	if (bShow && FlameParts.Num() == 0)
	{
		// A flame per module along the log, and one light in the middle.
		UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		const int32 Count = FMath::Clamp(FMath::RoundToInt((MaxX - MinX) / ModuleLength), 1, 6);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UStaticMeshComponent* Flame = NewObject<UStaticMeshComponent>(this);
			Flame->SetupAttachment(MeshComponent);
			Flame->SetStaticMesh(Cone);
			Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Flame->SetCastShadow(false);
			Flame->RegisterComponent();
			if (Base)
			{
				UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
				Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.45f, 0.05f));
				Flame->SetMaterial(0, Material);
			}
			Flame->SetRelativeLocation(FVector(MinX + (MaxX - MinX) * (Index + 0.5f) / Count, 0.f, Radius * 0.9f));
			FlameParts.Add(Flame);
		}
		FlameLight = NewObject<UPointLightComponent>(this);
		FlameLight->SetupAttachment(MeshComponent);
		FlameLight->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
		FlameLight->SetAttenuationRadius(600.f);
		FlameLight->SetCastShadows(false);
		FlameLight->RegisterComponent();
		FlameLight->SetRelativeLocation(FVector((MinX + MaxX) * 0.5f, 0.f, Radius + 30.f));
	}
	for (UStaticMeshComponent* Flame : FlameParts)
	{
		Flame->SetVisibility(bShow);
		Flame->SetRelativeScale3D(FVector(Radius / 100.f * 0.8f, Radius / 100.f * 0.8f, Radius / 100.f * 2.f * Strength));
	}
	if (FlameLight)
	{
		FlameLight->SetIntensity(bShow ? 3000.f * Strength : 0.f);
	}
}

bool ACarvableLog::CanReplace(const ACarvableLog* Old, const ACarvableLog* Replacement)
{
	if (!Old || !Replacement || Old == Replacement || !Old->IsAnchored() || Replacement->IsAnchored()
		|| Replacement->IsCarried() || Replacement->MeshComponent->IsSimulatingPhysics()
		|| Replacement->BurnState != EBurnState::Intact || Old->GetDamage() < 0.2f
		|| Replacement->GetDamage() >= Old->GetDamage() || Old->IsSplit() != Replacement->IsSplit()
		|| Old->GetSplitCount() != Replacement->GetSplitCount()
		|| !Replacement->JointCuts.IsEmpty() || !Replacement->CutSpans.IsEmpty())
	{
		return false;
	}

	// "Same size" includes cross-section: length alone lets a thin/quarter-split piece replace a full beam.
	const FVector OldSize = Old->GetLocalBox().GetSize();
	const FVector ReplacementSize = Replacement->GetLocalBox().GetSize();
	return FMath::IsNearlyEqual(Old->GetLength(), Replacement->GetLength(), 1.5f)
		&& FMath::IsNearlyEqual(Old->GetRadius(), Replacement->GetRadius(), 1.5f)
		&& FMath::IsNearlyEqual(OldSize.Y, ReplacementSize.Y, 1.5f)
		&& FMath::IsNearlyEqual(OldSize.Z, ReplacementSize.Z, 1.5f);
}

bool ACarvableLog::ReplaceDamaged(ACarvableLog* Old, ACarvableLog* Replacement)
{
	if (!CanReplace(Old, Replacement))
	{
		return false;
	}

	// Rebuilding joints is one repair transaction: never leave a partly notched replacement in the structure.
	UDynamicMesh* BackupMesh = NewObject<UDynamicMesh>(Replacement);
	if (!BackupMesh)
	{
		return false;
	}
	BackupMesh->SetMesh(Replacement->MeshComponent->GetDynamicMesh()->GetMeshRef());
	const FTransform OriginalTransform = Replacement->GetActorTransform();
	const TArray<FJointCut> OriginalJointCuts = Replacement->JointCuts;
	const TArray<FVector2f> OriginalCutSpans = Replacement->CutSpans;
	const float Shift = Replacement->MinX - Old->MinX;
	const float StockMinX = Replacement->MinX;
	const float StockMaxX = Replacement->MaxX;

	FTransform Where = Old->GetActorTransform();
	Where.AddToTranslation(-Where.GetRotation().RotateVector(FVector(Shift, 0.f, 0.f)));
	Replacement->PlaceAt(Where, /*bAnchorIt*/ true);
	Replacement->JointCuts.Reset();
	Replacement->CutSpans.Reset();

	auto RollbackReplacement = [&]()
	{
		Replacement->MeshComponent->GetDynamicMesh()->SetMesh(BackupMesh->GetMeshRef());
		Replacement->JointCuts = OriginalJointCuts;
		Replacement->CutSpans = OriginalCutSpans;
		Replacement->FinishEdit();
		Replacement->SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
		Replacement->bAnchored = false;
		Replacement->MeshComponent->SetSimulatePhysics(false);
		Replacement->SetActorTickEnabled(false);
		Replacement->UpdateDamageLook();
	};

	for (const FJointCut& Cut : Old->JointCuts)
	{
		const float X = Cut.LocalX + Shift;
		if (X < StockMinX || X > StockMaxX)
		{
			RollbackReplacement();
			return false;
		}

		bool bCutRebuilt = false;
		switch (Cut.Tool)
		{
		case ECarveTool::Notch:      bCutRebuilt = Replacement->Notch(X, Cut.SideAngleDeg, Cut.AngleDeg, Cut.MateRadius, Cut.Depth); break;
		case ECarveTool::NotchRound: bCutRebuilt = Replacement->CutProfile(X, Cut.SideAngleDeg, Cut.Width, Cut.Depth, true); break;
		case ECarveTool::NotchFlat:  bCutRebuilt = Replacement->CutProfile(X, Cut.SideAngleDeg, Cut.Width, Cut.Depth, false); break;
		case ECarveTool::Dovetail:   bCutRebuilt = Replacement->CutDovetail(X, Cut.SideAngleDeg, Cut.Width, Cut.Depth); break;
		default:                     bCutRebuilt = false; break;
		}

		if (!bCutRebuilt)
		{
			RollbackReplacement();
			return false;
		}
	}

	// The replacement's new geometry changes both the support graph and cached feature classification.
	Old->Release();
	StructureLink::Settle(GetWorld());
	OnLogRepaired.Broadcast(Replacement);
	return true;
}

bool ACarvableLog::Push(const FVector& Velocity)
{
	if (bAnchored || bCarried)
	{
		return false;
	}
	Resettle();
	MeshComponent->SetPhysicsLinearVelocity(Velocity);
	return true;
}

void ACarvableLog::AddImpactDamage(float ImpulseSize)
{
	if (ImpulseSize > ImpactThreshold)
	{
		AddDamage(FMath::Min(0.5f, (ImpulseSize - ImpactThreshold) / 4000000.f));
	}
}

void ACarvableLog::HitEnemy(ARaidEnemy* Enemy, float ImpulseSize)
{
	if (Enemy && ImpulseSize > 50000.f)
	{
		Enemy->TakeHit(ImpulseSize / 10000.f);
	}
}

void ACarvableLog::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
	FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	if (bAnchored || bCarried || !MeshComponent->IsSimulatingPhysics())
	{
		return;
	}
	if (ARaidEnemy* Enemy = Cast<ARaidEnemy>(Other))
	{
		HitEnemy(Enemy, NormalImpulse.Size());
		return;
	}
	AddImpactDamage(NormalImpulse.Size());
}

bool ACarvableLog::ComputeSlopePlacement(const FVector& LowEnd, const FVector& Forward, float AngleDeg, const ACarvableLog* Piece, FTransform& Out)
{
	const FVector Flat(Forward.X, Forward.Y, 0.f);
	if (!Piece || Flat.IsNearlyZero())
	{
		return false;
	}
	const float Angle = FMath::DegreesToRadians(FMath::Clamp(AngleDeg, 10.f, 80.f));
	const FVector Direction = Flat.GetSafeNormal() * FMath::Cos(Angle) + FVector::UpVector * FMath::Sin(Angle);
	const FQuat Rotation = FRotationMatrix::MakeFromX(Direction).ToQuat();
	// The lower end's centre is lifted just enough for the bark to touch the ground.
	const FVector EndCentre = LowEnd + FVector(0.f, 0.f, Piece->Radius * FMath::Cos(Angle) + 0.5f);
	Out = FTransform(Rotation, EndCentre - Rotation.RotateVector(FVector(Piece->MinX, 0.f, 0.f)));
	return true;
}

ACarvableLog* ACarvableLog::CutOpening(float CentreX, int32 Modules)
{
	TArray<float> Lines;
	GetModuleLines(Lines);
	Modules = FMath::Max(2, Modules + (Modules % 2));
	const float OneModule = ModuleLength - 1.f;
	const float Wanted = CentreX - Modules * ModuleLength * 0.5f;
	float X0 = TNumericLimits<float>::Max();
	for (const float Line : Lines)
	{
		if (FMath::Abs(Line - Wanted) < FMath::Abs(X0 - Wanted))
		{
			X0 = Line;
		}
	}
	const float X1 = X0 + Modules * ModuleLength;
	if (Lines.Num() == 0 || bCarried || X0 - MinX < OneModule || MaxX - X1 < OneModule)
	{
		return nullptr;
	}
	const bool bWasAnchored = bAnchored;
	ACarvableLog* Middle = CrossCut(X0);
	if (!Middle)
	{
		return nullptr;
	}
	ACarvableLog* Right = Middle->CrossCut(X1);
	Middle->Destroy();
	if (bWasAnchored)
	{
		Anchor();
		if (Right)
		{
			Right->Anchor();
		}
	}
	StructureLink::MarkChanged();
	StructureLink::Settle(GetWorld());
	return Right;
}

bool ACarvableLog::CutGroove(float SideAngleDeg)
{
	return Notch(GetCenterX(), SideAngleDeg, 0.f, Radius, Radius * 0.3f);
}

void ACarvableLog::LetFall()
{
	bAnchored = false;
	Resettle();
}

ACarvableLog* ACarvableLog::SnapAt(float LocalX)
{
	bAnchored = false;
	return CrossCut(FMath::Clamp(LocalX, MinX + 10.f, MaxX - 10.f));
}

void ACarvableLog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!MeshComponent->IsSimulatingPhysics())
	{
		SetActorTickEnabled(false);
		return;
	}
	SettleTime += DeltaSeconds;
	const bool bStill = MeshComponent->GetPhysicsLinearVelocity().SizeSquared() < 25.f
		&& MeshComponent->GetPhysicsAngularVelocityInDegrees().SizeSquared() < 25.f;
	StillTime = bStill ? StillTime + DeltaSeconds : 0.f;
	// Resting (or given up waiting): freeze where it lies. The next cut lets it settle again.
	if (StillTime > 0.3f || SettleTime > 6.f)
	{
		MeshComponent->SetSimulatePhysics(false);
		SetActorTickEnabled(false);
	}
}

void ACarvableLog::LoadSource()
{
	if (!SourceMesh)
	{
		return;
	}
	FGeometryScriptMeshReadLOD ReadLOD;
#if WITH_EDITOR
	ReadLOD.LODType = EGeometryScriptLODType::SourceModel;
#else
	ReadLOD.LODType = EGeometryScriptLODType::RenderData;
#endif
	EGeometryScriptOutcomePins Outcome;
	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMeshV2(
		SourceMesh, MeshComponent->GetDynamicMesh(), FGeometryScriptCopyMeshFromAssetOptions(), ReadLOD, Outcome);
	const FBox Box = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(MeshComponent->GetDynamicMesh());
	Radius = FMath::Max(Box.GetSize().Y, Box.GetSize().Z) * 0.5f;
	SetMaterials(SourceMesh->GetMaterial(0));
	if (SourceMesh->GetStaticMaterials().Num() > 1)
	{
		// Pieces whose surface is already cut wood keep it on the carved faces too.
		MeshComponent->SetMaterial(1, SourceMesh->GetMaterial(1));
	}
	FinishEdit();
}

void ACarvableLog::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Show the source in the editor; a fresh copy is loaded again when play begins.
	if (SourceMesh && MeshComponent->GetDynamicMesh()->GetTriangleCount() == 0)
	{
		LoadSource();
	}
}

ACarvableLog* ACarvableLog::SpawnFromStaticMesh(UWorld* World, UStaticMesh* Mesh, const FTransform& Transform)
{
	if (!World || !Mesh)
	{
		return nullptr;
	}
	ACarvableLog* Log = World->SpawnActorDeferred<ACarvableLog>(
		ACarvableLog::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Log)
	{
		return nullptr;
	}

	FGeometryScriptMeshReadLOD ReadLOD;
#if WITH_EDITOR
	ReadLOD.LODType = EGeometryScriptLODType::SourceModel;
#else
	// Cooked builds only have render data (the raw log meshes need Allow CPU Access).
	ReadLOD.LODType = EGeometryScriptLODType::RenderData;
#endif
	EGeometryScriptOutcomePins Outcome;
	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMeshV2(
		Mesh, Log->MeshComponent->GetDynamicMesh(), FGeometryScriptCopyMeshFromAssetOptions(), ReadLOD, Outcome);

	const FBox Box = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(Log->MeshComponent->GetDynamicMesh());
	Log->Radius = Box.GetSize().Z * 0.5f;
	Log->SetMaterials(Mesh->GetMaterial(0));
	Log->FinishEdit();
	Log->FinishSpawning(Transform);
	return Log;
}

ACarvableLog* ACarvableLog::SpawnFromPickup(APickupItem* Pickup)
{
	const UStaticMeshComponent* PickupMesh = Pickup ? Pickup->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!PickupMesh || !PickupMesh->GetStaticMesh() || Pickup->GetItem() != EForestItem::Log)
	{
		return nullptr;
	}
	const FTransform Where = PickupMesh->GetComponentTransform();
	UStaticMesh* LogMesh = PickupMesh->GetStaticMesh();
	UWorld* World = Pickup->GetWorld();
	Pickup->Destroy();
	return SpawnFromStaticMesh(World, LogMesh, Where);
}

void ACarvableLog::BeginPlay()
{
	Super::BeginPlay();
	if (SourceMesh && Radius <= 0.f)
	{
		LoadSource();
	}
	// Placed logs stay put until they are cut; spawned pieces fall and settle.
	if (SourceMesh && bStartAsleep)
	{
		MeshComponent->SetSimulatePhysics(false);
	}
	else
	{
		Resettle();
	}
}

void ACarvableLog::SetMaterials(UMaterialInterface* Bark)
{
	BarkMaterial = Bark;
	MeshComponent->SetMaterial(0, Bark);
	MeshComponent->SetMaterial(1, LoadObject<UMaterialInterface>(nullptr, CutWoodPath));
}

int32 ACarvableLog::GetTriangleCount() const
{
	return MeshComponent->GetDynamicMesh()->GetTriangleCount();
}

bool ACarvableLog::IsSpanFree(float X0, float X1) const
{
	constexpr float Gap = 5.f;
	for (const FVector2f& Span : CutSpans)
	{
		if (X1 > Span.X - Gap && X0 < Span.Y + Gap)
		{
			return false;
		}
	}
	return true;
}

void ACarvableLog::GetModuleLines(TArray<float>& OutX) const
{
	OutX.Reset();
	const float Origin = bGridSet ? GridOriginX : MinX;
	const int32 First = FMath::CeilToInt((MinX + 1.f - Origin) / ModuleLength);
	for (int32 Line = First; Origin + Line * ModuleLength < MaxX - 1.f; ++Line)
	{
		OutX.Add(Origin + Line * ModuleLength);
	}
}

void ACarvableLog::GetSnapPoints(ECarveTool Tool, float Width, TArray<float>& OutX) const
{
	OutX.Reset();
	TArray<float> Lines;
	GetModuleLines(Lines);
	// The shortest piece an axe takes off, and how far in a corner joint sits: one module, with a centimetre of slack.
	const float OneModule = ModuleLength - 1.f;
	switch (Tool)
	{
	case ECarveTool::Point:
	case ECarveTool::Chamfer:
	{
		// The ends of a log whose end is still flat and that has wood to spare.
		const float Reach = Tool == ECarveTool::Point ? PointLength : ChamferSize;
		if (GetLength() >= Reach + 30.f)
		{
			if (EndMin == ELogEnd::Flat && IsSpanFree(MinX, MinX + Reach))
			{
				OutX.Add(MinX);
			}
			if (EndMax == ELogEnd::Flat && IsSpanFree(MaxX - Reach, MaxX))
			{
				OutX.Add(MaxX);
			}
		}
		break;
	}
	case ECarveTool::Groove:
		if (CutSpans.Num() == 0)
		{
			OutX.Add(GetCenterX());
		}
		break;
	case ECarveTool::Trim:
		if (EndMin != ELogEnd::Flat)
		{
			OutX.Add(MinX);
		}
		if (EndMax != ELogEnd::Flat)
		{
			OutX.Add(MaxX);
		}
		break;
	case ECarveTool::Notch:
	case ECarveTool::NotchRound:
	case ECarveTool::NotchFlat:
	case ECarveTool::Dovetail:
	{
		TArray<float, TInlineAllocator<5>> Candidates;
		// Corners: the first and last grid lines a module in from the ends.
		const float* Near = Lines.FindByPredicate([this, OneModule](float X) { return X - MinX >= OneModule; });
		float Far = TNumericLimits<float>::Lowest();
		for (const float X : Lines)
		{
			Far = MaxX - X >= OneModule ? X : Far;
		}
		if (Near)
		{
			Candidates.Add(*Near);
		}
		if (Far > TNumericLimits<float>::Lowest())
		{
			Candidates.Add(Far);
		}
		// A cross wall meets a long log on its middle line, at least two modules from both corners.
		if (Near && Far > TNumericLimits<float>::Lowest())
		{
			float Middle = 0.f;
			float Best = TNumericLimits<float>::Max();
			for (const float X : Lines)
			{
				if (FMath::Abs(X - GetCenterX()) < Best)
				{
					Best = FMath::Abs(X - GetCenterX());
					Middle = X;
				}
			}
			if (Middle - *Near >= 2.f * OneModule && Far - Middle >= 2.f * OneModule)
			{
				Candidates.Add(Middle);
			}
		}
		// A flat notch can also take the corner itself: an end lap, open at the end of the log, no overhang.
		if (Tool == ECarveTool::NotchFlat)
		{
			Candidates.Add(MinX + Width * 0.5f);
			Candidates.Add(MaxX - Width * 0.5f);
		}
		for (const float X : Candidates)
		{
			if (X - Width * 0.5f >= MinX - 1.f && X + Width * 0.5f <= MaxX + 1.f && IsSpanFree(X - Width * 0.5f, X + Width * 0.5f))
			{
				OutX.AddUnique(X);
			}
		}
		break;
	}
	case ECarveTool::CrossCut:
		for (const float X : Lines)
		{
			if (X - MinX >= OneModule && MaxX - X >= OneModule && IsSpanFree(X, X))
			{
				OutX.Add(X);
			}
		}
		break;
	default:
		break;
	}
	OutX.Sort();
}

bool ACarvableLog::FindSnapPoint(ECarveTool Tool, float LocalX, float Width, float& OutX) const
{
	TArray<float> Points;
	GetSnapPoints(Tool, Width, Points);
	float Best = TNumericLimits<float>::Max();
	for (const float X : Points)
	{
		if (FMath::Abs(X - LocalX) < Best)
		{
			Best = FMath::Abs(X - LocalX);
			OutX = X;
		}
	}
	return Points.Num() > 0;
}

void ACarvableLog::GetNotchFrame(float LocalX, float SideAngleDeg, float CrossAngleDeg, float MateRadius, float Depth,
	FVector& OutCentre, FVector& OutDirection) const
{
	const float Side = FMath::DegreesToRadians(SideAngleDeg);
	const float Cross = FMath::DegreesToRadians(CrossAngleDeg);
	// From the log axis toward the mating log, and the direction square to both.
	const FVector Out(0.f, FMath::Sin(Side), FMath::Cos(Side));
	const FVector Across = FVector::CrossProduct(Out, FVector::XAxisVector);
	OutDirection = (FVector::XAxisVector * FMath::Cos(Cross) + Across * FMath::Sin(Cross)).GetSafeNormal();
	// The mating axis sits so the cup is exactly Depth deep below the log's surface.
	OutCentre = FVector(LocalX, 0.f, 0.f) + Out * (Radius + MateRadius * CupFit - Depth);
}

bool ACarvableLog::Notch(float LocalX, float SideAngleDeg, float CrossAngleDeg, float MateRadius, float Depth)
{
	FVector Centre, Direction;
	GetNotchFrame(LocalX, SideAngleDeg, CrossAngleDeg, MateRadius, Depth, Centre, Direction);

	UDynamicMesh* Tool = NewObject<UDynamicMesh>(this);
	const FTransform ToolTransform(FRotationMatrix::MakeFromZ(Direction).ToQuat(), Centre - Direction * (BigBox * 0.25f));
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
		Tool, FGeometryScriptPrimitiveOptions(), ToolTransform, MateRadius * CupFit, BigBox * 0.5f, 64, 8, true,
		EGeometryScriptPrimitiveOriginMode::Base);
	SubtractTool(MeshComponent->GetDynamicMesh(), Tool);
	FinishEdit();
	JointCuts.Add({ ECarveTool::Notch, LocalX, SideAngleDeg, CrossAngleDeg, 0.f, Depth, MateRadius });
	// A square cup is as long as the crossing log is thick; a lengthwise groove takes no corner.
	if (CrossAngleDeg >= 1.f)
	{
		const float Half = MateRadius * CupFit / FMath::Max(FMath::Sin(FMath::DegreesToRadians(CrossAngleDeg)), 0.2f);
		CutSpans.Add(FVector2f(LocalX - Half, LocalX + Half));
	}
	if (!(SourceMesh && bStartAsleep))
	{
		Resettle();
	}
	OnLogCarved.Broadcast(this, ECarveTool::Notch);
	return true;
}

void ACarvableLog::GetProfileFrame(float LocalX, float SideAngleDeg, float Width, float Depth, bool bRound,
	FVector& OutCentre, FVector& OutOut, FVector& OutAcross) const
{
	const float Side = FMath::DegreesToRadians(SideAngleDeg);
	OutOut = FVector(0.f, FMath::Sin(Side), FMath::Cos(Side));
	OutAcross = FVector::CrossProduct(OutOut, FVector::XAxisVector);
	Depth = FMath::Clamp(Depth, 0.f, Radius);
	// Round: the cutter circle touches the floor of the notch. Flat: the box floor sits Depth below the surface
	// and the box reaches a full radius past it, well outside the log.
	const float FromAxis = bRound ? Radius - Depth + Width * 0.5f : Radius - Depth + Radius;
	OutCentre = FVector(LocalX, 0.f, 0.f) + OutOut * FromAxis;
}

bool ACarvableLog::CutProfile(float LocalX, float SideAngleDeg, float Width, float Depth, bool bRound)
{
	if (Width <= 0.f || Depth <= 0.f)
	{
		return false;
	}
	FVector Centre, Out, Across;
	GetProfileFrame(LocalX, SideAngleDeg, Width, Depth, bRound, Centre, Out, Across);

	UDynamicMesh* Tool = NewObject<UDynamicMesh>(this);
	if (bRound)
	{
		// Cylinder axis runs across the log, square to both the log and the cut direction.
		const FTransform ToolTransform(FRotationMatrix::MakeFromZ(Across).ToQuat(), Centre);
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
			Tool, FGeometryScriptPrimitiveOptions(), ToolTransform, Width * 0.5f, Radius * 4.f, 48, 0, true,
			EGeometryScriptPrimitiveOriginMode::Center);
	}
	else
	{
		// Box: X along the log (Width), Z out of the log (2 * Radius tall), Y across (wider than the log).
		// An end lap reaches the end of the log: the box runs on past it so the end comes out clean and open.
		float X0 = LocalX - Width * 0.5f;
		float X1 = LocalX + Width * 0.5f;
		constexpr float PastEnd = 20.f;
		X0 = X0 <= MinX + 1.f ? X0 - PastEnd : X0;
		X1 = X1 >= MaxX - 1.f ? X1 + PastEnd : X1;
		Centre.X = (X0 + X1) * 0.5f;
		const FTransform ToolTransform(FRotationMatrix::MakeFromXZ(FVector::XAxisVector, Out).ToQuat(), Centre);
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
			Tool, FGeometryScriptPrimitiveOptions(), ToolTransform, X1 - X0, Radius * 4.f, Radius * 2.f, 0, 0, 0,
			EGeometryScriptPrimitiveOriginMode::Center);
	}
	SubtractTool(MeshComponent->GetDynamicMesh(), Tool);
	FinishEdit();
	CutSpans.Add(FVector2f(LocalX - Width * 0.5f, LocalX + Width * 0.5f));
	JointCuts.Add({ bRound ? ECarveTool::NotchRound : ECarveTool::NotchFlat, LocalX, SideAngleDeg, 0.f, Width, Depth, 0.f });
	if (!(SourceMesh && bStartAsleep))
	{
		Resettle();
	}
	OnLogCarved.Broadcast(this, bRound ? ECarveTool::NotchRound : ECarveTool::NotchFlat);
	return true;
}

bool ACarvableLog::CutDovetail(float LocalX, float SideAngleDeg, float Width, float Depth)
{
	if (Width <= 0.f || Depth <= 0.f)
	{
		return false;
	}
	const float Side = FMath::DegreesToRadians(SideAngleDeg);
	const FVector Out(0.f, FMath::Sin(Side), FMath::Cos(Side));
	const float Floor = Radius - FMath::Min(Depth, Radius);
	const float Neck = Width * DovetailNeck;

	// The socket's profile in the plane of the log axis and the cut direction: wide at the floor, narrow at the
	// surface (the undercut), then straight up and out of the log so the cut comes out clean. Counter-clockwise.
	const TArray<FVector2D> Profile = {
		FVector2D(-Width * 0.5f, Floor), FVector2D(Width * 0.5f, Floor),
		FVector2D(Neck * 0.5f, Radius), FVector2D(Neck * 0.5f, Radius * 2.f),
		FVector2D(-Neck * 0.5f, Radius * 2.f), FVector2D(-Neck * 0.5f, Radius) };

	UDynamicMesh* Tool = NewObject<UDynamicMesh>(this);
	// Profile X along the log, profile Y out of the log; the extrusion runs across it, through the whole log.
	const FTransform ToolTransform(FRotationMatrix::MakeFromXY(FVector::XAxisVector, Out).ToQuat(), FVector(LocalX, 0.f, 0.f));
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleExtrudePolygon(
		Tool, FGeometryScriptPrimitiveOptions(), ToolTransform, Profile, Radius * 4.f, 0, true,
		EGeometryScriptPrimitiveOriginMode::Center);
	SubtractTool(MeshComponent->GetDynamicMesh(), Tool);
	FinishEdit();
	CutSpans.Add(FVector2f(LocalX - Width * 0.5f, LocalX + Width * 0.5f));
	JointCuts.Add({ ECarveTool::Dovetail, LocalX, SideAngleDeg, 0.f, Width, Depth, 0.f });
	if (!(SourceMesh && bStartAsleep))
	{
		Resettle();
	}
	OnLogCarved.Broadcast(this, ECarveTool::Dovetail);
	return true;
}

bool ACarvableLog::ShapeEnd(bool bMaxEnd, ECarveTool Shape)
{
	ELogEnd& End = bMaxEnd ? EndMax : EndMin;
	UDynamicMesh* Mesh = MeshComponent->GetDynamicMesh();

	if (Shape == ECarveTool::Trim)
	{
		if (End == ELogEnd::Flat)
		{
			return false;
		}
		const float Cut = End == ELogEnd::Pointed ? PointLength : ChamferSize;
		if (GetLength() < Cut + 2.f)
		{
			return false;
		}
		if (bMaxEnd)
		{
			SubtractHalfSpace(Mesh, FVector::XAxisVector, MaxX - Cut);
		}
		else
		{
			SubtractHalfSpace(Mesh, -FVector::XAxisVector, -(MinX + Cut));
		}
		End = ELogEnd::Flat;
		FinishEdit();
		if (!(SourceMesh && bStartAsleep))
		{
			Resettle();
		}
		OnLogCarved.Broadcast(this, ECarveTool::Trim);
		return true;
	}

	const bool bPoint = Shape == ECarveTool::Point;
	const float Length = bPoint ? PointLength : ChamferSize;
	const float TipRadius = bPoint ? PointTipRadius : Radius - ChamferSize;
	if (End != ELogEnd::Flat || GetLength() < Length + 30.f)
	{
		return false;
	}

	// What comes off is a collar round the end: a cylinder just bigger than the log, from a little outside the end to a
	// little past where the cone meets the bark, minus the cone the log is left with. The cone starts outside the end
	// and runs on past the bark, so no surface of the tool lies on one of the log's.
	const FVector Inward = bMaxEnd ? -FVector::XAxisVector : FVector::XAxisVector;
	const FVector EndPoint(bMaxEnd ? MaxX : MinX, 0.f, 0.f);
	constexpr float LeadIn = 5.f;
	constexpr float PastBark = 8.f;
	const float Slope = (Radius - TipRadius) / Length;
	const FQuat Aim = FRotationMatrix::MakeFromZ(Inward).ToQuat();

	UDynamicMesh* Collar = NewObject<UDynamicMesh>(this);
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
		Collar, FGeometryScriptPrimitiveOptions(), FTransform(Aim, EndPoint - Inward * LeadIn), Radius + 10.f, Length + LeadIn + 4.f, 48, 0, true,
		EGeometryScriptPrimitiveOriginMode::Base);
	UDynamicMesh* Cone = NewObject<UDynamicMesh>(this);
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCone(
		Cone, FGeometryScriptPrimitiveOptions(), FTransform(Aim, EndPoint - Inward * LeadIn), FMath::Max(TipRadius - Slope * LeadIn, 0.5f),
		TipRadius + Slope * (Length + PastBark), Length + LeadIn + PastBark, 48, 0, true, EGeometryScriptPrimitiveOriginMode::Base);
	SubtractTool(Collar, Cone);
	SubtractTool(Mesh, Collar);

	End = bPoint ? ELogEnd::Pointed : ELogEnd::Chamfered;
	FinishEdit();
	if (!(SourceMesh && bStartAsleep))
	{
		Resettle();
	}
	OnLogCarved.Broadcast(this, Shape);
	return true;
}

namespace
{
	/** Guide lines float this far above the original bark so they are not buried in it. */
	constexpr float GuideLift = 0.8f;
	constexpr int32 GuideSteps = 96;

	FVector SurfacePoint(float X, float Angle, float Radius)
	{
		return FVector(X, FMath::Sin(Angle) * Radius, FMath::Cos(Angle) * Radius);
	}

	/**
	 * Rim of a cylindrical cutter (axis through Centre along Dir, radius CutRadius) on a log of Radius:
	 * for each angle round the log the bark inside the cutter is an interval along X (a quadratic in X).
	 * Runs of angles with an interval close into loops: one edge on the way out, the other on the way back.
	 */
	void CylinderRim(const FVector& Centre, const FVector& Dir, float CutRadius, float Radius, float MinX, float MaxX,
		float SideRad, TArray<TArray<FVector>>& OutLines, TArray<bool>& OutClosed)
	{
		const float A = 1.f - Dir.X * Dir.X;
		TArray<FVector> Near, Far;
		auto Flush = [&]()
		{
			if (Near.Num() > 1)
			{
				TArray<FVector>& Loop = OutLines.AddDefaulted_GetRef();
				Loop = Near;
				for (int32 Index = Far.Num() - 1; Index >= 0; --Index)
				{
					Loop.Add(Far[Index]);
				}
				OutClosed.Add(true);
			}
			Near.Reset();
			Far.Reset();
		};

		for (int32 Step = 0; Step <= GuideSteps; ++Step)
		{
			const float Angle = SideRad - PI + 2.f * PI * Step / GuideSteps;
			const FVector B = SurfacePoint(0.f, Angle, Radius) - Centre;
			const float BDir = FVector::DotProduct(B, Dir);
			const float C = B.SizeSquared() - BDir * BDir - CutRadius * CutRadius;
			float X0, X1;
			if (A < KINDA_SMALL_NUMBER)
			{
				// Cutter runs along the log: inside everywhere or nowhere at this angle.
				if (C >= 0.f)
				{
					Flush();
					continue;
				}
				X0 = MinX;
				X1 = MaxX;
			}
			else
			{
				const float Bq = 2.f * (B.X - Dir.X * BDir);
				const float Disc = Bq * Bq - 4.f * A * C;
				if (Disc <= 0.f)
				{
					Flush();
					continue;
				}
				const float Root = FMath::Sqrt(Disc);
				X0 = FMath::Max((-Bq - Root) / (2.f * A), MinX);
				X1 = FMath::Min((-Bq + Root) / (2.f * A), MaxX);
				if (X0 >= X1)
				{
					Flush();
					continue;
				}
			}
			Near.Add(SurfacePoint(X0, Angle, Radius + GuideLift));
			Far.Add(SurfacePoint(X1, Angle, Radius + GuideLift));
		}
		Flush();
	}
}

void ACarvableLog::GetCutGuide(ECarveTool Tool, float LocalX, float SideAngleDeg, float AngleDeg, float Width, float Depth,
	TArray<TArray<FVector>>& OutLines, TArray<bool>& OutClosed) const
{
	OutLines.Reset();
	OutClosed.Reset();
	const float Side = FMath::DegreesToRadians(SideAngleDeg);
	const float R = Radius;
	const float Lifted = R + GuideLift;

	switch (Tool)
	{
	case ECarveTool::CrossCut:
	{
		TArray<FVector>& Ring = OutLines.AddDefaulted_GetRef();
		for (int32 Step = 0; Step < GuideSteps; ++Step)
		{
			Ring.Add(SurfacePoint(LocalX, 2.f * PI * Step / GuideSteps, Lifted));
		}
		OutClosed.Add(true);
		break;
	}
	case ECarveTool::Split:
	{
		// The splitting plane holds the axis: it meets the bark along two lines and crosses both end faces.
		const float Angle = FMath::DegreesToRadians(AngleDeg);
		for (const float Edge : { Angle, Angle + PI })
		{
			OutLines.Add({ SurfacePoint(MinX, Edge, Lifted), SurfacePoint(MaxX, Edge, Lifted) });
			OutClosed.Add(false);
		}
		for (const float End : { MinX - GuideLift, MaxX + GuideLift })
		{
			OutLines.Add({ SurfacePoint(End, Angle, R), SurfacePoint(End, Angle + PI, R) });
			OutClosed.Add(false);
		}
		break;
	}
	case ECarveTool::Notch:
	{
		// Same cup as Notch(): crossing log of this radius, half its thickness deep (a shallow groove at 0 degrees).
		const float NotchDepth = AngleDeg < 1.f ? R * 0.134f : R;
		FVector Centre, Direction;
		GetNotchFrame(LocalX, SideAngleDeg, AngleDeg, R, NotchDepth, Centre, Direction);
		CylinderRim(Centre, Direction, R * CupFit, R, MinX, MaxX, Side, OutLines, OutClosed);
		break;
	}
	case ECarveTool::Groove:
		GetCutGuide(ECarveTool::Notch, GetCenterX(), SideAngleDeg, 0.f, Width, Depth, OutLines, OutClosed);
		break;
	case ECarveTool::NotchRound:
	{
		FVector Centre, Out, Across;
		GetProfileFrame(LocalX, SideAngleDeg, Width, Depth, true, Centre, Out, Across);
		CylinderRim(Centre, Across, Width * 0.5f, R, MinX, MaxX, Side, OutLines, OutClosed);
		break;
	}
	case ECarveTool::Point:
	case ECarveTool::Chamfer:
	case ECarveTool::Trim:
	{
		// A ring where the shaping starts: one module in from a stake's tip, a chamfer's width in, or where a shaped
		// end is cut back to. A stake also gets four lines running to its tip.
		const bool bMaxEnd = LocalX >= GetCenterX();
		const ELogEnd End = bMaxEnd ? EndMax : EndMin;
		const float Reach = Tool == ECarveTool::Point ? PointLength
			: Tool == ECarveTool::Chamfer ? ChamferSize : (End == ELogEnd::Pointed ? PointLength : ChamferSize);
		const float RingX = bMaxEnd ? MaxX - Reach : MinX + Reach;
		TArray<FVector>& Ring = OutLines.AddDefaulted_GetRef();
		for (int32 Step = 0; Step < GuideSteps; ++Step)
		{
			Ring.Add(SurfacePoint(RingX, 2.f * PI * Step / GuideSteps, Lifted));
		}
		OutClosed.Add(true);
		if (Tool == ECarveTool::Point)
		{
			for (int32 Spoke = 0; Spoke < 4; ++Spoke)
			{
				const float Angle = 0.5f * PI * Spoke;
				OutLines.Add({ SurfacePoint(RingX, Angle, Lifted), SurfacePoint(bMaxEnd ? MaxX : MinX, Angle, PointTipRadius + GuideLift) });
				OutClosed.Add(false);
			}
		}
		break;
	}
	case ECarveTool::NotchFlat:
	case ECarveTool::Dovetail:
	{
		// The floor is a plane Depth below the bark: the rim is a rectangle wrapped round the log. A dovetail's
		// opening at the surface is narrower than its floor.
		const float RimWidth = Tool == ECarveTool::Dovetail ? Width * DovetailNeck : Width;
		const float Half = FMath::Acos(FMath::Clamp((R - FMath::Min(Depth, R)) / R, -1.f, 1.f));
		const float X0 = FMath::Max(LocalX - RimWidth * 0.5f, MinX);
		const float X1 = FMath::Min(LocalX + RimWidth * 0.5f, MaxX);
		TArray<FVector>& Loop = OutLines.AddDefaulted_GetRef();
		const int32 ArcSteps = 16;
		for (int32 Step = 0; Step <= ArcSteps; ++Step)
		{
			Loop.Add(SurfacePoint(X0, Side - Half + 2.f * Half * Step / ArcSteps, Lifted));
		}
		for (int32 Step = 0; Step <= ArcSteps; ++Step)
		{
			Loop.Add(SurfacePoint(X1, Side + Half - 2.f * Half * Step / ArcSteps, Lifted));
		}
		OutClosed.Add(true);
		break;
	}
	default:
		break;
	}
}

bool ACarvableLog::GetLogAxis(const AActor* Log, FVector& OutCentre, FVector& OutAxis, float& OutRadius, float& OutHalfLength)
{
	if (const ACarvableLog* Carvable = Cast<ACarvableLog>(Log))
	{
		const FTransform& Frame = Carvable->GetActorTransform();
		OutCentre = Frame.TransformPosition(FVector(Carvable->GetCenterX(), 0.f, 0.f));
		OutAxis = Frame.GetUnitAxis(EAxis::X);
		OutRadius = Carvable->GetRadius() * Frame.GetScale3D().Z;
		OutHalfLength = Carvable->GetLength() * 0.5f * Frame.GetScale3D().X;
		return true;
	}
	const APickupItem* Pickup = Cast<APickupItem>(Log);
	const UStaticMeshComponent* PickupMesh = Pickup ? Pickup->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!PickupMesh || !PickupMesh->GetStaticMesh() || Pickup->GetItem() != EForestItem::Log)
	{
		return false;
	}
	const FBox Box = PickupMesh->GetStaticMesh()->GetBoundingBox();
	const FTransform& Frame = PickupMesh->GetComponentTransform();
	OutCentre = Frame.TransformPosition(Box.GetCenter());
	OutAxis = Frame.GetUnitAxis(EAxis::X);
	OutRadius = Box.GetExtent().Z * Frame.GetScale3D().Z;
	OutHalfLength = Box.GetExtent().X * Frame.GetScale3D().X;
	return true;
}

bool ACarvableLog::ComputeCrossPlacement(const AActor* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Upper,
	FCrossPlacement& Out)
{
	FVector Centre, Axis;
	float LowerRadius, HalfLength;
	if (!Upper || Upper == Lower || !GetLogAxis(Lower, Centre, Axis, LowerRadius, HalfLength) || FMath::Abs(Axis.Z) > 0.3f)
	{
		return false;
	}
	const FBox Box = Upper->GetLocalBox();
	const float UpperRadius = (Box.GetExtent().Y + Box.GetExtent().Z) * 0.5f;

	// The crossing point on the lower log's axis, kept a log's thickness in from its ends so the upper log rests on it.
	const float Room = HalfLength - UpperRadius * 1.2f;
	if (Room <= 0.f)
	{
		return false;
	}
	const float Along = FMath::Clamp(FVector::DotProduct(Aim - Centre, Axis), -Room, Room);
	const FVector Cross = Centre + Axis * Along;

	// Square across, running away from the player.
	FVector Direction = FVector::CrossProduct(FVector::UpVector, FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal());
	if (FVector::DotProduct(Direction, Cross - Viewer) < 0.f)
	{
		Direction = -Direction;
	}

	// The notch sits on the first module line from the near end, like a wall corner; the upper axis is a cup above the lower one.
	// The upper piece needs that line free and a module of wood beyond it.
	TArray<float> Lines;
	Upper->GetModuleLines(Lines);
	const float* First = Lines.FindByPredicate([&Box](float X) { return X - Box.Min.X >= ModuleLength - 1.f; });
	if (!First || Box.Max.X - *First < ModuleLength - 1.f
		|| !Upper->IsSpanFree(*First - LowerRadius * CupFit, *First + LowerRadius * CupFit))
	{
		return false;
	}
	Out.NotchX = *First;
	Out.LowerRadius = LowerRadius;
	Out.UpperRadius = UpperRadius;
	const float AxisGap = UpperRadius + LowerRadius * CupFit - LowerRadius;
	const FVector Location = Cross + FVector(0.f, 0.f, AxisGap) - Direction * Out.NotchX;
	Out.Upper = FTransform(FRotator(0.f, Direction.Rotation().Yaw, 0.f), Location);
	return true;
}

ACarvableLog* ACarvableLog::FindBeamOnPost(const ACarvableLog* Post)
{
	FVector PostCentre, PostAxis;
	float PostRadius, PostHalf;
	if (!Post || !GetLogAxis(Post, PostCentre, PostAxis, PostRadius, PostHalf) || FMath::Abs(PostAxis.Z) < 0.95f)
	{
		return nullptr;
	}
	const FVector Top = PostCentre + PostAxis * (PostAxis.Z > 0.f ? PostHalf : -PostHalf);
	for (TActorIterator<ACarvableLog> It(Post->GetWorld()); It; ++It)
	{
		FVector Centre, Axis;
		float Radius, Half;
		if (*It == Post || !It->IsAnchored() || !GetLogAxis(*It, Centre, Axis, Radius, Half) || FMath::Abs(Axis.Z) > 0.26f)
		{
			continue;
		}
		double T = 0.0;
		const double Distance = WoodStructure::Geo::DistToSegment(Top, Centre - Axis * Half, Centre + Axis * Half, T);
		if (Distance <= Radius + 12.0 && Centre.Z > Top.Z - 5.f)
		{
			return *It;
		}
	}
	return nullptr;
}

bool ACarvableLog::ComputeBracePlacement(const ACarvableLog* Post, const ACarvableLog* Piece, FTransform& OutTransform)
{
	const ACarvableLog* Beam = FindBeamOnPost(Post);
	if (!Beam || !Piece || Piece == Post || Piece == Beam)
	{
		return false;
	}
	FVector PostCentre, PostAxis, BeamCentre, BeamAxis;
	float PostRadius, PostHalf, BeamRadius, BeamHalf;
	GetLogAxis(Post, PostCentre, PostAxis, PostRadius, PostHalf);
	GetLogAxis(Beam, BeamCentre, BeamAxis, BeamRadius, BeamHalf);
	const FVector Top = PostCentre + PostAxis * (PostAxis.Z > 0.f ? PostHalf : -PostHalf);

	// Towards the middle of the beam from where it rests on the post.
	FVector Along = FVector(BeamAxis.X, BeamAxis.Y, 0.f).GetSafeNormal();
	if (FVector::DotProduct(Top - BeamCentre, Along) > 0.f)
	{
		Along = -Along;
	}

	// A and B are on the axes of the post and of the beam; the brace is a little longer than AB so its ends sit in them.
	const float Length = Piece->GetLength();
	const float AB = Length - Piece->GetRadius();
	const float Drop = AB / UE_SQRT_2 - BeamRadius;       // how far under the beam's top the brace starts on the post
	if (Length < 2.f * ModuleLength - 1.f || Length > 4.f * ModuleLength + 1.f || Drop < 15.f || Drop > 2.f * PostHalf - 10.f)
	{
		return false;
	}
	const float Run = Drop + BeamRadius;                 // 45 degrees: as far along the beam as up
	const FVector A(PostCentre.X, PostCentre.Y, Top.Z - Drop);
	const FVector B = A + Along * Run + FVector(0.f, 0.f, Run);
	if (FMath::Abs(FVector::DotProduct(B - BeamCentre, BeamAxis)) > BeamHalf - 5.f)
	{
		return false;
	}
	const FQuat Rotation = FRotationMatrix::MakeFromX((B - A).GetSafeNormal()).ToQuat();
	OutTransform = FTransform(Rotation, (A + B) * 0.5f - Rotation.RotateVector(FVector(Piece->GetCenterX(), 0.f, 0.f)));
	return true;
}

bool ACarvableLog::ComputeFloorPlacement(const AActor* Lower, const FVector& Aim, const FVector& Viewer, const ACarvableLog* Piece,
	FTransform& OutTransform)
{
	FVector Centre, Axis;
	float LowerRadius, HalfLength;
	if (!Piece || !Piece->IsSplit() || Piece == Lower || !GetLogAxis(Lower, Centre, Axis, LowerRadius, HalfLength) || FMath::Abs(Axis.Z) > 0.3f)
	{
		return false;
	}
	const float Along = FMath::Clamp(FVector::DotProduct(Aim - Centre, Axis), -(HalfLength - 10.f), HalfLength - 10.f);
	const FVector Cross = Centre + Axis * Along;
	FVector Direction = FVector::CrossProduct(FVector::UpVector, FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal());
	if (FVector::DotProduct(Direction, Cross - Viewer) < 0.f)
	{
		Direction = -Direction;
	}
	// Flat face up: the face's outward direction (the split angle) is turned to point up by rolling about the log's own axis.
	const float Roll = FMath::DegreesToRadians(Piece->GetFlatAngle() + 90.f);
	const FQuat Rotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f).Quaternion() * FQuat(FVector::XAxisVector, Roll);
	const FVector Near = Cross - Direction * LowerRadius;
	FVector AxisPoint = Near + Direction * (Piece->GetLength() * 0.5f);
	AxisPoint.Z = Centre.Z + LowerRadius + Piece->GetRadius() * 0.6f;
	OutTransform = FTransform(Rotation, AxisPoint - Rotation.RotateVector(FVector(Piece->GetCenterX(), 0.f, 0.f)));
	return true;
}

void ACarvableLog::GetCrossNotchGuide(const FCrossPlacement& Placement, const ACarvableLog* Upper,
	TArray<TArray<FVector>>& OutLines, TArray<bool>& OutClosed)
{
	OutLines.Reset();
	OutClosed.Reset();
	if (!Upper)
	{
		return;
	}
	const FBox Box = Upper->GetLocalBox();
	// Same cutter as Notch(NotchX, 180, 90, LowerRadius, LowerRadius): the lower log's round, its axis along local Y.
	const FVector Centre(Placement.NotchX, 0.f, -(Placement.UpperRadius + Placement.LowerRadius * CupFit - Placement.LowerRadius));
	CylinderRim(Centre, FVector::YAxisVector, Placement.LowerRadius * CupFit, Placement.UpperRadius, Box.Min.X, Box.Max.X,
		PI, OutLines, OutClosed);
}

ACarvableLog* ACarvableLog::PlaceAcross(AActor* Lower, const FCrossPlacement& Placement, ACarvableLog* Upper)
{
	if (!Lower || !Upper)
	{
		return nullptr;
	}
	// The lower log becomes part of the corner too: a lying log item is put on the bench first, then both hold still.
	ACarvableLog* LowerLog = Cast<ACarvableLog>(Lower);
	if (!LowerLog)
	{
		LowerLog = SpawnFromPickup(Cast<APickupItem>(Lower));
	}
	if (LowerLog)
	{
		LowerLog->Anchor();
	}

	Upper->PlaceAt(Placement.Upper, /*bAnchorIt*/ true);
	// Underside (180), square across (90), cup as round as the lower log and half of it deep: it seats onto it.
	Upper->Notch(Placement.NotchX, 180.f, 90.f, Placement.LowerRadius, Placement.LowerRadius);

	// A wall corner rests on more than one log: every other log the upper one crosses gets a cup too, cut to the
	// depth that seats it at the height it already has, and becomes part of the corner.
	const FTransform& UpperFrame = Upper->GetActorTransform();
	const FVector UA = UpperFrame.TransformPosition(FVector(Upper->GetLocalBox().Min.X, 0.f, 0.f));
	const FVector UB = UpperFrame.TransformPosition(FVector(Upper->GetLocalBox().Max.X, 0.f, 0.f));
	for (TActorIterator<ACarvableLog> It(Upper->GetWorld()); It; ++It)
	{
		ACarvableLog* Other = *It;
		if (Other == Upper || Other == LowerLog || Other->IsCarried() || Other->GetRootComponent()->IsSimulatingPhysics())
		{
			continue;
		}
		FVector OtherCentre, OtherAxis;
		float OtherRadius, OtherHalf;
		if (!GetLogAxis(Other, OtherCentre, OtherAxis, OtherRadius, OtherHalf)
			|| FMath::Abs(FVector::DotProduct(OtherAxis, (UB - UA).GetSafeNormal())) > 0.26f)
		{
			continue;
		}
		double S = 0.0, T = 0.0;
		WoodStructure::Geo::ClosestSegSeg(UA, UB, OtherCentre - OtherAxis * OtherHalf, OtherCentre + OtherAxis * OtherHalf, S, T);
		const FVector OnUpper = FMath::Lerp(UA, UB, static_cast<float>(S));
		const FVector OnOther = FMath::Lerp(OtherCentre - OtherAxis * OtherHalf, OtherCentre + OtherAxis * OtherHalf, static_cast<float>(T));
		const float Gap = FVector::Dist(OnUpper, OnOther);
		if (OnUpper.Z <= OnOther.Z || Gap >= Placement.UpperRadius + OtherRadius - 1.f)
		{
			continue;
		}
		const float LocalX = UpperFrame.InverseTransformPosition(OnUpper).X;
		const float Depth = Placement.UpperRadius + OtherRadius * CupFit - Gap;
		if (Upper->IsSpanFree(LocalX - OtherRadius, LocalX + OtherRadius))
		{
			Upper->Notch(LocalX, 180.f, 90.f, OtherRadius, Depth);
		}
		Other->Anchor();
	}
	OnLogPlacedAcross.Broadcast(Upper);
	return Upper;
}

ACarvableLog* ACarvableLog::CrossCut(float LocalX)
{
	UDynamicMesh* Mesh = MeshComponent->GetDynamicMesh();
	UDynamicMesh* Other = NewObject<UDynamicMesh>(this);
	Other->SetMesh(Mesh->GetMeshRef());

	SubtractHalfSpace(Mesh, FVector::XAxisVector, LocalX);
	SubtractHalfSpace(Other, -FVector::XAxisVector, -LocalX);
	// This piece keeps the near end as it was, the cut end is square; the other piece keeps the far end.
	const ELogEnd FarEnd = EndMax;
	EndMax = ELogEnd::Flat;
	FinishEdit();
	Resettle();
	ACarvableLog* Piece = SpawnPiece(Other);
	if (Piece)
	{
		Piece->EndMax = FarEnd;
		Piece->EndMin = ELogEnd::Flat;
	}
	OnLogCarved.Broadcast(this, ECarveTool::CrossCut);
	return Piece;
}

ACarvableLog* ACarvableLog::Split(float AngleDeg)
{
	const float Angle = FMath::DegreesToRadians(AngleDeg);
	// Normal of the splitting plane (the plane contains the log axis).
	const FVector Normal(0.f, FMath::Cos(Angle), -FMath::Sin(Angle));

	UDynamicMesh* Mesh = MeshComponent->GetDynamicMesh();
	UDynamicMesh* Other = NewObject<UDynamicMesh>(this);
	Other->SetMesh(Mesh->GetMeshRef());

	SubtractHalfSpace(Mesh, Normal, 0.f);
	SubtractHalfSpace(Other, -Normal, 0.f);
	FinishEdit();
	Resettle();
	bSplitPiece = true;
	++SplitCount;
	FlatAngle = AngleDeg;
	ACarvableLog* Piece = SpawnPiece(Other);
	if (Piece)
	{
		Piece->FlatAngle = AngleDeg + 180.f;
	}
	OnLogCarved.Broadcast(this, ECarveTool::Split);
	return Piece;
}

void ACarvableLog::SubtractTool(UDynamicMesh* Target, UDynamicMesh* Tool)
{
	// New faces come from the tool: give them the cut-wood slot.
	UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(Tool, 0, 1);
	FGeometryScriptMeshBooleanOptions Options;
	Options.bFillHoles = true;
	Options.bSimplifyOutput = false;
	UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
		Target, FTransform::Identity, Tool, FTransform::Identity, EGeometryScriptBooleanOperation::Subtract, Options);
}

void ACarvableLog::SubtractHalfSpace(UDynamicMesh* Target, const FVector& Normal, float Offset)
{
	// A big box whose near face lies on the plane (Normal . p = Offset), filling the side the normal points to.
	UDynamicMesh* Tool = NewObject<UDynamicMesh>(this);
	const FVector Centre = Normal * (Offset + BigBox * 0.5f);
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
		Tool, FGeometryScriptPrimitiveOptions(), FTransform(FRotationMatrix::MakeFromZ(Normal).ToQuat(), Centre),
		BigBox, BigBox, BigBox, 0, 0, 0, EGeometryScriptPrimitiveOriginMode::Center);
	SubtractTool(Target, Tool);
}

void ACarvableLog::FinishEdit()
{
	UDynamicMesh* Mesh = MeshComponent->GetDynamicMesh();

	FGeometryScriptSplitNormalsOptions SplitOptions;
	SplitOptions.bSplitByOpeningAngle = true;
	SplitOptions.OpeningAngleDeg = 40.f;
	UGeometryScriptLibrary_MeshNormalsFunctions::ComputeSplitNormals(Mesh, SplitOptions, FGeometryScriptCalculateNormalsOptions());

	const FBox Box = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(Mesh);
	MinX = Box.Min.X;
	MaxX = Box.Max.X;
	if (!bGridSet)
	{
		GridOriginX = MinX;
		bGridSet = true;
	}

	// Physics shape: a few convex hulls so cups and halves keep roughly their form.
	FGeometryScriptCollisionFromMeshOptions Collision;
	Collision.bEmitTransaction = false;
	Collision.Method = EGeometryScriptCollisionGenerationMethod::ConvexHulls;
	Collision.MaxConvexHullsPerMesh = 4;
	UGeometryScriptLibrary_CollisionFunctions::SetDynamicMeshCollisionFromMesh(Mesh, MeshComponent, Collision);

	float Area = 0.f;
	UGeometryScriptLibrary_MeshQueryFunctions::GetMeshVolumeArea(Mesh, Area, Volume);
	MeshComponent->SetMassOverrideInKg(NAME_None, FMath::Max(2.f, Volume * WoodDensity));
	MeshComponent->WakeAllRigidBodies();
}

ACarvableLog* ACarvableLog::SpawnPiece(UDynamicMesh* Mesh) const
{
	// Same transform, same local frame: the rings of the cut-wood material line up across both pieces.
	ACarvableLog* Piece = GetWorld()->SpawnActorDeferred<ACarvableLog>(
		ACarvableLog::StaticClass(), GetActorTransform(), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Piece)
	{
		return nullptr;
	}
	Piece->MeshComponent->GetDynamicMesh()->SetMesh(Mesh->GetMeshRef());
	Piece->Radius = Radius;
	Piece->CutSpans = CutSpans;
	Piece->JointCuts = JointCuts;
	Piece->GridOriginX = GridOriginX;
	Piece->bGridSet = bGridSet;
	Piece->bSplitPiece = bSplitPiece;
	Piece->SplitCount = SplitCount;
	Piece->FlatAngle = FlatAngle;
	Piece->EndMin = EndMin;
	Piece->EndMax = EndMax;
	Piece->bStartAsleep = false;
	Piece->SetMaterials(BarkMaterial);
	Piece->Damage = Damage;
	Piece->bTemporary = bTemporary;
	Piece->FinishEdit();
	Piece->FinishSpawning(GetActorTransform());
	Piece->UpdateDamageLook();
	return Piece;
}
