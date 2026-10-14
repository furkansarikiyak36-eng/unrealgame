#include "RaidInputNormalizer.h"
float URaidInputNormalizer::ClampPlayerLevel(int32 L,int32 Min,int32 Max){return (float)FMath::Clamp(L,Min,Max);}
float URaidInputNormalizer::NormalizeSurvivalTime(float T,float Max){return Max<=0.f?0.f:FMath::Clamp(T/Max,0.f,1.f);}
float URaidInputNormalizer::NormalizeDeathPenalty(int32 D,float F){return FMath::Max(0.f,(float)D*F);}
float URaidInputNormalizer::NormalizeWorldMultiplier(float R,float Min,float Max){return FMath::Clamp(R,Min,Max);}
float URaidInputNormalizer::NormalizeBiomeDifficulty(int32 R,int32 Max){return Max<=0?0.f:FMath::Clamp((float)R/(float)Max,0.f,1.f);}
