// Copyright Epic Games, Inc. All Rights Reserved.

#include "StructureLink.h"
#include "CarvableLog.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Wood/WoodStructureGeometry.h"

namespace
{
	/** Same tolerances the Wood structure subsystem uses. */
	constexpr float ContactToleranceCm = 8.f;
	constexpr float GroundToleranceCm = 15.f;

	/** The lowest point of the piece rests on the ground (not on another log). */
	bool IsGrounded(const UWorld* World, const AActor* Piece, const FVector& A, const FVector& B, float Radius)
	{
		const FVector Axis = (B - A).GetSafeNormal();
		const FVector Low = A.Z < B.Z ? A : B;
		// A lying log touches the ground with its side, a post with its end.
		const float Below = Radius * FMath::Sqrt(FMath::Max(0.f, 1.f - Axis.Z * Axis.Z));
		const FVector Bottom = Low - FVector(0.f, 0.f, Below);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(StructureGround), false, Piece);
		const bool bHitGround = World->LineTraceSingleByObjectType(Hit, Bottom + FVector(0.f, 0.f, 30.f), Bottom - FVector(0.f, 0.f, GroundToleranceCm),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		const AActor* HitActor = Hit.GetActor();
		return bHitGround && (!HitActor || (!HitActor->IsA<ACarvableLog>() && !HitActor->IsA<AStoneActor>()));
	}

	bool IsLogGrounded(const UWorld* World, const ACarvableLog* Log)
	{
		if (!World || !Log || Log->IsCarried())
		{
			return false;
		}
		const FTransform& Frame = Log->GetActorTransform();
		const FBox Box = Log->GetLocalBox();
		const FVector A = Frame.TransformPosition(FVector(Box.Min.X, 0.f, 0.f));
		const FVector B = Frame.TransformPosition(FVector(Box.Max.X, 0.f, 0.f));
		return IsGrounded(World, Log, A, B, Log->GetRadius());
	}

	TArray<ACarvableLog*> Installed(UWorld* World, const ACarvableLog* Except = nullptr)
	{
		TArray<ACarvableLog*> Logs;
		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			if (*It != Except && !It->IsCarried() && (It->IsAnchored() || IsLogGrounded(World, *It)))
			{
				Logs.Add(*It);
			}
		}
		return Logs;
	}

	/** Live structural pieces participating in the same geometry graph. Carried/loose stones are not structure nodes. */
	TArray<AActor*> InstalledStructuralParts(UWorld* World, const AActor* Except = nullptr)
	{
		TArray<AActor*> Parts;
		if (!World) return Parts;

		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			ACarvableLog* Log = *It;
			if (Log && Log != Except && !Log->IsCarried() && (Log->IsAnchored() || IsLogGrounded(World, Log)))
			{
				Parts.Add(Log);
			}
		}
		for (TActorIterator<AStoneActor> It(World); It; ++It)
		{
			AStoneActor* Stone = *It;
			if (Stone && Stone != Except && Stone->MeshComponent && Stone->MeshComponent->GetStaticMesh()
				&& Stone->GetStructuralState_Implementation() == EStructuralNodeState::Installed)
			{
				Parts.Add(Stone);
			}
		}
		Parts.Sort([](const AActor* A, const AActor* B)
		{
			if (!A) return B != nullptr;
			if (!B) return false;
			return A->GetPathName() < B->GetPathName();
		});
		return Parts;
	}

	WoodStructure::FNode MakeNodeForPart(AActor* Part, int32 Id)
	{
		if (ACarvableLog* Log = Cast<ACarvableLog>(Part))
		{
			return StructureLink::MakeNode(Log, Id);
		}
		if (AStoneActor* Stone = Cast<AStoneActor>(Part))
		{
			return StructureLink::MakeNode(Stone, Id);
		}
		WoodStructure::FNode Empty;
		Empty.Id = Id;
		return Empty;
	}


	/** Every connection between the nodes, each pair checked once. */
	TArray<WoodStructure::FEdge> Connect(const TArray<WoodStructure::FNode>& Nodes)
	{
		TArray<WoodStructure::FEdge> Edges;
		TArray<WoodStructure::FNode> Placed;
		for (const WoodStructure::FNode& Node : Nodes)
		{
			WoodStructure::FindConnections(Node, Placed, ContactToleranceCm, Edges);
			Placed.Add(Node);
		}
		return Edges;
	}
}

TSet<int32> StructureLink::Evaluate(TArray<WoodStructure::FNode>& Nodes, const TArray<WoodStructure::FEdge>& Edges, TMap<int32, float>* OutSnaps, FString* OutNotes)
{
	const float MaxGap = MaxSupportGapModules * ACarvableLog::ModuleLength;
	const float MaxOverhang = MaxOverhangModules * ACarvableLog::ModuleLength + 5.f;
	TMap<int32, const WoodStructure::FNode*> ById;
	for (int32 Pass = 0; Pass <= Nodes.Num(); ++Pass)
	{
		ById.Reset();
		for (const WoodStructure::FNode& Node : Nodes)
		{
			ById.Add(Node.Id, &Node);
		}
		const TSet<int32> Supported = WoodStructure::ComputeSupport(Nodes, Edges);
		bool bFailed = false;
		for (WoodStructure::FNode& Beam : Nodes)
		{
			if (Beam.Role != EWoodRole::Beam || Beam.bGrounded || !Supported.Contains(Beam.Id))
			{
				continue;
			}
			// Where along the beam (from its A end) what holds it up touches it.
			TArray<float> At;
			for (const WoodStructure::FEdge& Edge : Edges)
			{
				const int32 Other = Edge.A == Beam.Id ? Edge.B : (!Edge.bOneWay && Edge.B == Beam.Id ? Edge.A : INDEX_NONE);
				const WoodStructure::FNode* const* Holder = Other != INDEX_NONE ? ById.Find(Other) : nullptr;
				// A support whose joint has let go (burnt, broken) holds nothing, whatever the graph says about it.
				if (!Holder || !Supported.Contains(Other)
					|| (Edge.Kind != EWoodConnectionKind::Resting && FMath::Max(Beam.Damage, (*Holder)->Damage) >= WoodStructure::JointLooseDamage))
				{
					continue;
				}
				if (Edge.Kind == EWoodConnectionKind::Stacked)
				{
					// A bed: the beam lies on the holder over their whole common stretch.
					const FVector BeamAxis = (Beam.B - Beam.A).GetSafeNormal();
					const double H0 = FVector::DotProduct((*Holder)->A - Beam.A, BeamAxis);
					const double H1 = FVector::DotProduct((*Holder)->B - Beam.A, BeamAxis);
					const double BeamLength = Beam.Length();
					// A bed holds it all along: a support point every module over the common stretch, not just its two ends.
					const double Lo = FMath::Clamp(FMath::Min(H0, H1), 0.0, BeamLength);
					const double Hi = FMath::Clamp(FMath::Max(H0, H1), 0.0, BeamLength);
					for (double Point = Lo; Point < Hi; Point += ACarvableLog::ModuleLength)
					{
						At.Add(static_cast<float>(Point));
					}
					At.Add(static_cast<float>(Hi));
					continue;
				}
				double S = 0.0, T = 0.0;
				WoodStructure::Geo::ClosestSegSeg(Beam.A, Beam.B, (*Holder)->A, (*Holder)->B, S, T);
				At.Add(static_cast<float>(S * Beam.Length()));
			}
			if (At.Num() == 0)
			{
				continue;
			}
			At.Sort();
			if (OutNotes)
			{
				FString Positions;
				for (const float Position : At)
				{
					Positions += FString::Printf(TEXT(" %.0f"), Position);
				}
				*OutNotes += FString::Printf(TEXT("\n    kiriş #%d boy %.0f, destekler (A ucundan):%s"), Beam.Id, Beam.Length(), *Positions);
			}
			// The spans along the beam: before the first support, between supports, after the last.
			const float Length = static_cast<float>(Beam.Length());
			float BreakAt = -1.f;
			if (At[0] > MaxOverhang)
			{
				BreakAt = At[0] * 0.5f;
			}
			if (Length - At.Last() > MaxOverhang)
			{
				BreakAt = (At.Last() + Length) * 0.5f;
			}
			for (int32 Index = 1; Index < At.Num(); ++Index)
			{
				if (At[Index] - At[Index - 1] > MaxGap)
				{
					BreakAt = (At[Index] + At[Index - 1]) * 0.5f;
				}
			}
			if (BreakAt >= 0.f && !bPlacementGrace)
			{
				Beam.Damage = 1.f;
				bFailed = true;
				if (OutSnaps)
				{
					OutSnaps->Add(Beam.Id, BreakAt);
				}
			}
		}
		if (!bFailed)
		{
			return Supported;
		}
	}
	return WoodStructure::ComputeSupport(Nodes, Edges);
}

WoodStructure::FNode StructureLink::MakeNode(const ACarvableLog* Log, int32 Id, const FTransform* Transform)
{
	const FTransform& Frame = Transform ? *Transform : Log->GetActorTransform();
	const FBox Box = Log->GetLocalBox();
	WoodStructure::FNode Node;
	Node.Id = Id;
	// A is the end the module grid is counted from, so notch lines read the same as on the log.
	Node.A = Frame.TransformPosition(FVector(Box.Min.X, 0.f, 0.f));
	Node.B = Frame.TransformPosition(FVector(Box.Max.X, 0.f, 0.f));
	Node.RadiusCm = Log->GetRadius();
	Node.LengthModules = FMath::Clamp(FMath::RoundToInt(Log->GetLength() / ACarvableLog::ModuleLength), 1, 8);
	for (const FVector2f& Span : Log->GetCutSpans())
	{
		FWoodNotch Notch;
		Notch.ModuleLine = FMath::RoundToInt((0.5f * (Span.X + Span.Y) - Box.Min.X) / ACarvableLog::ModuleLength);
		Notch.JointType = EWoodJointType::Saddle;
		Node.Notches.Add(Notch);
	}
	FWoodPieceData Data;
	Data.CrossSection = Log->IsSplit() ? EWoodCrossSection::Half : EWoodCrossSection::Round;
	Data.EndA = Log->GetEnd(false) == ELogEnd::Pointed ? EWoodEndShape::Pointed : EWoodEndShape::Flat;
	Data.EndB = Log->GetEnd(true) == ELogEnd::Pointed ? EWoodEndShape::Pointed : EWoodEndShape::Flat;
	Node.Role = WoodStructure::DeriveRole(Data, WoodStructure::ClassifyOrientation(Node.B - Node.A));
	Node.bGrounded = IsGrounded(Log->GetWorld(), Log, Node.A, Node.B, Node.RadiusCm);
	Node.Damage = Log->GetDamage();
	Node.bTemporary = Log->IsTemporary();
	return Node;
}


WoodStructure::FNode StructureLink::MakeNode(const AStoneActor* Stone, int32 Id)
{
	WoodStructure::FNode Node;
	Node.Id = Id;
	if (!Stone || !Stone->MeshComponent || !Stone->MeshComponent->GetStaticMesh())
	{
		return Node;
	}

	const UStaticMeshComponent* Mesh = Stone->MeshComponent;
	const FBox LocalBox = Mesh->GetStaticMesh()->GetBoundingBox();
	const FVector LocalSize = LocalBox.GetSize();
	const FVector Scale = Mesh->GetComponentScale();
	const FVector AbsScale(FMath::Abs(Scale.X), FMath::Abs(Scale.Y), FMath::Abs(Scale.Z));
	const FVector Size = LocalSize * AbsScale;
	const FTransform Frame = Mesh->GetComponentTransform();
	const FVector Center = Frame.TransformPosition(LocalBox.GetCenter());
	const UStoneDefinition* Definition = Stone->GetDefinition();
	const EStoneRole StoneRole = Definition ? Definition->Role : EStoneRole::None;

	int32 AxisIndex = 0;
	if (StoneRole == EStoneRole::Pillar)
	{
		// A pillar follows its vertical centreline so its top can support beams or posts.
		AxisIndex = 2;
	}
	else if (StoneRole == EStoneRole::Foundation || StoneRole == EStoneRole::Floor
		|| StoneRole == EStoneRole::Path || StoneRole == EStoneRole::Wall)
	{
		// Foundations and masonry surfaces use their longest horizontal dimension as the contact axis.
		AxisIndex = Size.X >= Size.Y ? 0 : 1;
	}
	else
	{
		AxisIndex = Size.X >= Size.Y && Size.X >= Size.Z ? 0 : (Size.Y >= Size.Z ? 1 : 2);
	}

	const FVector LocalAxis = AxisIndex == 0 ? FVector::XAxisVector
		: (AxisIndex == 1 ? FVector::YAxisVector : FVector::ZAxisVector);
	const FVector Axis = Frame.TransformVectorNoScale(LocalAxis).GetSafeNormal();
	const float HalfLength = 0.5f * Size[AxisIndex];
	const float Radius = 0.5f * FMath::Max(
		Size[AxisIndex == 0 ? 1 : 0],
		Size[AxisIndex == 2 ? 1 : 2]);
	Node.A = Center - Axis * HalfLength;
	Node.B = Center + Axis * HalfLength;
	Node.RadiusCm = FMath::Max(1.f, Radius);
	Node.LengthModules = FMath::Clamp(FMath::RoundToInt((HalfLength * 2.f) / ACarvableLog::ModuleLength), 1, 8);
	Node.Damage = Stone->GetMaxHealth() > KINDA_SMALL_NUMBER
		? FMath::Clamp(1.f - Stone->GetHealth() / Stone->GetMaxHealth(), 0.f, 1.f) : 1.f;
	Node.bGrounded = IsGrounded(Stone->GetWorld(), Stone, Node.A, Node.B, Node.RadiusCm);
	Node.bTemporary = false;

	switch (StoneRole)
	{
	case EStoneRole::Pillar:
		Node.Role = EWoodRole::Post;
		break;
	case EStoneRole::Foundation:
		Node.Role = EWoodRole::Floor;
		break;
	case EStoneRole::Beam:
	case EStoneRole::Arch:
		Node.Role = EWoodRole::Beam;
		break;
	case EStoneRole::Wall:
		Node.Role = EWoodRole::Wall;
		break;
	case EStoneRole::Floor:
	case EStoneRole::Path:
	case EStoneRole::Stair:
		Node.Role = EWoodRole::Floor;
		break;
	case EStoneRole::None:
	case EStoneRole::Fireplace:
	case EStoneRole::Trap:
	default:
		Node.Role = EWoodRole::Unassigned;
		break;
	}
	return Node;
}

bool StructureLink::bLogFalls = false;
bool StructureLink::bPlacementGrace = false;

namespace
{
	int32 StructureVersion = 0;
	int32 CachedVersion = -1;
	TWeakObjectPtr<UWorld> CachedWorld;
	TArray<StructureLink::FFeatureInfo> CachedFeatures;
}

void StructureLink::MarkChanged()
{
	++StructureVersion;
}

int32 StructureLink::Version()
{
	return StructureVersion;
}

FString StructureLink::NameOf(EWoodFeatureKind Kind)
{
	switch (Kind)
	{
	case EWoodFeatureKind::HorizontalWall: return TEXT("Yatay kütük duvarı");
	case EWoodFeatureKind::VerticalWall:   return TEXT("Dikey kütük duvarı");
	case EWoodFeatureKind::StakeWall:      return TEXT("Sivri kazık duvarı");
	case EWoodFeatureKind::DefenseWall:    return TEXT("Savunma duvarı");
	case EWoodFeatureKind::Opening:        return TEXT("Geçit");
	case EWoodFeatureKind::DoorFrame:      return TEXT("Kapı kasası");
	case EWoodFeatureKind::WindowFrame:    return TEXT("Pencere kasası");
	case EWoodFeatureKind::Platform:       return TEXT("Platform");
	case EWoodFeatureKind::RoofFrame:      return TEXT("Çatı iskeleti");
	case EWoodFeatureKind::RoofSurface:    return TEXT("Çatı kaplaması");
	case EWoodFeatureKind::Ramp:           return TEXT("Rampa");
	case EWoodFeatureKind::Stairs:         return TEXT("Merdiven");
	case EWoodFeatureKind::Bridge:         return TEXT("Köprü");
	case EWoodFeatureKind::Railing:        return TEXT("Korkuluk");
	case EWoodFeatureKind::Scaffold:       return TEXT("İskele");
	case EWoodFeatureKind::Table:          return TEXT("Tezgâh");
	default:                               return TEXT("");
	}
}

const TArray<StructureLink::FFeatureInfo>& StructureLink::Features(UWorld* World)
{
	if (!World)
	{
		return CachedFeatures;
	}
	if (CachedVersion == StructureVersion && CachedWorld.Get() == World)
	{
		return CachedFeatures;
	}
	CachedVersion = StructureVersion;
	CachedWorld = World;
	CachedFeatures.Reset();
	const TArray<AActor*> Parts = InstalledStructuralParts(World);
	TArray<WoodStructure::FNode> Nodes;
	Nodes.Reserve(Parts.Num());
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		Nodes.Add(MakeNodeForPart(Parts[Index], Index));
	}
	for (const WoodStructure::FFeature& Found : WoodStructure::AnalyzeStructure(Nodes, Connect(Nodes), ContactToleranceCm))
	{
		FFeatureInfo& Info = CachedFeatures.AddDefaulted_GetRef();
		Info.Kind = Found.Kind;
		Info.SpanCm = static_cast<float>(Found.SpanCm);
		Info.Center = Found.Center;
		for (const int32 Id : Found.NodeIds)
		{
			if (!Parts.IsValidIndex(Id) || !IsValid(Parts[Id]))
			{
				continue;
			}
			Info.StructuralPieces.Add(Parts[Id]);
			if (ACarvableLog* Log = Cast<ACarvableLog>(Parts[Id]))
			{
				Info.Pieces.Add(Log);
			}
		}
	}
	return CachedFeatures;
}

FString StructureLink::DescribeFeaturesOf(UWorld* World, const ACarvableLog* Log)
{
	return DescribeFeaturesOfActor(World, Log);
}

FString StructureLink::DescribeFeaturesOfActor(UWorld* World, const AActor* Part)
{
	if (!World || !IsValid(Part))
	{
		return FString();
	}
	FString Text;
	for (const FFeatureInfo& Info : Features(World))
	{
		for (const TWeakObjectPtr<AActor>& Member : Info.StructuralPieces)
		{
			if (Member.Get() == Part)
			{
				const FString KindName = NameOf(Info.Kind);
				if (!KindName.IsEmpty())
				{
					Text += (Text.IsEmpty() ? TEXT("") : TEXT(", ")) + KindName;
				}
				break;
			}
		}
	}
	return Text;
}

FString StructureLink::Describe(UWorld* World)
{
	const TArray<AActor*> Parts = InstalledStructuralParts(World);
	TArray<WoodStructure::FNode> Nodes;
	Nodes.Reserve(Parts.Num());
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		Nodes.Add(MakeNodeForPart(Parts[Index], Index));
	}
	const TArray<WoodStructure::FEdge> Edges = Connect(Nodes);
	TMap<int32, float> Snaps;
	FString Text;
	const TSet<int32> Supported = Evaluate(Nodes, Edges, &Snaps, &Text);
	for (const WoodStructure::FNode& Node : Nodes)
	{
		const AActor* Part = Parts.IsValidIndex(Node.Id) ? Parts[Node.Id] : nullptr;
		if (!IsValid(Part))
		{
			continue;
		}
		Text += FString::Printf(TEXT("\n    #%d %s (%.0f,%.0f,%.0f) role %d grounded %d notches %d %s"),
			Node.Id, *Part->GetName(), Part->GetActorLocation().X, Part->GetActorLocation().Y,
			Part->GetActorLocation().Z, static_cast<int32>(Node.Role), Node.bGrounded ? 1 : 0,
			Node.Notches.Num(), Supported.Contains(Node.Id) ? TEXT("supported") : TEXT("UNSUPPORTED"));
	}
	for (const WoodStructure::FEdge& Edge : Edges)
	{
		Text += FString::Printf(TEXT("\n    edge #%d -> #%d kind %d %s"), Edge.A, Edge.B,
			static_cast<int32>(Edge.Kind), Edge.bOneWay ? TEXT("one-way") : TEXT("two-way"));
	}
	return Text;
}

int32 StructureLink::Settle(UWorld* World)
{
	if (!World)
	{
		return 0;
	}
	MarkChanged();
	int32 Fallen = 0;
	// Rebuild the mixed wood/stone graph after each failure: falling pieces are removed from the next installed pass.
	for (int32 Pass = 0; Pass < 16; ++Pass)
	{
		const TArray<AActor*> Parts = InstalledStructuralParts(World);
		TArray<WoodStructure::FNode> Nodes;
		Nodes.Reserve(Parts.Num());
		for (int32 Index = 0; Index < Parts.Num(); ++Index)
		{
			Nodes.Add(MakeNodeForPart(Parts[Index], Index));
		}

		TMap<int32, float> Snaps;
		const TSet<int32> Supported = StructureLink::Evaluate(Nodes, Connect(Nodes), &Snaps);
		int32 FellNow = 0;
		for (int32 Index = 0; Index < Parts.Num(); ++Index)
		{
			if (Supported.Contains(Index))
			{
				continue;
			}

			AActor* Part = Parts[Index];
			if (!IsValid(Part))
			{
				continue;
			}
			if (bLogFalls)
			{
				UE_LOG(LogTemp, Display, TEXT("[Structure] %s %s%s"), *Part->GetName(),
					Snaps.Contains(Index) ? TEXT("failed span / overhang limit") : TEXT("lost support"),
					*StructureLink::Describe(World));
			}

			if (ACarvableLog* Log = Cast<ACarvableLog>(Part))
			{
				if (const float* Where = Snaps.Find(Index))
				{
					Log->SnapAt(Log->GetMinX() + *Where);
				}
				else
				{
					Log->LetFall();
				}
			}
			else if (AStoneActor* Stone = Cast<AStoneActor>(Part))
			{
				// Stone blocks do not auto-snap into two pieces; an unsupported one returns to rigid-body physics.
				Stone->SetStructuralState_Implementation(EStructuralNodeState::Falling);
			}
			++FellNow;
		}

		Fallen += FellNow;
		if (FellNow == 0)
		{
			break;
		}
	}
	return Fallen;
}

bool StructureLink::WouldBeSupported(UWorld* World, const ACarvableLog* Piece, const FTransform& Transform)
{
	if (!World || !Piece)
	{
		return false;
	}
	const TArray<AActor*> Parts = InstalledStructuralParts(World, Piece);
	TArray<WoodStructure::FNode> Nodes;
	Nodes.Reserve(Parts.Num() + 1);
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		Nodes.Add(MakeNodeForPart(Parts[Index], Index));
	}
	const int32 PieceId = Nodes.Num();
	Nodes.Add(MakeNode(Piece, PieceId, &Transform));
	return StructureLink::Evaluate(Nodes, Connect(Nodes)).Contains(PieceId);
}
