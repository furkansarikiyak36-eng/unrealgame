#include "WoodStructureSubsystem.h"
#include "WoodPieceComponent.h"
#include "WoodCarrierComponent.h"
#include "WoodCutterComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UWoodStructureSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UWoodStructureSubsystem::Deinitialize()
{
	for (const TPair<int32, FRecord>& Pair : Records)
	{
		if (UWoodPieceComponent* P = Pair.Value.Piece.Get())
		{
			P->OnStateChanged.RemoveDynamic(this, &UWoodStructureSubsystem::HandlePieceStateChanged);
		}
	}
	Records.Reset();
	IdByPiece.Reset();
	Edges.Reset();
	Supported.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------- yardımcılar

FVector UWoodStructureSubsystem::GetLocalLengthAxis(const UWoodPieceComponent* Piece) const
{
	if (const AActor* Owner = Piece ? Piece->GetOwner() : nullptr)
	{
		if (const UWoodCutterComponent* Cutter = Owner->FindComponentByClass<UWoodCutterComponent>())
		{
			return Cutter->LocalLengthAxis.GetSafeNormal();
		}
	}
	return FVector(1.0, 0.0, 0.0);
}

WoodStructure::FNode UWoodStructureSubsystem::MakeNode(const UWoodPieceComponent* Piece, const FTransform& Xf, int32 Id) const
{
	WoodStructure::FNode N;
	N.Id = Id;

	const FWoodPieceData& D = Piece->PieceData;
	const FVector Axis = Xf.GetRotation().RotateVector(GetLocalLengthAxis(Piece)).GetSafeNormal();
	const double HalfLen = static_cast<double>(D.GetLengthCm()) * 0.5;

	N.A = Xf.GetLocation() - Axis * HalfLen;
	N.B = Xf.GetLocation() + Axis * HalfLen;
	N.RadiusCm = 0.5f * FMath::Max(D.GetLateralCm(), D.GetVerticalCm());
	N.LengthModules = D.LengthModules;
	N.Notches = D.Notches;
	N.Role = WoodStructure::DeriveRole(D, WoodStructure::ClassifyOrientation(Axis, OrientationToleranceDeg));
	N.Damage = D.DamageFraction;
	N.bTemporary = Piece->bTemporary;
	return N;
}

TArray<WoodStructure::FNode> UWoodStructureSubsystem::CollectNodes(int32 ExcludeId) const
{
	TArray<WoodStructure::FNode> Out;
	Out.Reserve(Records.Num());
	for (const TPair<int32, FRecord>& Pair : Records)
	{
		// Yok olmuş parçanın düğümü destek vermez; PruneInvalid bir sonraki kurulum/söküm sırasında temizler.
		if (Pair.Key != ExcludeId && Pair.Value.Piece.IsValid())
		{
			Out.Add(Pair.Value.Node);
		}
	}
	return Out;
}

bool UWoodStructureSubsystem::TraceGround(const WoodStructure::FNode& Node, const AActor* IgnoreOwner, float& OutGapCm) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Axis = Node.Axis();
	const FVector& Low = (Node.A.Z <= Node.B.Z) ? Node.A : Node.B;
	const float Z = static_cast<float>(Axis.Z);
	const float Horizontality = FMath::Sqrt(FMath::Max(0.f, 1.f - Z * Z));
	const float LowZ = static_cast<float>(Low.Z) - Node.RadiusCm * Horizontality;

	const FVector Start(Low.X, Low.Y, Low.Z + GroundTraceUpCm);
	const FVector End(Low.X, Low.Y, LowZ - GroundToleranceCm);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WoodGroundTrace), false, IgnoreOwner);
	for (const TPair<int32, FRecord>& Pair : Records)
	{
		if (const UWoodPieceComponent* P = Pair.Value.Piece.Get())
		{
			Params.AddIgnoredActor(P->GetOwner()); // diğer kütükler zemin sayılmaz
		}
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		return false;
	}

	OutGapCm = LowZ - static_cast<float>(Hit.ImpactPoint.Z);
	return true;
}

void UWoodStructureSubsystem::Reject(const FString& Reason)
{
	OnRejected.Broadcast(Reason);
}

void UWoodStructureSubsystem::PruneInvalid()
{
	TArray<int32> Dead;
	for (const TPair<int32, FRecord>& Pair : Records)
	{
		if (!Pair.Value.Piece.IsValid())
		{
			Dead.Add(Pair.Key);
		}
	}
	if (Dead.Num() == 0)
	{
		return;
	}

	for (int32 Id : Dead)
	{
		if (const FRecord* R = Records.Find(Id))
		{
			IdByPiece.Remove(R->Key);
		}
		Records.Remove(Id);
		Edges.RemoveAll([Id](const WoodStructure::FEdge& E) { return E.A == Id || E.B == Id; });
	}

	TArray<UWoodPieceComponent*> Lost;
	RecomputeSupport(&Lost);
	if (Lost.Num() > 0)
	{
		OnSupportLost.Broadcast(Lost);
		if (bCollapseUnsupported)
		{
			CollapsePieces(Lost);
		}
	}
	OnStructureChanged.Broadcast();
}

void UWoodStructureSubsystem::RecomputeSupport(TArray<UWoodPieceComponent*>* OutLost)
{
	const TSet<int32> Before = Supported;
	Supported = WoodStructure::ComputeSupport(CollectNodes(), Edges);

	if (!OutLost)
	{
		return;
	}
	for (int32 Id : Before)
	{
		if (Supported.Contains(Id))
		{
			continue;
		}
		if (const FRecord* R = Records.Find(Id))
		{
			if (UWoodPieceComponent* P = R->Piece.Get())
			{
				OutLost->Add(P);
			}
		}
	}
}

bool UWoodStructureSubsystem::UnregisterPiece(UWoodPieceComponent* Piece, TArray<UWoodPieceComponent*>* OutLost)
{
	const int32* IdPtr = Piece ? IdByPiece.Find(Piece) : nullptr;
	if (!IdPtr)
	{
		return false;
	}
	const int32 Id = *IdPtr;

	Piece->OnStateChanged.RemoveDynamic(this, &UWoodStructureSubsystem::HandlePieceStateChanged);
	Records.Remove(Id);
	IdByPiece.Remove(Piece);
	Edges.RemoveAll([Id](const WoodStructure::FEdge& E) { return E.A == Id || E.B == Id; });
	RecomputeSupport(OutLost);
	return true;
}

void UWoodStructureSubsystem::CollapsePieces(const TArray<UWoodPieceComponent*>& Pieces)
{
	for (UWoodPieceComponent* P : Pieces)
	{
		if (P && UnregisterPiece(P, nullptr))
		{
			P->Uninstall(); // Loose + fizik: düşer
		}
	}
}

void UWoodStructureSubsystem::HandlePieceStateChanged(UWoodPieceComponent* Piece, EWoodState OldState, EWoodState NewState)
{
	// Faz 1'in Uninstall()'ı doğrudan çağrılırsa grafik tutarlı kalsın.
	if (NewState == EWoodState::Installed)
	{
		return;
	}

	TArray<UWoodPieceComponent*> Lost;
	if (UnregisterPiece(Piece, &Lost))
	{
		if (Lost.Num() > 0)
		{
			OnSupportLost.Broadcast(Lost);
			if (bCollapseUnsupported)
			{
				CollapsePieces(Lost);
			}
		}
		OnStructureChanged.Broadcast();
	}
}

// ---------------------------------------------------------------- snap (12, 53)

FWoodSnapResult UWoodStructureSubsystem::FindSnap(UWoodPieceComponent* Piece, const FTransform& Proposed)
{
	FWoodSnapResult R;
	R.SnappedTransform = Proposed;
	if (!Piece)
	{
		return R;
	}

	// Not: hayalet önizleme her karede çağırır; bu yüzden burada PruneInvalid (olay yayar, parça düşürebilir) çağrılmaz.
	const int32* SelfId = IdByPiece.Find(Piece);
	WoodStructure::FNode Geom = MakeNode(Piece, Proposed, SelfId ? *SelfId : INDEX_NONE);
	const TArray<WoodStructure::FNode> Others = CollectNodes(Geom.Id);

	const WoodStructure::FSnap S = WoodStructure::FindBestSnap(Geom, Others, SnapRadiusCm, OrientationToleranceDeg);
	if (S.bFound)
	{
		R.bSnapped = true;
		R.SuggestedJoint = S.SuggestedJoint;
		R.SnappedTransform.AddToTranslation(S.Offset);
		Geom.A += S.Offset;
		Geom.B += S.Offset;
		if (const FRecord* Rec = Records.Find(S.TargetId))
		{
			R.Target = Rec->Piece.Get();
		}
	}

	R.Orientation = WoodStructure::ClassifyOrientation(Geom.Axis(), OrientationToleranceDeg);
	R.DerivedRole = WoodStructure::DeriveRole(Piece->PieceData, R.Orientation);

	float Gap = 0.f;
	if (TraceGround(Geom, Piece->GetOwner(), Gap))
	{
		R.bGrounded = true;
		if (!R.bSnapped)
		{
			// Parça başka bir parçaya snap olmadıysa zemine otursun.
			R.SnappedTransform.AddToTranslation(FVector(0.0, 0.0, -Gap));
		}
	}

	R.bPlaceable = R.bSnapped || R.bGrounded;
	return R;
}

// ---------------------------------------------------------------- kurulum (33-38, 54)

bool UWoodStructureSubsystem::InstallPiece(UWoodPieceComponent* Piece, const FWoodSnapResult& Snap, AActor* Structure)
{
	PruneInvalid();

	UPrimitiveComponent* Prim = Piece ? Piece->GetPrimitive() : nullptr;
	if (!Piece || !Prim)
	{
		Reject(TEXT("Kurulacak parça geçersiz."));
		return false;
	}
	if (IdByPiece.Contains(Piece))
	{
		Reject(TEXT("Parça zaten bu yapıda."));
		return false;
	}
	if (!Snap.bPlaceable)
	{
		Reject(TEXT("Geçerli bir bağlantı noktası veya zemin yok."));
		return false;
	}

	Prim->SetWorldTransform(Snap.SnappedTransform, false, nullptr, ETeleportType::TeleportPhysics);

	USceneComponent* AttachTo = Structure ? Structure->GetRootComponent() : nullptr;
	if (!Piece->Install(Structure, AttachTo))
	{
		Reject(TEXT("Parça kurulamadı."));
		return false;
	}

	const int32 Id = NextId++;
	FRecord Rec;
	Rec.Node = MakeNode(Piece, Prim->GetComponentTransform(), Id);
	Rec.Piece = Piece;
	Rec.Key = Piece;

	float Gap = 0.f;
	Rec.Node.bGrounded = TraceGround(Rec.Node, Piece->GetOwner(), Gap);

	WoodStructure::FindConnections(Rec.Node, CollectNodes(), ContactToleranceCm, Edges, OrientationToleranceDeg);

	Piece->Role = Rec.Node.Role;
	Records.Add(Id, Rec);
	IdByPiece.Add(Piece, Id);
	Piece->OnStateChanged.AddUniqueDynamic(this, &UWoodStructureSubsystem::HandlePieceStateChanged);

	RecomputeSupport(nullptr);

	if (bRequireSupportOnPlace && !Supported.Contains(Id))
	{
		UnregisterPiece(Piece, nullptr);
		Piece->Uninstall();
		Reject(TEXT("Parça yeterli destek bulamadı."));
		OnStructureChanged.Broadcast();
		return false;
	}

	OnStructureChanged.Broadcast();
	return true;
}

bool UWoodStructureSubsystem::PlaceCarried(UWoodCarrierComponent* Carrier, UWoodPieceComponent* Piece, const FTransform& Proposed, AActor* Structure)
{
	if (!Carrier || !Piece)
	{
		Reject(TEXT("Taşıyıcı veya parça geçersiz."));
		return false;
	}

	const FWoodSnapResult Snap = FindSnap(Piece, Proposed);
	if (!Snap.bPlaceable)
	{
		Reject(TEXT("Burası geçerli bir yerleştirme noktası değil."));
		return false;
	}

	if (Piece->GetState() == EWoodState::Carried && !Carrier->DropPiece(Piece))
	{
		Reject(TEXT("Parça bu taşıyıcıda değil."));
		return false;
	}

	return InstallPiece(Piece, Snap, Structure);
}

// ---------------------------------------------------------------- söküm (55-58)

bool UWoodStructureSubsystem::RemovePiece(UWoodPieceComponent* Piece, bool bForce)
{
	PruneInvalid();

	const int32* IdPtr = Piece ? IdByPiece.Find(Piece) : nullptr;
	if (!IdPtr)
	{
		Reject(TEXT("Parça bu yapıda değil."));
		return false;
	}
	const int32 Id = *IdPtr;

	const TArray<int32> Dependents = WoodStructure::FindDependentsOfRemoval(CollectNodes(), Edges, Id);
	if (Dependents.Num() > 0 && !bForce)
	{
		Reject(FString::Printf(TEXT("Bu parça %d parçayı taşıyor; önce destek ekle."), Dependents.Num()));
		return false;
	}

	TArray<UWoodPieceComponent*> LostPieces;
	for (int32 DepId : Dependents)
	{
		if (const FRecord* R = Records.Find(DepId))
		{
			if (UWoodPieceComponent* P = R->Piece.Get())
			{
				LostPieces.Add(P);
			}
		}
	}

	UnregisterPiece(Piece, nullptr);
	Piece->Uninstall(); // Faz 1: serbest, taşınabilir kütüğe dönüşür (mekanik 56, 64)

	if (LostPieces.Num() > 0)
	{
		OnSupportLost.Broadcast(LostPieces);
		if (bCollapseUnsupported)
		{
			CollapsePieces(LostPieces);
		}
	}

	OnStructureChanged.Broadcast();
	return true;
}

TArray<UWoodPieceComponent*> UWoodStructureSubsystem::PreviewRemoval(const UWoodPieceComponent* Piece) const
{
	TArray<UWoodPieceComponent*> Out;
	const int32* IdPtr = Piece ? IdByPiece.Find(Piece) : nullptr;
	if (!IdPtr)
	{
		return Out;
	}

	const TArray<int32> Dependents = WoodStructure::FindDependentsOfRemoval(CollectNodes(), Edges, *IdPtr);
	for (int32 DepId : Dependents)
	{
		if (const FRecord* R = Records.Find(DepId))
		{
			if (UWoodPieceComponent* P = R->Piece.Get())
			{
				Out.Add(P);
			}
		}
	}
	return Out;
}

// ---------------------------------------------------------------- sorgular

bool UWoodStructureSubsystem::IsPieceSupported(const UWoodPieceComponent* Piece) const
{
	const int32* IdPtr = Piece ? IdByPiece.Find(Piece) : nullptr;
	return IdPtr && Supported.Contains(*IdPtr);
}

bool UWoodStructureSubsystem::IsPieceInstalledHere(const UWoodPieceComponent* Piece) const
{
	return Piece && IdByPiece.Contains(Piece);
}

TArray<UWoodPieceComponent*> UWoodStructureSubsystem::GetConnectedPieces(const UWoodPieceComponent* Piece) const
{
	TArray<UWoodPieceComponent*> Out;
	const int32* IdPtr = Piece ? IdByPiece.Find(Piece) : nullptr;
	if (!IdPtr)
	{
		return Out;
	}

	const int32 Id = *IdPtr;
	for (const WoodStructure::FEdge& E : Edges)
	{
		int32 OtherId = INDEX_NONE;
		if (E.A == Id)      { OtherId = E.B; }
		else if (E.B == Id) { OtherId = E.A; }
		if (OtherId == INDEX_NONE)
		{
			continue;
		}
		if (const FRecord* R = Records.Find(OtherId))
		{
			if (UWoodPieceComponent* P = R->Piece.Get())
			{
				Out.AddUnique(P);
			}
		}
	}
	return Out;
}

TArray<UWoodPieceComponent*> UWoodStructureSubsystem::GetUnsupportedPieces() const
{
	TArray<UWoodPieceComponent*> Out;
	for (const TPair<int32, FRecord>& Pair : Records)
	{
		if (!Supported.Contains(Pair.Key))
		{
			if (UWoodPieceComponent* P = Pair.Value.Piece.Get())
			{
				Out.Add(P);
			}
		}
	}
	return Out;
}

EWoodOrientation UWoodStructureSubsystem::GetPieceOrientation(const UWoodPieceComponent* Piece) const
{
	const UPrimitiveComponent* Prim = Piece ? Piece->GetPrimitive() : nullptr;
	if (!Prim)
	{
		return EWoodOrientation::Horizontal;
	}
	const FVector Axis = Prim->GetComponentTransform().GetRotation().RotateVector(GetLocalLengthAxis(Piece));
	return WoodStructure::ClassifyOrientation(Axis, OrientationToleranceDeg);
}
