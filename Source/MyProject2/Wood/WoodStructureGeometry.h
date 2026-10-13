// Faz 4 - Ortak geometri yardımcıları (iç kullanım). Doğru parçası = iki uçlu kalın çubuk.
#pragma once

#include "CoreMinimal.h"

namespace WoodStructure
{
	namespace Geo
	{
		constexpr double Eps = 1e-9;

		inline double ClampD(double V, double Lo, double Hi)
		{
			return V < Lo ? Lo : (V > Hi ? Hi : V);
		}

		/** Noktanın doğru üzerindeki ham parametresi (kelepçesiz): 0 = A, 1 = B. */
		inline double RawParam(const FVector& P, const FVector& A, const FVector& B)
		{
			const FVector AB = B - A;
			const double L2 = FVector::DotProduct(AB, AB);
			return L2 > Eps ? FVector::DotProduct(P - A, AB) / L2 : 0.0;
		}

		inline FVector PointAt(const FVector& A, const FVector& B, double T)
		{
			return A + (B - A) * T;
		}

		inline double DistToSegment(const FVector& P, const FVector& A, const FVector& B, double& OutT)
		{
			OutT = ClampD(RawParam(P, A, B), 0.0, 1.0);
			return FVector::Dist(P, PointAt(A, B, OutT));
		}

		/** İki doğru parçasının en yakın noktalarının parametreleri (Ericson, Real-Time Collision Detection 5.1.9). */
		inline void ClosestSegSeg(const FVector& P1, const FVector& Q1, const FVector& P2, const FVector& Q2, double& OutS, double& OutT)
		{
			const FVector D1 = Q1 - P1;
			const FVector D2 = Q2 - P2;
			const FVector R = P1 - P2;
			const double A = FVector::DotProduct(D1, D1);
			const double E = FVector::DotProduct(D2, D2);
			const double F = FVector::DotProduct(D2, R);

			if (A <= Eps && E <= Eps)
			{
				OutS = 0.0;
				OutT = 0.0;
				return;
			}
			if (A <= Eps)
			{
				OutS = 0.0;
				OutT = ClampD(F / E, 0.0, 1.0);
				return;
			}

			const double C = FVector::DotProduct(D1, R);
			if (E <= Eps)
			{
				OutT = 0.0;
				OutS = ClampD(-C / A, 0.0, 1.0);
				return;
			}

			const double B = FVector::DotProduct(D1, D2);
			const double Denom = A * E - B * B;
			OutS = Denom > Eps ? ClampD((B * F - C * E) / Denom, 0.0, 1.0) : 0.0;
			OutT = (B * OutS + F) / E;
			if (OutT < 0.0)
			{
				OutT = 0.0;
				OutS = ClampD(-C / A, 0.0, 1.0);
			}
			else if (OutT > 1.0)
			{
				OutT = 1.0;
				OutS = ClampD((B - C) / A, 0.0, 1.0);
			}
		}
	}
}
