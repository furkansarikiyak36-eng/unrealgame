#include "world_mechanics/density_snapshot_codec.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_set>
#include <type_traits>

namespace wm {
namespace {
class Writer {
 public:
  std::vector<std::uint8_t> data;
  template<class T> void PutUnsigned(T v) {
    static_assert(std::is_unsigned<T>::value,"unsigned serialization type required");
    for(std::size_t i=0;i<sizeof(T);++i) data.push_back(static_cast<std::uint8_t>((v>>(i*8))&static_cast<T>(0xff)));
  }
  void PutDouble(double value) {
    std::uint64_t bits=0; static_assert(sizeof(bits)==sizeof(value),"IEEE double size required");
    std::memcpy(&bits,&value,sizeof(bits)); PutUnsigned(bits);
  }
  void PutBytes(const char* p,std::size_t n) {
    data.insert(data.end(),reinterpret_cast<const std::uint8_t*>(p),reinterpret_cast<const std::uint8_t*>(p)+n);
  }
  void PutString(const std::string& s) {
    PutUnsigned<std::uint32_t>(static_cast<std::uint32_t>(s.size()));
    PutBytes(s.data(),s.size());
  }
};
class Reader {
 public:
  Reader(const std::vector<std::uint8_t>& data,std::size_t length):data_(data),length_(length) {}
  template<class T> bool GetUnsigned(T& value) {
    static_assert(std::is_unsigned<T>::value,"unsigned serialization type required");
    if(Remaining()<sizeof(T)) return false;
    T v=0;
    for(std::size_t i=0;i<sizeof(T);++i) v|=static_cast<T>(data_[pos_++])<<(i*8);
    value=v; return true;
  }
  bool GetDouble(double& value) {
    std::uint64_t bits=0; if(!GetUnsigned(bits)) return false;
    std::memcpy(&value,&bits,sizeof(value)); return true;
  }
  bool GetBytes(char* p,std::size_t n) {
    if(Remaining()<n) return false;
    if(n) std::memcpy(p,data_.data()+pos_,n);
    pos_+=n; return true;
  }
  bool GetString(std::string& s,std::uint32_t cap) {
    std::uint32_t n=0; if(!GetUnsigned(n)||n>cap||Remaining()<n) return false;
    s.assign(reinterpret_cast<const char*>(data_.data()+pos_),n); pos_+=n; return true;
  }
  std::size_t Remaining() const noexcept { return length_-pos_; }
  std::size_t Position() const noexcept { return pos_; }
 private:
  const std::vector<std::uint8_t>& data_;
  std::size_t length_{0},pos_{0};
};
std::uint64_t Hash64(const std::vector<std::uint8_t>& bytes,std::size_t n) {
  std::uint64_t h=14695981039346656037ULL;
  for(std::size_t i=0;i<n;++i) { h^=bytes[i]; h*=1099511628211ULL; }
  return h;
}
bool ValidHeader(const DensitySnapshot& s) {
  return s.schemaVersion==1&&s.revision>0&&std::isfinite(s.spacingM)&&s.spacingM>0&&s.cellsPerAxis>=2;
}
}

Result<std::vector<std::uint8_t>> DensitySnapshotCodec::Encode(
    const DensitySnapshot& snapshot,const SnapshotCodecLimits& limits) {
  if(!ValidHeader(snapshot))
    return Result<std::vector<std::uint8_t>>::Failure(ErrorCode::InvalidArgument,"unsupported or invalid density snapshot header");
  if(limits.maxEncodedBytes<64||snapshot.overrides.size()>limits.maxOverrides||
     snapshot.appliedOperationIds.size()>limits.maxOperationIds)
    return Result<std::vector<std::uint8_t>>::Failure(ErrorCode::BudgetExceeded,"density snapshot exceeds codec limits");
  auto records=snapshot.overrides;
  std::sort(records.begin(),records.end(),[](const auto& a,const auto& b){return a.coord<b.coord;});
  for(std::size_t i=0;i<records.size();++i) {
    if(!std::isfinite(records[i].value.density)||records[i].value.density<0||records[i].value.density>1)
      return Result<std::vector<std::uint8_t>>::Failure(ErrorCode::CorruptData,"density override is outside [0,1]");
    if(i&&records[i-1].coord==records[i].coord)
      return Result<std::vector<std::uint8_t>>::Failure(ErrorCode::Conflict,"duplicate cell in density snapshot");
  }
  std::unordered_set<std::string> uniqueOps;
  for(const auto& id:snapshot.appliedOperationIds)
    if(id.empty()||id.size()>limits.maxOperationIdBytes||!uniqueOps.insert(id).second)
      return Result<std::vector<std::uint8_t>>::Failure(ErrorCode::CorruptData,"empty, oversized or duplicate operation ID");
  Writer w;
  w.PutBytes("WMDS",4);
  w.PutUnsigned<std::uint32_t>(snapshot.schemaVersion);
  w.PutUnsigned<std::uint64_t>(snapshot.revision);
  w.PutDouble(snapshot.spacingM);
  w.PutUnsigned<std::uint32_t>(snapshot.cellsPerAxis);
  w.PutUnsigned<std::uint64_t>(static_cast<std::uint64_t>(records.size()));
  for(const auto& r:records) {
    w.PutUnsigned<std::uint64_t>(static_cast<std::uint64_t>(r.coord.x));
    w.PutUnsigned<std::uint64_t>(static_cast<std::uint64_t>(r.coord.y));
    w.PutUnsigned<std::uint64_t>(static_cast<std::uint64_t>(r.coord.z));
    w.PutDouble(r.value.density);
    w.PutUnsigned<std::uint16_t>(r.value.materialId);
  }
  auto ops=snapshot.appliedOperationIds;
  std::sort(ops.begin(),ops.end());
  w.PutUnsigned<std::uint64_t>(static_cast<std::uint64_t>(ops.size()));
  for(const auto& id:ops) w.PutString(id);
  if(w.data.size()+sizeof(std::uint64_t)>limits.maxEncodedBytes)
    return Result<std::vector<std::uint8_t>>::Failure(ErrorCode::BudgetExceeded,"encoded snapshot exceeds byte cap");
  const auto checksum=Hash64(w.data,w.data.size());
  w.PutUnsigned<std::uint64_t>(checksum);
  return Result<std::vector<std::uint8_t>>::Success(std::move(w.data));
}

Result<DensitySnapshot> DensitySnapshotCodec::Decode(
    const std::vector<std::uint8_t>& bytes,const SnapshotCodecLimits& limits) {
  if(bytes.size()<4+4+8+8+4+8+8||bytes.size()>limits.maxEncodedBytes)
    return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"encoded snapshot size is invalid");
  const std::size_t payloadBytes=bytes.size()-sizeof(std::uint64_t);
  // Read the stored checksum from the final eight bytes using a bounded little-endian reader.
  Reader tail(bytes,bytes.size());
  std::uint64_t storedChecksum=0;
  for(std::size_t i=0;i<payloadBytes;++i) { std::uint8_t discard=0; if(!tail.GetUnsigned(discard)) return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"truncated checksum"); }
  if(!tail.GetUnsigned(storedChecksum)||storedChecksum!=Hash64(bytes,payloadBytes))
    return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"density snapshot checksum mismatch");
  Reader r(bytes,payloadBytes);
  char magic[4]{};
  if(!r.GetBytes(magic,4)||std::string(magic,4)!="WMDS")
    return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"density snapshot magic is invalid");
  DensitySnapshot s;
  std::uint64_t count=0;
  if(!r.GetUnsigned(s.schemaVersion)||!r.GetUnsigned(s.revision)||!r.GetDouble(s.spacingM)||
     !r.GetUnsigned(s.cellsPerAxis)||!r.GetUnsigned(count))
    return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"density snapshot header is truncated");
  if(!ValidHeader(s)) return Result<DensitySnapshot>::Failure(ErrorCode::Unsupported,"unsupported density snapshot schema/header");
  if(count>limits.maxOverrides||count>r.Remaining()/34)
    return Result<DensitySnapshot>::Failure(ErrorCode::BudgetExceeded,"density override count exceeds limits or payload");
  s.overrides.reserve(static_cast<std::size_t>(count));
  std::unordered_set<CellCoord,CellCoordHash> cells;
  for(std::uint64_t i=0;i<count;++i) {
    std::uint64_t x=0,y=0,z=0; double density=0; std::uint16_t material=0;
    if(!r.GetUnsigned(x)||!r.GetUnsigned(y)||!r.GetUnsigned(z)||!r.GetDouble(density)||!r.GetUnsigned(material))
      return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"density record is truncated");
    std::int64_t sx=0,sy=0,sz=0;
    std::memcpy(&sx,&x,sizeof(sx)); std::memcpy(&sy,&y,sizeof(sy)); std::memcpy(&sz,&z,sizeof(sz));
    CellCoord c{sx,sy,sz};
    if(!std::isfinite(density)||density<0||density>1||!cells.insert(c).second)
      return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"invalid density or duplicate cell record");
    s.overrides.push_back({c,{density,material}});
  }
  std::uint64_t opCount=0;
  if(!r.GetUnsigned(opCount)||opCount>limits.maxOperationIds||opCount>r.Remaining()/4)
    return Result<DensitySnapshot>::Failure(ErrorCode::BudgetExceeded,"operation ID count exceeds limits or payload");
  s.appliedOperationIds.reserve(static_cast<std::size_t>(opCount));
  std::unordered_set<std::string> uniqueOps;
  for(std::uint64_t i=0;i<opCount;++i) {
    std::string id;
    if(!r.GetString(id,limits.maxOperationIdBytes)||id.empty()||!uniqueOps.insert(id).second)
      return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"invalid, truncated or duplicate operation ID");
    s.appliedOperationIds.push_back(std::move(id));
  }
  if(r.Remaining()!=0)
    return Result<DensitySnapshot>::Failure(ErrorCode::CorruptData,"unexpected trailing snapshot payload");
  // Canonical deterministic record order, independent of producer order.
  std::sort(s.overrides.begin(),s.overrides.end(),[](const auto& a,const auto& b){return a.coord<b.coord;});
  std::sort(s.appliedOperationIds.begin(),s.appliedOperationIds.end());
  return Result<DensitySnapshot>::Success(std::move(s));
}
} // namespace wm
