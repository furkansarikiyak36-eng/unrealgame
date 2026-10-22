# Faz 1 — Ortak Runtime Sözleşmeleri ve Profil Omurgası

**Branch:** `feature/phase-01-world-contracts-lowmid`  
**Hedef:** Unreal Engine 5.8 / C++  
**Durum:** İlk kod katmanı yazıldı; UE 5.8 derleme/otomasyon/Editor testi bu çalışma alanında çalıştırılmadı.

## Bu fazda eklenenler

| Dosya | Sorumluluk | Gerçek durum |
|---|---|---|
| `WorldCore/WorldRuntimeProfile.h/.cpp` | LOW_MID ve HIGH_END profil verisi, default ayarlar, profil doğrulama | Kod yazıldı; UBT doğrulaması bekliyor |
| `WorldCore/WorldRuntimeProfileSubsystem.h/.cpp` | Dünya başına aktif profili seçmek, doğrulamak ve seans sırasında grid profili kilidini sağlamak | Kod yazıldı; henüz voxel bootstrap çağırmıyor |
| `WorldCore/WorldSystemContracts.h` | Classification, voxel edit/density, mesh, collision, scan, placement, support, persistence ve survival API sözleşmeleri | Header sözleşmeleri yazıldı; gerçek adapter'lar sonraki fazlarda |
| `Tests/WorldRuntimeProfileTests.cpp` | Profil değerleri/grid tutarlılığı için Unreal Automation Test tanımı | Test yazıldı; çalıştırılmadı |
| `Docs/PHASE_00_BASELINE_AUDIT.md` | Kanonik sistem sahipliği ve çakışma riskleri | Statik denetim belgesi |

## Başlangıç profil değerleri

| Değer | LOW_MID | HIGH_END |
|---|---:|---:|
| Örnek aralığı | 25 cm | 12,5 cm |
| Chunk kenarı | 800 cm | 800 cm |
| Hücre/samples per axis | 32 | 64 |
| Yaklaşık hacim örnek sayısı | 32³ = 32.768 | 64³ = 262.144 |
| Paralel mesh işi üst sınırı | 1 | 3 |
| Kuyruk sınırı | 8 | 32 |
| Kare başına mesh publish üst sınırı | 1 | 2 |
| Kare başına collision update üst sınırı | 1 | 2 |
| Yüzey örnekleri / eksen | 5 | 9 |
| Aktif editlenebilir chunk hedefi | 32 | 96 |
| Adaptive voxel LOD | Kapalı | Açık (ileride uygulanacak) |
| Asenkron collision izni | Kapalı | Açık (ileride doğrulanacak) |

Bunlar ilk profil konfigürasyonu ve test başlangıç değerleridir, performans garantisi veya benchmark sonucu değildir. HIGH_END profilinin daha ince yoğunluğu aynı 8 m hacimde yaklaşık sekiz kat yoğunluk örneği oluşturur. Üretilen mesh/collision, geçici buffer ve malzeme verisi gerçek bellek maliyetini yükseltir.

## Bilinçli sınırlar

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
4. Profili kilitlemeden yapılandırma yapılabilir; kilit sonrası değişiklik reddedilir.
5. Mevcut ana dal kütük/StructureLink davranışı değişmeden kalır.

## Sıradaki faz

**Faz 2 — LOW_MID Scan + Classification:** mevcut Scanner taslak PR'larını gözden geçirerek ortak sözleşmelere uyumlama; Actor tag/manual override/landscape layer tabanlı classification map; statik kaya ile EditableVoxel bölgelerinin deterministik ayrımı; editör önizlemesi ve yalnızca istenen/kirli bölgelerde çalışan Builder.

Faz 2 de kod olarak eklenecek, ardından UE 5.8 build ve küçük test haritası üzerinden doğrulanacak. Gerçek smooth terrain üretimi Faz 3'tedir.
