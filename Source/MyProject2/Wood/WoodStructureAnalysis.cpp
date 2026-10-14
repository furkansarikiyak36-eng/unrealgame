// Faz 4 - Yapı tanıma (saf mantık). Mekanik 37, 39-52. Bildirim: WoodStructureAnalysis.h
#include "WoodStructureAnalysis.h"
#include "WoodStructureGeometry.h"

namespace WoodStructure
{
	namespace
	{
		using namespace Geo;

		/** Yan yana duran iki dikey parçanın eksenleri arasındaki boşluk bu kadar (cm) olabilir: değiyor sayılırlar. */
		constexpr double SideBySideGapCm = 12.0;
		/** Aynı kat sayılan yükseklik farkı (cm). */
		constexpr double LayerToleranceCm = 14.0;
		/** Açıklık: iki duvar parçası arasındaki net boşluk (cm). */
		constexpr double OpeningMinCm = 70.0;
		constexpr double OpeningMaxCm = 400.0;
		/** Köprü: en az bu uzunlukta (cm) ve desteklerin arası bu kadar boş (uzunluğun payı). */
		constexpr double BridgeMinLengthCm = 250.0;

		struct FContext
		{
			const TArray<FNode>& Nodes;
			const TArray<FEdge>& Edges;
			TMap<int32, const FNode*> ById;
			TMap<int32, TArray<int32>> Neighbors; // bağlantı yönü önemsiz
			float OrientTol;
		};

		EWoodOrientation Orient(const FContext& C, const FNode& N)
		{
			return ClassifyOrientation(N.Axis(), C.OrientTol);
		}

		/** Eksenin yataydan yükselme açısı, derece (0..90). */
		double ElevationDeg(const FNode& N)
		{
			return FMath::RadiansToDegrees(FMath::Asin(ClampD(FMath::Abs(N.Axis().Z), 0.0, 1.0)));
		}

		FVector MidOf(const FNode& N) { return (N.A + N.B) * 0.5; }
		FVector TopOf(const FNode& N) { return N.A.Z >= N.B.Z ? N.A : N.B; }
		FVector BottomOf(const FNode& N) { return N.A.Z >= N.B.Z ? N.B : N.A; }
		bool IsFlat(const FNode& N) { return N.Role == EWoodRole::Floor || N.Role == EWoodRole::Wall || N.Role == EWoodRole::Roof; }
		bool IsPostLike(const FNode& N) { return N.Role == EWoodRole::Post || N.Role == EWoodRole::Stake; }

		FContext MakeContext(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges, float OrientTol)
		{
			FContext C{ Nodes, Edges, {}, {}, OrientTol };
			for (const FNode& N : Nodes)
			{
				C.ById.Add(N.Id, &N);
			}
			for (const FEdge& E : Edges)
			{
				C.Neighbors.FindOrAdd(E.A).AddUnique(E.B);
				C.Neighbors.FindOrAdd(E.B).AddUnique(E.A);
			}
			return C;
		}

		/** Bağlantı grafiğinde birbirine bağlı kümeler, yalnızca Keep(Id) olan düğümlerden. */
		template <typename FKeep>
		TArray<TArray<int32>> Components(const FContext& C, FKeep Keep)
		{
			TArray<TArray<int32>> Result;
			TSet<int32> Seen;
			for (const FNode& Start : C.Nodes)
			{
				if (Seen.Contains(Start.Id) || !Keep(Start))
				{
					continue;
				}
				TArray<int32> Stack = { Start.Id };
				TArray<int32> Group;
				Seen.Add(Start.Id);
				while (Stack.Num() > 0)
				{
					const int32 Id = Stack.Pop();
					Group.Add(Id);
					if (const TArray<int32>* Next = C.Neighbors.Find(Id))
					{
						for (const int32 Other : *Next)
						{
							const FNode* const* Node = C.ById.Find(Other);
							if (Node && !Seen.Contains(Other) && Keep(**Node))
							{
								Seen.Add(Other);
								Stack.Add(Other);
							}
						}
					}
				}
				Result.Add(MoveTemp(Group));
			}
			return Result;
		}

		bool Linked(const FContext& C, int32 X, int32 Y)
		{
			const TArray<int32>* Next = C.Neighbors.Find(X);
			return Next && Next->Contains(Y);
		}

		FFeature MakeFeature(const FContext& C, EWoodFeatureKind Kind, const TArray<int32>& Ids, double Span)
		{
			FFeature F;
			F.Kind = Kind;
			F.NodeIds = Ids;
			F.SpanCm = Span;
			FVector Sum = FVector::ZeroVector;
			for (const int32 Id : Ids)
			{
				Sum += MidOf(**C.ById.Find(Id));
			}
			F.Center = Ids.Num() > 0 ? Sum / Ids.Num() : FVector::ZeroVector;
			return F;
		}

		double Extent(const FContext& C, const TArray<int32>& Ids)
		{
			double Widest = 0.0;
			for (const int32 A : Ids)
			{
				for (const int32 B : Ids)
				{
					Widest = FMath::Max(Widest, static_cast<double>(FVector::Dist(MidOf(**C.ById.Find(A)), MidOf(**C.ById.Find(B)))));
				}
			}
			return Widest;
		}

		/** Parçanın ekseni boyunca, A ucundan, p noktasına en yakın nokta (cm). */
		double AlongOf(const FNode& N, const FVector& P)
		{
			double T = 0.0;
			DistToSegment(P, N.A, N.B, T);
			return T * N.Length();
		}

		struct FWallLine
		{
			TArray<int32> Members; // çizgi boyunca sıralı
			FVector Origin = FVector::ZeroVector;
			FVector Direction = FVector::ForwardVector;
		};

		/** Dikey duvar çizgileri: yan yana değen, doğrusal dizilmiş en az 3 dikey parça. */
		TArray<FWallLine> FindWallLines(const FContext& C)
		{
			auto IsVertical = [&C](const FNode& N) { return Orient(C, N) == EWoodOrientation::Vertical; };
			// Yan yana değme komşuluğu: eksenler arası yatay mesafe <= r1 + r2 + boşluk, yükseklik aralıkları örtüşüyor.
			TMap<int32, TArray<int32>> Adjacent;
			for (const FNode& X : C.Nodes)
			{
				for (const FNode& Y : C.Nodes)
				{
					if (X.Id >= Y.Id || !IsVertical(X) || !IsVertical(Y))
					{
						continue;
					}
					const FVector D(MidOf(X).X - MidOf(Y).X, MidOf(X).Y - MidOf(Y).Y, 0.0);
					const double Overlap = FMath::Min(TopOf(X).Z, TopOf(Y).Z) - FMath::Max(BottomOf(X).Z, BottomOf(Y).Z);
					if (D.Size() <= X.RadiusCm + Y.RadiusCm + SideBySideGapCm && Overlap >= 30.0)
					{
						Adjacent.FindOrAdd(X.Id).Add(Y.Id);
						Adjacent.FindOrAdd(Y.Id).Add(X.Id);
					}
				}
			}
			TArray<FWallLine> Lines;
			TSet<int32> Seen;
			for (const FNode& Start : C.Nodes)
			{
				if (!IsVertical(Start) || Seen.Contains(Start.Id) || !Adjacent.Contains(Start.Id))
				{
					continue;
				}
				TArray<int32> Stack = { Start.Id };
				TArray<int32> Group;
				Seen.Add(Start.Id);
				while (Stack.Num() > 0)
				{
					const int32 Id = Stack.Pop();
					Group.Add(Id);
					for (const int32 Next : Adjacent[Id])
					{
						if (!Seen.Contains(Next))
						{
							Seen.Add(Next);
							Stack.Add(Next);
						}
					}
				}
				if (Group.Num() < 3)
				{
					continue;
				}
				// Doğrusal mı: en uzak iki parça çizgiyi verir, hiçbiri ondan bir kalınlıktan fazla sapmaz.
				FVector Far0 = MidOf(**C.ById.Find(Group[0]));
				FVector Far1 = Far0;
				double Widest = -1.0;
				for (const int32 X : Group)
				{
					for (const int32 Y : Group)
					{
						const double D = FVector::Dist2D(MidOf(**C.ById.Find(X)), MidOf(**C.ById.Find(Y)));
						if (D > Widest)
						{
							Widest = D;
							Far0 = MidOf(**C.ById.Find(X));
							Far1 = MidOf(**C.ById.Find(Y));
						}
					}
				}
				FVector Direction(Far1.X - Far0.X, Far1.Y - Far0.Y, 0.0);
				if (!Direction.Normalize())
				{
					continue;
				}
				bool bStraight = true;
				for (const int32 Id : Group)
				{
					const FNode& N = **C.ById.Find(Id);
					const FVector P = MidOf(N) - Far0;
					const double Off = FMath::Abs(P.X * Direction.Y - P.Y * Direction.X);
					bStraight &= Off <= N.RadiusCm + 6.0;
				}
				if (!bStraight)
				{
					continue;
				}
				FWallLine Line;
				Line.Origin = Far0;
				Line.Direction = Direction;
				Line.Members = Group;
				Line.Members.Sort([&C, &Line](int32 X, int32 Y)
				{
					return FVector::DotProduct(MidOf(**C.ById.Find(X)) - Line.Origin, Line.Direction) < FVector::DotProduct(MidOf(**C.ById.Find(Y)) - Line.Origin, Line.Direction);
				});
				Lines.Add(MoveTemp(Line));
			}
			return Lines;
		}

		/** Üstten ya da yandan A ve B'ye değen yatay bir kiriş (lento / denizlik): iki ucu da bu iki parçanın üstünden geçer. */
		const FNode* FindSpanningBeam(const FContext& C, const FNode& A, const FNode& B, bool bAbove, const TSet<int32>& Skip)
		{
			for (const FNode& Beam : C.Nodes)
			{
				if (Beam.Role != EWoodRole::Beam || Skip.Contains(Beam.Id) || Orient(C, Beam) != EWoodOrientation::Horizontal)
				{
					continue;
				}
				double Ta = 0.0, Tb = 0.0;
				const FVector PointA = bAbove ? TopOf(A) : MidOf(A);
				const FVector PointB = bAbove ? TopOf(B) : MidOf(B);
				const double Da = DistToSegment(PointA, Beam.A, Beam.B, Ta);
				const double Db = DistToSegment(PointB, Beam.A, Beam.B, Tb);
				const double Reach = Beam.RadiusCm + (bAbove ? 12.0 : A.RadiusCm + 12.0);
				if (Da <= Reach && Db <= Reach)
				{
					const double Height = MidOf(Beam).Z;
					const double ClearTop = FMath::Min(TopOf(A).Z, TopOf(B).Z);
					if (bAbove ? Height >= ClearTop - 5.0 : Height < ClearTop - 40.0)
					{
						return &Beam;
					}
				}
			}
			return nullptr;
		}
	}

	TArray<FFeature> AnalyzeStructure(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges, float ContactTolCm, float OrientTolDeg)
	{
		TArray<FFeature> Features;
		const FContext C = MakeContext(Nodes, Edges, OrientTolDeg);
		TSet<int32> Claimed; // bir özelliğe giren kiriş ve direkler köprü ya da başka bir şey sayılmaz

		// ---- 39 yatay duvar: üst üste oturan yatay kütüklerin en az 3 katı
		for (const TArray<int32>& Group : Components(C, [&C](const FNode& N) { return N.Role == EWoodRole::Beam && Orient(C, N) == EWoodOrientation::Horizontal; }))
		{
			TArray<double> Heights;
			for (const int32 Id : Group)
			{
				Heights.Add(MidOf(**C.ById.Find(Id)).Z);
			}
			Heights.Sort();
			int32 Layers = 0;
			double LastLayer = -1e18;
			for (const double H : Heights)
			{
				if (H - LastLayer > LayerToleranceCm)
				{
					++Layers;
					LastLayer = H;
				}
			}
			if (Layers >= 3 && Group.Num() >= 4)
			{
				Features.Add(MakeFeature(C, EWoodFeatureKind::HorizontalWall, Group, Extent(C, Group)));
				Claimed.Append(Group);
			}
		}

		// ---- 40, 41, 51 dikey / sivri kazık / savunma duvarı
		const TArray<FWallLine> Lines = FindWallLines(C);
		for (const FWallLine& Line : Lines)
		{
			int32 Stakes = 0;
			for (const int32 Id : Line.Members)
			{
				Stakes += (*C.ById.Find(Id))->Role == EWoodRole::Stake ? 1 : 0;
			}
			TArray<int32> Ids = Line.Members;
			EWoodFeatureKind Kind = EWoodFeatureKind::VerticalWall;
			if (Stakes * 2 >= Line.Members.Num())
			{
				Kind = EWoodFeatureKind::StakeWall;
				// payandalı kazık duvarı: duvara değen çapraz bir parça
				for (const FNode& Brace : Nodes)
				{
					if (Brace.Role != EWoodRole::Brace)
					{
						continue;
					}
					for (const int32 Id : Line.Members)
					{
						if (Linked(C, Brace.Id, Id))
						{
							Kind = EWoodFeatureKind::DefenseWall;
							Ids.AddUnique(Brace.Id);
							break;
						}
					}
				}
			}
			const FNode& First = **C.ById.Find(Line.Members[0]);
			const FNode& Last = **C.ById.Find(Line.Members.Last());
			Features.Add(MakeFeature(C, Kind, Ids, FVector::Dist2D(MidOf(First), MidOf(Last)) + First.RadiusCm + Last.RadiusCm));
			Claimed.Append(Line.Members);
		}

		// ---- 42-44 açıklıklar: aynı doğrultudaki iki duvar parçası arasındaki boşluk; üstte lento varsa kapı kasası, altta da denizlik varsa pencere kasası
		for (int32 I = 0; I < Lines.Num(); ++I)
		{
			for (int32 J = I + 1; J < Lines.Num(); ++J)
			{
				const FWallLine& L1 = Lines[I];
				const FWallLine& L2 = Lines[J];
				if (FMath::Abs(FVector::DotProduct(L1.Direction, L2.Direction)) < 0.95)
				{
					continue;
				}
				// L2'nin ilk direği L1'in çizgisinin üstünde mi
				const FNode& E2 = **C.ById.Find(L2.Members[0]);
				const FVector Off = MidOf(E2) - L1.Origin;
				if (FMath::Abs(Off.X * L1.Direction.Y - Off.Y * L1.Direction.X) > E2.RadiusCm + 10.0)
				{
					continue;
				}
				// En yakın iki uç parça: L1 sonu ile L2 başı (ya da tersi).
				const FNode& End1 = **C.ById.Find(L1.Members.Last());
				const FNode& Start1 = **C.ById.Find(L1.Members[0]);
				const FNode& End2 = **C.ById.Find(L2.Members.Last());
				const FNode* Pair[2] = { &End1, &E2 };
				double Gap = FVector::Dist2D(MidOf(End1), MidOf(E2));
				const double Gap2 = FVector::Dist2D(MidOf(End2), MidOf(Start1));
				if (Gap2 < Gap)
				{
					Gap = Gap2;
					Pair[0] = &End2;
					Pair[1] = &Start1;
				}
				Gap -= Pair[0]->RadiusCm + Pair[1]->RadiusCm;
				if (Gap < OpeningMinCm || Gap > OpeningMaxCm)
				{
					continue;
				}
				TArray<int32> Parts = { Pair[0]->Id, Pair[1]->Id };
				EWoodFeatureKind OpeningKind = EWoodFeatureKind::Opening;
				if (const FNode* Lintel = FindSpanningBeam(C, *Pair[0], *Pair[1], /*bAbove*/ true, Claimed))
				{
					Parts.Add(Lintel->Id);
					Claimed.Add(Lintel->Id);
					OpeningKind = EWoodFeatureKind::DoorFrame;
					if (const FNode* Sill = FindSpanningBeam(C, *Pair[0], *Pair[1], /*bAbove*/ false, Claimed))
					{
						Parts.Add(Sill->Id);
						Claimed.Add(Sill->Id);
						OpeningKind = EWoodFeatureKind::WindowFrame;
					}
				}
				Features.Add(MakeFeature(C, OpeningKind, Parts, Gap));
			}
		}

		// ---- 37 platform: en az iki kirişe oturan döşeme, kirişlerin her biri bir direkte
		{
			TSet<int32> UsedFloors;
			for (const FNode& Floor : Nodes)
			{
				if (Floor.Role != EWoodRole::Floor || UsedFloors.Contains(Floor.Id))
				{
					continue;
				}
				TArray<int32> Carriers;
				for (const FEdge& E : Edges)
				{
					if (E.A == Floor.Id && E.Kind == EWoodConnectionKind::Resting && (*C.ById.Find(E.B))->Role == EWoodRole::Beam)
					{
						Carriers.AddUnique(E.B);
					}
				}
				if (Carriers.Num() < 2)
				{
					continue;
				}
				// Her kirişin altında bir direk var mı
				TArray<int32> Posts;
				bool bAllOnPosts = true;
				for (const int32 Carrier : Carriers)
				{
					bool bHasPost = false;
					for (const FEdge& E : Edges)
					{
						if (E.A == Carrier && IsPostLike(**C.ById.Find(E.B)) && Orient(C, **C.ById.Find(E.B)) == EWoodOrientation::Vertical)
						{
							Posts.AddUnique(E.B);
							bHasPost = true;
						}
					}
					bAllOnPosts &= bHasPost;
				}
				if (!bAllOnPosts)
				{
					continue;
				}
				// Aynı kirişleri paylaşan döşemeler tek platform
				TArray<int32> Ids = Carriers;
				Ids.Append(Posts);
				for (const FNode& Other : Nodes)
				{
					if (Other.Role != EWoodRole::Floor)
					{
						continue;
					}
					for (const FEdge& E : Edges)
					{
						if (E.A == Other.Id && Carriers.Contains(E.B))
						{
							Ids.AddUnique(Other.Id);
							UsedFloors.Add(Other.Id);
							break;
						}
					}
				}
				// Legs shorter than a metre make a table (a workbench), taller ones a platform.
				bool bShortLegs = Posts.Num() > 0;
				for (const int32 PostId : Posts)
				{
					bShortLegs &= (**C.ById.Find(PostId)).Length() < 100.0;
				}
				Features.Add(MakeFeature(C, bShortLegs ? EWoodFeatureKind::Table : EWoodFeatureKind::Platform, Ids, Extent(C, Ids)));
				Claimed.Append(Ids);
			}
		}

		// ---- 50 korkuluk: platformda, zeminde durmayan kısa direkler ve onları bağlayan kiriş
		{
			TSet<int32> PlatformIds;
			for (const FFeature& F : Features)
			{
				if (F.Kind == EWoodFeatureKind::Platform || F.Kind == EWoodFeatureKind::Table)
				{
					PlatformIds.Append(F.NodeIds);
				}
			}
			TArray<int32> Posts;
			for (const FNode& Post : Nodes)
			{
				if (!IsPostLike(Post) || Post.bGrounded || Orient(C, Post) != EWoodOrientation::Vertical || PlatformIds.Contains(Post.Id))
				{
					continue;
				}
				for (const FEdge& E : Edges)
				{
					if (E.A == Post.Id && PlatformIds.Contains(E.B))
					{
						Posts.AddUnique(Post.Id);
					}
				}
			}
			for (const FNode& Rail : Nodes)
			{
				if (Rail.Role != EWoodRole::Beam || Claimed.Contains(Rail.Id) || Orient(C, Rail) != EWoodOrientation::Horizontal)
				{
					continue;
				}
				TArray<int32> Held;
				for (const int32 PostId : Posts)
				{
					if (Linked(C, Rail.Id, PostId))
					{
						Held.Add(PostId);
					}
				}
				if (Held.Num() >= 2)
				{
					Held.Add(Rail.Id);
					Features.Add(MakeFeature(C, EWoodFeatureKind::Railing, Held, Rail.Length()));
					Claimed.Append(Held);
				}
			}
		}

		// ---- 45 çatı iskeleti: tepede buluşan iki karşılıklı çapraz payanda; 46 kaplama
		{
			TArray<const FNode*> Rafters;
			for (const FNode& N : Nodes)
			{
				const double Elevation = ElevationDeg(N);
				if (N.Role == EWoodRole::Brace && Elevation >= 15.0 && Elevation <= 60.0 && !N.bGrounded)
				{
					Rafters.Add(&N);
				}
			}
			for (int32 I = 0; I < Rafters.Num(); ++I)
			{
				for (int32 J = I + 1; J < Rafters.Num(); ++J)
				{
					const FNode& X = *Rafters[I];
					const FNode& Y = *Rafters[J];
					const FVector HX = FVector(TopOf(X).X - BottomOf(X).X, TopOf(X).Y - BottomOf(X).Y, 0.0).GetSafeNormal();
					const FVector HY = FVector(TopOf(Y).X - BottomOf(Y).X, TopOf(Y).Y - BottomOf(Y).Y, 0.0).GetSafeNormal();
					if (FVector::Dist(TopOf(X), TopOf(Y)) <= 25.0 && FVector::DotProduct(HX, HY) < -0.3)
					{
						TArray<int32> Ids = { X.Id, Y.Id };
						const double Span = FVector::Dist2D(BottomOf(X), BottomOf(Y));
						Features.Add(MakeFeature(C, EWoodFeatureKind::RoofFrame, Ids, Span));
						Claimed.Append(Ids);
						// kaplama: iskelete (çaprazlara ve onlara değen sırt kirişine) oturan çatı tahtaları
						TArray<int32> Frame = { X.Id, Y.Id };
						for (const FNode& Ridge : Nodes)
						{
							if (Ridge.Role == EWoodRole::Beam && (Linked(C, Ridge.Id, X.Id) || Linked(C, Ridge.Id, Y.Id)))
							{
								Frame.AddUnique(Ridge.Id);
							}
						}
						TArray<int32> Boards;
						for (const FNode& Board : Nodes)
						{
							if (Board.Role != EWoodRole::Roof)
							{
								continue;
							}
							for (const int32 Part : Frame)
							{
								if (Linked(C, Board.Id, Part))
								{
									Boards.AddUnique(Board.Id);
									break;
								}
							}
						}
						if (Boards.Num() >= 2)
						{
							Features.Add(MakeFeature(C, EWoodFeatureKind::RoofSurface, Boards, Extent(C, Boards)));
							Claimed.Append(Boards);
						}
					}
				}
			}
		}

		// ---- 47 merdiven (eğik taşıyıcı + en az 3 farklı yükseklikte yatay basamak) ve rampa (zemine oturan yatık eğik parça)
		for (const FNode& N : Nodes)
		{
			if (Claimed.Contains(N.Id) || Orient(C, N) != EWoodOrientation::Diagonal)
			{
				continue;
			}
			const double Elevation = ElevationDeg(N);
			TArray<int32> Steps;
			TArray<double> Heights;
			if (const TArray<int32>* Next = C.Neighbors.Find(N.Id))
			{
				for (const int32 Other : *Next)
				{
					const FNode& Step = **C.ById.Find(Other);
					if (Orient(C, Step) == EWoodOrientation::Horizontal && !Claimed.Contains(Step.Id))
					{
						Steps.Add(Step.Id);
						Heights.Add(MidOf(Step).Z);
					}
				}
			}
			Heights.Sort();
			int32 Distinct = 0;
			double Last = -1e18;
			for (const double H : Heights)
			{
				if (H - Last >= 12.0)
				{
					++Distinct;
					Last = H;
				}
			}
			if (Distinct >= 3 && Elevation >= 20.0 && Elevation <= 60.0)
			{
				TArray<int32> Ids = Steps;
				Ids.Add(N.Id);
				Features.Add(MakeFeature(C, EWoodFeatureKind::Stairs, Ids, N.Length()));
				Claimed.Append(Ids);
			}
			else if (N.bGrounded && Elevation <= 35.0 && Steps.Num() == 0)
			{
				Features.Add(MakeFeature(C, EWoodFeatureKind::Ramp, { N.Id }, N.Length()));
				Claimed.Add(N.Id);
			}
		}

		// ---- 48 köprü: uzun, zeminde olmayan kiriş; destekleri iki ucunda ve zemindeki parçalarda, ortası boş
		for (const FNode& Beam : Nodes)
		{
			if (Beam.Role != EWoodRole::Beam || Beam.bGrounded || Claimed.Contains(Beam.Id) || Beam.Length() < BridgeMinLengthCm
				|| Orient(C, Beam) != EWoodOrientation::Horizontal)
			{
				continue;
			}
			TArray<double> Holds;
			bool bAllGrounded = true;
			TArray<int32> Ids = { Beam.Id };
			for (const FEdge& E : Edges)
			{
				if (E.A == Beam.Id)
				{
					const FNode& Holder = **C.ById.Find(E.B);
					Holds.Add(AlongOf(Beam, MidOf(Holder)));
					bAllGrounded &= Holder.bGrounded;
					Ids.AddUnique(Holder.Id);
				}
			}
			if (Holds.Num() < 2 || !bAllGrounded)
			{
				continue;
			}
			Holds.Sort();
			const double Length = Beam.Length();
			// destekler yalnızca iki uçtaki dörtte birlerde, ortada boşluk var
			if (Holds[0] <= Length * 0.25 && Holds.Last() >= Length * 0.75 && Holds.Last() - Holds[0] >= Length * 0.5)
			{
				bool bMiddleFree = true;
				for (const double H : Holds)
				{
					bMiddleFree &= !(H > Length * 0.3 && H < Length * 0.7);
				}
				if (bMiddleFree)
				{
					Features.Add(MakeFeature(C, EWoodFeatureKind::Bridge, Ids, Length));
					Claimed.Append(Ids);
				}
			}
		}

		// ---- 49, 52 iskele: geçici işaretli parçaların bağlı kümeleri
		for (const TArray<int32>& Group : Components(C, [](const FNode& N) { return N.bTemporary; }))
		{
			if (Group.Num() >= 2)
			{
				Features.Add(MakeFeature(C, EWoodFeatureKind::Scaffold, Group, Extent(C, Group)));
			}
		}

		return Features;
	}
}
