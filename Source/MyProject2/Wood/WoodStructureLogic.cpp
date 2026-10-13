#include "WoodStructureLogic.h"
#include "WoodStructureGeometry.h"

namespace WoodStructure
{
	namespace
	{
		using namespace Geo;

		/** Notch.ModuleLine bu kadar modül çizgisi içindeyse temas noktasında sayılır. */
		constexpr double NotchLineTolerance = 0.75;

		/** Taşıyan/taşınan yönü ayırt etmek için yükseklik farkı eşiği (cm). */
		constexpr double HeightTieCm = 0.5;

		bool HasNotchNear(const FNode& N, double Param)
		{
			const double Line = Param * static_cast<double>(FMath::Max(N.LengthModules, 1));
			for (const FWoodNotch& Notch : N.Notches)
			{
				if (Notch.JointType != EWoodJointType::None && FMath::Abs(static_cast<double>(Notch.ModuleLine) - Line) <= NotchLineTolerance)
				{
					return true;
				}
			}
			return false;
		}

		bool IsSurfaceRole(EWoodRole Role)
		{
			return Role == EWoodRole::Floor || Role == EWoodRole::Wall || Role == EWoodRole::Roof;
		}

		int32 RequiredSupports(const FNode& N)
		{
			// Yatay taşıyıcılar ve yüzeyler tek noktadan değil, en az iki noktadan taşınır.
			return (N.Role == EWoodRole::Beam || N.Role == EWoodRole::Floor || N.Role == EWoodRole::Roof) ? 2 : 1;
		}

		bool AlreadyConnected(const TArray<FEdge>& Edges, int32 X, int32 Y)
		{
			for (const FEdge& E : Edges)
			{
				if ((E.A == X && E.B == Y) || (E.A == Y && E.B == X))
				{
					return true;
				}
			}
			return false;
		}

		FEdge MakeEdge(int32 IdX, int32 IdY, EWoodConnectionKind Kind, double DeltaX, double DeltaY, double Tol)
		{
			// Delta: bağlantı noktasından o parçanın DİĞER ucuna yükseklik farkı. Diğer ucu daha yüksek olan parça üstte oturur (taşınan).
			FEdge E;
			E.Kind = Kind;
			if (FMath::Abs(DeltaX - DeltaY) <= Tol)
			{
				E.A = IdX;
				E.B = IdY;
				E.bOneWay = false;
			}
			else if (DeltaX > DeltaY)
			{
				E.A = IdX; // X taşınan
				E.B = IdY;
				E.bOneWay = true;
			}
			else
			{
				E.A = IdY;
				E.B = IdX;
				E.bOneWay = true;
			}
			return E;
		}

		bool TryConnect(const FNode& N, const FNode& E, double ContactTol, double OrientTol, FEdge& Out)
		{
			const double DeltaTol = FMath::Max(ContactTol * 0.5, 1.0);
			const FVector NEnds[2] = { N.A, N.B };
			const FVector EEnds[2] = { E.A, E.B };

			// 1) uç - uç
			{
				double Best = TNumericLimits<double>::Max();
				int32 BI = 0, BJ = 0;
				for (int32 I = 0; I < 2; ++I)
				{
					for (int32 J = 0; J < 2; ++J)
					{
						const double D = FVector::Dist(NEnds[I], EEnds[J]);
						if (D < Best)
						{
							Best = D;
							BI = I;
							BJ = J;
						}
					}
				}
				if (Best <= ContactTol)
				{
					const double ContactZ = 0.5 * (NEnds[BI].Z + EEnds[BJ].Z);
					const double DN = NEnds[1 - BI].Z - ContactZ;
					const double DE = EEnds[1 - BJ].Z - ContactZ;
					Out = MakeEdge(N.Id, E.Id, EWoodConnectionKind::EndToEnd, DN, DE, DeltaTol);
					return true;
				}
			}

			// 2) uç - gövde
			{
				double BestMargin = TNumericLimits<double>::Max();
				bool bFound = false;
				bool bVisitorIsN = true;
				int32 VisitorEnd = 0;
				FVector Contact = FVector::ZeroVector;

				for (int32 I = 0; I < 2; ++I)
				{
					double T = 0.0;
					const double D = DistToSegment(NEnds[I], E.A, E.B, T);
					if (D <= E.RadiusCm + ContactTol && D < BestMargin)
					{
						BestMargin = D;
						bFound = true;
						bVisitorIsN = true;
						VisitorEnd = I;
						Contact = PointAt(E.A, E.B, T);
					}
				}
				for (int32 J = 0; J < 2; ++J)
				{
					double T = 0.0;
					const double D = DistToSegment(EEnds[J], N.A, N.B, T);
					if (D <= N.RadiusCm + ContactTol && D < BestMargin)
					{
						BestMargin = D;
						bFound = true;
						bVisitorIsN = false;
						VisitorEnd = J;
						Contact = PointAt(N.A, N.B, T);
					}
				}

				if (bFound)
				{
					// Ziyaretçi: ucu değen parça. Diğer ucu temas noktasından yüksekteyse ziyaretçi gövdeye oturur.
					const FVector VisitorFar = bVisitorIsN ? NEnds[1 - VisitorEnd] : EEnds[1 - VisitorEnd];
					const double Delta = VisitorFar.Z - Contact.Z;
					const int32 VisitorId = bVisitorIsN ? N.Id : E.Id;
					const int32 BodyId = bVisitorIsN ? E.Id : N.Id;

					FEdge Edge;
					Edge.Kind = EWoodConnectionKind::EndToBody;
					if (Delta > DeltaTol)
					{
						Edge.A = VisitorId;
						Edge.B = BodyId;
						Edge.bOneWay = true;
					}
					else if (Delta < -DeltaTol)
					{
						Edge.A = BodyId;
						Edge.B = VisitorId;
						Edge.bOneWay = true;
					}
					else
					{
						Edge.A = VisitorId;
						Edge.B = BodyId;
						Edge.bOneWay = false;
					}
					Out = Edge;
					return true;
				}
			}

			// 3) gövde - gövde çaprazı: çentik veya yaslanan tahta gerekir
			{
				const double CosTol = FMath::Cos(FMath::DegreesToRadians(OrientTol));
				const double Parallel = FMath::Abs(FVector::DotProduct(N.Axis(), E.Axis()));
				if (Parallel >= CosTol)
				{
					return false; // yan yana paralel parçalar bağlanmaz
				}

				double S = 0.0, T = 0.0;
				ClosestSegSeg(N.A, N.B, E.A, E.B, S, T);
				const FVector PN = PointAt(N.A, N.B, S);
				const FVector PE = PointAt(E.A, E.B, T);
				if (FVector::Dist(PN, PE) > N.RadiusCm + E.RadiusCm + ContactTol)
				{
					return false;
				}

				const double Dz = PN.Z - PE.Z;
				const bool bNotched = HasNotchNear(N, S) || HasNotchNear(E, T);

				if (bNotched)
				{
					FEdge Edge;
					Edge.Kind = EWoodConnectionKind::Notched;
					if (Dz > HeightTieCm)
					{
						Edge.A = N.Id; // üstteki taşınan
						Edge.B = E.Id;
						Edge.bOneWay = true;
					}
					else if (Dz < -HeightTieCm)
					{
						Edge.A = E.Id;
						Edge.B = N.Id;
						Edge.bOneWay = true;
					}
					else
					{
						Edge.A = N.Id; // aynı yükseklikte geçme: karşılıklı
						Edge.B = E.Id;
						Edge.bOneWay = false;
					}
					Out = Edge;
					return true;
				}

				// Çentiksiz: üstteki yüzey parçasıysa (döşeme/duvar/çatı) taşıyıcının üstüne yaslanır.
				if (Dz > HeightTieCm && IsSurfaceRole(N.Role))
				{
					Out.A = N.Id;
					Out.B = E.Id;
					Out.Kind = EWoodConnectionKind::Resting;
					Out.bOneWay = true;
					return true;
				}
				if (Dz < -HeightTieCm && IsSurfaceRole(E.Role))
				{
					Out.A = E.Id;
					Out.B = N.Id;
					Out.Kind = EWoodConnectionKind::Resting;
					Out.bOneWay = true;
					return true;
				}
			}
			return false;
		}

		EWoodJointType SuggestJoint(EWoodConnectionKind Kind, EWoodOrientation Visitor, EWoodOrientation Body)
		{
			if (Kind == EWoodConnectionKind::EndToEnd)
			{
				return Visitor == Body ? EWoodJointType::None : EWoodJointType::Corner;
			}
			// uç - gövde
			if (Visitor == EWoodOrientation::Vertical && Body == EWoodOrientation::Horizontal)
			{
				return EWoodJointType::Oval; // direk ucu kirişin yuvarlak yatağına
			}
			if (Visitor == EWoodOrientation::Horizontal && Body == EWoodOrientation::Horizontal)
			{
				return EWoodJointType::Saddle; // üst üste çaprazlanan kütükler
			}
			if (Visitor == EWoodOrientation::Diagonal || Body == EWoodOrientation::Vertical)
			{
				return EWoodJointType::FlatLap; // payanda ucu / direğin yanına dayanan kiriş
			}
			return EWoodJointType::None;
		}
	}

	// ------------------------------------------------------------------ yön ve rol

	EWoodOrientation ClassifyOrientation(const FVector& Axis, float OrientTolDeg)
	{
		const FVector D = Axis.GetSafeNormal();
		if (D.IsNearlyZero())
		{
			return EWoodOrientation::Horizontal;
		}
		const double Z = ClampD(FMath::Abs(D.Z), 0.0, 1.0);
		const double FromHorizontal = FMath::RadiansToDegrees(FMath::Asin(Z));
		if (FromHorizontal <= OrientTolDeg)
		{
			return EWoodOrientation::Horizontal;
		}
		if (90.0 - FromHorizontal <= OrientTolDeg)
		{
			return EWoodOrientation::Vertical;
		}
		return EWoodOrientation::Diagonal;
	}

	EWoodRole DeriveRole(const FWoodPieceData& Data, EWoodOrientation Orientation)
	{
		// Düz yüzeyli parçalar (tahta, yarım kütük) yüzey kapatır: bölüm D, mekanik 38.
		const bool bFlat = Data.CrossSection == EWoodCrossSection::Plank || Data.CrossSection == EWoodCrossSection::Half;
		if (bFlat)
		{
			switch (Orientation)
			{
			case EWoodOrientation::Horizontal: return EWoodRole::Floor;
			case EWoodOrientation::Vertical:   return EWoodRole::Wall;
			default:                           return EWoodRole::Roof;
			}
		}

		switch (Orientation)
		{
		case EWoodOrientation::Vertical:
			return (Data.EndA == EWoodEndShape::Pointed || Data.EndB == EWoodEndShape::Pointed) ? EWoodRole::Stake : EWoodRole::Post;
		case EWoodOrientation::Horizontal:
			return EWoodRole::Beam;
		default:
			return EWoodRole::Brace;
		}
	}

	// ------------------------------------------------------------------ bağlantılar

	void FindConnections(const FNode& NewNode, const TArray<FNode>& Existing, float ContactTolCm, TArray<FEdge>& OutEdges, float OrientTolDeg)
	{
		for (const FNode& Other : Existing)
		{
			if (Other.Id == NewNode.Id || AlreadyConnected(OutEdges, NewNode.Id, Other.Id))
			{
				continue;
			}
			FEdge Edge;
			if (TryConnect(NewNode, Other, ContactTolCm, OrientTolDeg, Edge))
			{
				OutEdges.Add(Edge);
			}
		}
	}

	// ------------------------------------------------------------------ destek

	TSet<int32> ComputeSupport(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges)
	{
		TMap<int32, const FNode*> ById;
		for (const FNode& N : Nodes)
		{
			ById.Add(N.Id, &N);
		}

		auto DamageOf = [&ById](int32 Id)
		{
			const FNode* const* P = ById.Find(Id);
			return P ? (*P)->Damage : 0.f;
		};

		// Gevşemiş eklem destek aktarmaz; yaslanma (tahta) eklem değildir, etkilenmez.
		auto JointHolds = [&DamageOf](const FEdge& E)
		{
			return E.Kind == EWoodConnectionKind::Resting || FMath::Max(DamageOf(E.A), DamageOf(E.B)) < JointLooseDamage;
		};

		TSet<int32> Supported;
		for (const FNode& N : Nodes)
		{
			if (N.bGrounded && N.Damage < FailureDamage)
			{
				Supported.Add(N.Id);
			}
		}

		// En küçük sabit nokta: zeminden başlayıp yayılır, bu yüzden döngüsel destek kendini ayakta tutamaz.
		bool bChanged = true;
		while (bChanged)
		{
			bChanged = false;
			for (const FNode& N : Nodes)
			{
				if (Supported.Contains(N.Id) || N.Damage >= FailureDamage)
				{
					continue; // kopmuş parça hiçbir zaman destekli sayılmaz
				}

				TSet<int32> Supporters;
				for (const FEdge& E : Edges)
				{
					if (!JointHolds(E))
					{
						continue;
					}
					if (E.A == N.Id && Supported.Contains(E.B))
					{
						Supporters.Add(E.B);
					}
					else if (!E.bOneWay && E.B == N.Id && Supported.Contains(E.A))
					{
						Supporters.Add(E.A);
					}
				}

				if (Supporters.Num() >= RequiredSupports(N))
				{
					Supported.Add(N.Id);
					bChanged = true;
				}
			}
		}
		return Supported;
	}

	TArray<int32> FindDependentsOfRemoval(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges, int32 RemovedId)
	{
		const TSet<int32> Before = ComputeSupport(Nodes, Edges);

		TArray<FNode> RestNodes;
		RestNodes.Reserve(Nodes.Num());
		for (const FNode& N : Nodes)
		{
			if (N.Id != RemovedId)
			{
				RestNodes.Add(N);
			}
		}
		TArray<FEdge> RestEdges;
		RestEdges.Reserve(Edges.Num());
		for (const FEdge& E : Edges)
		{
			if (E.A != RemovedId && E.B != RemovedId)
			{
				RestEdges.Add(E);
			}
		}
		const TSet<int32> After = ComputeSupport(RestNodes, RestEdges);

		TArray<int32> Out;
		for (const FNode& N : RestNodes)
		{
			if (Before.Contains(N.Id) && !After.Contains(N.Id))
			{
				Out.Add(N.Id);
			}
		}
		return Out;
	}

	// ------------------------------------------------------------------ snap

	FSnap FindBestSnap(const FNode& Geom, const TArray<FNode>& Others, float SnapRadiusCm, float OrientTolDeg)
	{
		FSnap Best;
		double BestScore = TNumericLimits<double>::Max();
		const EWoodOrientation MoverOrient = ClassifyOrientation(Geom.Axis(), OrientTolDeg);
		const FVector MEnds[2] = { Geom.A, Geom.B };

		auto Consider = [&](const FNode& Target, EWoodConnectionKind Kind, const FVector& Offset, double Dist, EWoodJointType Joint)
		{
			// Uç-uç hafif öncelikli: aynı mesafede uç oturumu gövdeye tercih edilir.
			const double Score = Dist - (Kind == EWoodConnectionKind::EndToEnd ? 5.0 : 0.0);
			if (Score < BestScore)
			{
				BestScore = Score;
				Best.bFound = true;
				Best.TargetId = Target.Id;
				Best.Kind = Kind;
				Best.Offset = Offset;
				Best.SuggestedJoint = Joint;
			}
		};

		for (const FNode& T : Others)
		{
			if (T.Id == Geom.Id)
			{
				continue;
			}
			const EWoodOrientation TargetOrient = ClassifyOrientation(T.Axis(), OrientTolDeg);
			const FVector TEnds[2] = { T.A, T.B };
			const double TModules = static_cast<double>(FMath::Max(T.LengthModules, 1));
			const double GModules = static_cast<double>(FMath::Max(Geom.LengthModules, 1));

			// A) uç - uç
			for (int32 I = 0; I < 2; ++I)
			{
				for (int32 J = 0; J < 2; ++J)
				{
					const double D = FVector::Dist(MEnds[I], TEnds[J]);
					if (D <= SnapRadiusCm)
					{
						Consider(T, EWoodConnectionKind::EndToEnd, TEnds[J] - MEnds[I], D, SuggestJoint(EWoodConnectionKind::EndToEnd, MoverOrient, TargetOrient));
					}
				}
			}

			// B) parçanın ucu - hedefin gövdesi (hedefin modül çizgisine oturur)
			for (int32 I = 0; I < 2; ++I)
			{
				const double Raw = RawParam(MEnds[I], T.A, T.B);
				if (Raw <= 0.0 || Raw >= 1.0)
				{
					continue; // gövdenin dışında: uç-uç durumu zaten yukarıda
				}
				const double D = FVector::Dist(MEnds[I], PointAt(T.A, T.B, Raw));
				if (D > SnapRadiusCm)
				{
					continue;
				}
				const double Line = FMath::RoundToDouble(Raw * TModules);
				const FVector SnapPt = PointAt(T.A, T.B, Line / TModules);
				const bool bAtEnd = Line <= 0.0 || Line >= TModules;
				const EWoodConnectionKind Kind = bAtEnd ? EWoodConnectionKind::EndToEnd : EWoodConnectionKind::EndToBody;
				Consider(T, Kind, SnapPt - MEnds[I], D, SuggestJoint(Kind, MoverOrient, TargetOrient));
			}

			// C) hedefin ucu - parçanın gövdesi (parçanın modül çizgisi hedefin ucuna oturur)
			for (int32 J = 0; J < 2; ++J)
			{
				const double Raw = RawParam(TEnds[J], Geom.A, Geom.B);
				if (Raw <= 0.0 || Raw >= 1.0)
				{
					continue;
				}
				const double D = FVector::Dist(TEnds[J], PointAt(Geom.A, Geom.B, Raw));
				if (D > SnapRadiusCm)
				{
					continue;
				}
				const double Line = FMath::RoundToDouble(Raw * GModules);
				if (Line <= 0.0 || Line >= GModules)
				{
					continue; // parçanın ucuna denk geliyorsa B/A durumu halleder
				}
				const FVector OnMover = PointAt(Geom.A, Geom.B, Line / GModules);
				Consider(T, EWoodConnectionKind::EndToBody, TEnds[J] - OnMover, D, SuggestJoint(EWoodConnectionKind::EndToBody, TargetOrient, MoverOrient));
			}
		}
		return Best;
	}

	float AccumulateDamage(float Current, float Amount)
	{
		return FMath::Clamp(Current + Amount, 0.f, 1.f);
	}

	TArray<int32> FindTemporaryRemovalDependents(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges)
	{
		const TSet<int32> Before = ComputeSupport(Nodes, Edges);

		TSet<int32> Temp;
		TArray<FNode> Rest;
		for (const FNode& N : Nodes)
		{
			if (N.bTemporary)
			{
				Temp.Add(N.Id);
			}
			else
			{
				Rest.Add(N);
			}
		}
		TArray<FEdge> RestEdges;
		for (const FEdge& E : Edges)
		{
			if (!Temp.Contains(E.A) && !Temp.Contains(E.B))
			{
				RestEdges.Add(E);
			}
		}
		const TSet<int32> After = ComputeSupport(Rest, RestEdges);

		TArray<int32> Out;
		for (const FNode& N : Rest)
		{
			if (Before.Contains(N.Id) && !After.Contains(N.Id))
			{
				Out.Add(N.Id);
			}
		}
		return Out;
	}
}
