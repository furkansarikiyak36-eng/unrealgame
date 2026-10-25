#include "HordePlanAsset.h"
float UHordePlanAsset::ComputeTotalDuration() const{float V=0.f;for(const auto&W:Waves)V+=W.Duration+W.Intermission;return V;}
bool UHordePlanAsset::ValidatePlan(FString& E) const
{
    if(PlanID.IsNone()){E=TEXT("PlanID boş");return false;}
    if(Waves.IsEmpty()||Waves.Num()>64){E=TEXT("Waves boş veya 64 dalga sınırını aşıyor");return false;}
    int64 TotalRequested=0;
    for(const FWavePlan& W:Waves)
    {
        if(W.GroupID.IsNone()||W.TotalCount<=0||W.TotalCount>10000||W.MaxAlive<=0||W.MaxAlive>256
            ||!FMath::IsFinite(W.Duration)||W.Duration<=0.f||W.Duration>86400.f
            ||!FMath::IsFinite(W.Intermission)||W.Intermission<0.f||W.Intermission>3600.f)
        {
            E=TEXT("Dalga kimliği, sayısı, kapasitesi veya süresi geçersiz");return false;
        }
        TotalRequested+=W.TotalCount;
        if(TotalRequested>100000){E=TEXT("Horde plan toplamı 100000 spawn sınırını aşıyor");return false;}
    }
    E.Empty();return true;
}
const FEnemyGroupDefinition* UEnemyGroupLibrary::FindGroup(FName ID) const{for(const auto&G:Groups)if(G.GroupID==ID)return &G;return nullptr;}
const FEnemyDefinition* UEnemyLibrary::FindEnemy(FName ID) const{for(const auto&E:Enemies)if(E.EnemyID==ID)return &E;return nullptr;}
