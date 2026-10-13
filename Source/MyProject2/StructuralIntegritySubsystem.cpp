// Copyright Epic Games, Inc. All Rights Reserved.

#include "StructuralIntegritySubsystem.h"
#include "BuildingNodeActor.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UStructuralIntegritySubsystem::UStructuralIntegritySubsystem()
	: TimeSinceLastEvaluation(0.0f)
{
}

void UStructuralIntegritySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveNodes.Empty();
}

void UStructuralIntegritySubsystem::Deinitialize()
{
	ActiveNodes.Empty();
	Super::Deinitialize();
}

void UStructuralIntegritySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastEvaluation += DeltaTime;
	if (TimeSinceLastEvaluation >= EvaluationInterval)
	{
		TimeSinceLastEvaluation = 0.0f;
		RecalculateLoadNetwork();
	}
}

TStatId UStructuralIntegritySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStructuralIntegritySubsystem, STATGROUP_Tickables);
}

void UStructuralIntegritySubsystem::RegisterNode(ABuildingNodeActor* Node)
{
	if (!Node || ActiveNodes.Contains(Node))
	{
		return;
	}

	ActiveNodes.Add(Node);

	// Ground detection: check if node is on terrain/bedrock
	UWorld* World = GetWorld();
	if (World)
	{
		FHitResult GroundHit;
		const FVector NodeLoc = Node->GetActorLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GroundCheck), false, Node);

		// Line trace 50cm down
		if (World->LineTraceSingleByChannel(GroundHit, NodeLoc, NodeLoc - FVector(0.f, 0.f, 60.f), ECC_Visibility, Params))
		{
			// If hit actor is not a building node, it's natural terrain/floor
			if (!GroundHit.GetActor() || !GroundHit.GetActor()->IsA<ABuildingNodeActor>())
			{
				Node->SetGrounded(true);
			}
		}
	}

	// Connect to adjacent/underlying building nodes
	const FVector LocA = Node->GetActorLocation();
	for (const TWeakObjectPtr<ABuildingNodeActor>& OtherPtr : ActiveNodes)
	{
		ABuildingNodeActor* Other = OtherPtr.Get();
		if (!Other || Other == Node || Other->IsCollapsed())
		{
			continue;
		}

		const FVector LocB = Other->GetActorLocation();
		const float HorizontalDist = FVector::Dist2D(LocA, LocB);
		const float VerticalDelta = LocA.Z - LocB.Z;

		// Horizontal overlap within standard log / stone dimension (~250cm)
		if (HorizontalDist < 250.0f)
		{
			// Other is beneath Node (within ~150cm height)
			if (VerticalDelta > 15.0f && VerticalDelta < 150.0f)
			{
				Other->AddSupportingNode(Node);
			}
			// Node is beneath Other
			else if (VerticalDelta < -15.0f && VerticalDelta > -150.0f)
			{
				Node->AddSupportingNode(Other);
			}
		}
	}

	RecalculateLoadNetwork();
}

void UStructuralIntegritySubsystem::UnregisterNode(ABuildingNodeActor* Node)
{
	if (!Node)
	{
		return;
	}

	ActiveNodes.Remove(Node);
}

void UStructuralIntegritySubsystem::RecalculateLoadNetwork()
{
	// Clean up stale pointers
	ActiveNodes.RemoveAll([](const TWeakObjectPtr<ABuildingNodeActor>& Ptr) { return !Ptr.IsValid(); });

	// Gravity acceleration g = 9.81 m/s^2; 1 kg mass = ~0.00981 kN
	constexpr float KgToKN = 0.00981f;

	// Reset loads
	for (const TWeakObjectPtr<ABuildingNodeActor>& NodePtr : ActiveNodes)
	{
		if (ABuildingNodeActor* Node = NodePtr.Get())
		{
			if (!Node->IsCollapsed())
			{
				Node->SetCurrentLoadKN(Node->GetSelfWeight() * KgToKN);
			}
		}
	}

	// Propagate loads downward (sort by Z descending)
	TArray<ABuildingNodeActor*> SortedNodes;
	for (const TWeakObjectPtr<ABuildingNodeActor>& NodePtr : ActiveNodes)
	{
		if (ABuildingNodeActor* Node = NodePtr.Get())
		{
			if (!Node->IsCollapsed())
			{
				SortedNodes.Add(Node);
			}
		}
	}

	SortedNodes.Sort([](const ABuildingNodeActor& A, const ABuildingNodeActor& B)
	{
		return A.GetActorLocation().Z > B.GetActorLocation().Z;
	});

	for (ABuildingNodeActor* Node : SortedNodes)
	{
		const float TotalNodeWeight = Node->GetCurrentLoadKN();
		const TArray<TWeakObjectPtr<ABuildingNodeActor>>& Parents = Node->GetSupportedByNodes();

		int32 ValidParentCount = 0;
		for (const auto& ParentPtr : Parents)
		{
			if (ParentPtr.IsValid() && !ParentPtr->IsCollapsed())
			{
				++ValidParentCount;
			}
		}

		if (ValidParentCount > 0)
		{
			const float LoadPerParent = TotalNodeWeight / ValidParentCount;
			for (const auto& ParentPtr : Parents)
			{
				if (ABuildingNodeActor* Parent = ParentPtr.Get())
				{
					if (!Parent->IsCollapsed())
					{
						Parent->SetCurrentLoadKN(Parent->GetCurrentLoadKN() + LoadPerParent);
					}
				}
			}
		}
	}

	// Check for compressive overload failures
	for (ABuildingNodeActor* Node : SortedNodes)
	{
		if (Node->GetCurrentLoadKN() > Node->GetCompressiveCapacityKN())
		{
			const FVector CollapseImpulse(
				FMath::RandRange(-100.f, 100.f),
				FMath::RandRange(-100.f, 100.f),
				-50.f
			);
			Node->TriggerChaosCollapse(CollapseImpulse);
		}
	}
}

void UStructuralIntegritySubsystem::TriggerCascadeFailure(ABuildingNodeActor* OriginNode, const FVector& InitialImpulse)
{
	if (!OriginNode)
	{
		return;
	}

	// Gather child nodes that were dependent on OriginNode
	TArray<ABuildingNodeActor*> NodesToCheck;
	for (const auto& ChildPtr : OriginNode->GetSupportingNodes())
	{
		if (ABuildingNodeActor* Child = ChildPtr.Get())
		{
			if (!Child->IsCollapsed())
			{
				NodesToCheck.Add(Child);
			}
		}
	}

	for (ABuildingNodeActor* ChildNode : NodesToCheck)
	{
		TSet<ABuildingNodeActor*> Visited;
		Visited.Add(OriginNode); // Exclude the fallen node

		if (!HasPathToGround(ChildNode, Visited))
		{
			// No remaining support path to ground -> trigger cascading collapse!
			const FVector ChildImpulse = (!InitialImpulse.IsNearlyZero())
				? InitialImpulse * 0.7f + FVector(FMath::RandRange(-150.f, 150.f), FMath::RandRange(-150.f, 150.f), FMath::RandRange(-50.f, 100.f))
				: FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 100.f);

			ChildNode->TriggerChaosCollapse(ChildImpulse);
		}
	}
}

bool UStructuralIntegritySubsystem::HasPathToGround(ABuildingNodeActor* Node, TSet<ABuildingNodeActor*>& Visited) const
{
	if (!Node || Node->IsCollapsed() || Visited.Contains(Node))
	{
		return false;
	}

	Visited.Add(Node);

	if (Node->IsGrounded())
	{
		return true;
	}

	for (const auto& ParentPtr : Node->GetSupportedByNodes())
	{
		if (ABuildingNodeActor* Parent = ParentPtr.Get())
		{
			if (!Parent->IsCollapsed() && !Visited.Contains(Parent))
			{
				if (HasPathToGround(Parent, Visited))
				{
					return true;
				}
			}
		}
	}

	return false;
}
