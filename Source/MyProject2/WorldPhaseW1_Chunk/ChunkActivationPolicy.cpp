#include "ChunkActivationPolicy.h"
EChunkActivationMode UChunkActivationPolicy::DetermineMode(float D) const{if(D<=ContinuousRadiusMeters)return EChunkActivationMode::Continuous;if(D<=ScheduledRadiusMeters)return EChunkActivationMode::Scheduled;return EChunkActivationMode::Passive;}
float UChunkActivationPolicy::GetUpdateIntervalSeconds(EChunkActivationMode M) const{switch(M){case EChunkActivationMode::Continuous:return 0.0f;case EChunkActivationMode::Scheduled:return 5.0f;default:return 60.0f;}}
