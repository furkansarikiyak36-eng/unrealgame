# Faz 3 — LOW_MID Smooth Voxel Density, Mesh ve Collision

**Branch:** `feature/phase-03-lowmid-smooth-voxel`  
**Base:** Faz 2 scan/classification  
**Engine target:** Unreal Engine 5.8  
**Durum:** Kod taslağı ve statik kaynak kontrolleri yapıldı. UBT/UHT, Automation, Editor/PIE ve gerçek Landscape testleri bu oturumda çalıştırılmadı.

## 1. Bu fazın sonucu

Faz 3, önceki dalda var olan sparse excavation çekirdeğini tekrar kullanıp Faz 1/2 sözleşmelerinin arkasına adapte eder. Density'nin tek sahibi `UExcavationVolumeSubsystem` olarak kalır. Render/collision aktörü kendi içinde kalıcı ikinci bir density kopyası tutmaz.

Veri akışı:

```text
Landscape heightfield veya flat fallback
        ↓
Base density sample (0 = empty, 1 = solid)
        ↓
Sparse excavation override
        ↓
Versioned FWorldDensityChunkSnapshot (+ 1-cell border)
        ↓
CPU Marching Tetrahedra → vertices / normals / UV / triangle materials
        ↓
UExcavationTerrainChunkActor → UDynamicMeshComponent
        ↓
Collision regeneration + navigation refresh
```

## 2. Eklenen ve değiştirilen kod

| Dosya | Sorumluluk |
|---|---|
| `WorldCore/ExcavationVolumeSubsystem.h/.cpp` | `IVoxelDensitySource` implementasyonu, profile göre grid konfigürasyonu, base+override density, revision numarası, density/material ID içeren chunk snapshot |
| `WorldCore/WorldVoxelMesher.h/.cpp` | CPU-only Marching Tetrahedra, iso-level 0.5, edge vertex cache, gradient tabanlı smooth normals, UV, triangle material IDs, output triangle hard cap ve topology validation |
| `WorldCore/ExcavationTerrainChunkActor.h/.cpp` | Chunk world transform, snapshot-to-mesh conversion, Dynamic Mesh overlay'leri, material slot mapping, stale-revision rejection, collision ve navigation update |
| `WorldCore/ExcavationTerrainChunkSubsystem.h/.cpp` | LOW_MID/HIGH_END profilini alma, density grid ile eşleme, profile lock, dirty chunk queue, kare başına yayın/collision bütçesi, yakın chunk önceliği ve aktif chunk sınırı |
| `WorldCore/ExcavationLandscapeDensitySource.h/.cpp` | Landscape height sample cache'i, fiziksel materyal→material ID cache'i ve keskin binary sınır yerine continuous density ramp |
| `WorldCore/ExcavationFlatBaseDensitySource.h/.cpp` | Test/prototip düz yüzeyi için aynı smooth density ramp |
| `Tests/WorldVoxelMesherTests.cpp` | Düz iso-surface, normal yönü, edge vertex paylaşımı, geçersiz density/material buffer testleri |
| `Tests/ExcavationDensitySnapshotTests.cpp` | Snapshot boyutu/border, material ID sayısı, revision, sparse carve ve grid'i sonradan değiştirmeyi reddetme testi |
| `Scripts/run_phase3_tests.sh` | Faz 1–3 test adlarını bekleyen UE 5.8 build + automation test runner |

Eski dalda bulunan renderer'ın her üçgen için yeni üç vertex üretmesi yerine aynı density kenarındaki yüzey vertex'leri ortak kullanılıyor. Bu, tekrar eden vertex ve overlay verisini azaltır. Gerçek performans kazancı henüz benchmark edilmedi.

## 3. Grid ve profile kuralları

- LOW_MID başlangıç grid'i: `25 cm` örnek aralığı, `32` hücre/eksen, `8 m` chunk.
- HIGH_END tanımı: `12.5 cm` örnek aralığı, `64` hücre/eksen, `8 m` chunk. Faz 3 kodu bu profile göre ayar alabilir; HIGH_END performans kabulü veya benchmark sonucu değildir.
- Bir chunk snapshot'ı her eksende `Cells + 1 + 2*BorderCells` sample tutar. Border bir hücre olduğu için LOW_MID snapshot boyutu `35³`; renderer içteki hücreleri işler ve dış örnekleri gradient/normaller için kullanır.
- Density revision, edit/import/rollback/base-provider değişiminde ilerler. Snapshot örnekleme sırasında revision değişirse snapshot reddedilir. Mesher sonucu da yayınlanmadan önce güncel revision ile eşleştirilir.
- Sparse override içeren kaydedilmiş hücreler varken profile grid'i farklıysa subsystem, hücre koordinatlarını sessizce yeniden yorumlamak yerine başlangıcı reddeder. Profil oturum sırasında kilitlenir.
- Landscape height/material sorguları XY kolon anahtarıyla sınırlı bir cache kullanır. Cache sınırına gelince sıfırlanır; sınırsız büyümez.

## 4. Smooth density ve mesh

Base density 0–1 aralığında normalize edilir. Iso-surface değeri 0.5'tir. Landscape/flat prototype kaynakları için geçiş alanı yaklaşık bir voxel aralığı genişliğinde continuous ramp olarak hesaplanır; düz binary eşik doğrudan kullanılıp yüzey basamaklandırılmaz.

Marching Tetrahedra her cube'u deterministik altı tetrahedra olarak işler. Edge cache aynı sample-edge kesişimini yeniden kullanır; density gradient'inden smooth normal çıkarılır ve sıfır gradient için solid→empty yönüyle fallback yapılır. Üçgen winding'i dışarıya, boşluk tarafına doğru yönlendirilir. Density, material ID ya da mesh indeksleri geçersizse mesh yayınlanmaz.

Landscape source şu an heightfield yüksekliği ve fiziksel material mapping kullanır. Bu, Landscape'ın boyanmış weightmap katmanlarını okuduğu ya da Unreal Landscape verisini yazarak değiştirdiği anlamına gelmez.

## 5. Collision ve çalışma bütçesi

- Chunk actor, doğrulanmış CPU mesh sonucunu `UDynamicMeshComponent` içine aktarır.
- Üçgen varsa collision `QueryAndPhysics` olur ve collision update çağrılır; boş mesh'te collision kapalıdır.
- Her density edit, etkilenen chunk'ları dirty queue'ya ekler; aynı koordinat kuyruğa ikinci kez eklenmez. Kuyruk taşarsa tek tek kayıtları sessizce kaybetmek yerine full refresh işareti konur.
- Her frame işlenecek chunk sayısı `min(MaxChunksPublishedPerFrame, MaxCollisionUpdatesPerFrame)` ile sınırlandırılır. Otomatik streaming yakın chunk'lara öncelik verir ve seçili profilin `MaxActiveEditableChunks` sınırını uygular.
- `bAllowAsyncCollision` Dynamic Mesh collision cooking seçeneğine aktarılır.
- **CPU meshing şu an kuyruk işleyicisi içinde senkron çalışır**; `MaxMeshJobsInFlight` / background meshing worker'ları henüz uygulanmış değildir. Frame budget aynı frame'de kaç chunk işlendiğini sınırlar ama tek chunk rebuild'inin game-thread süresini sınırlamaz. Gerçek Low–Mid kabulünden önce profili UE 5.8'de ölçüp spike varsa CPU meshing'i worker task'ına taşıma işi gerekir.

## 6. Otomasyon testleri

Yeni test adları:

- `MyProject2.World.VoxelMesher.HorizontalPlane`
- `MyProject2.World.VoxelMesher.InvalidSampleRejected`
- `MyProject2.World.VoxelDensity.SnapshotRevisionAndGrid`

Test runner aşağıdaki Faz 1/2 testlerini de ister; toplam 10 test adı kontrol edilir.

Windows Git Bash ve UE 5.8 kurulu ortamda, Unreal Editor kapalıyken:

```bash
bash Scripts/run_phase3_tests.sh
```

Derleme, UHT ve Automation testleri bu ortamda çalıştırılmadı. PR taslak kalmalı ve bu testler geçmeden faz motor-doğrulamalı olarak kabul edilmemeli.

## 7. Faz 3'te yapılmayanlar

- Oyuncu input'undan kazı işlemini başlatan uçtan uca Scan→Edit→mesh akışı (Faz 4).
- Worker-thread/background mesh jobs; mesher saf CPU kodu fakat şu an çağrı senkron.
- Dedicated collision update scheduler / tamamlanma callback'li async cooking ölçümü.
- Oyuna özgü stratified 3D materyal katmanlarının density-depth fonksiyonu. Mevcut material IDs Landscape physical-material mapping'e dayanır.
- Editor içi Landscape Builder overlay/weightmap painter.
- Tahkimat, destek/çökme, survival ve kalıcı kayıt uçtan uca akışı.
- Low–Mid/High-End cihaz benchmark'ı.

## 8. Sonraki kapı

Önce `Scripts/run_phase3_tests.sh` UE 5.8 altında başarılı çalışmalı. Ardından Editor/PIE üzerinde en az şu kontrol yapılmalı: flat fallback yüzeyi, gerçek Landscape proxy yüzeyi, komşu chunk seam'i, carve edit sonrası görünür mesh ve collision, rollback/import sonrası revision + rebuild ve streaming aktif chunk sınırı. Bu kapılar geçilmeden Faz 4'e geçişte oyun içi kazı akışının çalıştığı varsayılmamalı.
