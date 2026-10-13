#include "WoodCutLibrary.h"

namespace
{
	FWoodCutResult Fail(const TCHAR* Reason)
	{
		FWoodCutResult R;
		R.bSuccess = false;
		R.Reason = Reason;
		return R;
	}

	FWoodCutResult Ok(const TArray<FWoodPieceData>& Pieces)
	{
		FWoodCutResult R;
		R.bSuccess = true;
		R.Pieces = Pieces;
		return R;
	}

	FWoodCutResult OkOne(const FWoodPieceData& Piece)
	{
		return Ok(TArray<FWoodPieceData>{Piece});
	}
}

int32 UWoodCutLibrary::SnapToModuleLine(const FWoodPieceData& Data, float DistanceFromEndACm)
{
	const int32 N = FMath::Max(Data.LengthModules, 1);
	const float Module = FMath::Max(Data.GetModuleLengthCm(), 1.f);
	const int32 Line = FMath::RoundToInt(DistanceFromEndACm / Module);
	return FMath::Clamp(Line, 1, FMath::Max(N - 1, 1));
}

FWoodCutResult UWoodCutLibrary::CrossCut(const FWoodPieceData& Data, int32 ModuleLine)
{
	const int32 N = Data.LengthModules;
	if (N < 2)
	{
		return Fail(TEXT("1M parça daha fazla kesilemez."));
	}
	if (ModuleLine < 1 || ModuleLine > N - 1)
	{
		return Fail(TEXT("Geçersiz modül çizgisi."));
	}

	FWoodPieceData A = Data;
	FWoodPieceData B = Data;
	A.LengthModules = ModuleLine;
	B.LengthModules = N - ModuleLine;
	A.EndB = EWoodEndShape::Flat;
	B.EndA = EWoodEndShape::Flat;
	A.Notches.Reset();
	B.Notches.Reset();

	for (const FWoodNotch& Notch : Data.Notches)
	{
		if (Notch.ModuleLine < ModuleLine)
		{
			A.Notches.Add(Notch);
		}
		else if (Notch.ModuleLine > ModuleLine)
		{
			FWoodNotch Shifted = Notch;
			Shifted.ModuleLine -= ModuleLine;
			B.Notches.Add(Shifted);
		}
		// Kesim çizgisine denk gelen çentik kesimle yok olur.
	}

	return Ok({A, B});
}

FWoodCutResult UWoodCutLibrary::SplitLengthwise(const FWoodPieceData& Data)
{
	if (Data.CrossSection != EWoodCrossSection::Round)
	{
		return Fail(TEXT("Sadece yuvarlak kütük uzunlamasına yarılabilir."));
	}

	FWoodPieceData A = Data;
	FWoodPieceData B = Data;
	A.CrossSection = EWoodCrossSection::Half;
	B.CrossSection = EWoodCrossSection::Half;
	return Ok({A, B});
}

FWoodCutResult UWoodCutLibrary::SplitQuarter(const FWoodPieceData& Data)
{
	if (Data.CrossSection != EWoodCrossSection::Half)
	{
		return Fail(TEXT("Çeyrek parça için önce yarım kütük gerekir."));
	}

	FWoodPieceData A = Data;
	FWoodPieceData B = Data;
	A.CrossSection = EWoodCrossSection::Quarter;
	B.CrossSection = EWoodCrossSection::Quarter;
	return Ok({A, B});
}

FWoodCutResult UWoodCutLibrary::MakePlank(const FWoodPieceData& Data)
{
	if (Data.CrossSection != EWoodCrossSection::Half && Data.CrossSection != EWoodCrossSection::Quarter)
	{
		return Fail(TEXT("Tahta için yarım veya çeyrek parça gerekir."));
	}

	FWoodPieceData P = Data;
	P.CrossSection = EWoodCrossSection::Plank;
	P.bHollow = false;
	P.bLengthGroove = false;
	return OkOne(P);
}

FWoodCutResult UWoodCutLibrary::TrimEnd(const FWoodPieceData& Data, bool bEndA)
{
	FWoodPieceData P = Data;
	EWoodEndShape& End = bEndA ? P.EndA : P.EndB;
	if (End == EWoodEndShape::Flat)
	{
		return Fail(TEXT("Uç zaten düz."));
	}
	End = EWoodEndShape::Flat;
	return OkOne(P);
}

FWoodCutResult UWoodCutLibrary::AddNotch(const FWoodPieceData& Data, int32 ModuleLine, EWoodJointType JointType)
{
	const int32 N = Data.LengthModules;

	if (JointType == EWoodJointType::None)
	{
		return Fail(TEXT("Çentik türü seçilmedi."));
	}
	if (JointType == EWoodJointType::Groove)
	{
		return Fail(TEXT("Boyuna oluk için CutGroove kullanılır."));
	}
	if (ModuleLine < 0 || ModuleLine > N)
	{
		return Fail(TEXT("Geçersiz modül çizgisi."));
	}

	const EWoodCrossSection CS = Data.CrossSection;
	if (CS == EWoodCrossSection::Quarter && JointType != EWoodJointType::FlatLap)
	{
		return Fail(TEXT("Çeyrek parçada sadece düz çentik açılabilir."));
	}
	if (CS == EWoodCrossSection::Plank && JointType != EWoodJointType::FlatLap && JointType != EWoodJointType::Dovetail)
	{
		return Fail(TEXT("Tahtada sadece düz çentik veya kırlangıç kuyruğu açılabilir."));
	}
	if (JointType == EWoodJointType::Dovetail && N < 2)
	{
		return Fail(TEXT("Kırlangıç kuyruğu için en az 2M parça gerekir."));
	}
	if (Data.bHollow && JointType != EWoodJointType::FlatLap)
	{
		return Fail(TEXT("İçi boş parçada sadece düz çentik açılabilir."));
	}

	for (const FWoodNotch& Existing : Data.Notches)
	{
		if (Existing.ModuleLine == ModuleLine)
		{
			return Fail(TEXT("Bu çizgide zaten çentik var."));
		}
	}

	FWoodPieceData P = Data;
	FWoodNotch Notch;
	Notch.ModuleLine = ModuleLine;
	Notch.JointType = JointType;
	P.Notches.Add(Notch);
	return OkOne(P);
}

FWoodCutResult UWoodCutLibrary::CutGroove(const FWoodPieceData& Data)
{
	if (Data.CrossSection != EWoodCrossSection::Round && Data.CrossSection != EWoodCrossSection::Half)
	{
		return Fail(TEXT("Oluk sadece yuvarlak veya yarım kütükte açılabilir."));
	}
	if (Data.bHollow)
	{
		return Fail(TEXT("İçi boş parçada oluk açılamaz."));
	}
	if (Data.bLengthGroove)
	{
		return Fail(TEXT("Oluk zaten açık."));
	}

	FWoodPieceData P = Data;
	P.bLengthGroove = true;
	return OkOne(P);
}

FWoodCutResult UWoodCutLibrary::SharpenEnd(const FWoodPieceData& Data, bool bEndA)
{
	if (Data.CrossSection == EWoodCrossSection::Plank)
	{
		return Fail(TEXT("Tahta sivriltilemez."));
	}
	if (Data.bHollow)
	{
		return Fail(TEXT("İçi boş parça sivriltilemez."));
	}

	const int32 EndLine = bEndA ? 0 : Data.LengthModules;
	for (const FWoodNotch& Notch : Data.Notches)
	{
		if (Notch.ModuleLine == EndLine)
		{
			return Fail(TEXT("Uçta çentik var, önce çentiksiz bir uç gerekir."));
		}
	}

	FWoodPieceData P = Data;
	EWoodEndShape& End = bEndA ? P.EndA : P.EndB;
	if (End == EWoodEndShape::Pointed)
	{
		return Fail(TEXT("Uç zaten sivri."));
	}
	End = EWoodEndShape::Pointed;
	return OkOne(P);
}

FWoodCutResult UWoodCutLibrary::ChamferEnd(const FWoodPieceData& Data, bool bEndA)
{
	FWoodPieceData P = Data;
	EWoodEndShape& End = bEndA ? P.EndA : P.EndB;
	if (End != EWoodEndShape::Flat)
	{
		return Fail(TEXT("Pah sadece düz uca kırılır."));
	}
	End = EWoodEndShape::Chamfered;
	return OkOne(P);
}

FWoodCutResult UWoodCutLibrary::MakeFirewood(const FWoodPieceData& Data)
{
	if (Data.CrossSection == EWoodCrossSection::Plank)
	{
		return Fail(TEXT("Tahta ateşliğe bölünmez."));
	}

	int32 PerModule = 1;
	if (Data.CrossSection == EWoodCrossSection::Round)
	{
		PerModule = 4;
	}
	else if (Data.CrossSection == EWoodCrossSection::Half)
	{
		PerModule = 2;
	}

	FWoodPieceData Piece = Data;
	Piece.CrossSection = EWoodCrossSection::Quarter;
	Piece.LengthModules = 1;
	Piece.EndA = EWoodEndShape::Flat;
	Piece.EndB = EWoodEndShape::Flat;
	Piece.Notches.Reset();
	Piece.bLengthGroove = false;
	Piece.bHollow = false;

	TArray<FWoodPieceData> Out;
	Out.Init(Piece, Data.LengthModules * PerModule);
	return Ok(Out);
}
