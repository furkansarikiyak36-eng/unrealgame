#include "WaveDurationCondition.h"
bool UWaveDurationCondition::IsDurationExpired(float S,float C,float D)const{return C-S>=FMath::Max(0.f,D);}
