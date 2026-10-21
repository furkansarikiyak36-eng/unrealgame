// Copyright Epic Games, Inc. All Rights Reserved.

#include "RaidEnemy.h"
#include "CarvableLog.h"
#include "ForestCharacter.h"
#include "StickStructure.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RaidCore/RaidDirectorSubsystem.h"
#include "RaidPhaseR2_Difficulty/RaidStageTableAsset.h"
#include "RaidPhaseR3_Planner/HordePlanAsset.h"
#include "RaidPhaseR8_Defense/StructureDamageResolver.h"

namespace
{
	constexpr float HalfHeight = 90.f;
	constexpr float BodyRadius = 32.f;
}

ARaidEnemy::ARaidEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(BodyRadius, HalfHeight);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	Capsule->SetNotifyRigidBodyCollision(true);
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = Capsule;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Capsule);
	Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.7f, 0.05f, 0.04f));
		Body->SetMaterial(0, Material);
	}
}

void ARaidEnemy::BeginPlay()
{
    Super::BeginPlay();
    DamageResolver=NewObject<UStructureDamageResolver>(this);
}

void ARaidEnemy::TakeHit(float Amount)
{
	if (Health <= 0.f || Amount <= 0.f)
	{
		return;
	}
	Health -= Amount;
	if (Health <= 0.f)
	{
		Destroy();
	}
}

void ARaidEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StepAI(DeltaSeconds);
}

void ARaidEnemy::StepAI(float Dt)
{
	UWorld* World = GetWorld();
	if (!World || Health <= 0.f || Dt <= 0.f)
	{
		return;
	}
	const FVector Here = GetActorLocation();

	// Pointed posts hurt what walks into them.
	for (TActorIterator<ACarvableLog> It(World); It; ++It)
	{
		FVector Centre, Axis;
		float Radius, Half;
		if (!It->IsAnchored() || (It->GetEnd(false) != ELogEnd::Pointed && It->GetEnd(true) != ELogEnd::Pointed)
			|| !ACarvableLog::GetLogAxis(*It, Centre, Axis, Radius, Half) || FMath::Abs(Axis.Z) < 0.95f
			|| FMath::Abs(Here.Z - Centre.Z) > Half + HalfHeight)
		{
			continue;
		}
		if (FVector::Dist2D(Here, Centre) - Radius - BodyRadius < StakeReachCm)
		{
			TakeHit(StakeDps * Dt);
			if (Health <= 0.f)
			{
				return;
			}
		}
	}

	// Stick spikes hurt the same way.
	for (TActorIterator<AStickStructure> It(World); It; ++It)
	{
		if (FVector::Dist2D(Here, It->GetActorLocation()) - BodyRadius - AStickStructure::RadiusCm < AStickStructure::SpikeReachCm)
		{
			TakeHit(AStickStructure::SpikeDps * Dt);
			if (Health <= 0.f)
			{
				return;
			}
		}
	}

	// The target: the one set, else the player if near, else the nearest installed wood.
	AActor* Target = TargetOverride.Get();
	if (!Target)
	{
		APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
		if (Player && FVector::Dist(Player->GetActorLocation(), Here) < 6000.f)
		{
			Target = Player;
		}
		else
		{
			float Best = TNumericLimits<float>::Max();
			for (TActorIterator<ACarvableLog> It(World); It; ++It)
			{
				const float Distance = It->IsAnchored() ? FVector::DistSquared(It->GetActorLocation(), Here) : Best;
				if (Distance < Best)
				{
					Best = Distance;
					Target = *It;
				}
			}
		}
	}
	if (!Target)
	{
		return;
	}

	// The nearest point of the target: along a log's axis, else its centre.
	FVector Goal = Target->GetActorLocation();
	float GoalRadius = BodyRadius;
	FVector Centre, Axis;
	float Radius, Half;
	if (ACarvableLog::GetLogAxis(Target, Centre, Axis, Radius, Half))
	{
		Goal = FMath::ClosestPointOnSegment(Here, Centre - Axis * Half, Centre + Axis * Half);
		GoalRadius = Radius;
	}
	const FVector ToGoal(Goal.X - Here.X, Goal.Y - Here.Y, 0.f);
	const float Distance = ToGoal.Size();
	if (Distance <= AttackReachCm + GoalRadius + BodyRadius)
	{
		if (ACarvableLog* Wood = Cast<ACarvableLog>(Target))
		{
			Wood->AddDamage(WoodDps * Dt);
		}
		else if (AForestCharacter* Player = Cast<AForestCharacter>(Target))
		{
			Player->TakeEnemyHit(PlayerDps * Dt);
		}
		else if (DamageResolver)
		{
			DamageResolver->ApplyDamageToBlock(Target, WoodDps * Dt * 100.0f);
		}
		return;
	}

	// Use the Raid Director's per-frame navigation query budget when a navmesh exists.
	// Greybox levels without navigation data retain the original collision-swept direct mover.
	FVector MoveDirection = ToGoal.GetSafeNormal();
	if (URaidDirectorSubsystem* Director = World->GetSubsystem<URaidDirectorSubsystem>(); Director && Director->HasNavigationData())
	{
		const double NowSeconds = World->GetTimeSeconds();
		const bool bTargetChanged = CachedNavTarget.Get() != Target;
		const bool bGoalMoved = FVector::DistSquared(CachedNavGoal, Goal) > FMath::Square(200.0f);
		if (bTargetChanged || bGoalMoved || NowSeconds >= NextPathRefreshTime)
		{
			CachedNavPath.Reset();
			CachedNavWaypoint = 0;
			CachedNavTarget = Target;
			CachedNavGoal = Goal;
			NextPathRefreshTime = NowSeconds + 0.35;
			TArray<FVector> NewPath;
			if (!Director->FindPathForEnemy(this, Goal, NewPath))
			{
				return; // Never bypass an available navmesh when its budget/path query fails.
			}
			CachedNavPath = MoveTemp(NewPath);
			CachedNavWaypoint = 1; // the first point is normally this actor's projected start
		}
		while (CachedNavPath.IsValidIndex(CachedNavWaypoint))
		{
			const FVector Delta = CachedNavPath[CachedNavWaypoint] - Here;
			if (Delta.Size2D() > 55.0f)
			{
				MoveDirection = FVector(Delta.X, Delta.Y, 0.0f).GetSafeNormal();
				break;
			}
			++CachedNavWaypoint;
		}
		if (!CachedNavPath.IsValidIndex(CachedNavWaypoint))
		{
			return;
		}
	}

	FHitResult Hit;
	AddActorWorldOffset(MoveDirection * MoveSpeed * Dt, true, &Hit);
	if (Hit.bBlockingHit)
	{
		AActor* BlockerActor = Hit.GetActor();
		if (ACarvableLog* Blocker = Cast<ACarvableLog>(BlockerActor); Blocker && Blocker->IsAnchored())
		{
			Blocker->AddDamage(WoodDps * Dt);
		}
		else if (AStickStructure* Fence = Cast<AStickStructure>(BlockerActor))
		{
			// A stick build is weaker than a log wall.
			Fence->AddDamage(WoodDps * 3.f * Dt);
		}
		else if (DamageResolver && IsValid(BlockerActor))
		{
			DamageResolver->ApplyDamageToBlock(BlockerActor, WoodDps * Dt * 100.0f);
		}
	}
	// Keep to the ground.
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RaidEnemyGround), false, this);
	const FVector Now = GetActorLocation();
	if (World->LineTraceSingleByObjectType(Ground, Now + FVector(0.f, 0.f, 100.f), Now - FVector(0.f, 0.f, 400.f),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params) && !Ground.GetActor()->IsA<ACarvableLog>())
	{
		SetActorLocation(FVector(Now.X, Now.Y, Ground.ImpactPoint.Z + HalfHeight + 1.f));
	}
}

bool RaidKit::StartDefaultRaid(UWorld* World)
{
	return StartRaid(World, TEXT("Night1"), 6, 3);
}

bool RaidKit::StartRaid(UWorld* World, FName EventID, int32 Total, int32 AtOnce)
{
	URaidDirectorSubsystem* Director = World ? World->GetSubsystem<URaidDirectorSubsystem>() : nullptr;
	if (!Director)
	{
		return false;
	}
	URaidStageTableAsset* Stages = NewObject<URaidStageTableAsset>(Director);
	FRaidStageConfig Stage;
	Stage.StageIndex = 0;
	Stage.Threshold = 0;
	Stage.Budget.ConcurrentPopulation = FMath::Max(1, AtOnce + 1);
	Stage.Budget.TotalBudget = FMath::Max(1, Total + 2);
	Stage.Budget.RaidDuration = 120.f;
	Stage.PlanID = TEXT("Night1");
	Stages->Stages.Add(Stage);

	UHordePlanAsset* Plan = NewObject<UHordePlanAsset>(Director);
	Plan->PlanID = TEXT("Night1");
	Plan->StageIndex = 0;
	FWavePlan Wave;
	Wave.GroupID = TEXT("Raiders");
	Wave.TotalCount = FMath::Max(1, Total);
	Wave.MaxAlive = FMath::Max(1, AtOnce);
	Wave.Duration = 60.f;
	Wave.Intermission = 5.f;
	Plan->Waves.Add(Wave);

	UEnemyGroupLibrary* Groups = NewObject<UEnemyGroupLibrary>(Director);
	FEnemyGroupDefinition Group;
	Group.GroupID = TEXT("Raiders");
	FWeightedEntry Member;
	Member.EnemyID = TEXT("Raider");
	Member.Weight = 1.f;
	Group.Members.Add(Member);
	Groups->Groups.Add(Group);

	UEnemyLibrary* Enemies = NewObject<UEnemyLibrary>(Director);
	FEnemyDefinition Definition;
	Definition.EnemyID = TEXT("Raider");
	Definition.EnemyClass = ARaidEnemy::StaticClass();
	Enemies->Enemies.Add(Definition);

	Director->ConfigureRaidAssets(Stages, { Plan }, Groups, Enemies);
	Director->StartRaid(EventID, ERaidEventType::HordeNight);
	return Director->GetContext().bEventActive;
}

void RaidKit::StopRaid(UWorld* World)
{
	// The director is the single owner of spawned raid actors. Never destroy every
	// ARaidEnemy in the world; that could remove actors belonging to another system.
	if (URaidDirectorSubsystem* Director = World ? World->GetSubsystem<URaidDirectorSubsystem>() : nullptr)
	{
		Director->CancelRaid(TEXT("Stopped"));
	}
}

int32 RaidKit::CountEnemies(UWorld* World)
{
	int32 Count = 0;
	for (TActorIterator<ARaidEnemy> It(World); It; ++It)
	{
		Count += It->IsAlive() ? 1 : 0;
	}
	return Count;
}
