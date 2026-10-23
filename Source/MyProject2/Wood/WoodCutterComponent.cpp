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

namespace
{
	bool IsValidSlice(const FWoodSlice& Slice)
	{
		const bool bFinite =
			FMath::IsFinite(Slice.S0) && FMath::IsFinite(Slice.S1) &&
			FMath::IsFinite(Slice.L0) && FMath::IsFinite(Slice.L1) &&
			FMath::IsFinite(Slice.V0) && FMath::IsFinite(Slice.V1);

		if (!bFinite)
		{
			return false;
		}

		constexpr float Epsilon = KINDA_SMALL_NUMBER;
		return Slice.S0 >= 0.f && Slice.S1 <= 1.f &&
			Slice.S1 - Slice.S0 > Epsilon &&
			Slice.L0 >= -0.5f && Slice.L1 <= 0.5f &&
			Slice.L1 - Slice.L0 > Epsilon &&
			Slice.V0 >= -0.5f && Slice.V1 <= 0.5f &&
			Slice.V1 - Slice.V0 > Epsilon;
	}

	bool IsValidPieceData(const FWoodPieceData& Data)
	{
		return Data.LengthModules >= 1 && Data.LengthModules <= 6 &&
			FMath::IsFinite(Data.SourceTreeHeightCm) && Data.SourceTreeHeightCm >= 0.f &&
			FMath::IsFinite(Data.DiameterCm) && Data.DiameterCm > 0.f &&
			FMath::IsFinite(Data.FullLogLengthCm) && Data.FullLogLengthCm > 0.f &&
			FMath::IsFinite(Data.DamageFraction) &&
			Data.DamageFraction >= 0.f && Data.DamageFraction <= 1.f;
	}
}

bool UWoodCutterComponent::Finish(const FWoodCutResult& Result, const TArray<FWoodSlice>& Slices, const TCHAR* OperationName)
{
	auto FailCut = [this](const FString& Reason)
	{
		OnCutFailed.Broadcast(Reason);
		return false;
	};

	if (!Result.bSuccess)
	{
		return FailCut(Result.Reason);
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	UWoodPieceComponent* Piece = GetPiece();
	UPrimitiveComponent* Prim = Piece ? Piece->GetPrimitive() : nullptr;
	if (!IsValid(Owner) || !IsValid(World) || !IsValid(Piece) || !IsValid(Prim) ||
		!OperationName || Result.Pieces.Num() != Slices.Num() || Result.Pieces.Num() == 0)
	{
		return FailCut(TEXT("Kesim uygulanamadı: eksik bileşen veya uyumsuz kesim çıktısı."));
	}

	if (Piece->GetState() == EWoodState::Installed)
	{
		return FailCut(TEXT("Yapıya bağlı parça kesilemez; önce sök."));
	}

	// Preflight: doğrulama bitmeden aktör veya kaynak parça değiştirilmez.
	for (int32 Index = 0; Index < Result.Pieces.Num(); ++Index)
	{
		if (!IsValidPieceData(Result.Pieces[Index]) || !IsValidSlice(Slices[Index]))
		{
			return FailCut(TEXT("Kesim uygulanamadı: geçersiz parça verisi veya dilim sınırları."));
		}
	}

	const FWoodPieceData OldData = Piece->PieceData;
	const FTransform OldXf = Prim->GetComponentTransform();
	const bool bWasSimulating = Prim->IsSimulatingPhysics();
	const ECollisionEnabled::Type OldCollision = Prim->GetCollisionEnabled();
	const FVector OldLinearVelocity = Prim->GetPhysicsLinearVelocity();
	const FVector OldAngularVelocity = Prim->GetPhysicsAngularVelocityInRadians();

	UStaticMesh* OldStaticMesh = nullptr;
	if (const UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Prim))
	{
		OldStaticMesh = SMC->GetStaticMesh();
	}

	// Yeni aktörler staged tutulur: tümü hazır değilse kaynak parça değişmeden
	// temizlenirler. ReshapeVisual'un görsel oluşturması ve oyun durumunu değiştirmemesi
	// bu transaction'ın sözleşmesidir.
	TArray<AActor*> StagedActors;
	StagedActors.Reserve(FMath::Max(0, Result.Pieces.Num() - 1));

	auto RollbackStagedActors = [&StagedActors]()
	{
		for (int32 Index = StagedActors.Num() - 1; Index >= 0; --Index)
		{
			if (IsValid(StagedActors[Index]))
			{
				StagedActors[Index]->Destroy();
			}
		}
		StagedActors.Reset();
	};

	auto FailAndRollback = [&FailCut, &RollbackStagedActors](const FString& Reason)
	{
		RollbackStagedActors();
		return FailCut(Reason);
	};

	for (int32 Index = 1; Index < Result.Pieces.Num(); ++Index)
	{
		const FTransform NewXf = MakeSliceTransform(OldData, OldXf, Slices[Index]);

		// Deferred spawn, parça verisini construction/BeginPlay başlamadan önce
		// hazırlamaya izin verir. Yeni parçalar commit anına dek fizik ve çarpışma dışıdır.
		AActor* NewActor = World->SpawnActorDeferred<AActor>(
			Owner->GetClass(),
			NewXf,
			nullptr,
			Owner->GetInstigator(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!IsValid(NewActor))
		{
			return FailAndRollback(TEXT("Kesim uygulanamadı: yeni parça aktörü oluşturulamadı."));
		}
		StagedActors.Add(NewActor);

		if (UWoodPieceComponent* DeferredPiece = NewActor->FindComponentByClass<UWoodPieceComponent>())
		{
			DeferredPiece->PieceData = Result.Pieces[Index];
			DeferredPiece->Role = Piece->Role;
			if (UPrimitiveComponent* DeferredPrim = DeferredPiece->GetPrimitive())
			{
				DeferredPrim->SetSimulatePhysics(false);
				DeferredPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}

		NewActor->FinishSpawning(NewXf);

		UWoodPieceComponent* NewPiece = NewActor->FindComponentByClass<UWoodPieceComponent>();
		UPrimitiveComponent* NewPrim = NewPiece ? NewPiece->GetPrimitive() : nullptr;
		if (!IsValid(NewPiece) || !IsValid(NewPrim))
		{
			return FailAndRollback(TEXT("Kesim uygulanamadı: yeni aktörde gerekli WoodPiece/Primitive bileşeni yok."));
		}

		NewPiece->PieceData = Result.Pieces[Index];
		NewPiece->Role = Piece->Role;
		NewPrim->SetSimulatePhysics(false);
		NewPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (UStaticMeshComponent* NewSMC = Cast<UStaticMeshComponent>(NewPrim))
		{
			if (OldStaticMesh)
			{
				NewSMC->SetStaticMesh(OldStaticMesh);
			}
		}

		NewPrim->SetWorldTransform(NewXf, false, nullptr, ETeleportType::TeleportPhysics);

		if (UWoodCutterComponent* NewCutter = NewActor->FindComponentByClass<UWoodCutterComponent>())
		{
			NewCutter->LocalLengthAxis = LocalLengthAxis;
			NewCutter->LocalLateralAxis = LocalLateralAxis;
			NewCutter->ReshapeVisual(NewPiece, Result.Pieces[Index], Slices[Index]);
		}

		// Görsel callback'i component durumunu değiştirmiş olsa dahi staged parça,
		// bütün çıktılar doğrulanana kadar simülasyona katılamaz.
		NewPrim = IsValid(NewPiece) ? NewPiece->GetPrimitive() : nullptr;
		if (!IsValid(NewPrim))
		{
			return FailAndRollback(TEXT("Kesim uygulanamadı: görsel hazırlık sonrasında Primitive bileşeni kayboldu."));
		}
		NewPrim->SetSimulatePhysics(false);
		NewPrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Son commit öncesi bir tur daha doğrula. Kaynak parça hâlâ aynı sahibi ve
	// geçerli primitive ile duruyorsa artık kaynak durumunu değiştirmeye geçebiliriz.
	if (!IsValid(Owner) || !IsValid(Piece) || !IsValid(Prim) ||
		Piece->GetOwner() != Owner || Piece->GetState() == EWoodState::Installed)
	{
		return FailAndRollback(TEXT("Kesim uygulanamadı: kaynak parça hazırlık sırasında değişti."));
	}
	for (AActor* StagedActor : StagedActors)
	{
		UWoodPieceComponent* StagedPiece = IsValid(StagedActor)
			? StagedActor->FindComponentByClass<UWoodPieceComponent>() : nullptr;
		if (!IsValid(StagedPiece) || !IsValid(StagedPiece->GetPrimitive()))
		{
			return FailAndRollback(TEXT("Kesim uygulanamadı: hazırlanan parçalardan biri artık geçerli değil."));
		}
	}

	// Commit: kaynak parçanın değişimi ancak bütün yeni parçalar hazır olduğunda yapılır.
	const FTransform SelfXf = MakeSliceTransform(OldData, OldXf, Slices[0]);
	Piece->PieceData = Result.Pieces[0];
	Prim->SetWorldTransform(SelfXf, false, nullptr, ETeleportType::TeleportPhysics);
	ReshapeVisual(Piece, Result.Pieces[0], Slices[0]);

	auto RestoreSource = [&]()
	{
		if (!IsValid(Piece) || !IsValid(Prim))
		{
			return;
		}
		Piece->PieceData = OldData;
		Prim->SetWorldTransform(OldXf, false, nullptr, ETeleportType::TeleportPhysics);
		ReshapeVisual(Piece, OldData, FWoodSlice{});
		Prim->SetCollisionEnabled(OldCollision);
		Prim->SetSimulatePhysics(bWasSimulating);
		if (bWasSimulating)
		{
			Prim->SetPhysicsLinearVelocity(OldLinearVelocity);
			Prim->SetPhysicsAngularVelocityInRadians(OldAngularVelocity);
		}
	};

	Prim->SetCollisionEnabled(OldCollision);
	Prim->SetSimulatePhysics(bWasSimulating);
	if (bWasSimulating)
	{
		Prim->SetPhysicsLinearVelocity(OldLinearVelocity);
		Prim->SetPhysicsAngularVelocityInRadians(OldAngularVelocity);
	}

	// Bütün staged parçaları görünür/fiziksel duruma geçir; source collision
	// ayarı korunur, yeni fizik yalnızca ayar açıksa ve kaynak simüle oluyorsa açılır.
	for (AActor* StagedActor : StagedActors)
	{
		UWoodPieceComponent* NewPiece = IsValid(StagedActor)
			? StagedActor->FindComponentByClass<UWoodPieceComponent>() : nullptr;
		UPrimitiveComponent* NewPrim = NewPiece ? NewPiece->GetPrimitive() : nullptr;
		if (!IsValid(NewPiece) || !IsValid(NewPrim))
		{
			// Beklenmeyen Blueprint yan etkisi commit sonrası bir parçayı bozduysa,
			// kaynak parçayı eski veriye döndür ve hazırlanan çıktıların hepsini temizle.
			RestoreSource();
			RollbackStagedActors();
			return FailCut(TEXT("Kesim geri alındı: commit sırasında hazırlanan parçalardan biri geçersizleşti."));
		}

		NewPrim->SetCollisionEnabled(OldCollision);
		const bool bEnableNewPhysics = bCopySimulationToNewPieces && bWasSimulating;
		NewPrim->SetSimulatePhysics(bEnableNewPhysics);
		if (bEnableNewPhysics)
		{
			NewPrim->SetPhysicsLinearVelocity(OldLinearVelocity);
			NewPrim->SetPhysicsAngularVelocityInRadians(OldAngularVelocity);
		}
	}

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
