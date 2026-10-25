#include "StakeActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AStakeActor::AStakeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	StakeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StakeMesh"));
	RootComponent = StakeMesh;
	StakeMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	StakeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StakeMesh->SetSimulatePhysics(false);

	DamageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));
	DamageSphere->SetupAttachment(StakeMesh);
	DamageSphere->SetSphereRadius(18.f);
	DamageSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageSphere->SetGenerateOverlapEvents(true);
	DamageSphere->OnComponentBeginOverlap.AddDynamic(this, &AStakeActor::HandleDamageOverlap);
}

void AStakeActor::BeginPlay()
{
	Super::BeginPlay();
	Health = FMath::Clamp(Health, 1.f, FMath::Max(MaxHealth, 1.f));
	UpdateDamageVolume();
}

void AStakeActor::UpdateDamageVolume()
{
	if (!DamageSphere) return;
	if (StakeMesh && StakeMesh->GetStaticMesh())
	{
		const FBox Bounds = StakeMesh->GetStaticMesh()->GetBoundingBox();
		const FVector Extent = Bounds.GetExtent();
		DamageSphere->SetRelativeLocation(FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z - 12.f));
		DamageSphere->SetSphereRadius(FMath::Clamp(FMath::Max(Extent.X, Extent.Y) * 0.75f, 12.f, 35.f));
	}
	else
	{
		DamageSphere->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	}
	DamageSphere->SetCollisionEnabled(bSharpened ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AStakeActor::Sharpen()
{
	if (bSharpened) return;
	bSharpened = true;
	Damage *= 1.5f;
	UpdateDamageVolume();
}

bool AStakeActor::ApplyStoneReinforcement(int32 StoneCount, float StrengthBonusPerStone)
{
	if (bReinforced || StoneCount < 1 || !FMath::IsFinite(StrengthBonusPerStone) || StrengthBonusPerStone < 0.f)
	{
		return false;
	}
	UStaticMesh* StoneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Greybox/Meshes/SM_GB_Stone.SM_GB_Stone"));
	if (!StoneMesh) return false;

	TArray<TObjectPtr<UStaticMeshComponent>> Created;
	Created.Reserve(StoneCount);
	const FVector BoundsExtent = StakeMesh && StakeMesh->GetStaticMesh()
		? StakeMesh->GetStaticMesh()->GetBoundingBox().GetExtent() : FVector(15.f, 15.f, 60.f);
	for (int32 Index = 0; Index < StoneCount; ++Index)
	{
		UStaticMeshComponent* Stone = NewObject<UStaticMeshComponent>(
			this, *FString::Printf(TEXT("StakeReinforcementStone_%d"), Index));
		if (!Stone)
		{
			for (UStaticMeshComponent* Existing : Created) if (Existing) Existing->DestroyComponent();
			return false;
		}
		Stone->SetStaticMesh(StoneMesh);
		Stone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Stone->SetSimulatePhysics(false);
		Stone->SetMobility(EComponentMobility::Movable);
		Stone->SetRelativeScale3D(FVector(0.18f));
		Stone->SetupAttachment(StakeMesh);
		Stone->RegisterComponent();
		const float Angle = 2.f * PI * static_cast<float>(Index) / static_cast<float>(StoneCount);
		Stone->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * (BoundsExtent.X + 8.f),
			FMath::Sin(Angle) * (BoundsExtent.Y + 8.f),
			-BoundsExtent.Z * 0.65f));
		Created.Add(Stone);
	}
	ReinforcementStones.Append(Created);
	ReinforcementStoneCount = StoneCount;
	StructuralStrengthMultiplier = 1.f + StoneCount * StrengthBonusPerStone;
	bReinforced = true;
	return true;
}

void AStakeActor::ApplyDamageToTarget(AActor* Target, float DamageAmount)
{
	if (!bSharpened || !IsValid(Target) || Target == this || DamageAmount <= 0.f) return;
	UGameplayStatics::ApplyDamage(Target, DamageAmount, nullptr, this, UDamageType::StaticClass());
}

void AStakeActor::HandleDamageOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (bSharpened && IsValid(OtherActor) && OtherActor != this)
	{
		ApplyDamageToTarget(OtherActor, Damage);
	}
}

void AStakeActor::ApplyStructuralDamage(float Amount)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.f || Health <= 0.f) return;
	Health = FMath::Max(0.f, Health - Amount / FMath::Max(1.f, StructuralStrengthMultiplier));
	if (Health <= 0.f) Destroy();
}

float AStakeActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	ApplyStructuralDamage(Applied);
	return Applied;
}
