// Copyright Epic Games, Inc. All Rights Reserved.

#include "RawLogShapes.h"
#include "CarvableLog.h"
#include "Engine/StaticMesh.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Components/DynamicMeshComponent.h"
#include "StructureLink.h"

namespace RawLogShapes
{
namespace
{
	struct FProfileInfo
	{
		EProfile Profile;
		const TCHAR* Suffix;   // mesh name suffix after SM_RawLog_<L>
		const TCHAR* Name;     // shown to the player
	};

	const FProfileInfo Profiles[] = {
		{ EProfile::Raw, TEXT(""), TEXT("kütük") },
		{ EProfile::Half, TEXT("_Half"), TEXT("yarım kütük") },
		{ EProfile::Quarter, TEXT("_Quarter_04"), TEXT("çeyrek kütük") },
		{ EProfile::Hewn1, TEXT("_Hewn1"), TEXT("bir yüzü düzlenmiş kütük") },
		{ EProfile::Hewn2, TEXT("_Hewn2"), TEXT("iki yüzü düzlenmiş kütük") },
		{ EProfile::Kalas, TEXT("_Kalas"), TEXT("kalas") },
		{ EProfile::Plank, TEXT("_Plank"), TEXT("tahta") },
		{ EProfile::Gutter, TEXT("_Gutter"), TEXT("oluklu kütük") },
		{ EProfile::Trough, TEXT("_Trough"), TEXT("yalak") },
		{ EProfile::FlatNotch, TEXT("_FlatNotch_TopBottom"), TEXT("çentikli kütük") },
	};

	const FProfileInfo* Find(EProfile Profile)
	{
		for (const FProfileInfo& Info : Profiles)
		{
			if (Info.Profile == Profile)
			{
				return &Info;
			}
		}
		return nullptr;
	}

	/** Pieces an action leaves: their mesh and how many, laid side by side across the log. */
	struct FResult
	{
		int32 Length = 0;
		EProfile Profile = EProfile::Unknown;
		int32 Count = 0;
		bool bAlongLength = false;   // cross cut: the pieces follow each other along the log
	};

	bool Outcome(const FShape& Shape, EAction Action, FResult& Out, FString& OutWhy)
	{
		Out = FResult();
		switch (Action)
		{
		case EAction::CrossCut:
			if (Shape.Length <= ShortestCm)
			{
				OutWhy = TEXT("Daha fazla kesilemez  —  en kısa parça 75 cm");
				return false;
			}
			if (Shape.Profile == EProfile::FlatNotch)
			{
				OutWhy = TEXT("Çentikli parça kesilmez");
				return false;
			}
			Out = { Shape.Length / 2, Shape.Profile, 2, true };
			return true;
		case EAction::Split:
			switch (Shape.Profile)
			{
			case EProfile::Raw:   Out = { Shape.Length, EProfile::Half, 2, false }; return true;
			case EProfile::Half:  Out = { Shape.Length, EProfile::Quarter, 2, false }; return true;
			case EProfile::Kalas: Out = { Shape.Length, EProfile::Plank, 4, false }; return true;
			case EProfile::Quarter:
			case EProfile::Plank:
				OutWhy = TEXT("Daha fazla yarılamaz");
				return false;
			default:
				OutWhy = TEXT("Bu parça boyuna yarılmaz  —  önce kalas yap (iki yanı da düzle)");
				return false;
			}
		case EAction::Hew:
			switch (Shape.Profile)
			{
			case EProfile::Raw:   Out = { Shape.Length, EProfile::Hewn1, 1, false }; return true;
			case EProfile::Hewn1: Out = { Shape.Length, EProfile::Hewn2, 1, false }; return true;
			case EProfile::Hewn2: Out = { Shape.Length, EProfile::Kalas, 1, false }; return true;
			case EProfile::Kalas:
				OutWhy = TEXT("Kalas hazır  —  ucuna vurup 4 tahtaya yar");
				return false;
			default:
				OutWhy = TEXT("Bu parça düzlenmez");
				return false;
			}
		case EAction::Groove:
			switch (Shape.Profile)
			{
			case EProfile::Raw:    Out = { Shape.Length, EProfile::Gutter, 1, false }; return true;
			case EProfile::Gutter: Out = { Shape.Length, EProfile::Trough, 1, false }; return true;
			case EProfile::Trough:
				OutWhy = TEXT("Yalak hazır, daha oyulmaz");
				return false;
			default:
				OutWhy = TEXT("Oluk yalnız yuvarlak kütüğe açılır");
				return false;
			}
		}
		return false;
	}
}

FShape FromMesh(const UStaticMesh* Mesh)
{
	FShape Shape;
	if (!Mesh)
	{
		return Shape;
	}
	FString Name = Mesh->GetName();
	if (!Name.RemoveFromStart(TEXT("SM_RawLog_")))
	{
		return Shape;
	}
	int32 Digits = 0;
	while (Digits < Name.Len() && FChar::IsDigit(Name[Digits]))
	{
		++Digits;
	}
	if (Digits == 0)
	{
		return Shape;
	}
	const int32 Length = FCString::Atoi(*Name.Left(Digits));
	const FString Suffix = Name.Mid(Digits);
	for (const FProfileInfo& Info : Profiles)
	{
		if (Suffix == Info.Suffix)
		{
			Shape.Length = Length;
			Shape.Profile = Info.Profile;
			break;
		}
	}
	return Shape;
}

FShape FromLog(const ACarvableLog* Log)
{
	return Log ? FromMesh(Log->SourceMesh) : FShape();
}

UStaticMesh* LoadMesh(int32 Length, EProfile Profile)
{
	const FProfileInfo* Info = Find(Profile);
	if (!Info)
	{
		return nullptr;
	}
	const FString Name = FString::Printf(TEXT("SM_RawLog_%d%s"), Length, Info->Suffix);
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/UnrealExport/%s.%s"), *Name, *Name));
}

EAction ClassifyHit(const ACarvableLog* Log, const FHitResult& Hit, bool bChannelMode)
{
	const FTransform Frame = Log->GetActorTransform();
	const FVector LocalPoint = Frame.InverseTransformPosition(Hit.ImpactPoint);
	const FVector LocalNormal = Frame.InverseTransformVectorNoScale(Hit.ImpactNormal);
	// An end face: the grain runs into it, the axe splits along it.
	if (FMath::Abs(LocalNormal.X) > 0.7f)
	{
		return EAction::Split;
	}
	if (bChannelMode)
	{
		return EAction::Groove;
	}
	const FBox Box = Log->GetLocalBox();
	const float Middle = Box.GetCenter().X;
	return FMath::Abs(LocalPoint.X - Middle) <= Box.GetSize().X * 0.2f ? EAction::CrossCut : EAction::Hew;
}

bool CanApply(const FShape& Shape, EAction Action, FString& OutWhy)
{
	FResult Result;
	return Shape.IsValid() && Outcome(Shape, Action, Result, OutWhy) && LoadMesh(Result.Length, Result.Profile);
}

int32 HitsFor(EAction Action)
{
	return 2;
}

bool Apply(ACarvableLog* Log, EAction Action, FString& OutMessage)
{
	const FShape Shape = FromLog(Log);
	FResult Result;
	if (!IsValid(Log) || !Shape.IsValid() || !Outcome(Shape, Action, Result, OutMessage))
	{
		return false;
	}
	UStaticMesh* PieceMesh = LoadMesh(Result.Length, Result.Profile);
	UWorld* World = Log->GetWorld();
	if (!PieceMesh || !World || Result.Count < 1 || Result.Count > 4)
	{
		OutMessage = TEXT("Bu dönüşüm için model veya geçerli parça sayısı yok");
		return false;
	}

	// First calculate the complete result. No actor/resource state is mutated during preflight.
	const FTransform Frame(Log->GetActorQuat(), Log->GetActorLocation());
	const FBox Old = Log->GetLocalBox();
	const FBox New = PieceMesh->GetBoundingBox();
	if (!Old.IsValid || !New.IsValid)
	{
		OutMessage = TEXT("Kesim geometrisi geçersiz; kaynak kütük korundu");
		return false;
	}
	const FVector NewSize = New.GetSize();
	if (NewSize.ContainsNaN() || NewSize.X <= 0.f || NewSize.Y <= 0.f || NewSize.Z <= 0.f)
	{
		OutMessage = TEXT("Kesim modeli ölçüleri geçersiz; kaynak kütük korundu");
		return false;
	}

	TArray<FTransform> PlannedTransforms;
	PlannedTransforms.Reserve(Result.Count);
	const float Bottom = Old.Min.Z;
	const FVector OldCentre = Old.GetCenter();
	for (int32 Index = 0; Index < Result.Count; ++Index)
	{
		const float Offset = Index - (Result.Count - 1) * 0.5f;
		FVector Centre(OldCentre.X, OldCentre.Y, Bottom + NewSize.Z * 0.5f);
		if (Result.bAlongLength)
		{
			Centre.X += Offset * (NewSize.X + 4.f);
		}
		else
		{
			Centre.Y += Offset * (NewSize.Y + 4.f);
		}
		const FVector Location = Frame.TransformPosition(Centre - New.GetCenter());
		const FTransform PieceTransform(Frame.GetRotation(), Location);
		if (Location.ContainsNaN() || PieceTransform.ContainsNaN())
		{
			OutMessage = TEXT("Kesim yerleşimi hesaplanamadı; kaynak kütük korundu");
			return false;
		}
		PlannedTransforms.Add(PieceTransform);
	}

	// Disable the source while outputs spawn so the old collision hull cannot push the new pieces apart.
	UDynamicMeshComponent* SourceComponent = Log->GetMeshComponent();
	if (!IsValid(SourceComponent))
	{
		OutMessage = TEXT("Kaynak kütüğün çarpışma bileşeni bulunamadı");
		return false;
	}
	const ECollisionEnabled::Type OldCollision = SourceComponent->GetCollisionEnabled();
	const bool bOldSimulatesPhysics = SourceComponent->IsSimulatingPhysics();
	SourceComponent->SetSimulatePhysics(false);
	SourceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TArray<ACarvableLog*> SpawnedPieces;
	SpawnedPieces.Reserve(Result.Count);
	for (const FTransform& PieceTransform : PlannedTransforms)
	{
		ACarvableLog* Piece = ACarvableLog::SpawnFromStaticMesh(World, PieceMesh, PieceTransform);
		if (!IsValid(Piece) || Piece->GetLength() <= 0.f || Piece->GetRadius() <= 0.f)
		{
			if (IsValid(Piece))
			{
				Piece->Destroy();
			}
			for (ACarvableLog* Spawned : SpawnedPieces)
			{
				if (IsValid(Spawned)) Spawned->Destroy();
			}
			SourceComponent->SetCollisionEnabled(OldCollision);
			SourceComponent->SetSimulatePhysics(bOldSimulatesPhysics);
			OutMessage = TEXT("Parçalardan biri üretilemedi; işlem geri alındı ve kaynak kütük korundu");
			return false;
		}
		SpawnedPieces.Add(Piece);
	}

	// Commit point: all requested outputs exist and carry valid dimensions.
	Log->Destroy();
	StructureLink::MarkChanged();
	StructureLink::Settle(World);

	const FShape Made{ Result.Length, Result.Profile };
	OutMessage = Result.Count > 1
		? FString::Printf(TEXT("%d × %s"), Result.Count, *Describe(Made))
		: Describe(Made);
	return true;
}

FString ProfileName(EProfile Profile)
{
	const FProfileInfo* Info = Find(Profile);
	return Info ? Info->Name : TEXT("parça");
}

FString Describe(const FShape& Shape)
{
	return FString::Printf(TEXT("%d cm %s"), Shape.Length, *ProfileName(Shape.Profile));
}

FString ActionName(EAction Action)
{
	switch (Action)
	{
	case EAction::CrossCut: return TEXT("Enine kesiliyor");
	case EAction::Split:    return TEXT("Boyuna yarılıyor");
	case EAction::Hew:      return TEXT("Düzleniyor");
	case EAction::Groove:   return TEXT("Oluk açılıyor");
	}
	return TEXT("");
}
}
