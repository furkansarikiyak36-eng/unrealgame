#include "HordePlanAsset.h"
float UHordePlanAsset::ComputeTotalDuration() const{float V=0.f;for(const auto&W:Waves)V+=W.Duration+W.Intermission;return V;}
bool UHordePlanAsset::ValidatePlan(FString& E) const{if(PlanID.IsNone()){E=TEXT("PlanID boş");return false;}if(Waves.IsEmpty()){E=TEXT("Waves boş");return false;}for(const auto&W:Waves)if(W.TotalCount<=0||W.MaxAlive<=0||W.Duration<=0.f||W.Intermission<0.f){E=TEXT("Dalga sayısı, kapasitesi veya süreleri geçersiz");return false;}return true;}
const FEnemyGroupDefinition* UEnemyGroupLibrary::FindGroup(FName ID) const{for(const auto&G:Groups)if(G.GroupID==ID)return &G;return nullptr;}
const FEnemyDefinition* UEnemyLibrary::FindEnemy(FName ID) const{for(const auto&E:Enemies)if(E.EnemyID==ID)return &E;return nullptr;}
