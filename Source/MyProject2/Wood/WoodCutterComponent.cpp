#include "WoodCutterComponent.h"
#include "WoodPieceComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UWoodCutterComponent::UWoodCutterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UWoodPieceComponent* UWoodCutterComponent::GetPiece() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UWoodPieceComponent>() : nullptr;
}

FVector UWoodCutterComponent::GetVerticalAxis() const
{
	return FVector::CrossProduct(LocalLengthAxis.GetSafeNormal(), LocalLateralAxis.GetSafeNormal()).GetSafeNormal();
}

TArray<FVector> UWoodCutterComponent::GetCutLineWorldPositions() const
{
	TArray<FVector> Out;
	const UWoodPieceComponent* Piece = GetPiece();
	const UPrimitiveComponent* Prim = Piece ? Piece->GetPrimitive() : nullptr;
	if (!Piece || !Prim)
	{
		return Out;
	}

	const FWoodPieceData& D = Piece->PieceData;
	const FTransform Xf = Prim->GetComponentTransform();
	const FVector Len = LocalLengthAxis.GetSafeNormal();
	const int32 N = D.LengthModules;

	for (int32 Line = 1; Line <= N - 1; ++Line)
	{
		const float Along = (static_cast<float>(Line) / static_cast<float>(N) - 0.5f) * D.GetLengthCm();
		Out.Add(Xf.GetLocation() + Xf.GetRotation().RotateVector(Len * Along));
	}
	return Out;
}

int32 UWoodCutterComponent::FindNearestCutLine(FVector WorldPoint) const
{
	const UWoodPieceComponent* Piece = GetPiece();
	const UPrimitiveComponent* Prim = Piece ? Piece->GetPrimitive() : nullptr;
	if (!Piece || !Prim)
	{
		return INDEX_NONE;
	}

	const FWoodPieceData& D = Piece->PieceData;
	const FTransform Xf = Prim->GetComponentTransform();
	const FVector Local = Xf.InverseTransformPositionNoScale(WorldPoint);
	const float Along = FVector::DotProduct(Local, LocalLengthAxis.GetSafeNormal()) + D.GetLengthCm() * 0.5f;
	return UWoodCutLibrary::SnapToModuleLine(D, Along);
}

FTransform UWoodCutterComponent::MakeSliceTransform(const FWoodPieceData& OldData, const FTransform& OldXf, const FWoodSlice& Slice) const
{
	const FVector Len = LocalLengthAxis.GetSafeNormal();
	const FVector Lat = LocalLateralAxis.GetSafeNormal();
	const FVector Vert = GetVerticalAxis();

	const float LenCm = OldData.GetLengthCm();
	const float LatCm = OldData.GetLateralCm();
	const float VertCm = OldData.GetVerticalCm();

	const FVector LocalOffset =
		Len * (((Slice.S0 + Slice.S1) * 0.5f - 0.5f) * LenCm) +
		Lat * (((Slice.L0 + Slice.L1) * 0.5f) * LatCm) +
		Vert * (((Slice.V0 + Slice.V1) * 0.5f) * VertCm);

	const float dS = Slice.S1 - Slice.S0;
	const float dL = Slice.L1 - Slice.L0;
	const float dV = Slice.V1 - Slice.V0;

	FVector Scale = OldXf.GetScale3D();
	Scale.X *= FMath::Abs(Len.X) * dS + FMath::Abs(Lat.X) * dL + FMath::Abs(Vert.X) * dV;
	Scale.Y *= FMath::Abs(Len.Y) * dS + FMath::Abs(Lat.Y) * dL + FMath::Abs(Vert.Y) * dV;
	Scale.Z *= FMath::Abs(Len.Z) * dS + FMath::Abs(Lat.Z) * dL + FMath::Abs(Vert.Z) * dV;

	FTransform Result = OldXf;
	Result.SetLocation(OldXf.GetLocation() + OldXf.GetRotation().RotateVector(LocalOffset));
	Result.SetScale3D(Scale);
	return Result;
}

bool UWoodCutterComponent::Finish(const FWoodCutResult& Result, const TArray<FWoodSlice>& Slices, const TCHAR* OperationName)
{
	if (!Result.bSuccess)
	{
		OnCutFailed.Broadcast(Result.Reason);
		return false;
	}

	AActor* Owner = GetOwner();
	UWoodPieceComponent* Piece = GetPiece();
	UPrimitiveComponent* Prim = Piece ? Piece->GetPrimitive() : nullptr;
	if (!Owner || !Piece || !Prim || Result.Pieces.Num() != Slices.Num() || Result.Pieces.Num() == 0)
	{
		OnCutFailed.Broadcast(TEXT("Kesim uygulanamadı: eksik bileşen."));
		return false;
	}

	if (Piece->GetState() == EWoodState::Installed)
	{
		OnCutFailed.Broadcast(TEXT("Yapıya bağlı parça kesilemez; önce sök."));
		return false;
	}

	const FWoodPieceData OldData = Piece->PieceData;
	const FTransform OldXf = Prim->GetComponentTransform();
	const bool bWasSimulating = Prim->IsSimulatingPhysics();
	UStaticMesh* OldStaticMesh = nullptr;
	if (const UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Prim))
	{
		OldStaticMesh = SMC->GetStaticMesh();
	}

	// Yeni parçalar önce, sonra kesilen parça kendi yerine geçer.
	for (int32 i = 1; i < Result.Pieces.Num(); ++i)
	{
		const FTransform NewXf = MakeSliceTransform(OldData, OldXf, Slices[i]);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Instigator = Owner->GetInstigator();

		AActor* NewActor = GetWorld() ? GetWorld()->SpawnActor<AActor>(Owner->GetClass(), NewXf, Params) : nullptr;
		if (!NewActor)
		{
			continue;
		}

		UWoodPieceComponent* NewPiece = NewActor->FindComponentByClass<UWoodPieceComponent>();
		UPrimitiveComponent* NewPrim = NewPiece ? NewPiece->GetPrimitive() : nullptr;
		if (!NewPiece || !NewPrim)
		{
			continue;
		}

		NewPiece->PieceData = Result.Pieces[i];
		NewPiece->Role = Piece->Role;

		if (UStaticMeshComponent* NewSMC = Cast<UStaticMeshComponent>(NewPrim))
		{
			if (OldStaticMesh)
			{
				NewSMC->SetStaticMesh(OldStaticMesh);
			}
		}

		NewPrim->SetWorldTransform(NewXf, false, nullptr, ETeleportType::TeleportPhysics);
		if (bCopySimulationToNewPieces && bWasSimulating)
		{
			NewPrim->SetSimulatePhysics(true);
		}

		if (UWoodCutterComponent* NewCutter = NewActor->FindComponentByClass<UWoodCutterComponent>())
		{
			NewCutter->LocalLengthAxis = LocalLengthAxis;
			NewCutter->LocalLateralAxis = LocalLateralAxis;
			NewCutter->ReshapeVisual(NewPiece, Result.Pieces[i], Slices[i]);
		}
	}

	// Kesilen parçanın kendisi
	Piece->PieceData = Result.Pieces[0];
	const FTransform SelfXf = MakeSliceTransform(OldData, OldXf, Slices[0]);
	Prim->SetWorldTransform(SelfXf, false, nullptr, ETeleportType::TeleportPhysics);
	ReshapeVisual(Piece, Result.Pieces[0], Slices[0]);

	OnCutDone.Broadcast(FString(OperationName));
	return true;
}

bool UWoodCutterComponent::FinishInPlace(const FWoodCutResult& Result, const TCHAR* OperationName)
{
	return Finish(Result, TArray<FWoodSlice>{FWoodSlice()}, OperationName);
}

void UWoodCutterComponent::ReshapeVisual_Implementation(UWoodPieceComponent* Piece, const FWoodPieceData& NewData, const FWoodSlice& Slice)
{
	// Varsayılan: ölçek tabanlı yaklaşık görsel (MakeSliceTransform içinde uygulandı).
}

bool UWoodCutterComponent::CutAtModule(int32 ModuleLine)
{
	const UWoodPieceComponent* Piece = GetPiece();
	if (!Piece)
	{
		return false;
	}

	const FWoodCutResult R = UWoodCutLibrary::CrossCut(Piece->PieceData, ModuleLine);
	const float N = static_cast<float>(Piece->PieceData.LengthModules);

	FWoodSlice A;
	A.S0 = 0.f;
	A.S1 = static_cast<float>(ModuleLine) / N;
	FWoodSlice B;
	B.S0 = A.S1;
	B.S1 = 1.f;
	return Finish(R, {A, B}, TEXT("CrossCut"));
}

bool UWoodCutterComponent::CutAtWorldPoint(FVector WorldPoint)
{
	const int32 Line = FindNearestCutLine(WorldPoint);
	return Line != INDEX_NONE && CutAtModule(Line);
}

bool UWoodCutterComponent::SplitLengthwise()
{
	const UWoodPieceComponent* Piece = GetPiece();
	if (!Piece)
	{
		return false;
	}

	FWoodSlice A;
	A.L0 = -0.5f;
	A.L1 = 0.f;
	FWoodSlice B;
	B.L0 = 0.f;
	B.L1 = 0.5f;
	return Finish(UWoodCutLibrary::SplitLengthwise(Piece->PieceData), {A, B}, TEXT("SplitLengthwise"));
}

bool UWoodCutterComponent::SplitQuarter()
{
	const UWoodPieceComponent* Piece = GetPiece();
	if (!Piece)
	{
		return false;
	}

	FWoodSlice A;
	A.V0 = -0.5f;
	A.V1 = 0.f;
	FWoodSlice B;
	B.V0 = 0.f;
	B.V1 = 0.5f;
	return Finish(UWoodCutLibrary::SplitQuarter(Piece->PieceData), {A, B}, TEXT("SplitQuarter"));
}

bool UWoodCutterComponent::MakePlank()
{
	const UWoodPieceComponent* Piece = GetPiece();
	if (!Piece)
	{
		return false;
	}

	const FWoodPieceData& D = Piece->PieceData;
	const float TargetVertical = D.DiameterCm * 0.47f;
	const float Fraction = FMath::Min(1.f, TargetVertical / FMath::Max(D.GetVerticalCm(), 1.f));

	FWoodSlice S;
	S.V0 = -0.5f * Fraction;
	S.V1 = 0.5f * Fraction;
	return Finish(UWoodCutLibrary::MakePlank(D), {S}, TEXT("MakePlank"));
}

bool UWoodCutterComponent::TrimEnd(bool bEndA)
{
	const UWoodPieceComponent* Piece = GetPiece();
	return Piece && FinishInPlace(UWoodCutLibrary::TrimEnd(Piece->PieceData, bEndA), TEXT("TrimEnd"));
}

bool UWoodCutterComponent::AddNotch(int32 ModuleLine, EWoodJointType JointType)
{
	const UWoodPieceComponent* Piece = GetPiece();
	return Piece && FinishInPlace(UWoodCutLibrary::AddNotch(Piece->PieceData, ModuleLine, JointType), TEXT("AddNotch"));
}

bool UWoodCutterComponent::CutGroove()
{
	const UWoodPieceComponent* Piece = GetPiece();
	return Piece && FinishInPlace(UWoodCutLibrary::CutGroove(Piece->PieceData), TEXT("CutGroove"));
}

bool UWoodCutterComponent::SharpenEnd(bool bEndA)
{
	const UWoodPieceComponent* Piece = GetPiece();
	return Piece && FinishInPlace(UWoodCutLibrary::SharpenEnd(Piece->PieceData, bEndA), TEXT("SharpenEnd"));
}

bool UWoodCutterComponent::ChamferEnd(bool bEndA)
{
	const UWoodPieceComponent* Piece = GetPiece();
	return Piece && FinishInPlace(UWoodCutLibrary::ChamferEnd(Piece->PieceData, bEndA), TEXT("ChamferEnd"));
}

bool UWoodCutterComponent::MakeFirewood()
{
	const UWoodPieceComponent* Piece = GetPiece();
	if (!Piece)
	{
		return false;
	}

	const FWoodPieceData& D = Piece->PieceData;
	const FWoodCutResult R = UWoodCutLibrary::MakeFirewood(D);
	if (!R.bSuccess)
	{
		OnCutFailed.Broadcast(R.Reason);
		return false;
	}

	const int32 N = D.LengthModules;
	TArray<FWoodSlice> Slices;

	for (int32 m = 0; m < N; ++m)
	{
		const float S0 = static_cast<float>(m) / static_cast<float>(N);
		const float S1 = static_cast<float>(m + 1) / static_cast<float>(N);

		auto Add = [&Slices, S0, S1](float L0, float L1, float V0, float V1)
		{
			FWoodSlice S;
			S.S0 = S0; S.S1 = S1; S.L0 = L0; S.L1 = L1; S.V0 = V0; S.V1 = V1;
			Slices.Add(S);
		};

		switch (D.CrossSection)
		{
		case EWoodCrossSection::Round:
			Add(-0.5f, 0.f, -0.5f, 0.f);
			Add(-0.5f, 0.f, 0.f, 0.5f);
			Add(0.f, 0.5f, -0.5f, 0.f);
			Add(0.f, 0.5f, 0.f, 0.5f);
			break;
		case EWoodCrossSection::Half:
			Add(-0.5f, 0.5f, -0.5f, 0.f);
			Add(-0.5f, 0.5f, 0.f, 0.5f);
			break;
		default:
			Add(-0.5f, 0.5f, -0.5f, 0.5f);
			break;
		}
	}

	return Finish(R, Slices, TEXT("MakeFirewood"));
}
