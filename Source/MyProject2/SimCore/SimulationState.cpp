#include "SimulationState.h"
void USimulationState::Reset() { DayNumber=1; HourOfDay=0; TimeOfDay=0.f; Temperature=20.f; TempMin=15.f; TempMax=25.f; Rainfall=0.f; bIsRaining=false; bIsSnowing=false; WindSpeed=0.f; }
