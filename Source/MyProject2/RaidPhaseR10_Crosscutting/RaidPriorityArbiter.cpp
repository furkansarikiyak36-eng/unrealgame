#include "RaidPriorityArbiter.h"
void URaidPriorityArbiter::SubmitRequest(ERaidRequestType T,float P){if(T==ERaidRequestType::None)return;FRaidRequest R;R.Type=T;R.Priority=P;R.SubmitTime=FPlatformTime::Seconds();PendingRequests.Add(R);PendingRequests.Sort([](const FRaidRequest&A,const FRaidRequest&B){return A.Priority==B.Priority?A.SubmitTime<B.SubmitTime:A.Priority>B.Priority;});}
ERaidRequestType URaidPriorityArbiter::GetNextRequest(){if(PendingRequests.IsEmpty())return ERaidRequestType::None;const auto T=PendingRequests[0].Type;PendingRequests.RemoveAt(0);return T;}
