# Faz 1 — Ortak Runtime Sözleşmeleri ve Profil Omurgası

**Branch:** `feature/phase-01-world-contracts-lowmid`  
**Hedef:** Unreal Engine 5.8 / C++  
**Durum:** İlk kod katmanı yazıldı; UE 5.8 derleme/otomasyon/Editor testi bu çalışma alanında çalıştırılmadı.

## Bu fazda eklenenler

| Dosya | Sorumluluk | Gerçek durum |
|---|---|---|
| `WorldCore/WorldRuntimeProfile.h/.cpp` | LOW_MID ve HIGH_END profil verisi, default ayarlar, grid tutarlılığı, enum ve profil doğrulama | Kod yazıldı; UBT doğrulaması bekliyor |
| `WorldCore/WorldRuntimeProfileSubsystem.h/.cpp` | Dünya başına aktif profili seçmek, doğrulamak ve seans sırasında grid profili kilidini sağlamak | Kod yazıldı; henüz voxel bootstrap çağırmıyor |
| `WorldCore/WorldSystemContracts.h` | Classification, voxel edit/density, mesh, collision, scan, placement, support, persistence ve survival API sözleşmeleri; sınıflandırma/edit/raycast/trace/overlap/sample/placement istek validasyonu; density sample storage ve mesh topology doğrulaması | Header sözleşmeleri yazıldı; gerçek adapter'lar sonraki fazlarda |
| `Tests/WorldRuntimeProfileTests.cpp` | LOW_MID/HIGH_END defaultları, grid/enum/queue doğrulaması, profile lock, request validation, density buffer boyutu ve mesh indeksleri için Unreal Automation Test tanımları | 3 test tanımı yazıldı; UE 5.8'de çalıştırılmadı |
| `Scripts/run_phase1_tests.sh` | UE 5.8 build + `MyProject2.World` automation grubunu çalıştırır; beklenen üç test adını ve başarı/failure satırlarını doğrular | Bash sözdizimi kontrol edildi; Windows Git Bash + Unreal koşusu bekliyor |
| `Docs/PHASE_00_BASELINE_AUDIT.md` | Kanonik sistem sahipliği ve çakışma riskleri | Statik denetim belgesi |

## Başlangıç profil değerleri

| Değer | LOW_MID | HIGH_END |
|---|---:|---:|
| Örnek aralığı | 25 cm | 12,5 cm |
| Chunk kenarı | 800 cm | 800 cm |
| Chunk grid cells per axis | 32 | 64 |
| Hücre sayısı yaklaşımı (32³ / 64³) | 32.768 | 262.144 |
| Paralel mesh işi üst sınırı | 1 | 3 |
| Kuyruk sınırı | 8 | 32 |
| Kare başına mesh publish üst sınırı | 1 | 2 |
| Kare başına collision update üst sınırı | 1 | 2 |
| Yüzey örnekleri / eksen | 5 | 9 |
| Aktif editlenebilir chunk hedefi | 32 | 96 |
| Adaptive voxel LOD | Kapalı | Açık (ileride uygulanacak) |
| Asenkron collision izni | Kapalı | Açık (ileride doğrulanacak) |

Bunlar ilk profil konfigürasyonu ve test başlangıç değerleridir, performans garantisi veya benchmark sonucu değildir. HIGH_END profilinin daha ince yoğunluğu aynı 8 m hacimde yaklaşık sekiz kat hücre oluşturur. Density snapshot buffer'ı ayrıca iki chunk sınır düzlemini ve border örneklerini tutar; örnek sayısı `(CellsPerChunkAxis + 1 + 2 * BorderCells)^3` şeklinde hesaplanır. Üretilen mesh/collision, geçici buffer ve malzeme verisi gerçek bellek maliyetini yükseltir.

## Bilinçli sınırlar

Contract header'ındaki doğrulayıcılar ön koşul kontrolüdür; bir isteği gerçekleştirmez, collision channel seçmez ve kaynak ayırmaz. Mesher, voxel edit servisi, collision updater ve classification/placement adapter'ları sonraki fazların sorumluluğundadır.

Bu fazda **bilerek yapılmayanlar**:

- Landscape Builder'ın gerçek dünya tarama ve sınıflandırma algoritması.
- Sparse voxel çekirdeğinin ana dala taşınması/entegrasyonu.
- Smooth meshing algoritması ve collision güncellemesi.
- Scan sorgularının var olan `AForestCharacter` akışına bağlanması.
- Voxel yüzeyine snap edilen tahkimat aktörleri ve destek grafiği.
- Survival, su/toprak ve persistence uçtan uca entegrasyonu.
- Unreal Engine 5.8 UBT build, Automation Test çalıştırması veya Editor/PIE testi.

Bu sınırlar, sözleşmelerin gerçek implementasyonlar gibi yanlışlıkla kullanılmasını önlemek içindir.

## Faz 1 çıkış kapısı

Faz 1 ancak şu koşullarda kapatılır:

1. Unreal Engine 5.8 UBT build başarılı.
2. `WorldRuntimeProfile.DefaultProfilesValidate` otomasyon testi çalışır ve geçer.
3. LOW_MID varsayılan seçim olarak yüklenir; geçersiz grid ayarı reddedilir.
4. Profili kilitlemeden yapılandırma yapılabilir; kilit sonrası değişiklik reddedilir (`MyProject2.World.RuntimeProfile.ConfigureAndLock`).
5. Faz 1 otomasyon betiği build sonucunu, üç beklenen test adını ve test sonuçlarını doğrular. Komut: `bash Scripts/run_phase1_tests.sh`.
6. Request validation bozuk edit, trace, surface sample ve placement isteklerini reddeder; density sample buffer boyutu ve mesh üçgen indeksleri kontrol edilir (`MyProject2.World.Contracts.ValidateRequestsAndBuffers`).
7. Mevcut ana dal kütük/StructureLink davranışı değişmeden kalır.

## Sıradaki faz

**Faz 2 — LOW_MID Scan + Classification:** mevcut Scanner taslak PR'larını gözden geçirerek ortak sözleşmelere uyumlama; Actor tag/manual override/landscape layer tabanlı classification map; statik kaya ile EditableVoxel bölgelerinin deterministik ayrımı; editör önizlemesi ve yalnızca istenen/kirli bölgelerde çalışan Builder.

Faz 2 de kod olarak eklenecek, ardından UE 5.8 build ve küçük test haritası üzerinden doğrulanacak. Gerçek smooth terrain üretimi Faz 3'tedir.
