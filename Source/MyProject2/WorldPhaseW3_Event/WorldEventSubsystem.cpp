#include "WorldEventSubsystem.h"
void UWorldEventSubsystem::Initialize(FSubsystemCollectionBase& C){Super::Initialize(C);}
TStatId UWorldEventSubsystem::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldEventSubsystem,STATGROUP_Tickables);}
void UWorldEventSubsystem::Tick(float D){Super::Tick(D);const int32 Limit=FMath::Max(0,MaxEventsPerFrame);int32 Done=0;while(PendingEvents.Num()>0&&Done<Limit){const FWorldEvent E=PendingEvents[0];PendingEvents.RemoveAt(0,EAllowShrinking::No);const int32 Depth=FMath::Max(0,FMath::RoundToInt(E.NumericPayload.FindRef(TEXT("PropagationDepth"))));ProcessEvent(E,Depth);++Done;}}
void UWorldEventSubsystem::BroadcastEvent(const FWorldEvent& In){if(In.Type==EWorldEventType::None)return;PendingEvents.Add(In);}
void UWorldEventSubsystem::ProcessEvent(const FWorldEvent& E,int32 Depth){if(Depth>FMath::Max(0,MaxChainDepth))return;OnWorldEventBroadcast.Broadcast(E);for(int32 I=Responders.Num()-1;I>=0;--I){if(!Responders[I].GetObject()){Responders.RemoveAt(I);continue;}if(IWorldEventResponder::Execute_HandleWorldEvent(Responders[I].GetObject(),E))break;}}
void UWorldEventSubsystem::RegisterResponder(TScriptInterface<IWorldEventResponder> R){if(R.GetObject())Responders.AddUnique(R);}
void UWorldEventSubsystem::UnregisterResponder(TScriptInterface<IWorldEventResponder> R){Responders.Remove(R);}
