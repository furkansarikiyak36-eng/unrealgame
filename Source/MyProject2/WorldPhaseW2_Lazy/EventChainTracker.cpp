#include "EventChainTracker.h"
bool UEventChainTracker::HasInterruptingEvent(const FIntPoint& C,float Since) const{for(const FTrackedEvent& T:TrackedEvents)if(T.RegisteredDay>=Since&&T.Event.ChunkCoord==C)return true;return false;}
void UEventChainTracker::RegisterEvent(const FWorldEvent& E,float D){FTrackedEvent T;T.Event=E;T.RegisteredDay=D;TrackedEvents.Add(MoveTemp(T));}
void UEventChainTracker::ClearOldEvents(float Now,float Age){TrackedEvents.RemoveAll([Now,Age](const FTrackedEvent& T){return Now-T.RegisteredDay>Age;});}
