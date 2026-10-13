// Faz 3 - Yapı mantığı otomasyon testleri. Editör: Session Frontend > Automation > "MyProject2.Wood.Structure"
#include "Misc/AutomationTest.h"
#include "WoodStructureLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WoodStructure;
// The engine has a global FEdge too: always mean the structure graph edge here.
using WoodEdge = WoodStructure::FEdge;

namespace
{
	FNode MkNode(int32 Id, const FVector& A, const FVector& B, EWoodRole Role, bool bGrounded, float Radius = 15.f)
	{
		FNode N;
		N.Id = Id;
		N.A = A;
		N.B = B;
		N.Role = Role;
		N.bGrounded = bGrounded;
		N.RadiusCm = Radius;
		N.LengthModules = 6;
		return N;
	}

	/** Düğümleri sırayla yerleştirir; her biri öncekilerle bağlanır (kurulum sırası). */
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodStructureOrientationRoleTest, "MyProject2.Wood.Structure.OrientationAndRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWoodStructureOrientationRoleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Z ekseni dikey"), ClassifyOrientation(FVector(0, 0, 1)) == EWoodOrientation::Vertical);
	TestTrue(TEXT("X ekseni yatay"), ClassifyOrientation(FVector(1, 0, 0)) == EWoodOrientation::Horizontal);
	TestTrue(TEXT("45 derece çapraz"), ClassifyOrientation(FVector(1, 0, 1)) == EWoodOrientation::Diagonal);
	TestTrue(TEXT("Hafif eğim hâlâ yatay"), ClassifyOrientation(FVector(1, 0, 0.2)) == EWoodOrientation::Horizontal);
	TestTrue(TEXT("Ters dikey"), ClassifyOrientation(FVector(0, 0, -1)) == EWoodOrientation::Vertical);

	FWoodPieceData Round;
	TestTrue(TEXT("Dikey kütük direk"), DeriveRole(Round, EWoodOrientation::Vertical) == EWoodRole::Post);
	TestTrue(TEXT("Yatay kütük kiriş"), DeriveRole(Round, EWoodOrientation::Horizontal) == EWoodRole::Beam);
	TestTrue(TEXT("Çapraz kütük payanda"), DeriveRole(Round, EWoodOrientation::Diagonal) == EWoodRole::Brace);

	FWoodPieceData Stake = Round;
	Stake.EndA = EWoodEndShape::Pointed;
	TestTrue(TEXT("Sivri dikey kütük kazık"), DeriveRole(Stake, EWoodOrientation::Vertical) == EWoodRole::Stake);

	FWoodPieceData Plank = Round;
	Plank.CrossSection = EWoodCrossSection::Plank;
	TestTrue(TEXT("Yatay tahta döşeme"), DeriveRole(Plank, EWoodOrientation::Horizontal) == EWoodRole::Floor);
	TestTrue(TEXT("Dikey tahta duvar"), DeriveRole(Plank, EWoodOrientation::Vertical) == EWoodRole::Wall);
	TestTrue(TEXT("Çapraz tahta çatı"), DeriveRole(Plank, EWoodOrientation::Diagonal) == EWoodRole::Roof);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodStructureSupportTest, "MyProject2.Wood.Structure.SupportPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWoodStructureSupportTest::RunTest(const FString& Parameters)
{
	// İki direk + kiriş: bir direk kalkarsa kiriş düşer.
	{
		TArray<FNode> Ns;
		Ns.Add(MkNode(1, FVector(0, 0, 0), FVector(0, 0, 300), EWoodRole::Post, true));
		Ns.Add(MkNode(2, FVector(300, 0, 0), FVector(300, 0, 300), EWoodRole::Post, true));
		Ns.Add(MkNode(3, FVector(0, 0, 300), FVector(300, 0, 300), EWoodRole::Beam, false));
		const TArray<WoodEdge> Es = Wire(Ns);
		TestEqual(TEXT("Kiriş iki direğe bağlı"), Es.Num(), 2);
		TestTrue(TEXT("Kiriş destekli"), ComputeSupport(Ns, Es).Contains(3));

		const TArray<int32> Dep = FindDependentsOfRemoval(Ns, Es, 1);
		TestEqual(TEXT("Direk kalkınca bir parça etkilenir"), Dep.Num(), 1);
		if (Dep.Num() == 1)
		{
			TestEqual(TEXT("Etkilenen kiriş"), Dep[0], 3);
		}
	}

	// Ara direk (mekanik 55): ortadaki direk kalkınca kiriş düşmemeli.
	{
		TArray<FNode> Ns;
		Ns.Add(MkNode(1, FVector(0, 0, 0), FVector(0, 0, 300), EWoodRole::Post, true));
		Ns.Add(MkNode(2, FVector(300, 0, 0), FVector(300, 0, 300), EWoodRole::Post, true));
		Ns.Add(MkNode(4, FVector(150, 0, 0), FVector(150, 0, 300), EWoodRole::Post, true));
		Ns.Add(MkNode(3, FVector(0, 0, 300), FVector(300, 0, 300), EWoodRole::Beam, false));
		const TArray<WoodEdge> Es = Wire(Ns);
		TestEqual(TEXT("Kiriş üç direğe bağlı"), Es.Num(), 3);
		TestEqual(TEXT("Ara direk güvenle kalkar"), FindDependentsOfRemoval(Ns, Es, 4).Num(), 0);
		TestEqual(TEXT("Uç direk de güvenle kalkar"), FindDependentsOfRemoval(Ns, Es, 1).Num(), 0);
	}

	// Döngüsel destek geçersiz: birbirini tutan, zemine bağlanmayan kirişler havada kalır.
	{
		TArray<FNode> Ns;
		Ns.Add(MkNode(1, FVector(0, 0, 300), FVector(300, 0, 300), EWoodRole::Beam, false));
		Ns.Add(MkNode(2, FVector(300, 0, 300), FVector(300, 300, 300), EWoodRole::Beam, false));
		Ns.Add(MkNode(3, FVector(300, 300, 300), FVector(0, 300, 300), EWoodRole::Beam, false));
		Ns.Add(MkNode(4, FVector(0, 300, 300), FVector(0, 0, 300), EWoodRole::Beam, false));
		const TArray<WoodEdge> Es = Wire(Ns);
		const TSet<int32> S = ComputeSupport(Ns, Es);
		TestEqual(TEXT("Zemine bağlanmayan çerçeve desteksiz"), S.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodStructureConnectionTest, "MyProject2.Wood.Structure.Connections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWoodStructureConnectionTest::RunTest(const FString& Parameters)
{
	// Sadece yan yana durmak bağlantı değildir.
	{
		TArray<FNode> Ns;
		Ns.Add(MkNode(1, FVector(0, 0, 0), FVector(300, 0, 0), EWoodRole::Beam, true));
		Ns.Add(MkNode(2, FVector(0, 30, 0), FVector(300, 30, 0), EWoodRole::Beam, false));
		TestEqual(TEXT("Çentiksiz paralel parçalar bağlanmaz"), Wire(Ns).Num(), 0);
	}

	// Çentikli geçme: üstteki alttakine oturur, tek yönlü.
	{
		TArray<FNode> Ns;
		Ns.Add(MkNode(1, FVector(-150, 0, 0), FVector(150, 0, 0), EWoodRole::Beam, true));
		Ns.Add(MkNode(2, FVector(0, -150, 30), FVector(0, 150, 30), EWoodRole::Beam, false));
		TestEqual(TEXT("Çentik yokken bağlantı yok"), Wire(Ns).Num(), 0);

		FWoodNotch Notch;
		Notch.ModuleLine = 3;
		Notch.JointType = EWoodJointType::Saddle;
		Ns[1].Notches.Add(Notch);

		const TArray<WoodEdge> Es = Wire(Ns);
		TestEqual(TEXT("Çentikle bağlantı oluşur"), Es.Num(), 1);
		if (Es.Num() == 1)
		{
			TestTrue(TEXT("Türü Notched"), Es[0].Kind == EWoodConnectionKind::Notched);
			TestTrue(TEXT("Tek yönlü"), Es[0].bOneWay);
			TestEqual(TEXT("Üstteki taşınan"), Es[0].A, 2);
			TestEqual(TEXT("Alttaki taşıyan"), Es[0].B, 1);
		}
	}

	// Döşeme iki kirişe yaslanır; kirişleri taşımaz.
	{
		TArray<FNode> Ns;
		Ns.Add(MkNode(1, FVector(0, -100, 0), FVector(0, 100, 0), EWoodRole::Beam, true));
		Ns.Add(MkNode(2, FVector(300, -100, 0), FVector(300, 100, 0), EWoodRole::Beam, true));
		Ns.Add(MkNode(3, FVector(-50, 0, 28), FVector(350, 0, 28), EWoodRole::Floor, false));
		const TArray<WoodEdge> Es = Wire(Ns);
		TestEqual(TEXT("Döşeme iki kirişe yaslanır"), Es.Num(), 2);
		TestTrue(TEXT("Döşeme destekli"), ComputeSupport(Ns, Es).Contains(3));

		TArray<FNode> Floating = Ns;
		Floating[0].bGrounded = false;
		Floating[1].bGrounded = false;
		TestFalse(TEXT("Döşeme kirişleri taşımaz"), ComputeSupport(Floating, Es).Contains(3));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWoodStructureSnapTest, "MyProject2.Wood.Structure.Snap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWoodStructureSnapTest::RunTest(const FString& Parameters)
{
	// Uç - uç
	{
		TArray<FNode> Ex;
		Ex.Add(MkNode(1, FVector(0, 0, 0), FVector(0, 0, 300), EWoodRole::Post, true));
		const FNode P = MkNode(9, FVector(20, 5, 310), FVector(320, 5, 310), EWoodRole::Beam, false);

		const FSnap S = FindBestSnap(P, Ex, 40.f);
		TestTrue(TEXT("Snap bulundu"), S.bFound);
		TestEqual(TEXT("Hedef direk"), S.TargetId, 1);
		TestTrue(TEXT("Uç-uç"), S.Kind == EWoodConnectionKind::EndToEnd);
		TestTrue(TEXT("A ucu direk tepesine oturur"), FVector::Dist(P.A + S.Offset, FVector(0, 0, 300)) < 0.01);
	}

	// Uç - gövde: modül çizgisine oturur (390 cm kiriş, 65 cm modül; 200 -> 195)
	{
		TArray<FNode> Ex;
		Ex.Add(MkNode(1, FVector(0, 0, 300), FVector(390, 0, 300), EWoodRole::Beam, false));
		const FNode Post = MkNode(9, FVector(200, 3, 0), FVector(200, 3, 290), EWoodRole::Post, false);

		const FSnap S = FindBestSnap(Post, Ex, 40.f);
		TestTrue(TEXT("Uç-gövde snap"), S.bFound && S.Kind == EWoodConnectionKind::EndToBody);
		const FVector Top = Post.B + S.Offset;
		TestTrue(TEXT("Modül çizgisine oturdu"), FMath::Abs(Top.X - 195.0) < 0.01 && FMath::Abs(Top.Z - 300.0) < 0.01);
		TestTrue(TEXT("Dikey uç yatay gövdeye oval çentik önerir"), S.SuggestedJoint == EWoodJointType::Oval);
	}

	// Uzaktaysa snap yok
	{
		TArray<FNode> Ex;
		Ex.Add(MkNode(1, FVector(0, 0, 0), FVector(0, 0, 300), EWoodRole::Post, true));
		const FNode P = MkNode(9, FVector(500, 500, 0), FVector(500, 500, 300), EWoodRole::Post, false);
		TestFalse(TEXT("Uzakta snap yok"), FindBestSnap(P, Ex, 40.f).bFound);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
