#include "RaidFeedbackBus.h"
void URaidFeedbackBus::Publish(FName EventTag,const FRaidFeedbackEvent& Event){ FRaidFeedbackEvent Copy=Event; Copy.Timestamp=FPlatformTime::Seconds(); if(FOnFeedbackEvent* Cb=Subscribers.Find(EventTag)) Cb->ExecuteIfBound(Copy); }
void URaidFeedbackBus::Subscribe(FName EventTag,FOnFeedbackEvent Callback){ if(!EventTag.IsNone() && Callback.IsBound()) Subscribers.Add(EventTag,Callback); }
void URaidFeedbackBus::Unsubscribe(FName EventTag){ Subscribers.Remove(EventTag); }
