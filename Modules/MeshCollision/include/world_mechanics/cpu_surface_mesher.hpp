#pragma once
#include "world_mechanics/core.hpp"
#include <array>
#include <vector>

namespace wm {
struct DensityGridSnapshot {
  ChunkCoord chunk{};
  Vec3 origin{};
  double spacingM{0.25};
  std::uint32_t sizeX{0},sizeY{0},sizeZ{0}; // sample counts, including any border samples
  Revision sourceRevision{0};
  std::uint64_t profileFingerprint{0};
  std::vector<double> density;
  std::vector<std::uint16_t> materialIds;
};
struct TriangleMeshData {
  ChunkCoord chunk{};
  Revision sourceRevision{0};
  std::uint64_t profileFingerprint{0};
  std::vector<Vec3> vertices;
  std::vector<Vec3> normals;
  std::vector<std::array<double,2>> uvs;
  std::vector<std::uint32_t> indices;
  std::vector<std::uint16_t> triangleMaterialIds;
  bool Validate(std::uint64_t maxTriangles,std::string* reason=nullptr) const;
};
struct MesherSettings {
  double isoLevel{0.5};
  std::uint64_t maxTriangles{200000};
  // Reject oversized grids before scanning samples or allocating edge caches.
  std::uint64_t maxInputSamples{1000000};
};
class CpuSurfaceMesher {
 public:
  static Result<TriangleMeshData> Build(const DensityGridSnapshot& snapshot,
                                         const MesherSettings& settings={});
};
} // namespace wm
