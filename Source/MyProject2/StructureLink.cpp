// Copyright Epic Games, Inc. All Rights Reserved.

#include "StructureLink.h"
#include "CarvableLog.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Wood/WoodStructureGeometry.h"

namespace
{
	/** Same tolerances the Wood structure subsystem uses. */
	constexpr float ContactToleranceCm = 8.f;
	constexpr float GroundToleranceCm = 15.f;

	/** The lowest point of the piece rests on the ground (not on another log). */
	bool IsGrounded(const UWorld* World, const ACarvableLog* Log, const FVector& A, const FVector& B, float Radius)
	{
		const FVector Axis = (B - A).GetSafeNormal();
		const FVector Low = A.Z < B.Z ? A : B;
		// A lying log touches the ground with its side, a post with its end.
		const float Below = Radius * FMath::Sqrt(FMath::Max(0.f, 1.f - Axis.Z * Axis.Z));
		const FVector Bottom = Low - FVector(0.f, 0.f, Below);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(StructureGround), false, Log);
		return World->LineTraceSingleByObjectType(Hit, Bottom + FVector(0.f, 0.f, 30.f), Bottom - FVector(0.f, 0.f, GroundToleranceCm),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params) && !Hit.GetActor()->IsA<ACarvableLog>();
	}

	TArray<ACarvableLog*> Installed(UWorld* World, const ACarvableLog* Except = nullptr)
	{
		TArray<ACarvableLog*> Logs;
		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			if (*It != Except && It->IsAnchored() && !It->IsCarried())
			{
				Logs.Add(*It);
			}
		}
		return Logs;
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
			if (BreakAt >= 0.f)
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
	Node.LengthModules = FMath::Clamp(FMath::RoundToInt(Log->GetLength() / ACarvableLog::ModuleLength), 1, 6);
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

bool StructureLink::bLogFalls = false;

namespace
{
	int32 StructureVersion = 0;
	int32 CachedVersion = -1;
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
	if (CachedVersion == StructureVersion || !World)
	{
		return CachedFeatures;
	}
	CachedVersion = StructureVersion;
	CachedFeatures.Reset();
	const TArray<ACarvableLog*> Logs = Installed(World);
	TArray<WoodStructure::FNode> Nodes;
	for (int32 Index = 0; Index < Logs.Num(); ++Index)
	{
		Nodes.Add(MakeNode(Logs[Index], Index));
	}
	for (const WoodStructure::FFeature& Found : WoodStructure::AnalyzeStructure(Nodes, Connect(Nodes), ContactToleranceCm))
	{
		FFeatureInfo& Info = CachedFeatures.AddDefaulted_GetRef();
		Info.Kind = Found.Kind;
		Info.SpanCm = static_cast<float>(Found.SpanCm);
		Info.Center = Found.Center;
		for (const int32 Id : Found.NodeIds)
		{
			Info.Pieces.Add(Logs[Id]);
		}
	}
	return CachedFeatures;
}

FString StructureLink::DescribeFeaturesOf(UWorld* World, const ACarvableLog* Log)
{
	FString Text;
	for (const FFeatureInfo& Info : Features(World))
	{
		for (const TWeakObjectPtr<ACarvableLog>& Piece : Info.Pieces)
		{
			if (Piece.Get() == Log)
			{
				Text += (Text.IsEmpty() ? TEXT("") : TEXT(", ")) + NameOf(Info.Kind);
				break;
			}
		}
	}
	return Text;
}

FString StructureLink::Describe(UWorld* World)
{
	const TArray<ACarvableLog*> Logs = Installed(World);
	TArray<WoodStructure::FNode> Nodes;
	for (int32 Index = 0; Index < Logs.Num(); ++Index)
	{
		Nodes.Add(MakeNode(Logs[Index], Index));
	}
	const TArray<WoodStructure::FEdge> Edges = Connect(Nodes);
	TMap<int32, float> Snaps;
	FString Text;
	const TSet<int32> Supported = Evaluate(Nodes, Edges, &Snaps, &Text);
	for (const WoodStructure::FNode& Node : Nodes)
	{
		Text += FString::Printf(TEXT("\n    #%d %s (%.0f,%.0f,%.0f) rol %d zemin %d notches %d %s"), Node.Id, *Logs[Node.Id]->GetName(),
			Logs[Node.Id]->GetActorLocation().X, Logs[Node.Id]->GetActorLocation().Y, Logs[Node.Id]->GetActorLocation().Z,
			static_cast<int32>(Node.Role), Node.bGrounded ? 1 : 0, Node.Notches.Num(), Supported.Contains(Node.Id) ? TEXT("destekli") : TEXT("DESTEKSİZ"));
	}
	for (const WoodStructure::FEdge& Edge : Edges)
	{
		Text += FString::Printf(TEXT("\n    bağ #%d -> #%d tür %d %s"), Edge.A, Edge.B, static_cast<int32>(Edge.Kind), Edge.bOneWay ? TEXT("tek yön") : TEXT("çift yön"));
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
	// A piece that falls can take others with it: repeat until everything left standing is held up.
	for (int32 Pass = 0; Pass < 16; ++Pass)
	{
		const TArray<ACarvableLog*> Logs = Installed(World);
		TArray<WoodStructure::FNode> Nodes;
		for (int32 Index = 0; Index < Logs.Num(); ++Index)
		{
			Nodes.Add(MakeNode(Logs[Index], Index));
		}
		TMap<int32, float> Snaps;
		const TSet<int32> Supported = StructureLink::Evaluate(Nodes, Connect(Nodes), &Snaps);
		int32 FellNow = 0;
		for (int32 Index = 0; Index < Logs.Num(); ++Index)
		{
			if (Supported.Contains(Index))
			{
				continue;
			}
			if (bLogFalls)
			{
				UE_LOG(LogTemp, Display, TEXT("[Structure] %s %s%s"), *Logs[Index]->GetName(), Snaps.Contains(Index) ? TEXT("kırıldı: açıklık ya da taşma sınırı aşıldı") : TEXT("desteksiz kaldı"),
					*StructureLink::Describe(World));
			}
			// A beam that failed on its span snaps; anything else just loses its hold.
			if (const float* Where = Snaps.Find(Index))
			{
				Logs[Index]->SnapAt(Logs[Index]->GetMinX() + *Where);
			}
			else
			{
				Logs[Index]->LetFall();
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
	const TArray<ACarvableLog*> Logs = Installed(World, Piece);
	TArray<WoodStructure::FNode> Nodes;
	for (int32 Index = 0; Index < Logs.Num(); ++Index)
	{
		Nodes.Add(MakeNode(Logs[Index], Index));
	}
	const int32 PieceId = Nodes.Num();
	Nodes.Add(MakeNode(Piece, PieceId, &Transform));
	return StructureLink::Evaluate(Nodes, Connect(Nodes)).Contains(PieceId);
}
