// Yapı tanıma otomasyon testleri. Editör: Session Frontend > Automation > MyProject2.Wood.Analysis
#include "Misc/AutomationTest.h"
#include "WoodStructureAnalysis.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WoodStructure;
using WoodEdge = WoodStructure::FEdge;

namespace
{
	FNode Mk(int32 Id, const FVector& A, const FVector& B, EWoodRole Role, bool bGrounded, float Radius = 15.f, int32 Modules = 6)
	{
		FNode N;
		N.Id = Id;
		N.A = A;
		N.B = B;
		N.Role = Role;
		N.bGrounded = bGrounded;
		N.RadiusCm = Radius;
		N.LengthModules = Modules;
		return N;
	}

	TArray<WoodEdge> Wire(const TArray<FNode>& Nodes)
	{
		TArray<WoodEdge> Edges;
		TArray<FNode> Placed;
		for (const FNode& N : Nodes)
		{
			FindConnections(N, Placed, 8.f, Edges);
			Placed.Add(N);
		}
		return Edges;
	}

	bool Has(const TArray<FFeature>& Features, EWoodFeatureKind Kind, int32 MinNodes = 1)
	{
		for (const FFeature& F : Features)
		{
			if (F.Kind == Kind && F.NodeIds.Num() >= MinNodes)
			{
				return true;
			}
		}
		return false;
	}

	/** Dikey direkler Y = 0 doğrusunda, ilk X'ten başlayıp Spacing aralıkla. */
	void AddPosts(TArray<FNode>& Nodes, int32& Id, float FirstX, int32 Count, float Spacing, EWoodRole Role, float Height = 200.f)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Nodes.Add(Mk(Id++, FVector(FirstX + Index * Spacing, 0.f, 0.f), FVector(FirstX + Index * Spacing, 0.f, Height), Role, true));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisWallsTest, "MyProject2.Wood.Analysis.Walls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisWallsTest::RunTest(const FString& Parameters)
{
	// Yan yana duran dört direk: dikey duvar. İki direk duvar değil.
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		AddPosts(Ns, Id, 0.f, 4, 32.f, EWoodRole::Post);
		TestTrue(TEXT("4 direk dikey duvar"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::VerticalWall, 4));
		TArray<FNode> Two;
		Id = 0;
		AddPosts(Two, Id, 0.f, 2, 32.f, EWoodRole::Post);
		TestFalse(TEXT("2 direk duvar değil"), Has(AnalyzeStructure(Two, Wire(Two)), EWoodFeatureKind::VerticalWall));
	}
	// Aralıklı direkler duvar değil.
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		AddPosts(Ns, Id, 0.f, 4, 80.f, EWoodRole::Post);
		TestFalse(TEXT("aralıklı direkler duvar değil"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::VerticalWall));
	}
	// Sivri uçlu direkler: kazık duvarı; duvara değen bir çapraz: savunma duvarı.
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		AddPosts(Ns, Id, 0.f, 4, 32.f, EWoodRole::Stake);
		TArray<FFeature> Stake = AnalyzeStructure(Ns, Wire(Ns));
		TestTrue(TEXT("sivri direkler kazık duvarı"), Has(Stake, EWoodFeatureKind::StakeWall, 4));
		TestFalse(TEXT("payandasız savunma duvarı değil"), Has(Stake, EWoodFeatureKind::DefenseWall));
		Ns.Add(Mk(Id++, FVector(-60.f, 0.f, 0.f), FVector(0.f, 0.f, 110.f), EWoodRole::Brace, false));
		TestTrue(TEXT("payandalı kazık duvarı savunma duvarı"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::DefenseWall, 5));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisOpeningsTest, "MyProject2.Wood.Analysis.Openings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisOpeningsTest::RunTest(const FString& Parameters)
{
	// 3 + 3 direk, aralarında 100 cm boşluk (eksenler 130 cm).
	auto Wall = [](TArray<FNode>& Ns, int32& Id)
	{
		AddPosts(Ns, Id, 0.f, 3, 32.f, EWoodRole::Post);
		AddPosts(Ns, Id, 64.f + 130.f, 3, 32.f, EWoodRole::Post);
	};
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		Wall(Ns, Id);
		const TArray<FFeature> F = AnalyzeStructure(Ns, Wire(Ns));
		TestTrue(TEXT("kasasız açıklık"), Has(F, EWoodFeatureKind::Opening));
		TestFalse(TEXT("lentosuz kapı değil"), Has(F, EWoodFeatureKind::DoorFrame));
	}
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		Wall(Ns, Id);
		Ns.Add(Mk(Id++, FVector(40.f, 0.f, 215.f), FVector(240.f, 0.f, 215.f), EWoodRole::Beam, false));
		const TArray<FFeature> F = AnalyzeStructure(Ns, Wire(Ns));
		TestTrue(TEXT("lento: kapı kasası"), Has(F, EWoodFeatureKind::DoorFrame, 3));
		TestFalse(TEXT("denizliksiz pencere değil"), Has(F, EWoodFeatureKind::WindowFrame));
		Ns.Add(Mk(Id++, FVector(64.f, 0.f, 90.f), FVector(194.f, 0.f, 90.f), EWoodRole::Beam, false));
		TestTrue(TEXT("lento + denizlik: pencere kasası"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::WindowFrame, 4));
	}
	// Boşluk çok dar (50 cm) ya da çok geniş (500 cm): açıklık değil.
	for (const float Centers : { 80.f, 560.f })
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		AddPosts(Ns, Id, 0.f, 3, 32.f, EWoodRole::Post);
		AddPosts(Ns, Id, 64.f + Centers, 3, 32.f, EWoodRole::Post);
		TestFalse(TEXT("sınır dışı boşluk açıklık değil"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::Opening));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisFloorsTest, "MyProject2.Wood.Analysis.Floors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisFloorsTest::RunTest(const FString& Parameters)
{
	// Dört direk, iki kiriş (y = 0 ve 195), iki döşeme tahtası: platform. Üstüne korkuluk.
	TArray<FNode> Ns;
	int32 Id = 0;
	for (const float Y : { 0.f, 195.f })
	{
		Ns.Add(Mk(Id++, FVector(0.f, Y, 0.f), FVector(0.f, Y, 130.f), EWoodRole::Post, true));
		Ns.Add(Mk(Id++, FVector(240.f, Y, 0.f), FVector(240.f, Y, 130.f), EWoodRole::Post, true));
	}
	for (const float Y : { 0.f, 195.f })
	{
		Ns.Add(Mk(Id++, FVector(-35.f, Y, 145.f), FVector(355.f, Y, 145.f), EWoodRole::Beam, false));
	}
	Ns.Add(Mk(Id++, FVector(60.f, -60.f, 170.f), FVector(60.f, 255.f, 170.f), EWoodRole::Floor, false));
	Ns.Add(Mk(Id++, FVector(140.f, -60.f, 170.f), FVector(140.f, 255.f, 170.f), EWoodRole::Floor, false));
	TArray<FFeature> F = AnalyzeStructure(Ns, Wire(Ns));
	TestTrue(TEXT("platform"), Has(F, EWoodFeatureKind::Platform, 8));

	// Döşemesiz kirişler platform değil.
	TArray<FNode> NoFloor = Ns;
	NoFloor.SetNum(Ns.Num() - 2);
	TestFalse(TEXT("döşemesiz platform değil"), Has(AnalyzeStructure(NoFloor, Wire(NoFloor)), EWoodFeatureKind::Platform));

	// Platformun üstünde kısa iki direk ve onları bağlayan kiriş: korkuluk.
	for (const float X : { 60.f, 140.f })
	{
		Ns.Add(Mk(Id++, FVector(X, 0.f, 170.f), FVector(X, 0.f, 260.f), EWoodRole::Post, false));
	}
	Ns.Add(Mk(Id++, FVector(0.f, 0.f, 275.f), FVector(240.f, 0.f, 275.f), EWoodRole::Beam, false));
	F = AnalyzeStructure(Ns, Wire(Ns));
	TestTrue(TEXT("platformda korkuluk"), Has(F, EWoodFeatureKind::Railing, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisCabinTest, "MyProject2.Wood.Analysis.Cabin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisCabinTest::RunTest(const FString& Parameters)
{
	// Üç kat: Y boyunca iki kütük, X boyunca iki kütük (çentikli), yine Y boyunca iki kütük.
	auto Cabin = [](int32 Layers)
	{
		TArray<FNode> Ns;
		int32 Id = 0;
		for (int32 Layer = 0; Layer < Layers; ++Layer)
		{
			const float Z = 33.f + 36.f * Layer;
			for (int32 Index = 0; Index < 2; ++Index)
			{
				FNode N;
				if (Layer % 2 == 0)
				{
					N = Mk(Id++, FVector(260.f * Index, -195.f, Z), FVector(260.f * Index, 195.f, Z), EWoodRole::Beam, Layer == 0, 35.f);
				}
				else
				{
					N = Mk(Id++, FVector(-65.f, -130.f + 260.f * Index, Z), FVector(325.f, -130.f + 260.f * Index, Z), EWoodRole::Beam, false, 35.f);
				}
				if (Layer > 0)
				{
					N.Notches = { FWoodNotch{ 1, EWoodJointType::Saddle }, FWoodNotch{ 5, EWoodJointType::Saddle } };
				}
				Ns.Add(N);
			}
		}
		return Ns;
	};
	{
		TArray<FNode> Ns = Cabin(3);
		const TArray<FFeature> F = AnalyzeStructure(Ns, Wire(Ns));
		TestTrue(TEXT("üç kat: yatay kütük duvarı"), Has(F, EWoodFeatureKind::HorizontalWall, 6));
	}
	{
		TArray<FNode> Ns = Cabin(2);
		TestFalse(TEXT("iki kat duvar değil, temel çerçevesi"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::HorizontalWall));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisSlopesTest, "MyProject2.Wood.Analysis.Slopes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisSlopesTest::RunTest(const FString& Parameters)
{
	// Rampa: zemine oturan yatık eğik parça. Dik eğik parça rampa değil.
	{
		TArray<FNode> Ns = { Mk(0, FVector(0.f, 0.f, 0.f), FVector(300.f, 0.f, 120.f), EWoodRole::Roof, true) };
		TestTrue(TEXT("rampa"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::Ramp));
		TArray<FNode> Steep = { Mk(0, FVector(0.f, 0.f, 0.f), FVector(100.f, 0.f, 200.f), EWoodRole::Brace, true) };
		TestFalse(TEXT("dik eğik parça rampa değil"), Has(AnalyzeStructure(Steep, Wire(Steep)), EWoodFeatureKind::Ramp));
	}
	// Merdiven: eğik taşıyıcı ve üç ayrı yükseklikte basamak.
	{
		TArray<FNode> Ns;
		Ns.Add(Mk(0, FVector(0.f, 0.f, 0.f), FVector(200.f, 0.f, 120.f), EWoodRole::Brace, true));
		int32 Id = 1;
		for (const float X : { 50.f, 100.f, 150.f })
		{
			Ns.Add(Mk(Id++, FVector(X, -40.f, 0.6f * X + 25.f), FVector(X, 40.f, 0.6f * X + 25.f), EWoodRole::Floor, false));
		}
		const TArray<FFeature> F = AnalyzeStructure(Ns, Wire(Ns));
		TestTrue(TEXT("merdiven"), Has(F, EWoodFeatureKind::Stairs, 4));
		TestFalse(TEXT("basamaklı taşıyıcı rampa değil"), Has(F, EWoodFeatureKind::Ramp));
	}
	// Köprü: 300 cm kiriş, iki ucunda zemindeki direkler, ortası boş. Ortada direk varsa köprü değil.
	{
		TArray<FNode> Ns;
		Ns.Add(Mk(0, FVector(15.f, 0.f, 0.f), FVector(15.f, 0.f, 85.f), EWoodRole::Post, true));
		Ns.Add(Mk(1, FVector(285.f, 0.f, 0.f), FVector(285.f, 0.f, 85.f), EWoodRole::Post, true));
		Ns.Add(Mk(2, FVector(0.f, 0.f, 100.f), FVector(300.f, 0.f, 100.f), EWoodRole::Beam, false));
		TestTrue(TEXT("köprü"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::Bridge, 3));
		Ns.Add(Mk(3, FVector(150.f, 0.f, 0.f), FVector(150.f, 0.f, 85.f), EWoodRole::Post, true));
		TestFalse(TEXT("ortada direk: köprü değil"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::Bridge));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisRoofTest, "MyProject2.Wood.Analysis.Roof",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisRoofTest::RunTest(const FString& Parameters)
{
	// İki karşılıklı çapraz tepede buluşur: çatı iskeleti. Sırt kirişi ve iki tahta: kaplama.
	TArray<FNode> Ns;
	Ns.Add(Mk(0, FVector(0.f, 0.f, 0.f), FVector(100.f, 0.f, 100.f), EWoodRole::Brace, false));
	Ns.Add(Mk(1, FVector(200.f, 0.f, 0.f), FVector(100.f, 0.f, 100.f), EWoodRole::Brace, false));
	TestTrue(TEXT("çatı iskeleti"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::RoofFrame, 2));
	// Aynı yöne bakan iki çapraz çatı iskeleti değil.
	TArray<FNode> Same;
	Same.Add(Mk(0, FVector(0.f, 0.f, 0.f), FVector(100.f, 0.f, 100.f), EWoodRole::Brace, false));
	Same.Add(Mk(1, FVector(0.f, 40.f, 0.f), FVector(100.f, 40.f, 100.f), EWoodRole::Brace, false));
	TestFalse(TEXT("paralel çaprazlar iskelet değil"), Has(AnalyzeStructure(Same, Wire(Same)), EWoodFeatureKind::RoofFrame));

	Ns.Add(Mk(2, FVector(100.f, -100.f, 100.f), FVector(100.f, 200.f, 100.f), EWoodRole::Beam, false));
	Ns.Add(Mk(3, FVector(100.f, 0.f, 125.f), FVector(210.f, 0.f, 15.f), EWoodRole::Roof, false));
	Ns.Add(Mk(4, FVector(100.f, 40.f, 125.f), FVector(210.f, 40.f, 15.f), EWoodRole::Roof, false));
	TestTrue(TEXT("çatı kaplaması"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::RoofSurface, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodAnalysisScaffoldTest, "MyProject2.Wood.Analysis.Scaffold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWoodAnalysisScaffoldTest::RunTest(const FString& Parameters)
{
	TArray<FNode> Ns;
	Ns.Add(Mk(0, FVector(0.f, 0.f, 0.f), FVector(0.f, 0.f, 150.f), EWoodRole::Post, true));
	Ns.Add(Mk(1, FVector(-35.f, 0.f, 165.f), FVector(100.f, 0.f, 165.f), EWoodRole::Beam, false));
	for (FNode& N : Ns)
	{
		N.bTemporary = true;
	}
	TestTrue(TEXT("geçici parçalar iskele"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::Scaffold, 2));
	for (FNode& N : Ns)
	{
		N.bTemporary = false;
	}
	TestFalse(TEXT("kalıcı parçalar iskele değil"), Has(AnalyzeStructure(Ns, Wire(Ns)), EWoodFeatureKind::Scaffold));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
