#include "SimulationBudgetAllocator.h"
void USimulationBudgetAllocator::ResetFrame(){FrameStartTime=FPlatformTime::Seconds();ChunksProcessed=0;}
bool USimulationBudgetAllocator::TryAllocateChunk(){if(ChunksProcessed>=FMath::Max(0,MaxChunksPerFrame)||!HasTimeRemaining())return false;++ChunksProcessed;return true;}
bool USimulationBudgetAllocator::HasTimeRemaining() const{return FrameStartTime>0.0&&(FPlatformTime::Seconds()-FrameStartTime)*1000.0<double(FMath::Max(0.1f,MaxTickMilliseconds));}
