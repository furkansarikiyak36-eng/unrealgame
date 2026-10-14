#pragma once
#include "CoreMinimal.h"
class MYPROJECT2_API FDeterministicRandom { public:
 FDeterministicRandom(): State(12345u) {} explicit FDeterministicRandom(uint32 Seed): State(Seed) {}
 void SetSeed(uint32 Seed){State=Seed;} uint32 GetSeed() const{return State;}
 uint32 NextUInt(){State=State*1664525u+1013904223u; return State;}
 float NextFloat(){return float(NextUInt() & 0x00FFFFFFu)/float(0x01000000u);}
 float Range(float Min,float Max){return Min+NextFloat()*(Max-Min);}
 int32 Range(int32 Min,int32 Max){if(Max<=Min)return Min; return Min+int32(NextFloat()*float(Max-Min+1));}
 bool Roll(float P){return NextFloat()<FMath::Clamp(P,0.f,1.f);}
private: uint32 State;
};
