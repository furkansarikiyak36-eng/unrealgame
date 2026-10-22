#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wm {
using Id = std::uint64_t;
using Revision = std::uint64_t;
using Seconds = double;

enum class ErrorCode {
  None, InvalidArgument, OutOfRange, Unsupported, StaleRevision, Conflict,
  BudgetExceeded, MissingDependency, CorruptData, NotFound, Failed
};

template<class T>
struct Result {
  bool ok{false};
  T value{};
  ErrorCode error{ErrorCode::None};
  std::string message;

  static Result Success(T v) {
    Result r; r.ok = true; r.value = std::move(v); return r;
  }
  static Result Failure(ErrorCode c, std::string m) {
    Result r; r.error = c; r.message = std::move(m); return r;
  }
  explicit operator bool() const noexcept { return ok; }
};

struct Vec3 {
  double x{0}, y{0}, z{0};
  bool IsFinite() const noexcept {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
  }
  Vec3 operator+(const Vec3& b) const noexcept { return {x+b.x,y+b.y,z+b.z}; }
  Vec3 operator-(const Vec3& b) const noexcept { return {x-b.x,y-b.y,z-b.z}; }
  Vec3 operator*(double s) const noexcept { return {x*s,y*s,z*s}; }
  Vec3 operator/(double s) const noexcept { return {x/s,y/s,z/s}; }
};

inline double Dot(const Vec3& a, const Vec3& b) noexcept { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline double LengthSquared(const Vec3& v) noexcept { return Dot(v,v); }
inline double Length(const Vec3& v) noexcept { return std::sqrt(LengthSquared(v)); }
inline Vec3 Normalize(const Vec3& v) noexcept {
  const double n=Length(v); return n>1e-12 ? v/n : Vec3{};
}
inline double Clamp01(double v) noexcept { return std::max(0.0,std::min(1.0,v)); }

struct Aabb {
  Vec3 min, max;
  bool IsValid() const noexcept {
    return min.IsFinite() && max.IsFinite() && min.x<=max.x && min.y<=max.y && min.z<=max.z;
  }
  bool Contains(const Vec3& p) const noexcept {
    return IsValid() && p.x>=min.x && p.x<=max.x && p.y>=min.y && p.y<=max.y && p.z>=min.z && p.z<=max.z;
  }
  bool Intersects(const Aabb& b) const noexcept {
    return IsValid() && b.IsValid() && min.x<=b.max.x && max.x>=b.min.x &&
      min.y<=b.max.y && max.y>=b.min.y && min.z<=b.max.z && max.z>=b.min.z;
  }
};

struct ChunkCoord {
  std::int64_t x{0}, y{0}, z{0};
  bool operator==(const ChunkCoord& b) const noexcept { return x==b.x && y==b.y && z==b.z; }
  bool operator<(const ChunkCoord& b) const noexcept {
    if (x!=b.x) return x<b.x; if (y!=b.y) return y<b.y; return z<b.z;
  }
};

inline std::int64_t FloorDiv(std::int64_t value, std::int64_t divisor) {
  if (divisor<=0) return 0;
  std::int64_t q=value/divisor, r=value%divisor;
  return (r<0) ? q-1 : q;
}

struct Transform {
  Vec3 position{};
  Vec3 forward{1,0,0};
  bool IsFinite() const noexcept { return position.IsFinite() && forward.IsFinite(); }
};
} // namespace wm
