#include "world_mechanics/core.hpp"
#include "world_mechanics/runtime_profile.hpp"
#include "world_mechanics/world_scan.hpp"
#include "world_mechanics/world_classification.hpp"
#include "world_mechanics/classification_map.hpp"
#include "world_mechanics/cpu_surface_mesher.hpp"
#include "world_mechanics/terrain_voxel.hpp"
#include "world_mechanics/revision_gate.hpp"
#include "world_mechanics/placement_snap.hpp"
#include "world_mechanics/excavation.hpp"
#include "world_mechanics/shoring.hpp"
#include "world_mechanics/structural_integrity.hpp"
#include "world_mechanics/survival.hpp"
#include "world_mechanics/persistence_transaction.hpp"
#include <cmath>
#include <iostream>
#include <string>

#define CHECK(expr) do { if(!(expr)) { std::cerr << "FAIL line " << __LINE__ << ": " #expr "\n"; return false; } } while(0)

namespace {
class FakeQuery final : public wm::IWorldQueryProvider {
 public:
  wm::Result<wm::RawHit> Raycast(const wm::RayRequest& r) const override {
    lastDirection=r.direction;
    return wm::Result<wm::RawHit>::Success({true,42,{1,0,0},{0,0,1},1.0,3});
  }
  wm::Result<std::vector<wm::RawHit>> ShapeSweep(const wm::ShapeSweepRequest&) const override {
    return wm::Result<std::vector<wm::RawHit>>::Success({
      {true,43,{2,0,0},{0,0,1},2.0,4},
      {true,42,{1,0,0},{0,0,1},1.0,3}
    });
  }
  wm::Result<std::vector<wm::SurfaceSample>> SampleSurface(const wm::Aabb&,std::uint32_t) const override {
    return wm::Result<std::vector<wm::SurfaceSample>>::Success({{{0,0,0},{0,0,1},0.0,true}});
  }
  mutable wm::Vec3 lastDirection{};
};

class FakeRestore final : public wm::IRestoreParticipant {
 public:
  int value{0},target{0},old{0};
  bool failStage{false},failCommit{false},staged{false};
  wm::Result<bool> Preflight() override { return wm::Result<bool>::Success(true); }
  wm::Result<bool> Stage() override {
    if(failStage) return wm::Result<bool>::Failure(wm::ErrorCode::Failed,"injected stage failure");
    old=value; staged=true; return wm::Result<bool>::Success(true);
  }
  wm::Result<bool> Commit() override {
    value=target;
    if(failCommit) return wm::Result<bool>::Failure(wm::ErrorCode::Failed,"injected commit failure");
    return wm::Result<bool>::Success(true);
  }
  void Rollback() noexcept override { value=old; }
  void Discard() noexcept override { staged=false; }
};

bool TestProfilesAndCoordinates() {
  auto low=wm::RuntimeProfile::LowMid(), high=wm::RuntimeProfile::HighEnd();
  CHECK(low.Validate()); CHECK(high.Validate());
  CHECK(wm::FloorDiv(-1,32)==-1); CHECK(wm::FloorDiv(-32,32)==-1); CHECK(wm::FloorDiv(-33,32)==-2);
  wm::ProfileSessionLock lock; CHECK(lock.Lock(low)); CHECK(!lock.Lock(high));
  return true;
}
bool TestScanAndClassification() {
  FakeQuery provider; wm::WorldScan scan(provider,wm::RuntimeProfile::LowMid());
  auto hit=scan.Ray({{0,0,0},{2,0,0},5,{}});
  CHECK(hit); CHECK(hit.value.hit); CHECK(std::abs(provider.lastDirection.x-1.0)<1e-9);
  auto bad=scan.Ray({{0,0,0},{0,0,0},5,{}});
  CHECK(!bad&&bad.error==wm::ErrorCode::InvalidArgument);
  std::vector<wm::ClassificationEvidence> evidence{
    {wm::RegionKind::EditableVoxel,wm::EvidenceSource::GeometryFallback,false,90,"inferred soil"},
    {wm::RegionKind::StaticGeometry,wm::EvidenceSource::ExplicitTag,true,10,"authored deny"}
  };
  auto c=wm::RegionClassifier::Classify(evidence,8);
  CHECK(c.kind==wm::RegionKind::StaticGeometry); CHECK(!c.canDig); CHECK(c.mapRevision==8);
  CHECK(wm::RegionClassifier::Classify({},1).kind==wm::RegionKind::Unknown);
  return true;
}
bool TestClassificationMapAndQueryBudgets() {
  FakeQuery provider; wm::WorldScan scan(provider,wm::RuntimeProfile::LowMid());
  wm::ShapeSweepRequest sweep; sweep.bounds={{-1,-1,-1},{1,1,1}}; sweep.start={0,0,0}; sweep.end={1,0,0};
  sweep.maxDistanceM=2; sweep.maxResults=1;
  auto hits=scan.ShapeSweep(sweep); CHECK(hits); CHECK(hits.value.size()==1); CHECK(hits.value.front().targetId==42);
  auto samples=scan.SampleSurface({{-1,-1,-1},{1,1,1}},3); CHECK(samples); CHECK(samples.value.size()==1);
  auto tooMany=scan.SampleSurface({{-1,-1,-1},{1,1,1}},6); CHECK(!tooMany);

  wm::ClassificationMap map({0,0,0},8.0);
  CHECK(map.SetCellEvidence({0,0,0},{{wm::RegionKind::EditableVoxel,wm::EvidenceSource::ExistingCell,false,10,"editable cell"}}));
  const auto before=map.GetRevision();
  auto editable=map.ClassifyAt({1,1,1}); CHECK(editable); CHECK(editable.value.canDig);
  wm::ManualRegionOverride deny; deny.overrideId=1; deny.bounds={{-1,-1,-1},{2,2,2}};
  deny.evidence={wm::RegionKind::StaticGeometry,wm::EvidenceSource::ManualOverride,true,1,"explicit denial"};
  CHECK(map.AddOverride(deny));
  auto blocked=map.ClassifyAt({1,1,1}); CHECK(blocked); CHECK(blocked.value.kind==wm::RegionKind::StaticGeometry); CHECK(!blocked.value.canDig);
  const auto beforeBadRebuild=map.GetRevision();
  auto badRebuild=map.RebuildCells({{{0,0,0},{{wm::RegionKind::Unknown,wm::EvidenceSource::ExistingCell,false,0,"unknown"}}},
                                    {{0,0,0},{{wm::RegionKind::Water,wm::EvidenceSource::ExistingCell,false,0,"duplicate"}}}});
  CHECK(!badRebuild&&badRebuild.error==wm::ErrorCode::Conflict); CHECK(map.GetRevision()==beforeBadRebuild);
  CHECK(map.RemoveOverride(1));
  auto afterRemove=map.ClassifyAt({1,1,1}); CHECK(afterRemove); CHECK(afterRemove.value.canDig);
  CHECK(map.GetRevision()>before);
  return true;
}
bool TestCpuMesherAndRevisionGate() {
  wm::DensityGridSnapshot snapshot; snapshot.origin={0,0,0}; snapshot.spacingM=1;
  snapshot.sizeX=3; snapshot.sizeY=3; snapshot.sizeZ=3; snapshot.sourceRevision=4; snapshot.profileFingerprint=17;
  snapshot.density.reserve(27); snapshot.materialIds.reserve(27);
  for(std::uint32_t z=0;z<3;++z) for(std::uint32_t y=0;y<3;++y) for(std::uint32_t x=0;x<3;++x) {
    snapshot.density.push_back(static_cast<double>(z)/2.0);
    snapshot.materialIds.push_back(z==0?1:2);
  }
  wm::MesherSettings settings; settings.isoLevel=0.6; settings.maxTriangles=1000;
  auto mesh=wm::CpuSurfaceMesher::Build(snapshot,settings);
  CHECK(mesh); CHECK(!mesh.value.indices.empty()); CHECK(mesh.value.indices.size()%3==0);
  CHECK(mesh.value.triangleMaterialIds.size()==mesh.value.indices.size()/3);
  CHECK(mesh.value.Validate(settings.maxTriangles));
  CHECK(wm::DerivedRevisionGate::MayPublish({{},4,17},4,17));
  CHECK(!wm::DerivedRevisionGate::MayPublish({{},3,17},4,17));
  wm::DensityGridSnapshot invalid=snapshot; invalid.density.pop_back();
  CHECK(!wm::CpuSurfaceMesher::Build(invalid,settings));
  return true;
}
bool TestTerrainEditRollbackAndSnapshot() {
  const auto profile=wm::RuntimeProfile::LowMid();
  wm::DensityVolume terrain(profile,[](const wm::Vec3&){return wm::CellValue{1.0,2};});
  wm::EditRequest req; req.operationId="dig-a"; req.expectedRevision=terrain.GetRevision();
  req.shape.kind=wm::EditShapeKind::Sphere; req.shape.center={0,0,0}; req.shape.radius=0.3; req.densityDelta=-1.0;
  auto edit=terrain.ApplyEdit(req); CHECK(edit); CHECK(edit.value.changed); CHECK(terrain.Sample({0,0,0}).density==0.0);
  auto duplicate=terrain.ApplyEdit(req); CHECK(!duplicate&&duplicate.error==wm::ErrorCode::Conflict);
  auto stale=req; stale.operationId="dig-stale"; stale.expectedRevision=1;
  auto rejected=terrain.ApplyEdit(stale); CHECK(!rejected&&rejected.error==wm::ErrorCode::StaleRevision);
  auto snapshot=terrain.ExportSnapshot(); CHECK(snapshot); CHECK(!snapshot.value.overrides.empty());
  wm::DensityVolume restored(profile,[](const wm::Vec3&){return wm::CellValue{1.0,2};});
  CHECK(restored.ImportSnapshot(snapshot.value)); CHECK(restored.Sample({0,0,0}).density==0.0);
  auto rolled=terrain.Rollback(edit.value); CHECK(rolled); CHECK(terrain.Sample({0,0,0}).density==1.0);
  auto corrupt=snapshot.value; corrupt.overrides.push_back(corrupt.overrides.front());
  auto badImport=restored.ImportSnapshot(corrupt); CHECK(!badImport);
  return true;
}
bool TestSnapAndExcavation() {
  wm::SnapRequest sr; sr.expectedSurfaceRevision=3; sr.expectedClassificationRevision=7;
  wm::SnapCandidate candidate; candidate.candidateId=11; candidate.transform={{0,0,0},{1,0,0}};
  candidate.surfaceNormal={0,0,1}; candidate.distanceM=0.2; candidate.explicitSocket=true;
  candidate.regionAllowed=true; candidate.overlapFree=true; candidate.surfaceRevision=3; candidate.classificationRevision=7;
  auto chosen=wm::PlacementSnapResolver::Resolve(sr,{candidate}); CHECK(chosen); CHECK(chosen.value.candidateId==11);
  auto stale=wm::PlacementSnapResolver::ValidateCommit(chosen.value,4,7,true);
  CHECK(!stale&&stale.error==wm::ErrorCode::StaleRevision);

  wm::DensityVolume terrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService dig(terrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::EditableVoxel; c.canDig=true; c.mapRevision=5; return c;
  });
  wm::ExcavationRequest req; req.operationId="dig-live"; req.intendedTargetId=42;
  req.observation={true,42,{0,0,0},{0,0,1},1.0,3}; req.expectedClassificationRevision=5;
  req.edit.expectedRevision=terrain.GetRevision(); req.edit.shape.center={0,0,0}; req.edit.shape.radius=0.3; req.edit.densityDelta=-1;
  auto result=dig.Execute(req); CHECK(result); CHECK(terrain.Sample({0,0,0}).density==0.0);
  wm::DensityVolume blockedTerrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService blocked(blockedTerrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::StaticGeometry; c.mapRevision=5; return c;
  });
  req.operationId="blocked-dig"; req.edit.expectedRevision=blockedTerrain.GetRevision();
  auto denied=blocked.Execute(req); CHECK(!denied&&denied.error==wm::ErrorCode::Unsupported);
  CHECK(blockedTerrain.GetRevision()==1);
  return true;
}
bool TestShoringPlanner() {
  wm::TrenchGeometry g; g.planId=20; g.start={0,0,0}; g.end={5,0,0};
  g.widthM=2; g.depthM=2; g.stationSpacingM=1.5; g.boardHeightM=0.5; g.maxPieces=1000;
  auto p=wm::ShoringPlanner::Build(g,9); CHECK(p); CHECK(p.value.complete);
  CHECK(p.value.pieces.size()==47); CHECK(p.value.stationSpacingUsedM<=1.5+1e-9);
  g.maxPieces=10; auto tooMany=wm::ShoringPlanner::Build(g,9); CHECK(!tooMany&&tooMany.error==wm::ErrorCode::BudgetExceeded);
  return true;
}
bool TestStructuralIntegrity() {
  wm::StructuralGraph graph;
  CHECK(graph.AddNode({1,true,true,0,100,100,100,false}));
  CHECK(graph.AddNode({2,false,true,10,30,30,30,false}));
  CHECK(graph.AddEdge({1,1,2,20,true}));
  auto first=graph.Assess(); CHECK(first); CHECK(first.value.nodes.size()==2);
  CHECK(first.value.nodes[1].state==wm::SupportState::Supported);
  CHECK(graph.ApplyDamage(1,100));
  auto after=graph.Assess(); CHECK(after);
  CHECK(after.value.nodes[1].state==wm::SupportState::Unsupported);
  CHECK(graph.LatchCollapse(2));
  auto once=graph.LatchCollapse(2); CHECK(once&&!once.value);
  CHECK(!graph.Repair(2,5));
  return true;
}
bool TestSurvivalAndRestoreTransaction() {
  wm::SurvivalExposure survival;
  wm::EnvironmentSample sample; sample.sourceRevision=1; sample.sampledAtSeconds=1;
  sample.valid=true; sample.temperatureC=-5; sample.wetness01=1; sample.light01=0;
  sample.waterDepthM=0.5; sample.underground=true; sample.undergroundDepthM=2;
  auto exposure=survival.Update({},sample,1.0); CHECK(exposure);
  CHECK(exposure.value.thermalStress>0); CHECK(exposure.value.wetness>0);
  CHECK(exposure.value.darknessExposure>0); CHECK(exposure.value.waterExposure>0);
  auto cost=survival.EffortCost(wm::EffortKind::HeavyExcavation,2); CHECK(cost); CHECK(cost.value==14.0);
  sample.stale=true; auto stale=survival.Update(exposure.value,sample,0.1); CHECK(!stale);

  FakeRestore a,b; a.target=10; b.target=20; b.failCommit=true;
  wm::RestoreTransaction tx; auto r=tx.Execute({&a,&b}); CHECK(!r);
  CHECK(a.value==0); CHECK(b.value==0); CHECK(!a.staged&&!b.staged);
  FakeRestore c,d; c.target=1; d.target=2;
  auto ok=tx.Execute({&c,&d}); CHECK(ok); CHECK(c.value==1&&d.value==2);
  return true;
}
}

int main() {
  struct Test { const char* name; bool (*run)(); };
  const Test tests[]={
    {"profiles/coordinates",TestProfilesAndCoordinates},
    {"scan/classification",TestScanAndClassification},
    {"classification map/query budgets",TestClassificationMapAndQueryBudgets},
    {"CPU mesher/revision gate",TestCpuMesherAndRevisionGate},
    {"terrain edit/rollback/snapshot",TestTerrainEditRollbackAndSnapshot},
    {"snap/excavation",TestSnapAndExcavation},
    {"shoring planner",TestShoringPlanner},
    {"structural integrity",TestStructuralIntegrity},
    {"survival/restore transaction",TestSurvivalAndRestoreTransaction}
  };
  unsigned passed=0;
  for(const auto& t:tests) {
    if(!t.run()) return 1;
    ++passed; std::cout<<"PASS: "<<t.name<<"\n";
  }
  std::cout<<passed<<"/"<<(sizeof(tests)/sizeof(tests[0]))<<" test groups passed\n";
  return 0;
}
