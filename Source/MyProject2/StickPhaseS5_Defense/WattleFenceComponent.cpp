#include "WattleFenceComponent.h"

#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickConnectionGraph.h"
#include "StickCore/StickDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

UWattleFenceComponent::UWattleFenceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWattleFenceComponent::RollbackPieces(const TArray<AStickActor*>& Pieces)
{
	for (AStickActor* Piece : Pieces)
	{
		if (!IsValid(Piece)) continue;
		if (UStickConnectionGraph* Graph = Piece->FindComponentByClass<UStickConnectionGraph>())
		{
			const TArray<AActor*> Connected = Graph->GetConnectedNodes();
			for (AActor* Other : Connected)
			{
				if (IsValid(Other))
				{
					if (UStructuralGraph* OtherGraph = Other->FindComponentByClass<UStructuralGraph>())
					{
						OtherGraph->DisconnectFrom(Piece);
					}
					Graph->DisconnectFrom(Other);
				}
			}
		}
		Piece->Destroy();
	}
}

bool UWattleFenceComponent::BuildFence(const TArray<FVector>& PostPositions, int32 WeaveRows)
{
	UWorld* World = GetWorld();
	if (!World || !StickClass || !FenceStickDefinition || PostPositions.Num() < 2
		|| PostPositions.Num() > 128 || WeaveRows < 1 || WeaveRows > 8
		|| !FMath::IsFinite(PostHeight) || PostHeight <= 0.f
		|| !FMath::IsFinite(MinimumPostSpacing) || MinimumPostSpacing <= 0.f)
	{
		return false;
	}

	for (int32 Index = 0; Index < PostPositions.Num(); ++Index)
	{
		if (PostPositions[Index].ContainsNaN()) return false;
		if (Index > 0 && FVector::Dist2D(PostPositions[Index - 1], PostPositions[Index]) < MinimumPostSpacing)
		{
			return false;
		}
	}

	TArray<AStickActor*> NewPieces;
	TArray<AStickActor*> Posts;
	Posts.Reserve(PostPositions.Num());
	NewPieces.Reserve(PostPositions.Num() + (PostPositions.Num() - 1) * WeaveRows);

	auto SpawnFenceStick = [&](const FVector& Location, const FRotator& Rotation) -> AStickActor*
	{
		AStickActor* Stick = World->SpawnActor<AStickActor>(StickClass, FTransform(Rotation, Location));
		if (!IsValid(Stick)) return nullptr;
		if (!Stick->GetDefinition()) Stick->InitializeFromDefinition(FenceStickDefinition);
		if (!Stick->GetDefinition() || !Stick->MeshComponent || !Stick->MeshComponent->GetStaticMesh()
			|| !Stick->SetStructuralState_Implementation(EStructuralNodeState::Installed))
		{
			Stick->Destroy();
			return nullptr;
		}
		NewPieces.Add(Stick);
		return Stick;
	};

	const FQuat VerticalRotation = FRotationMatrix::MakeFromX(FVector::UpVector).ToQuat();
	for (const FVector& Ground : PostPositions)
	{
		AStickActor* Post = SpawnFenceStick(
			Ground + FVector(0.f, 0.f, PostHeight * 0.5f),
			VerticalRotation.Rotator());
		if (!Post)
		{
			RollbackPieces(NewPieces);
			return false;
		}
		FVector PostScale = Post->MeshComponent->GetRelativeScale3D();
		PostScale.X *= PostHeight / FMath::Max(1.f, FenceStickDefinition->Length);
		Post->MeshComponent->SetRelativeScale3D(PostScale);
		Posts.Add(Post);
	}

	for (int32 Row = 0; Row < WeaveRows; ++Row)
	{
		const float RowZ = PostHeight * static_cast<float>(Row + 1) / static_cast<float>(WeaveRows + 1);
		for (int32 Segment = 0; Segment < Posts.Num() - 1; ++Segment)
		{
			const FVector A = PostPositions[Segment] + FVector(0.f, 0.f, RowZ);
			const FVector B = PostPositions[Segment + 1] + FVector(0.f, 0.f, RowZ);
			const FVector Delta = B - A;
			const float Span = Delta.Size();
			if (Span <= KINDA_SMALL_NUMBER)
			{
				RollbackPieces(NewPieces);
				return false;
			}

			AStickActor* Rail = SpawnFenceStick((A + B) * 0.5f, Delta.Rotation());
			if (!Rail)
			{
				RollbackPieces(NewPieces);
				return false;
			}
			FVector RailScale = Rail->MeshComponent->GetRelativeScale3D();
			RailScale.X *= Span / FMath::Max(1.f, FenceStickDefinition->Length);
			Rail->MeshComponent->SetRelativeScale3D(RailScale);

			UStickConnectionGraph* RailGraph = Rail->FindComponentByClass<UStickConnectionGraph>();
			UStickConnectionGraph* LeftGraph = Posts[Segment]->FindComponentByClass<UStickConnectionGraph>();
			UStickConnectionGraph* RightGraph = Posts[Segment + 1]->FindComponentByClass<UStickConnectionGraph>();
			const FName LeftSocket(*FString::Printf(TEXT("Rail_%d_Start"), Row));
			const FName RightSocket(*FString::Printf(TEXT("Rail_%d_End"), Row));
			if (!RailGraph || !LeftGraph || !RightGraph
				|| !LeftGraph->ConnectWoven(Rail, LeftSocket, TEXT("RailStart"))
				|| !RightGraph->ConnectWoven(Rail, RightSocket, TEXT("RailEnd")))
			{
				RollbackPieces(NewPieces);
				return false;
			}
		}
	}

	// Swap the complete new fence in only after all post and weave segments connected successfully.
	TArray<AStickActor*> PreviousPieces;
	PreviousPieces.Reserve(BuiltPieces.Num());
	for (AStickActor* Previous : BuiltPieces) PreviousPieces.Add(Previous);
	RollbackPieces(PreviousPieces);
	BuiltPieces.Reset();
	for (AStickActor* Piece : NewPieces) BuiltPieces.Add(Piece);
	ClayFillBySection.Reset();
	return true;
}

bool UWattleFenceComponent::AddClayFill(AStickActor* FenceSection)
{
	if (!IsValid(FenceSection) || !FenceSection->MeshComponent
		|| !FenceSection->MeshComponent->GetStaticMesh()
		|| FenceSection->GetStructuralState_Implementation() != EStructuralNodeState::Installed
		|| ClayFillBySection.Contains(FenceSection))
	{
		return false;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) return false;

	UStaticMeshComponent* Fill = NewObject<UStaticMeshComponent>(
		FenceSection, *FString::Printf(TEXT("ClayInfill_%s"), *FenceSection->GetName()));
	if (!Fill) return false;
	Fill->SetStaticMesh(Cube);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Fill->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FenceSection->AddInstanceComponent(Fill);
	Fill->SetupAttachment(FenceSection->GetRootComponent());
	Fill->RegisterComponent();

	const FBox MeshBounds = FenceSection->MeshComponent->GetStaticMesh()->GetBoundingBox();
	const FVector MeshScale = FenceSection->MeshComponent->GetRelativeScale3D();
	const FVector AbsScale(FMath::Abs(MeshScale.X), FMath::Abs(MeshScale.Y), FMath::Abs(MeshScale.Z));
	const FVector Size = MeshBounds.GetSize() * AbsScale;
	const float PanelHeight = FMath::Max(12.f, PostHeight * 0.65f);
	Fill->SetRelativeLocation(FVector(
		MeshBounds.GetCenter().X * MeshScale.X,
		MeshBounds.GetCenter().Y * MeshScale.Y,
		MeshBounds.GetCenter().Z * MeshScale.Z));
	Fill->SetRelativeScale3D(FVector(
		FMath::Max(0.1f, Size.X / 100.f),
		FMath::Max(0.1f, Size.Y / 100.f + 0.12f),
		FMath::Max(0.1f, PanelHeight / 100.f)));

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Clay = UMaterialInstanceDynamic::Create(Base, Fill);
		if (Clay)
		{
			Clay->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.32f, 0.20f, 0.12f));
			Fill->SetMaterial(0, Clay);
		}
	}

	ClayFillBySection.Add(FenceSection, Fill);
	return true;
}
