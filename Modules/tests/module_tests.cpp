#include "world_mechanics/core.hpp"
#include "world_mechanics/runtime_profile.hpp"
#include "world_mechanics/world_scan.hpp"
#include "world_mechanics/world_classification.hpp"
#include "world_mechanics/classification_map.hpp"
#include "world_mechanics/landscape_builder.hpp"
#include "world_mechanics/dirty_chunk_queue.hpp"
#include "world_mechanics/cpu_surface_mesher.hpp"
#include "world_mechanics/terrain_voxel.hpp"
#include "world_mechanics/revision_gate.hpp"
#include "world_mechanics/placement_snap.hpp"
#include "world_mechanics/excavation.hpp"
#include "world_mechanics/shoring.hpp"
#include "world_mechanics/structural_integrity.hpp"
#include "world_mechanics/survival.hpp"
#include "world_mechanics/world_environment_simulation.hpp"
#include "world_mechanics/gameplay_quality.hpp"
#include "world_mechanics/persistence_transaction.hpp"
#include "world_mechanics/density_snapshot_codec.hpp"
#include "world_mechanics/local_stone_ring_recognition.hpp"
#include "world_mechanics/stone_fireplace_lifecycle.hpp"
#include "world_mechanics/trap_lifecycle.hpp"
#include <algorithm>
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
  wm::Result<std::vector<wm::RawHit>> Overlap(const wm::OverlapRequest&) const override {
    return wm::Result<std::vector<wm::RawHit>>::Success({
      {true,42,{1,0,0},{0,0,1},0.0,3},
      {true,43,{2,0,0},{0,0,1},0.0,4}
    });
  }
  wm::Result<std::vector<wm::SurfaceSample>> SampleSurface(const wm::Aabb&,std::uint32_t) const override {
    return wm::Result<std::vector<wm::SurfaceSample>>::Success({{{0,0,0},{0,0,1},0.0,true}});
  }
  mutable wm::Vec3 lastDirection{};
};

class FakeProbe final : public wm::IClassificationProbeProvider {
 public:
  wm::Result<std::vector<wm::ClassificationEvidence>> ProbeCell(
      const wm::RegionCellCoord& cell,const wm::Vec3&) const override {
    if(cell.x==99)
      return wm::Result<std::vector<wm::ClassificationEvidence>>::Failure(wm::ErrorCode::Failed,"injected probe failure");
    if(cell.x==2) return wm::Result<std::vector<wm::ClassificationEvidence>>::Success({});
    const auto kind=cell.x==0?wm::RegionKind::EditableVoxel:wm::RegionKind::StaticGeometry;
    return wm::Result<std::vector<wm::ClassificationEvidence>>::Success({
      {kind,wm::EvidenceSource::LayerMarker,false,static_cast<wm::Id>(100+cell.x),"fake layer sample"}
    });
  }
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
  wm::ProfileSessionLock lock; CHECK(lock.Lock(low));
  auto budgetTuned=low; budgetTuned.maxQueryResults=32;
  CHECK(lock.Lock(budgetTuned)); CHECK(lock.Get().maxQueryResults==32);
  CHECK(!lock.Lock(high));
  return true;
}
bool TestScanAndClassification() {
  FakeQuery provider; wm::WorldScan scan(provider,wm::RuntimeProfile::LowMid());
  auto hit=scan.Ray({{0,0,0},{2,0,0},5,{}});
  CHECK(hit); CHECK(hit.value.hit); CHECK(std::abs(provider.lastDirection.x-1.0)<1e-9);
  auto bad=scan.Ray({{0,0,0},{0,0,0},5,{}});
  CHECK(!bad&&bad.error==wm::ErrorCode::InvalidArgument);
  auto ignored=scan.Ray({{0,0,0},{1,0,0},5,{42}});
  CHECK(ignored&&!ignored.value.hit);
  auto duplicateIgnore=scan.Ray({{0,0,0},{1,0,0},5,{42,42}});
  CHECK(!duplicateIgnore&&duplicateIgnore.error==wm::ErrorCode::Conflict);
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
  auto ignoredSweep=sweep; ignoredSweep.ignored={42};
  auto filteredHits=scan.ShapeSweep(ignoredSweep);
  CHECK(filteredHits); CHECK(filteredHits.value.size()==1); CHECK(filteredHits.value.front().targetId==43);
  auto duplicateSweep=sweep; duplicateSweep.ignored={42,42};
  auto duplicateSweepResult=scan.ShapeSweep(duplicateSweep);
  CHECK(!duplicateSweepResult&&duplicateSweepResult.error==wm::ErrorCode::Conflict);
  wm::OverlapRequest overlap; overlap.bounds={{-1,-1,-1},{3,3,3}}; overlap.maxResults=2; overlap.ignored={42};
  auto overlapHits=scan.Overlap(overlap);
  CHECK(overlapHits); CHECK(overlapHits.value.size()==1); CHECK(overlapHits.value.front().targetId==43);
  auto oversizedOverlap=overlap; oversizedOverlap.maxResults=65;
  auto oversizedOverlapResult=scan.Overlap(oversizedOverlap);
  CHECK(!oversizedOverlapResult&&oversizedOverlapResult.error==wm::ErrorCode::BudgetExceeded);
  auto oversizedSweep=sweep; oversizedSweep.maxResults=65;
  auto oversizedSweepResult=scan.ShapeSweep(oversizedSweep);
  CHECK(!oversizedSweepResult&&oversizedSweepResult.error==wm::ErrorCode::BudgetExceeded);
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

  FakeProbe probe;
  const auto beforeBuild=map.GetRevision();
  auto report=wm::LandscapeBuilder::BuildDirtyCells(map,{{0,0,0},{1,0,0},{2,0,0}},probe,10);
  CHECK(report); CHECK(report.value.cellsProbed==3); CHECK(report.value.cellsMarkedUnknown==1);
  CHECK(report.value.previousRevision==beforeBuild); CHECK(report.value.newRevision>beforeBuild);
  auto cell0=map.ClassifyAt({1,1,1}); CHECK(cell0); CHECK(cell0.value.kind==wm::RegionKind::EditableVoxel);
  auto cell1=map.ClassifyAt({9,1,1}); CHECK(cell1); CHECK(cell1.value.kind==wm::RegionKind::StaticGeometry);
  auto cell2=map.ClassifyAt({17,1,1}); CHECK(cell2); CHECK(cell2.value.kind==wm::RegionKind::Unknown);
  const auto beforeFailedBatch=map.GetRevision();
  auto failedBatch=wm::LandscapeBuilder::BuildDirtyCells(map,{{3,0,0},{99,0,0}},probe,10);
  CHECK(!failedBatch); CHECK(map.GetRevision()==beforeFailedBatch);
  auto untouched=map.ClassifyAt({25,1,1}); CHECK(untouched); CHECK(untouched.value.kind==wm::RegionKind::Unknown);

  wm::ClassificationMap boundedMap({0,0,0},8.0);
  CHECK(boundedMap.SetCellEvidence({0,0,0},{{wm::RegionKind::EditableVoxel,wm::EvidenceSource::ExistingCell,false,1,"a"}}));
  CHECK(boundedMap.SetCellEvidence({1,0,0},{{wm::RegionKind::StaticGeometry,wm::EvidenceSource::ExistingCell,false,2,"b"}}));
  auto updateExisting=boundedMap.UpsertCells({{{0,0,0},{{wm::RegionKind::Water,wm::EvidenceSource::LayerMarker,false,3,"updated a"}}}},2);
  CHECK(updateExisting); CHECK(boundedMap.CellCount()==2);
  const auto revisionAtCap=boundedMap.GetRevision();
  auto overCap=boundedMap.UpsertCells({{{2,0,0},{{wm::RegionKind::Water,wm::EvidenceSource::LayerMarker,false,4,"new c"}}}},2);
  CHECK(!overCap&&overCap.error==wm::ErrorCode::BudgetExceeded);
  CHECK(boundedMap.GetRevision()==revisionAtCap); CHECK(boundedMap.CellCount()==2);
  auto duplicateDirty=boundedMap.UpsertCells({
    {{0,0,0},{{wm::RegionKind::Water,wm::EvidenceSource::LayerMarker,false,3,"one"}}},
    {{0,0,0},{{wm::RegionKind::StaticGeometry,wm::EvidenceSource::LayerMarker,false,4,"two"}}}
  },2);
  CHECK(!duplicateDirty&&duplicateDirty.error==wm::ErrorCode::Conflict);
  CHECK(boundedMap.GetRevision()==revisionAtCap);
  return true;
}
bool TestCpuMesherAndRevisionGate() {
  wm::DirtyChunkQueue queue(2);
  CHECK(queue.Enqueue({{0,0,0},1,1}));
  CHECK(queue.Enqueue({{1,0,0},1,1}));
  CHECK(queue.Enqueue({{0,0,0},3,2})); // coalesces and advances revision
  CHECK(queue.Size()==2);
  CHECK(!queue.Enqueue({{2,0,0},1,1}));
  auto popped=queue.Pop(1); CHECK(popped.size()==1); CHECK(popped.front().densityRevision==3);
  CHECK(queue.Size()==1);

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
  auto limitedSettings=settings; limitedSettings.maxInputSamples=26;
  auto overBudgetMesh=wm::CpuSurfaceMesher::Build(snapshot,limitedSettings);
  CHECK(!overBudgetMesh&&overBudgetMesh.error==wm::ErrorCode::BudgetExceeded);
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
  auto encoded=wm::DensitySnapshotCodec::Encode(snapshot.value); CHECK(encoded);
  auto decoded=wm::DensitySnapshotCodec::Decode(encoded.value); CHECK(decoded);
  CHECK(decoded.value.revision==snapshot.value.revision);
  CHECK(decoded.value.overrides.size()==snapshot.value.overrides.size());
  CHECK(decoded.value.appliedOperationIds.size()==snapshot.value.appliedOperationIds.size());
  auto damagedBytes=encoded.value; damagedBytes[8]^=0x01;
  CHECK(!wm::DensitySnapshotCodec::Decode(damagedBytes));
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
  auto stale=wm::PlacementSnapResolver::ValidateCommit(chosen.value,4,7,true,true);
  CHECK(!stale&&stale.error==wm::ErrorCode::StaleRevision);
  auto regionDenied=wm::PlacementSnapResolver::ValidateCommit(chosen.value,3,7,false,true);
  CHECK(!regionDenied&&regionDenied.error==wm::ErrorCode::Unsupported);
  auto overlapDenied=wm::PlacementSnapResolver::ValidateCommit(chosen.value,3,7,true,false);
  CHECK(!overlapDenied&&overlapDenied.error==wm::ErrorCode::Conflict);
  auto occupiedAfterPreview=wm::PlacementSnapResolver::ValidateCommit(chosen.value,3,7,true,true,true,false,true);
  CHECK(!occupiedAfterPreview&&occupiedAfterPreview.error==wm::ErrorCode::Conflict);
  auto incompatibleAtCommit=wm::PlacementSnapResolver::ValidateCommit(chosen.value,3,7,true,true,false,true,true);
  CHECK(!incompatibleAtCommit&&incompatibleAtCommit.error==wm::ErrorCode::Unsupported);
  auto unsupportedAtCommit=wm::PlacementSnapResolver::ValidateCommit(chosen.value,3,7,true,true,true,true,false);
  CHECK(!unsupportedAtCommit&&unsupportedAtCommit.error==wm::ErrorCode::MissingDependency);

  auto occupiedCandidate=candidate; occupiedCandidate.socketOccupied=true;
  auto occupiedResolve=wm::PlacementSnapResolver::Resolve(sr,{occupiedCandidate});
  CHECK(!occupiedResolve&&occupiedResolve.error==wm::ErrorCode::NotFound);
  auto incompatibleCandidate=candidate; incompatibleCandidate.targetCompatible=false;
  auto incompatibleResolve=wm::PlacementSnapResolver::Resolve(sr,{incompatibleCandidate});
  CHECK(!incompatibleResolve&&incompatibleResolve.error==wm::ErrorCode::NotFound);
  auto unsupportedCandidate=candidate; unsupportedCandidate.supportValid=false;
  auto unsupportedResolve=wm::PlacementSnapResolver::Resolve(sr,{unsupportedCandidate});
  CHECK(!unsupportedResolve&&unsupportedResolve.error==wm::ErrorCode::NotFound);
  wm::SnapRequest socketOnly=sr; socketOnly.requireExplicitSocket=true;
  auto nonSocket=candidate; nonSocket.explicitSocket=false;
  auto socketRequired=wm::PlacementSnapResolver::Resolve(socketOnly,{nonSocket});
  CHECK(!socketRequired&&socketRequired.error==wm::ErrorCode::NotFound);
  wm::SnapRequest boundedRequest=sr; boundedRequest.maxCandidates=1;
  auto tooManyCandidates=wm::PlacementSnapResolver::Resolve(boundedRequest,{candidate,candidate});
  CHECK(!tooManyCandidates&&tooManyCandidates.error==wm::ErrorCode::BudgetExceeded);
  wm::SnapRequest badThreshold=sr; badThreshold.minimumNormalDot=1.5;
  auto badThresholdResult=wm::PlacementSnapResolver::Resolve(badThreshold,{candidate});
  CHECK(!badThresholdResult&&badThresholdResult.error==wm::ErrorCode::InvalidArgument);

  auto plankGrid=wm::PlacementSnapResolver::SnapPlankPositionToGrid({0.29,-0.31,1.1},{0,0,0});
  CHECK(plankGrid);
  CHECK(std::abs(plankGrid.value.x-0.2)<1e-9);
  CHECK(std::abs(plankGrid.value.y+0.4)<1e-9);
  CHECK(std::abs(plankGrid.value.z-1.2)<1e-9);
  auto invalidPlankGrid=wm::PlacementSnapResolver::SnapPlankPositionToGrid({0,0,0},{0,0,0},0);
  CHECK(!invalidPlankGrid&&invalidPlankGrid.error==wm::ErrorCode::InvalidArgument);

  wm::DensityVolume terrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService dig(terrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::EditableVoxel; c.canDig=true; c.mapRevision=5; return c;
  },[](const wm::RawHit& observed){
    return wm::Result<wm::RawHit>::Success(observed);
  });
  wm::ExcavationRequest req; req.operationId="dig-live"; req.intendedTargetId=42;
  req.observation={true,42,{0,0,0},{0,0,1},1.0,3}; req.expectedClassificationRevision=5;
  req.edit.expectedRevision=terrain.GetRevision(); req.edit.shape.center={0,0,0}; req.edit.shape.radius=0.3; req.edit.densityDelta=-1;
  auto result=dig.Execute(req); CHECK(result); CHECK(terrain.Sample({0,0,0}).density==0.0);
  wm::DensityVolume blockedTerrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService blocked(blockedTerrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::StaticGeometry; c.mapRevision=5; return c;
  },[](const wm::RawHit& observed){
    return wm::Result<wm::RawHit>::Success(observed);
  });
  req.operationId="blocked-dig"; req.edit.expectedRevision=blockedTerrain.GetRevision();
  auto denied=blocked.Execute(req); CHECK(!denied&&denied.error==wm::ErrorCode::Unsupported);
  CHECK(blockedTerrain.GetRevision()==1);

  wm::DensityVolume staleTerrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService staleDig(staleTerrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::EditableVoxel; c.canDig=true; c.mapRevision=5; return c;
  },[](const wm::RawHit& observed){
    wm::RawHit newer=observed; ++newer.sourceRevision;
    return wm::Result<wm::RawHit>::Success(newer);
  });
  req.operationId="stale-target-dig"; req.edit.expectedRevision=staleTerrain.GetRevision();
  auto staleTarget=staleDig.Execute(req);
  CHECK(!staleTarget&&staleTarget.error==wm::ErrorCode::StaleRevision);
  CHECK(staleTerrain.GetRevision()==1);

  wm::DensityVolume movedTerrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService movedDig(movedTerrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::EditableVoxel; c.canDig=true; c.mapRevision=5; return c;
  },[](const wm::RawHit& observed){
    wm::RawHit moved=observed; moved.point.x+=0.2;
    return wm::Result<wm::RawHit>::Success(moved);
  });
  req.operationId="moved-target-dig"; req.edit.expectedRevision=movedTerrain.GetRevision();
  auto movedTarget=movedDig.Execute(req);
  CHECK(!movedTarget&&movedTarget.error==wm::ErrorCode::StaleRevision);
  CHECK(movedTerrain.GetRevision()==1);

  wm::DensityVolume noValidatorTerrain(wm::RuntimeProfile::LowMid());
  wm::ExcavationService noValidator(noValidatorTerrain,[](const wm::Vec3&){
    wm::ClassificationResult c; c.kind=wm::RegionKind::EditableVoxel; c.canDig=true; c.mapRevision=5; return c;
  },{});
  req.operationId="no-validator-dig"; req.edit.expectedRevision=noValidatorTerrain.GetRevision();
  auto missingValidator=noValidator.Execute(req);
  CHECK(!missingValidator&&missingValidator.error==wm::ErrorCode::MissingDependency);
  CHECK(noValidatorTerrain.GetRevision()==1);
  return true;
}
bool TestShoringPlanner() {
  wm::TrenchGeometry g; g.planId=20; g.start={0,0,0}; g.end={5,0,0};
  g.widthM=2; g.depthM=2; g.stationSpacingM=1.5; g.boardHeightM=0.5; g.maxPieces=1000;
  auto p=wm::ShoringPlanner::Build(g,9); CHECK(p); CHECK(p.value.complete);
  CHECK(p.value.pieces.size()==47); CHECK(p.value.stationSpacingUsedM<=1.5+1e-9);
  wm::ShoringAssembly assembly;
  CHECK(assembly.RegisterPlan(p.value,100,50));
  auto proposed=assembly.GetPlan(20); CHECK(proposed); CHECK(proposed.value.state==wm::ShoringPlanState::Proposed);
  CHECK(assembly.ValidatePlan(20,9,true));
  CHECK(assembly.InstallPlan(20,9));
  auto firstPiece=assembly.GetPiece(100); CHECK(firstPiece); CHECK(firstPiece.value.state==wm::ShoringPieceState::Installed);
  assembly.NotifyTerrainRevision(10);
  auto dirtyPlan=assembly.GetPlan(20); CHECK(dirtyPlan); CHECK(dirtyPlan.value.state==wm::ShoringPlanState::NeedsRevalidation);
  auto dirtyPiece=assembly.GetPiece(100); CHECK(dirtyPiece); CHECK(dirtyPiece.value.state==wm::ShoringPieceState::NeedsRevalidation);
  CHECK(assembly.RevalidatePlan(20,10,true));
  CHECK(assembly.DamagePiece(100,20));
  auto damaged=assembly.GetPiece(100); CHECK(damaged); CHECK(damaged.value.state==wm::ShoringPieceState::Damaged); CHECK(damaged.value.health==30);
  CHECK(assembly.RepairPiece(100,20));
  auto repaired=assembly.GetPiece(100); CHECK(repaired); CHECK(repaired.value.state==wm::ShoringPieceState::Installed);
  CHECK(assembly.RemovePiece(100));
  auto invalidated=assembly.GetPlan(20); CHECK(invalidated); CHECK(invalidated.value.state==wm::ShoringPlanState::NeedsRevalidation);
  CHECK(!assembly.RevalidatePlan(20,10,true));
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

  // Two individual dependents fit under 50 each, but their combined load does not.
  wm::StructuralGraph sharedSupport;
  CHECK(sharedSupport.AddNode({1,true,true,0,50,50,50,false}));
  CHECK(sharedSupport.AddNode({2,false,true,30,100,100,100,false}));
  CHECK(sharedSupport.AddNode({3,false,true,30,100,100,100,false}));
  CHECK(sharedSupport.AddEdge({21,1,2,30,true}));
  CHECK(sharedSupport.AddEdge({22,1,3,30,true}));
  auto combined=sharedSupport.Assess(); CHECK(combined);
  CHECK(combined.value.nodes.size()==3);
  CHECK(combined.value.nodes[0].nodeId==1);
  CHECK(combined.value.nodes[0].state==wm::SupportState::Overloaded);
  CHECK(std::abs(combined.value.nodes[0].aggregateDemand-60.0)<1e-9);
  CHECK(std::abs(combined.value.nodes[0].demandCapacityRatio-1.2)<1e-9);

  const auto revisionBeforeMutation=sharedSupport.GetRevision();
  CHECK(sharedSupport.SetNodeDemand(2,40.0));
  CHECK(sharedSupport.GetRevision()>revisionBeforeMutation);
  auto updatedDemand=sharedSupport.Assess(); CHECK(updatedDemand);
  CHECK(std::abs(updatedDemand.value.nodes[0].aggregateDemand-70.0)<1e-9);
  CHECK(updatedDemand.value.nodes[0].state==wm::SupportState::Overloaded);

  CHECK(sharedSupport.RemoveEdge(22));
  auto supportRemoved=sharedSupport.Assess(); CHECK(supportRemoved);
  bool dependentThreeUnsupported=false;
  for(const auto& node:supportRemoved.value.nodes) {
    if(node.nodeId==3) dependentThreeUnsupported=node.state==wm::SupportState::Unsupported;
  }
  CHECK(dependentThreeUnsupported);
  CHECK(std::abs(supportRemoved.value.nodes[0].aggregateDemand-40.0)<1e-9);
  auto missingEdge=sharedSupport.RemoveEdge(22);
  CHECK(!missingEdge&&missingEdge.error==wm::ErrorCode::NotFound);
  CHECK(sharedSupport.SetEdgeActive(21,false));
  auto disabledEdge=sharedSupport.Assess(); CHECK(disabledEdge);
  bool dependentTwoUnsupported=false;
  for(const auto& node:disabledEdge.value.nodes) {
    if(node.nodeId==2) dependentTwoUnsupported=node.state==wm::SupportState::Unsupported;
  }
  CHECK(dependentTwoUnsupported);
  CHECK(sharedSupport.SetEdgeActive(21,true));
  CHECK(sharedSupport.SetNodeGrounded(1,false));
  auto lostGround=sharedSupport.Assess(); CHECK(lostGround);
  CHECK(lostGround.value.nodes[0].state==wm::SupportState::Unsupported);
  CHECK(sharedSupport.SetNodeGrounded(1,true));
  CHECK(sharedSupport.SetNodeCapacity(1,80.0));
  auto increasedCapacity=sharedSupport.Assess(); CHECK(increasedCapacity);
  CHECK(increasedCapacity.value.nodes[0].state==wm::SupportState::Supported);
  CHECK(!sharedSupport.SetNodeCapacity(1,-1.0));
  return true;
}

bool TestEnvironmentHydrologyAndBudgets() {
  wm::EnvironmentSimulationConfig config;
  config.fixedStepSeconds = 0.1;
  config.maxSubstepsPerCall = 2;
  config.maxQueuedSeconds = 1.0;
  config.maxCells = 8;
  config.maxConnections = 8;
  config.maxWaterTransferFractionPerStep = 0.25;
  config.infiltrationRateMPerSecond = 0.0;
  config.evaporationRateMPerSecond = 0.0;
  config.deepDrainageRateMPerSecond = 0.0;
  config.nutrientLeachFractionPerSecond = 0.0;
  config.temperatureRelaxationPerSecond = 0.0;
  config.fireHeatCPerSecond = 0.0;
  config.fireBurnRateKgPerSecond = 0.0;
  wm::WorldEnvironmentSimulation water(config);

  wm::EnvironmentCellState source;
  source.cellId = 1;
  source.surfaceWaterM3 = 1.0;
  source.surfaceWaterCapacityM3 = 2.0;
  source.soilWaterCapacityM3 = 0.0;
  wm::EnvironmentCellState target;
  target.cellId = 2;
  target.positionM = {1,0,0};
  target.surfaceWaterCapacityM3 = 2.0;
  target.soilWaterCapacityM3 = 0.0;

  CHECK(water.AddCell(source));
  CHECK(water.AddCell(target));
  auto duplicate = water.AddCell(target);
  CHECK(!duplicate && duplicate.error == wm::ErrorCode::Conflict);
  CHECK(water.Connect({1,2,0.5,0.0}));
  auto duplicateConnection = water.Connect({2,1,0.5,0.0});
  CHECK(!duplicateConnection && duplicateConnection.error == wm::ErrorCode::Conflict);
  CHECK(water.TotalWaterM3() == 1.0);

  auto stepped = water.Step(0.5,20.0);
  CHECK(stepped);
  CHECK(stepped.value.completedSubsteps == 2);
  CHECK(std::abs(stepped.value.simulatedSeconds - 0.2) < 1e-9);
  CHECK(std::abs(stepped.value.pendingTimeSeconds - 0.3) < 1e-9);
  CHECK(std::abs(water.TotalWaterM3() - 1.0) < 1e-9);
  CHECK(std::abs(water.WaterBalanceErrorM3()) < 1e-9);

  auto backlogOne = water.Step(0.0,20.0);
  CHECK(backlogOne);
  CHECK(backlogOne.value.completedSubsteps == 2);
  CHECK(std::abs(backlogOne.value.pendingTimeSeconds - 0.1) < 1e-9);
  auto backlogTwo = water.Step(0.0,20.0);
  CHECK(backlogTwo);
  CHECK(backlogTwo.value.completedSubsteps == 1);
  CHECK(backlogTwo.value.pendingTimeSeconds < 1e-9);

  const double pendingBeforeReject = water.GetPendingTimeSeconds();
  const double elapsedBeforeReject = water.GetElapsedSimulationSeconds();
  auto excessiveDelta = water.Step(2.0,20.0);
  CHECK(!excessiveDelta && excessiveDelta.error == wm::ErrorCode::BudgetExceeded);
  CHECK(std::abs(water.GetPendingTimeSeconds() - pendingBeforeReject) < 1e-9);
  CHECK(std::abs(water.GetElapsedSimulationSeconds() - elapsedBeforeReject) < 1e-9);

  const auto waterIn = water.AddSurfaceWater(2,3.0);
  CHECK(waterIn);
  CHECK(std::abs(waterIn.value.requestedM3 - 3.0) < 1e-9);
  CHECK(waterIn.value.acceptedM3 > 0.0);
  CHECK(waterIn.value.runoffM3 > 0.0);
  CHECK(std::abs(waterIn.value.acceptedM3 + waterIn.value.runoffM3 - 3.0) < 1e-9);
  CHECK(std::abs(water.WaterBalanceErrorM3()) < 1e-8);

  auto unknownWaterTarget = water.AddSurfaceWater(999,0.5);
  CHECK(!unknownWaterTarget && unknownWaterTarget.error == wm::ErrorCode::NotFound);
  auto missingLink = water.Connect({1,999,0.1,0.1});
  CHECK(!missingLink && missingLink.error == wm::ErrorCode::NotFound);
  auto badTime = water.Step(-0.1,20.0);
  CHECK(!badTime && badTime.error == wm::ErrorCode::InvalidArgument);

  // The soil submodel accounts for infiltration, evaporation and deep drainage as
  // explicit sinks, so the cumulative water-balance ledger remains closed.
  wm::EnvironmentSimulationConfig soilConfig;
  soilConfig.fixedStepSeconds = 0.1;
  soilConfig.maxSubstepsPerCall = 10;
  soilConfig.maxQueuedSeconds = 1.0;
  soilConfig.infiltrationRateMPerSecond = 0.5;
  soilConfig.evaporationRateMPerSecond = 0.05;
  soilConfig.deepDrainageRateMPerSecond = 0.1;
  soilConfig.nutrientLeachFractionPerSecond = 0.5;
  soilConfig.temperatureRelaxationPerSecond = 0.0;
  soilConfig.fireHeatCPerSecond = 0.0;
  soilConfig.fireBurnRateKgPerSecond = 0.0;
  wm::WorldEnvironmentSimulation soil(soilConfig);
  wm::EnvironmentCellState soilCell;
  soilCell.cellId = 7;
  soilCell.surfaceWaterM3 = 0.5;
  soilCell.surfaceWaterCapacityM3 = 1.0;
  soilCell.soilWaterM3 = 0.25;
  soilCell.soilWaterCapacityM3 = 1.0;
  soilCell.nitrogenKg = 10.0;
  soilCell.phosphorusKg = 5.0;
  soilCell.potassiumKg = 2.0;
  CHECK(soil.AddCell(soilCell));
  auto soilStep = soil.Step(0.5,20.0);
  CHECK(soilStep);
  CHECK(soilStep.value.completedSubsteps == 5);
  CHECK(soilStep.value.evaporationThisCallM3 > 0.0);
  CHECK(soilStep.value.drainageThisCallM3 > 0.0);
  CHECK(std::abs(soilStep.value.waterBalanceErrorM3) < 1e-8);
  auto afterSoil = soil.GetCell(7);
  CHECK(afterSoil);
  CHECK(afterSoil.value.nitrogenKg < 10.0);
  CHECK(afterSoil.value.phosphorusKg < 5.0);
  CHECK(afterSoil.value.potassiumKg < 2.0);
  return true;
}

bool TestEnvironmentFireAndSurvival() {
  wm::EnvironmentSimulationConfig config;
  config.fixedStepSeconds = 0.1;
  config.maxSubstepsPerCall = 4;
  config.maxQueuedSeconds = 5.0;
  config.maxCells = 8;
  config.maxConnections = 8;
  config.infiltrationRateMPerSecond = 0.0;
  config.evaporationRateMPerSecond = 0.0;
  config.deepDrainageRateMPerSecond = 0.0;
  config.nutrientLeachFractionPerSecond = 0.0;
  config.temperatureRelaxationPerSecond = 0.0;
  config.fireHeatCPerSecond = 1000.0;
  config.fireBurnRateKgPerSecond = 0.1;
  config.autoIgnitionTemperatureC = 80.0;
  config.maximumFuelMoisture01 = 0.65;
  config.windHeatBoost = 2.0;
  wm::WorldEnvironmentSimulation environment(config);

  wm::EnvironmentCellState fireSource;
  fireSource.cellId = 10;
  fireSource.fuelKg = 5.0;
  fireSource.fuelMoisture01 = 0.0;
  fireSource.soilWaterCapacityM3 = 0.0;
  wm::EnvironmentCellState dryNeighbor = fireSource;
  dryNeighbor.cellId = 20;
  dryNeighbor.positionM = {1,0,0};
  wm::EnvironmentCellState wetCell = fireSource;
  wetCell.cellId = 30;
  wetCell.positionM = {2,0,0};
  wetCell.soilWaterCapacityM3 = 1.0;
  wetCell.soilWaterM3 = 1.0;

  CHECK(environment.AddCell(fireSource));
  CHECK(environment.AddCell(dryNeighbor));
  CHECK(environment.AddCell(wetCell));
  CHECK(environment.Connect({10,20,0.0,1.0}));

  auto wetIgnite = environment.Ignite(30);
  CHECK(!wetIgnite && wetIgnite.error == wm::ErrorCode::Unsupported);
  CHECK(environment.Ignite(10));
  auto alreadyBurning = environment.Ignite(10);
  CHECK(!alreadyBurning && alreadyBurning.error == wm::ErrorCode::Conflict);

  auto fireStep = environment.Step(0.5,20.0,{1,0,0});
  CHECK(fireStep);
  CHECK(fireStep.value.completedSubsteps == 4);
  CHECK(fireStep.value.newIgnitions >= 1);
  CHECK(fireStep.value.fuelBurnedThisCallKg > 0.0);

  auto neighbor = environment.GetCell(20);
  CHECK(neighbor);
  CHECK(neighbor.value.burning);
  CHECK(neighbor.value.temperatureC >= config.autoIgnitionTemperatureC);

  auto hotSample = environment.Sample(10,environment.GetElapsedSimulationSeconds());
  CHECK(hotSample);
  CHECK(hotSample.value.Validate());
  wm::SurvivalExposure survival;
  auto exposure = survival.Update({},hotSample.value,0.1);
  CHECK(exposure);
  CHECK(exposure.value.thermalStress > 0.0);

  auto wetSample = environment.Sample(30,environment.GetElapsedSimulationSeconds());
  CHECK(wetSample);
  CHECK(wetSample.value.Validate());
  CHECK(wetSample.value.wetness01.has_value());
  CHECK(*wetSample.value.wetness01 > 0.99);

  const auto elapsedBeforeReject = environment.GetElapsedSimulationSeconds();
  auto queueOverflow = environment.Step(6.0,20.0);
  CHECK(!queueOverflow && queueOverflow.error == wm::ErrorCode::BudgetExceeded);
  CHECK(std::abs(environment.GetElapsedSimulationSeconds() - elapsedBeforeReject) < 1e-9);

  CHECK(environment.Extinguish(20));
  auto extinguished = environment.GetCell(20);
  CHECK(extinguished);
  CHECK(!extinguished.value.burning);
  return true;
}


bool TestRaidTargetingAndSpawnOutcomes() {
  std::vector<wm::RaidTargetCandidate> targets{
    {10,7,1.0,0.2,0.2,0.2,100.0,true,true,true},
    {20,7,0.5,1.0,1.0,1.0,10.0,true,true,true},
    {30,6,1.0,1.0,1.0,1.0,0.0,true,true,true},
    {40,7,1.0,1.0,1.0,1.0,0.0,false,true,true}
  };
  auto target = wm::RaidTargetScorer::Choose(targets,7);
  CHECK(target);
  CHECK(target.value.targetId == 20);
  CHECK(target.value.eligibleCandidates == 2);
  CHECK(target.value.worldRevision == 7);

  std::vector<wm::RaidTargetCandidate> tie{
    {9,3,0.5,0.5,0.5,0.5,10.0,true,true,true},
    {4,3,0.5,0.5,0.5,0.5,10.0,true,true,true}
  };
  auto tieResult = wm::RaidTargetScorer::Choose(tie,3);
  CHECK(tieResult);
  CHECK(tieResult.value.targetId == 4);

  auto none = wm::RaidTargetScorer::Choose({{1,3,1,1,1,1,1,true,false,true}},3);
  CHECK(!none && none.error == wm::ErrorCode::NotFound);
  auto duplicate = wm::RaidTargetScorer::Choose({tie[0],tie[0]},3);
  CHECK(!duplicate && duplicate.error == wm::ErrorCode::Conflict);
  auto badFeature = tie;
  badFeature[0].threat01 = 2.0;
  CHECK(!wm::RaidTargetScorer::Choose(badFeature,3));

  wm::SpawnRequest request{501,9,4};
  std::vector<wm::SpawnCandidate> candidates{
    {1,9,{1,0,0},0.9,true,false,true},
    {2,9,{2,0,0},0.7,true,true,true},
    {3,8,{3,0,0},1.0,true,true,true}
  };
  auto spawn = wm::SpawnDecisionPipeline::Choose(request,candidates,4);
  CHECK(spawn);
  CHECK(spawn.value.disposition == wm::SpawnDisposition::Approved);
  CHECK(spawn.value.candidateId == 2);
  CHECK(spawn.value.attemptsUsed == 3);

  auto noAttempts = wm::SpawnDecisionPipeline::Choose(request,candidates,0);
  CHECK(noAttempts);
  CHECK(noAttempts.value.disposition == wm::SpawnDisposition::AttemptBudgetExhausted);

  auto noNavigation = wm::SpawnDecisionPipeline::Choose(
      request,{{1,9,{0,0,0},1.0,false,true,true}},4);
  CHECK(noNavigation);
  CHECK(noNavigation.value.disposition == wm::SpawnDisposition::NavigationUnavailable);

  auto blocked = wm::SpawnDecisionPipeline::Choose(
      request,{{1,9,{0,0,0},1.0,true,false,false}},4);
  CHECK(blocked);
  CHECK(blocked.value.disposition == wm::SpawnDisposition::AllCandidatesBlocked);

  auto attemptCap = wm::SpawnDecisionPipeline::Choose(
      request,{{1,9,{0,0,0},0.9,true,false,true},
               {2,9,{1,0,0},0.8,true,true,true}},1);
  CHECK(attemptCap);
  CHECK(attemptCap.value.disposition == wm::SpawnDisposition::AttemptBudgetExhausted);
  CHECK(attemptCap.value.attemptsUsed == 1);
  return true;
}

bool TestChunkSchedulerFairnessAndStaleWork() {
  wm::ChunkSchedulerConfig cfg;
  cfg.maxQueuedJobs = 16;
  cfg.maxJobCostUnits = 8;
  cfg.maxCostPerDrain = 8;
  cfg.maxJobsPerDrain = 1;
  cfg.maxExaminedPerDrain = 16;
  wm::ChunkWorkScheduler scheduler(cfg);
  const wm::ChunkCoord a{0,0,0};
  const wm::ChunkCoord b{1,0,0};

  CHECK(scheduler.Schedule({1,a,wm::ChunkWorkKind::SnapshotRead,1,1}));
  CHECK(scheduler.Schedule({2,a,wm::ChunkWorkKind::MeshBuild,1,1}));
  CHECK(scheduler.Schedule({3,b,wm::ChunkWorkKind::Classification,1,1}));
  CHECK(scheduler.QueuedCount() == 3);

  const std::map<wm::ChunkCoord,wm::Revision> current{{a,1},{b,1}};
  auto first = scheduler.Drain(current);
  CHECK(first);
  CHECK(first.value.ready.size() == 1);
  CHECK(first.value.ready.front().chunk == a);
  auto second = scheduler.Drain(current);
  CHECK(second);
  CHECK(second.value.ready.size() == 1);
  CHECK(second.value.ready.front().chunk == b);
  CHECK(scheduler.QueuedCount() == 1);

  wm::ChunkWorkScheduler coalescing(cfg);
  CHECK(coalescing.Schedule({10,a,wm::ChunkWorkKind::MeshBuild,4,2}));
  const auto before = coalescing.QueuedCount();
  auto old = coalescing.Schedule({11,a,wm::ChunkWorkKind::MeshBuild,3,2});
  CHECK(!old && old.error == wm::ErrorCode::StaleRevision);
  CHECK(coalescing.Schedule({12,a,wm::ChunkWorkKind::MeshBuild,5,3}));
  CHECK(coalescing.QueuedCount() == before);
  auto batch = coalescing.Drain({{a,5}});
  CHECK(batch);
  CHECK(batch.value.ready.size() == 1);
  CHECK(batch.value.ready.front().workId == 12);
  CHECK(batch.value.ready.front().sourceRevision == 5);

  wm::ChunkWorkScheduler staleScheduler(cfg);
  CHECK(staleScheduler.Schedule({20,a,wm::ChunkWorkKind::DensityEdit,2,2}));
  auto staleBatch = staleScheduler.Drain({{a,3}});
  CHECK(staleBatch);
  CHECK(staleBatch.value.ready.empty());
  CHECK(staleBatch.value.staleDiscarded == 1);
  CHECK(staleScheduler.QueuedCount() == 0);

  wm::ChunkWorkScheduler budgetScheduler(cfg);
  for (wm::Id i=1;i<=16;++i)
    CHECK(budgetScheduler.Schedule({100+i,{static_cast<std::int64_t>(i),0,0},
        wm::ChunkWorkKind::SnapshotRead,1,1}));
  auto full = budgetScheduler.Schedule({999,{99,0,0},wm::ChunkWorkKind::SnapshotRead,1,1});
  CHECK(!full && full.error == wm::ErrorCode::BudgetExceeded);
  auto badCost = budgetScheduler.Schedule({1000,{99,0,0},wm::ChunkWorkKind::MeshBuild,1,9});
  CHECK(!badCost && badCost.error == wm::ErrorCode::BudgetExceeded);
  return true;
}

bool TestMaterialRulePrecedenceAndInvalidation() {
  wm::MaterialRuleResolver resolver(16);
  wm::MaterialInteractionRule global;
  global.ruleId=1; global.scope=wm::MaterialRuleScope::Global;
  global.interaction=wm::MaterialInteraction::Fire;
  global.minObservedValue=-1000; global.maxObservedValue=1000;
  global.durabilityDelta=-1;
  CHECK(resolver.UpsertRule(global));

  wm::MaterialInteractionRule definition=global;
  definition.ruleId=2; definition.scope=wm::MaterialRuleScope::Definition;
  definition.materialId=7; definition.durabilityDelta=-2; definition.priority=1;
  CHECK(resolver.UpsertRule(definition));

  wm::MaterialInteractionRule composite=definition;
  composite.ruleId=3; composite.scope=wm::MaterialRuleScope::Composite;
  composite.contextId=80; composite.durabilityDelta=-3;
  CHECK(resolver.UpsertRule(composite));

  wm::MaterialInteractionRule instance=definition;
  instance.ruleId=4; instance.scope=wm::MaterialRuleScope::Instance;
  instance.contextId=900; instance.durabilityDelta=-4;
  CHECK(resolver.UpsertRule(instance));

  const auto initialRevision=resolver.GetRevision();
  auto exact=resolver.Resolve(7,80,900,wm::MaterialInteraction::Fire,150);
  CHECK(exact);
  CHECK(exact.value.ruleId==4);
  CHECK(exact.value.durabilityDelta==-4);
  CHECK(exact.value.registryRevision==initialRevision);
  CHECK(resolver.CachedQueryCount()==1);

  instance.minObservedValue=200;
  instance.maxObservedValue=300;
  instance.durabilityDelta=-9;
  CHECK(resolver.UpsertRule(instance));
  CHECK(resolver.CachedQueryCount()==0);
  auto fallsBack=resolver.Resolve(7,80,900,wm::MaterialInteraction::Fire,150);
  CHECK(fallsBack);
  CHECK(fallsBack.value.ruleId==3);
  CHECK(fallsBack.value.durabilityDelta==-3);

  auto unrelated = resolver.Resolve(7,0,0,wm::MaterialInteraction::Fire,150);
  CHECK(unrelated);
  CHECK(unrelated.value.ruleId==2);
  auto missing = resolver.Resolve(99,0,0,wm::MaterialInteraction::Fire,150);
  CHECK(missing);
  CHECK(missing.value.ruleId==1);

  auto invalid=global;
  invalid.ruleId=5;
  invalid.materialId=10;
  CHECK(!resolver.UpsertRule(invalid));
  auto removed=resolver.RemoveRule(4);
  CHECK(removed);
  CHECK(resolver.GetRevision()>initialRevision);
  auto missingRule=resolver.RemoveRule(999);
  CHECK(!missingRule && missingRule.error==wm::ErrorCode::NotFound);
  return true;
}

bool TestPlantAndAnimalDeterminism() {
  wm::PlantGrowthDefinition plantDef;
  plantDef.definitionId=101;
  plantDef.growthRatePerDay=0.1;
  plantDef.minimumTemperatureC=0;
  plantDef.optimalTemperatureC=20;
  plantDef.maximumTemperatureC=40;
  plantDef.idealLight01=0.5;
  plantDef.waterNeedM3PerDay=0.1;
  plantDef.nitrogenNeedKgPerDay=0.01;
  plantDef.phosphorusNeedKgPerDay=0.002;
  plantDef.potassiumNeedKgPerDay=0.005;

  wm::PlantGrowthState plant{501,101,2.0,0.5,3};
  wm::PlantGrowthInput ideal{1.0,20.0,0.5,0.1,0.01,0.002,0.005};
  auto grown=wm::PlantGrowthModel::Advance(plantDef,plant,ideal);
  CHECK(grown);
  CHECK(grown.value.stepsProcessed==4);
  CHECK(std::abs(grown.value.nextState.growth01-0.6)<1e-9);
  CHECK(std::abs(grown.value.nextState.ageDays-3.0)<1e-9);
  CHECK(std::abs(grown.value.waterConsumedM3-0.1)<1e-9);
  CHECK(grown.value.nextState.revision==4);

  auto dryInput=ideal;
  dryInput.waterAvailableM3=0.0;
  auto dry=wm::PlantGrowthModel::Advance(plantDef,plant,dryInput);
  CHECK(dry);
  CHECK(std::abs(dry.value.nextState.growth01-plant.growth01)<1e-9);
  CHECK(dry.value.waterConsumedM3==0.0);

  auto invalidPlant=plant;
  invalidPlant.definitionId=999;
  CHECK(!wm::PlantGrowthModel::Advance(plantDef,invalidPlant,ideal));

  wm::AnimalCareDefinition animalDef;
  animalDef.definitionId=201;
  animalDef.speciesId=55;
  animalDef.feedNeedKgPerDay=1.0;
  animalDef.waterNeedLitersPerDay=2.0;
  animalDef.growthRatePerDay=0.1;
  animalDef.productionUnitsPerDay=2.0;
  animalDef.maturityGrowth01=0.5;
  animalDef.starvationHealthLossPerDay=0.1;
  animalDef.recoveryPerDay=0.02;
  animalDef.reproductionHealthMinimum01=0.6;
  animalDef.reproductionReadyDaysRequired=1.0;
  wm::AnimalCareState animal{601,201,10.0,0.7,0.8,0.0,7};
  wm::AnimalCareInput adequate{2.0,2.0,4.0};
  auto cared=wm::AnimalCareModel::Advance(animalDef,animal,adequate);
  CHECK(cared);
  CHECK(cared.value.feedConsumedKg==2.0);
  CHECK(cared.value.waterConsumedLiters==4.0);
  CHECK(cared.value.productionUnits>0.0);
  CHECK(cared.value.nextState.health01>=animal.health01);
  CHECK(cared.value.reproductionReady);

  wm::AnimalCareInput hungry{2.0,0.0,0.0};
  auto starved=wm::AnimalCareModel::Advance(animalDef,animal,hungry);
  CHECK(starved);
  CHECK(starved.value.nextState.health01<animal.health01);
  CHECK(starved.value.nextState.growth01<cared.value.nextState.growth01);
  CHECK(starved.value.productionUnits==0.0);

  std::vector<wm::AnimalReproductionCandidate> herd{
    {9,55,false,true,true,true},
    {8,55,true,true,true,true},
    {2,55,true,true,true,true},
    {4,55,false,true,true,true},
    {1,55,true,false,true,true},
    {3,99,false,true,true,true}
  };
  auto pairsA=wm::AnimalReproductionPlanner::BuildPairs(herd);
  CHECK(pairsA);
  CHECK(pairsA.value.size()==2);
  CHECK(pairsA.value[0].speciesId==55);
  CHECK(pairsA.value[0].femaleId==2 && pairsA.value[0].maleId==4);
  CHECK(pairsA.value[1].femaleId==8 && pairsA.value[1].maleId==9);
  std::reverse(herd.begin(),herd.end());
  auto pairsB=wm::AnimalReproductionPlanner::BuildPairs(herd);
  CHECK(pairsB);
  CHECK(pairsB.value.size()==pairsA.value.size());
  for(std::size_t i=0;i<pairsA.value.size();++i) {
    CHECK(pairsA.value[i].speciesId==pairsB.value[i].speciesId);
    CHECK(pairsA.value[i].femaleId==pairsB.value[i].femaleId);
    CHECK(pairsA.value[i].maleId==pairsB.value[i].maleId);
  }
  return true;
}

bool TestCraftingTransactionPlanner() {
  wm::CraftingInventorySnapshot inventory;
  inventory.revision=5;
  inventory.slotCapacity=5;
  inventory.weightLimitKg=10.0;
  inventory.slots={{1,3,10,0.5},{2,2,10,1.0}};

  const std::vector<wm::CraftIngredient> ingredients{{1,2},{2,1}};
  const std::vector<wm::CraftOutput> outputs{{2,2,10,1.0},{3,5,2,0.25}};
  const auto beforeSlots=inventory.slots;
  auto plan=wm::CraftingTransactionPlanner::Build(inventory,ingredients,outputs);
  CHECK(plan);
  CHECK(plan.value.expectedInventoryRevision==5);
  CHECK(plan.value.baseFingerprint!=0);
  CHECK(plan.value.resultingSlots.size()==5);
  CHECK(std::abs(plan.value.resultingWeightKg-4.75)<1e-9);
  CHECK(inventory.slots.size()==beforeSlots.size());
  CHECK(inventory.slots[0].count==beforeSlots[0].count);
  CHECK(inventory.slots[1].count==beforeSlots[1].count);

  auto valid=wm::CraftingTransactionPlanner::Revalidate(plan.value,inventory);
  CHECK(valid);
  auto staleInventory=inventory;
  staleInventory.revision=6;
  auto stale=wm::CraftingTransactionPlanner::Revalidate(plan.value,staleInventory);
  CHECK(!stale && stale.error==wm::ErrorCode::StaleRevision);

  auto sameRevMutation=inventory;
  sameRevMutation.slots[0].count=2;
  auto fingerprintMismatch=wm::CraftingTransactionPlanner::Revalidate(plan.value,sameRevMutation);
  CHECK(!fingerprintMismatch && fingerprintMismatch.error==wm::ErrorCode::Conflict);

  auto insufficient=wm::CraftingTransactionPlanner::Build(
      inventory,{{1,999}},{{3,1,1,0.1}});
  CHECK(!insufficient && insufficient.error==wm::ErrorCode::OutOfRange);

  auto fullInventory=inventory;
  fullInventory.slotCapacity=2;
  auto noSlots=wm::CraftingTransactionPlanner::Build(fullInventory,ingredients,outputs);
  CHECK(!noSlots && noSlots.error==wm::ErrorCode::BudgetExceeded);

  auto overweight=inventory;
  overweight.weightLimitKg=4.0;
  auto tooHeavy=wm::CraftingTransactionPlanner::Build(overweight,ingredients,outputs);
  CHECK(!tooHeavy && tooHeavy.error==wm::ErrorCode::BudgetExceeded);
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

bool TestStoneFireplaceLifecycle() {
  std::vector<wm::LocalStoneSample> stones;
  for (std::uint64_t i=0; i<8; ++i) {
    const double angle = static_cast<double>(i) * 6.283185307179586 / 8.0;
    stones.push_back({i+1, 60.0*std::cos(angle), 60.0*std::sin(angle), 0.0, true});
  }

  wm::StoneFireplaceLifecycle fireplace;
  auto prematureIgnition=fireplace.Ignite(fireplace.GetRevision());
  CHECK(!prematureIgnition);
  auto ring=fireplace.ReassessRing(stones,0,0,0,1);
  CHECK(ring && ring.value.recognized);
  CHECK(fireplace.Snapshot().state==wm::StoneFireplaceState::Ready);
  auto duplicateObservation=fireplace.ReassessRing(stones,0,0,0,1);
  CHECK(!duplicateObservation && duplicateObservation.error==wm::ErrorCode::StaleRevision);

  auto fuel=fireplace.AddFuel(30.0,fireplace.GetRevision());
  CHECK(fuel && std::abs(fuel.value-30.0)<1e-9);
  auto staleIgnition=fireplace.Ignite(2);
  CHECK(!staleIgnition && staleIgnition.error==wm::ErrorCode::StaleRevision);
  auto lit=fireplace.Ignite(fireplace.GetRevision());
  CHECK(lit && lit.value);
  auto pot=fireplace.PlaceCookingPot(fireplace.GetRevision());
  CHECK(pot && pot.value);
  auto tick=fireplace.Advance(1.5,fireplace.GetRevision());
  CHECK(tick && std::abs(tick.value.fuelConsumed-3.0)<1e-9);
  CHECK(std::abs(tick.value.fuelRemaining-27.0)<1e-9);
  CHECK(std::abs(tick.value.heatOutput-135.0)<1e-9);

  auto savedFire=fireplace.CreateSaveRecord();
  CHECK(savedFire && savedFire.value.wasBurning && savedFire.value.hasCookingPot);
  wm::StoneFireplaceLifecycle restoredFire;
  CHECK(restoredFire.RestoreFromSaveRecord(savedFire.value));
  CHECK(restoredFire.Snapshot().state==wm::StoneFireplaceState::NotBuilt);
  CHECK(restoredFire.Snapshot().heatOutput==0.0);
  CHECK(std::abs(restoredFire.Snapshot().fuelUnits-27.0)<1e-9);
  auto ringAfterRestore=restoredFire.ReassessRing(stones,0,0,0,5);
  CHECK(ringAfterRestore && ringAfterRestore.value.recognized);
  CHECK(restoredFire.Snapshot().state==wm::StoneFireplaceState::Burning);
  CHECK(std::abs(restoredFire.Snapshot().heatOutput-135.0)<1e-9);
  const auto restoredFireRevision=restoredFire.GetRevision();
  auto corruptFireSave=savedFire.value;
  corruptFireSave.fuelUnits=1000.0;
  auto invalidFireRestore=restoredFire.RestoreFromSaveRecord(corruptFireSave);
  CHECK(!invalidFireRestore);
  CHECK(restoredFire.GetRevision()==restoredFireRevision);

  auto brokenRing=stones;
  brokenRing[0].active=false;
  brokenRing[1].active=false;
  brokenRing[2].active=false;
  auto lost=fireplace.ReassessRing(brokenRing,0,0,0,2);
  CHECK(lost && !lost.value.recognized);
  const auto afterLoss=fireplace.Snapshot();
  CHECK(afterLoss.state==wm::StoneFireplaceState::NotBuilt);
  CHECK(afterLoss.heatOutput==0.0);
  CHECK(afterLoss.hasCookingPot);
  CHECK(afterLoss.fuelUnits>0.0);
  return true;
}

bool TestTrapLifecycle() {
  wm::TrapLifecycle trap({700,wm::TrapTriggerSource::Pressure,1.0,0.01,40.0,false,3600.0});
  const auto initial=trap.Snapshot();
  auto wrongSource=trap.TryTrigger({1,42,wm::TrapTriggerSource::TripWire,10.0,initial.stateRevision});
  CHECK(!wrongSource && wrongSource.error==wm::ErrorCode::Unsupported);
  CHECK(trap.GetRevision()==initial.stateRevision);

  auto fired=trap.TryTrigger({1,42,wm::TrapTriggerSource::Pressure,10.0,trap.GetRevision()});
  CHECK(fired && fired.value.applyDamage);
  CHECK(fired.value.damage==40.0);
  CHECK(fired.value.resultingState==wm::TrapLifecycleState::CoolingDown);
  CHECK(std::abs(fired.value.cooldownEndsAt-11.0)<1e-9);
  auto replay=trap.TryTrigger({1,42,wm::TrapTriggerSource::Pressure,10.1,trap.GetRevision()});
  CHECK(!replay && replay.error==wm::ErrorCode::Conflict);
  auto early=trap.Advance(10.999,trap.GetRevision());
  CHECK(early && !early.value);
  auto ready=trap.Advance(11.0,trap.GetRevision());
  CHECK(ready && ready.value);
  auto firedAgain=trap.TryTrigger({2,43,wm::TrapTriggerSource::Pressure,11.0,trap.GetRevision()});
  CHECK(firedAgain && trap.Snapshot().triggerCount==2);

  auto trapSave=trap.CreateSaveRecord(11.2);
  CHECK(trapSave && trapSave.value.hasTriggered);
  CHECK(std::abs(trapSave.value.secondsSinceLastTrigger-0.2)<1e-9);
  CHECK(std::abs(trapSave.value.remainingCooldownSeconds-0.8)<1e-9);
  wm::TrapLifecycle restoredTrap({700,wm::TrapTriggerSource::Pressure,1.0,0.01,40.0,false,3600.0});
  CHECK(restoredTrap.RestoreFromSaveRecord(trapSave.value,2.0));
  CHECK(restoredTrap.Snapshot().state==wm::TrapLifecycleState::CoolingDown);
  CHECK(std::abs(restoredTrap.Snapshot().cooldownEndsAt-2.8)<1e-9);
  auto duplicateAfterRestore=restoredTrap.TryTrigger({2,43,wm::TrapTriggerSource::Pressure,2.1,restoredTrap.GetRevision()});
  CHECK(!duplicateAfterRestore && duplicateAfterRestore.error==wm::ErrorCode::Conflict);
  auto restoreEarly=restoredTrap.Advance(2.79,restoredTrap.GetRevision());
  CHECK(restoreEarly && !restoreEarly.value);
  auto restoreReady=restoredTrap.Advance(2.8,restoredTrap.GetRevision());
  CHECK(restoreReady && restoreReady.value);
  auto postRestoreTrigger=restoredTrap.TryTrigger({3,44,wm::TrapTriggerSource::Pressure,2.8,restoredTrap.GetRevision()});
  CHECK(postRestoreTrigger && restoredTrap.Snapshot().triggerCount==3);
  const auto restoredTrapRevision=restoredTrap.GetRevision();
  auto wrongTrapSave=trapSave.value;
  wrongTrapSave.trapId=9999;
  auto rejectWrongTrap=restoredTrap.RestoreFromSaveRecord(wrongTrapSave,50.0);
  CHECK(!rejectWrongTrap && restoredTrap.GetRevision()==restoredTrapRevision);

  wm::TrapLifecycle oneShot({701,wm::TrapTriggerSource::Manual,0.0,0.01,0.0,true,3600.0});
  auto consumed=oneShot.TryTrigger({1,0,wm::TrapTriggerSource::Manual,1.0,oneShot.GetRevision()});
  CHECK(consumed && !consumed.value.applyDamage);
  CHECK(consumed.value.resultingState==wm::TrapLifecycleState::Consumed);
  auto cannotRearm=oneShot.Arm(oneShot.GetRevision());
  CHECK(!cannotRearm && cannotRearm.error==wm::ErrorCode::Unsupported);
  auto consumedSave=oneShot.CreateSaveRecord(1.5);
  CHECK(consumedSave && consumedSave.value.state==wm::TrapLifecycleState::Consumed);
  wm::TrapLifecycle restoredOneShot({701,wm::TrapTriggerSource::Manual,0.0,0.01,0.0,true,3600.0});
  CHECK(restoredOneShot.RestoreFromSaveRecord(consumedSave.value,100.0));
  CHECK(restoredOneShot.Snapshot().state==wm::TrapLifecycleState::Consumed);
  CHECK(!restoredOneShot.Arm(restoredOneShot.GetRevision()));
  return true;
}

bool TestLocalStoneRingRecognition() {
  std::vector<wm::LocalStoneSample> stones;
  for (std::uint64_t i=0; i<8; ++i) {
    const double angle = static_cast<double>(i) * 6.283185307179586 / 8.0;
    stones.push_back({i+1, 60.0*std::cos(angle), 60.0*std::sin(angle), 0.0, true});
  }
  auto accepted = wm::LocalStoneRingRecognizer::Assess(stones,0,0,0);
  CHECK(accepted.recognized);
  CHECK(accepted.contributingStones==8);
  CHECK(accepted.largestAngularGapDegrees <= 45.001);

  auto openRing = stones;
  openRing[0].active=false; openRing[1].active=false; openRing[2].active=false;
  auto rejectedGap = wm::LocalStoneRingRecognizer::Assess(openRing,0,0,0);
  CHECK(!rejectedGap.recognized);
  CHECK(rejectedGap.reason.find("open") != std::string::npos);

  auto obstructed = stones;
  obstructed.push_back({99,2,1,0,true});
  auto rejectedCenter = wm::LocalStoneRingRecognizer::Assess(obstructed,0,0,0);
  CHECK(!rejectedCenter.recognized);
  CHECK(rejectedCenter.reason.find("obstructed") != std::string::npos);

  auto tooFew = stones;
  for (std::size_t i=0;i<3;++i) tooFew[i].active=false;
  CHECK(!wm::LocalStoneRingRecognizer::Assess(tooFew,0,0,0).recognized);

  auto invalid = wm::StoneRingConfig{};
  invalid.maxRadius=invalid.minRadius;
  CHECK(!wm::LocalStoneRingRecognizer::Assess(stones,0,0,0,invalid).recognized);
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
    {"environment hydrology/budgets",TestEnvironmentHydrologyAndBudgets},
    {"environment fire/survival",TestEnvironmentFireAndSurvival},
    {"raid targeting/spawn outcomes",TestRaidTargetingAndSpawnOutcomes},
    {"chunk scheduler fairness/stale work",TestChunkSchedulerFairnessAndStaleWork},
    {"material rule precedence/cache",TestMaterialRulePrecedenceAndInvalidation},
    {"plant/animal determinism",TestPlantAndAnimalDeterminism},
    {"crafting transaction planner",TestCraftingTransactionPlanner},
    {"survival/restore transaction",TestSurvivalAndRestoreTransaction},
    {"stone fireplace lifecycle",TestStoneFireplaceLifecycle},
    {"trap lifecycle",TestTrapLifecycle},
    {"local stone ring recognition",TestLocalStoneRingRecognition}
  };
  unsigned passed=0;
  for(const auto& t:tests) {
    if(!t.run()) return 1;
    ++passed; std::cout<<"PASS: "<<t.name<<"\n";
  }
  std::cout<<passed<<"/"<<(sizeof(tests)/sizeof(tests[0]))<<" test groups passed\n";
  return 0;
}
