# Faz 3 — LOW_MID Smooth Voxel Density, Mesh ve Collision

**Branch:** `feature/phase-03-lowmid-smooth-voxel`  
**Base:** `feature/phase-02-scan-classification-lowmid`  
**Engine hedefi:** Unreal Engine 5.8  
**Durum:** Uygulama kodu ve statik tutarlılık kontrolleri hazır. UBT/UHT derlemesi, Automation testleri ve Editor/PIE doğrulaması henüz çalıştırılmadı; PR bu yüzden taslak kalmalı.

## 1. Faz 3'ün sonucu

Faz 3, daha önce ayrı bir dalda hazırlanmış sparse excavation çekirdeğini Faz 1/2 sözleşmelerine bağlar. Density'nin tek sahibi `UExcavationVolumeSubsystem` olarak kalır; renderer kendi içinde kalıcı ikinci bir kazı verisi tutmaz.

Veri akışı:

```text
Landscape heightfield veya flat fallback
        ↓
Base density (0 = empty, 1 = solid)
        ↓
Sparse excavation override
        ↓
Revision taşıyan FWorldDensityChunkSnapshot (+ 1-cell border)
        ↓
Game thread: snapshot örneklemesi
        ↓
Worker pool: CPU Marching Tetrahedra → vertex / normal / UV / triangle material ID
        ↓
Game thread: revision kontrolü ve Dynamic Mesh yayını
        ↓
Frame bütçesi içinde collision + navigation güncellemesi
```

## 2. Eklenen/değiştirilen kaynaklar

| Dosya | Sorumluluk |
|---|---|
| `WorldCore/ExcavationVolumeSubsystem.h/.cpp` | `IVoxelDensitySource` implementasyonu, LOW_MID grid konfigürasyonu, base + sparse override örnekleme, revision, bordered density/material snapshot |
| `WorldCore/WorldSystemContracts.h` | Density snapshot'ına source revision ve material-sample alanlarının eklenmesi |
| `WorldCore/WorldVoxelMesher.h/.cpp` | Saf CPU Marching Tetrahedra, shared edge/node cache, gradient normals, UV, material ID, hard triangle cap ve topology validation |
| `WorldCore/ExcavationTerrainChunkActor.h/.cpp` | CPU mesh sonucunu Dynamic Mesh'e çevirme, source revision kontrolü, material slot eşleme, collision ve navigation update |
| `WorldCore/ExcavationTerrainChunkSubsystem.h/.cpp` | Runtime profile ile grid'i eşleme, dirty/full-refresh kuyruğu, worker-job yönetimi, tamamlanan mesh sonuçlarını yayın bütçesine alma, chunk streaming/aktif chunk sınırı |
| `WorldCore/ExcavationLandscapeDensitySource.h/.cpp` | Landscape heightfield density ramp'i, XY yükseklik cache'i ve fiziksel material→material ID cache'i |
| `WorldCore/ExcavationFlatBaseDensitySource.h/.cpp` | Test/prototip düz yüzeyi için continuous density ramp'i |
| `Tests/WorldVoxelMesherTests.cpp` | Düz yüzey, shared vertex, exact ISO düğümleri ve geçersiz buffer testleri |
| `Tests/ExcavationDensitySnapshotTests.cpp` | Bordered snapshot boyutu, carve sonrası revision/density ve grid'i değiştirmeyi reddetme |
| `Scripts/run_phase3_tests.sh` | Faz 1, 2 ve 3 test adlarını zorunlu tutan UE 5.8 build + Automation runner |

## 3. Density grid ve snapshot

- LOW_MID başlangıç profili: `25 cm` örnek aralığı, `32` hücre/eksen, `8 m` chunk.
- Normalized density: `0` boşluk, `1` katı, `0.5` ISO-surface.
- Bir chunk snapshot'ı eksen başına `Cells + 1 + 2*BorderCells` örnek taşır. Bir hücrelik border ile LOW_MID örnek boyutu `35³` olur.
- Snapshot hem density hem temel yüzey material ID örneklerini, ayrıca source revision'ı taşır. Storage, sayısal aralıklar ve metadata meshing öncesinde kontrol edilir.
- Edit, rollback, import ve base density kaynağı değişimi revision'ı ilerletir. Snapshot sonunda revision tekrar kontrol edilir; mesh sonucu da yayınlanmadan önce güncel revision'a karşı doğrulanır. Eski bir worker sonucu mevcut mesh'in yerini alamaz.
- Var olan sparse override'lar farklı hücre ölçüsü/axis grid'inde saklanmışsa runtime bu koordinatları sessizce yeniden yorumlamaz; başlangıcı reddeder. Runtime profile oturum açılırken kilitlenir.
- LOW_MID ayarına geçmeden önce mevcut sparse data kullanımı varsa grid eşleşmesi ayrıca doğrulanmalıdır.

## 4. Smooth mesh üretimi

`FWorldMarchingTetrahedraMesher`, her hücre küpünü deterministik altı tetrahedron olarak işler. Çıktı chunk-local santimetre koordinatlarında vertex, normal, metre ölçekli planar UV ve üçgen başına material ID içerir.

Mesher:
- density 0.5 eşik yüzeyini kenar boyunca interpolasyonla çıkarır;
- aynı voxel edge ve tam ISO-level grid node kesişimini yeniden kullanır;
- density gradient'inden smooth normal çıkarır; sıfır gradient için sonlu fallback normal üretir;
- üçgen yönünü katı taraftan boşluğa doğru hizalar;
- hatalı density/material storage ve geçersiz indeksleri reddeder;
- üçgen/attribute buffer tutarlılığını ve çıktı üçgen limitini denetler.

Mesher hard cap'i chunk başına 1.000.000 üçgendir; chunk actor'ın varsayılan kabul limiti 200.000 üçgendir. Gerçek vertex/triangle sayıları veya performans kazancı için henüz Unreal benchmark'ı yapılmadı.

Landscape ve flat fallback kaynakları keskin binary yükseklik eşiği yerine yaklaşık bir örnek aralığı genişliğinde continuous density ramp'i üretir. Böylece yükseklik iso yüzeyi örnek hücre merkezlerine kilitlenmez. Landscape entegrasyonu loaded heightfield ve physical-material mapping kullanır; painted Landscape weightmap katmanlarını okumaz veya Landscape verisini değiştirmez.

## 5. Worker meshing, yayın ve collision bütçesi

Density snapshot'ı game thread'de örneklenir; çünkü Landscape/base-density provider UObjects kullanabilir. Sadece immutable sample/material array'lerini alan saf CPU mesher worker pool'a gider. Worker tarafında UObject veya component değiştirilmez.

Tamamlanan sonuç game thread'e geri gelir ve şu kapılardan geçer:
1. chunk koordinatı ve mesh topology kontrolü;
2. snapshot source revision kontrolü;
3. material slot mapping;
4. Dynamic Mesh'e atomik yayın;
5. collision ve navigation güncellemesi.

Aynı, hâlâ güncel density revision'ı için snapshot veya meshing başarısız olursa eski chunk mesh/collision'ı güvenli biçimde geçersizleştirilir. Hata daha eski bir revision'dan geldiyse bu eski sonuç yeni geometriyi silmez; chunk güncel revision ile yeniden kuyruğa alınır.

LOW_MID başlangıç değerleri:
- `MaxMeshJobsInFlight = 1`;
- `MaxQueuedMeshJobs = 8`;
- frame başına en fazla bir mesh yayını ve bir collision güncellemesi;
- `MaxActiveEditableChunks = 32`;
- async collision cooking kapalı.

Dirty queue tekrar eden chunk isteklerini birleştirir. Kuyruk taşarsa tüm active kayıtları tek seferde kuyruğa boşaltmak yerine incremental full-refresh cursor kullanılır; kuyruğa ve tamamlanmış worker sonuçlarına profil limitleri uygulanır. Otomatik streaming yakın chunk'ları önceliklendirir. Sistem, profile göre async collision cooking ayarını da component'e aktarır.

Bu değerler başlangıç politikasıdır; hedef düşük/orta donanım üzerinde henüz ölçülmemiştir. HIGH_END profili ve kabul performans testi bu fazın onay kapsamına girmez.

## 6. Otomasyon kapsamı

Yeni test adları:
- `MyProject2.World.VoxelMesher.HorizontalPlane` — ISO yüksekliği, üçgen üretimi, vertex paylaşımı, normal yönü ve material ID buffer'ı.
- `MyProject2.World.VoxelMesher.ExactIsoNodesStayConnected` — density örneklerinin tam 0.5 olduğu durumda vertex paylaşımı/topoloji.
- `MyProject2.World.VoxelMesher.InvalidSampleRejected` — bozuk density/material storage'ın reddedilmesi.
- `MyProject2.World.VoxelDensity.SnapshotRevisionAndGrid` — snapshot border/örnek sayısı, sparse carve sonrası revision/density ve farklı profile grid'i uygulamayı reddetme.

Runner ayrıca Faz 1/2'deki yedi mevcut dünya testini de arar; toplam **11 test adı** beklenir.

Windows Git Bash/MSYS ve Unreal Engine 5.8 kurulu ortamda, Unreal Editor kapalıyken:

```bash
bash Scripts/run_phase3_tests.sh
# Motor başka dizindeyse:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase3_tests.sh
```

Runner önce `MyProject2Editor` Development build yapar, sonra `Automation RunTests MyProject2.World` çalıştırır. Bu işlem bu oturumda çalıştırılmadı.

## 7. Faz 3 kapanmadan önce zorunlu doğrulamalar

- UE 5.8 UBT/UHT build ve runner'ın aradığı 11 Automation testinin başarıyla geçmesi.
- Editor'de flat fallback ve gerçek Landscape yüzeyini inceleme.
- PIE'da chunk sınırını kesen carve/fill testi; iki komşu chunk'ın yeniden üretilmesi.
- Güncellenen mesh collision'ının karakteri engellediğinin kontrolü.
- Density değişirken eski worker sonucunun yayımlanmadığının ve full-refresh/dirty queue'nun sonunda güncel geometriye ulaştığının kontrolü.
- Phase 2 scan'in `World.Object.VoxelSurface` işaretli terrain'i bulması; kazma yetkisinin yalnızca Phase 2 capability kuralları izin verdiğinde verilmesi.
- Map/world teardown sırasında bekleyen worker işi, Landscape cache'i ve collision log'larının incelenmesi.
- Düşük/orta hedef donanımda CPU zamanı, worker kuyruğu, collision cook, bellek ve üçgen sayısı benchmark'ı.

## 8. Bu fazda yapılmayanlar

- Input'tan başlayan uçtan uca Scan → edit → mesh oyun kazısı akışı (Faz 4).
- LOD/adaptive resolution ve GPU meshing.
- Stratified material katmanlarını 3D derinlik fonksiyonundan hesaplama; şimdiki material ID kaynağı Landscape physical-material mapping'dir.
- Painted Landscape weightmap okuma/yazma.
- Destek/tahkimat, hasar/çökme, survival ve kayıt sisteminin uçtan uca entegrasyonu.
- HIGH_END kabulü ve cihaz benchmark'ı.

**Kapanış ölçütü:** Kaynak değişiklikleri ve statik kontroller hazır; motor içinde doğrulanmadığından Faz 3 henüz “testlerden geçti” veya “tamamıyla doğrulandı” sayılmaz. Runner ve Editor/PIE kapıları geçtikten sonra Faz 4'e geçilmelidir.
