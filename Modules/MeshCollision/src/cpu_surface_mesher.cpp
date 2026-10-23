#include "world_mechanics/cpu_surface_mesher.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace wm {
namespace {
struct GridPoint { std::uint32_t x,y,z; };
std::size_t Count(const DensityGridSnapshot& s) {
  const auto a=static_cast<std::uint64_t>(s.sizeX),b=static_cast<std::uint64_t>(s.sizeY),c=static_cast<std::uint64_t>(s.sizeZ);
  if(a==0||b==0||c==0||a>std::numeric_limits<std::size_t>::max()/b||
     a*b>std::numeric_limits<std::size_t>::max()/c) return 0;
  return static_cast<std::size_t>(a*b*c);
}
std::size_t Index(const DensityGridSnapshot& s,std::uint32_t x,std::uint32_t y,std::uint32_t z) {
  return (static_cast<std::size_t>(z)*s.sizeY+y)*s.sizeX+x;
}
Vec3 Position(const DensityGridSnapshot& s,const GridPoint& p) {
  return s.origin+Vec3{p.x*s.spacingM,p.y*s.spacingM,p.z*s.spacingM};
}
Vec3 Gradient(const DensityGridSnapshot& s,const GridPoint& p) {
  auto sample=[&](std::uint32_t x,std::uint32_t y,std::uint32_t z){return s.density[Index(s,x,y,z)];};
  const auto xm=p.x>0?p.x-1:p.x, xp=std::min(p.x+1,s.sizeX-1);
  const auto ym=p.y>0?p.y-1:p.y, yp=std::min(p.y+1,s.sizeY-1);
  const auto zm=p.z>0?p.z-1:p.z, zp=std::min(p.z+1,s.sizeZ-1);
  const double dx=(xp==xm)?0:(sample(xp,p.y,p.z)-sample(xm,p.y,p.z))/(static_cast<double>(xp-xm)*s.spacingM);
  const double dy=(yp==ym)?0:(sample(p.x,yp,p.z)-sample(p.x,ym,p.z))/(static_cast<double>(yp-ym)*s.spacingM);
  const double dz=(zp==zm)?0:(sample(p.x,p.y,zp)-sample(p.x,p.y,zm))/(static_cast<double>(zp-zm)*s.spacingM);
  return Normalize(Vec3{-dx,-dy,-dz});
}
}

bool TriangleMeshData::Validate(std::uint64_t maxTriangles,std::string* reason) const {
  auto fail=[&](const char* m){if(reason)*reason=m;return false;};
  if(sourceRevision==0) return fail("mesh source revision must be nonzero");
  if(indices.size()%3!=0) return fail("index buffer is not a triangle list");
  const auto triCount=indices.size()/3;
  if(triCount>maxTriangles||triCount!=triangleMaterialIds.size()) return fail("triangle count/material count invalid or over budget");
  if(vertices.size()!=normals.size()||vertices.size()!=uvs.size()) return fail("vertex attribute buffer lengths differ");
  for(const auto& v:vertices) if(!v.IsFinite()) return fail("non-finite vertex");
  for(const auto& n:normals) if(!n.IsFinite()) return fail("non-finite normal");
  for(const auto& uv:uvs) if(!std::isfinite(uv[0])||!std::isfinite(uv[1])) return fail("non-finite UV");
  for(auto i:indices) if(i>=vertices.size()) return fail("triangle index is outside vertex buffer");
  return true;
}

Result<TriangleMeshData> CpuSurfaceMesher::Build(const DensityGridSnapshot& s,const MesherSettings& settings) {
  if(!s.origin.IsFinite()||!std::isfinite(s.spacingM)||s.spacingM<=0||
     s.sizeX<2||s.sizeY<2||s.sizeZ<2||s.sourceRevision==0||
     !std::isfinite(settings.isoLevel)||settings.isoLevel<0||settings.isoLevel>1||
     settings.maxTriangles==0||settings.maxInputSamples==0)
    return Result<TriangleMeshData>::Failure(ErrorCode::InvalidArgument,"invalid density-grid snapshot or mesher settings");
  const auto count=Count(s);
  if(count==0)
    return Result<TriangleMeshData>::Failure(ErrorCode::CorruptData,"density-grid dimensions overflow supported sample count");
  if(static_cast<std::uint64_t>(count)>settings.maxInputSamples)
    return Result<TriangleMeshData>::Failure(ErrorCode::BudgetExceeded,"density-grid exceeds configured input-sample budget");
  if(s.density.size()!=count||(!s.materialIds.empty()&&s.materialIds.size()!=count))
    return Result<TriangleMeshData>::Failure(ErrorCode::CorruptData,"density/material grid sizes do not match dimensions");
  for(double d:s.density) if(!std::isfinite(d)||d<0||d>1)
    return Result<TriangleMeshData>::Failure(ErrorCode::CorruptData,"density sample outside [0,1]");

  TriangleMeshData mesh; mesh.chunk=s.chunk; mesh.sourceRevision=s.sourceRevision;
  mesh.profileFingerprint=s.profileFingerprint;
  std::map<std::pair<std::size_t,std::size_t>,std::uint32_t> edgeCache;
  const std::array<GridPoint,8> offsets{{
    {0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}
  }};
  const std::array<std::array<int,4>,6> tets{{
    {{0,5,1,6}},{{0,1,2,6}},{{0,2,3,6}},{{0,3,7,6}},{{0,7,4,6}},{{0,4,5,6}}
  }};
  auto interpolate=[&](const GridPoint& a,const GridPoint& b,double va,double vb)->Result<std::uint32_t> {
    std::size_t ia=Index(s,a.x,a.y,a.z), ib=Index(s,b.x,b.y,b.z);
    const std::pair<std::size_t,std::size_t> key{std::min(ia,ib),std::max(ia,ib)};
    auto found=edgeCache.find(key);
    if(found!=edgeCache.end()) return Result<std::uint32_t>::Success(found->second);
    double denom=vb-va;
    double t=std::abs(denom)<1e-12?0.5:(settings.isoLevel-va)/denom;
    t=Clamp01(t);
    const Vec3 pos=Position(s,a)+(Position(s,b)-Position(s,a))*t;
    const Vec3 ga=Gradient(s,a),gb=Gradient(s,b);
    Vec3 normal=Normalize(ga*(1.0-t)+gb*t);
    if(LengthSquared(normal)<1e-12) normal={0,0,1};
    if(mesh.vertices.size()>=std::numeric_limits<std::uint32_t>::max())
      return Result<std::uint32_t>::Failure(ErrorCode::BudgetExceeded,"vertex index limit exceeded");
    const auto index=static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(pos); mesh.normals.push_back(normal);
    mesh.uvs.push_back({pos.x-s.origin.x,pos.y-s.origin.y});
    edgeCache.emplace(key,index);
    return Result<std::uint32_t>::Success(index);
  };
  auto triangle=[&](std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint16_t material)->bool {
    const Vec3 ab=mesh.vertices[b]-mesh.vertices[a];
    const Vec3 ac=mesh.vertices[c]-mesh.vertices[a];
    const Vec3 cross{
      ab.y*ac.z-ab.z*ac.y,
      ab.z*ac.x-ab.x*ac.z,
      ab.x*ac.y-ab.y*ac.x
    };
    // Ignore degenerate triangles before checking the output budget. They do
    // not consume a triangle slot and should not cause a false budget failure.
    if(LengthSquared(cross)<=1e-20) return true;
    if(mesh.indices.size()/3>=settings.maxTriangles) return false;
    const Vec3 face=Normalize(cross);
    const Vec3 average=Normalize(mesh.normals[a]+mesh.normals[b]+mesh.normals[c]);
    if(Dot(face,average)<0) std::swap(b,c);
    mesh.indices.insert(mesh.indices.end(),{a,b,c});
    mesh.triangleMaterialIds.push_back(material);
    return true;
  };
  auto addTet=[&](const std::array<GridPoint,4>& points,const std::array<double,4>& values,
                  const std::array<std::uint16_t,4>& materials)->bool {
    std::vector<int> inside,outside;
    for(int i=0;i<4;++i) (values[i]>=settings.isoLevel?inside:outside).push_back(i);
    if(inside.empty()||inside.size()==4) return true;
    // Material policy: majority among inside vertices; ties resolve to the
    // lowest material ID. This is deterministic and avoids arbitrary first
    // vertex selection on mixed-material tetrahedra.
    std::uint16_t material=std::numeric_limits<std::uint16_t>::max();
    unsigned bestCount=0;
    for(const int index:inside) {
      unsigned count=0;
      for(const int other:inside) if(materials[other]==materials[index]) ++count;
      if(count>bestCount||(count==bestCount&&materials[index]<material)) {
        bestCount=count;
        material=materials[index];
      }
    }
    auto edge=[&](int a,int b)->Result<std::uint32_t>{return interpolate(points[a],points[b],values[a],values[b]);};
    if(inside.size()==1||inside.size()==3) {
      const bool oneInside=inside.size()==1;
      const int pivot=oneInside?inside.front():outside.front();
      const auto& others=oneInside?outside:inside;
      auto a=edge(pivot,others[0]),b=edge(pivot,others[1]),c=edge(pivot,others[2]);
      if(!a||!b||!c) return false;
      return triangle(a.value,b.value,c.value,material);
    }
    const int i0=inside[0],i1=inside[1],o0=outside[0],o1=outside[1];
    auto a=edge(i0,o0),b=edge(i0,o1),c=edge(i1,o0),d=edge(i1,o1);
    if(!a||!b||!c||!d) return false;
    return triangle(a.value,b.value,c.value,material)&&triangle(b.value,d.value,c.value,material);
  };

  for(std::uint32_t z=0;z<s.sizeZ-1;++z) for(std::uint32_t y=0;y<s.sizeY-1;++y)
    for(std::uint32_t x=0;x<s.sizeX-1;++x) {
      std::array<GridPoint,8> cp; std::array<double,8> cv{}; std::array<std::uint16_t,8> cm{};
      for(std::size_t i=0;i<8;++i) {
        cp[i]={x+offsets[i].x,y+offsets[i].y,z+offsets[i].z};
        const auto ix=Index(s,cp[i].x,cp[i].y,cp[i].z); cv[i]=s.density[ix];
        cm[i]=s.materialIds.empty()?0:s.materialIds[ix];
      }
      for(const auto& ti:tets) {
        std::array<GridPoint,4> tp; std::array<double,4> tv{}; std::array<std::uint16_t,4> tm{};
        for(int i=0;i<4;++i){tp[i]=cp[ti[i]];tv[i]=cv[ti[i]];tm[i]=cm[ti[i]];}
        const auto before=mesh.indices.size();
        if(!addTet(tp,tv,tm))
          return Result<TriangleMeshData>::Failure(ErrorCode::BudgetExceeded,"triangle budget or vertex index cap exceeded");
        if(mesh.indices.size()<before)
          return Result<TriangleMeshData>::Failure(ErrorCode::Failed,"mesher index count regressed");
      }
    }
  std::string why;
  if(!mesh.Validate(settings.maxTriangles,&why))
    return Result<TriangleMeshData>::Failure(ErrorCode::CorruptData,"mesh validation failed: "+why);
  return Result<TriangleMeshData>::Success(std::move(mesh));
}
} // namespace wm
