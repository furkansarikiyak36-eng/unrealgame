# Faz 0 — Kod Sahipliği ve Çakışma Denetimi

**Proje:** MyProject2 / Unreal Engine 5.8  
**Branch:** feature/phase-01-world-contracts-lowmid  
**Durum:** Kaynak ve mimari envanteri; UE 5.8 derleme raporu değildir.

## Amaç

Yeni sistemleri var olan sınıfların üzerine eklemek; ikinci bir kazı motoru, ikinci bir kütük sistemi veya ikinci bir dünya kayıt otoritesi oluşturmamak. Bu denetim, mevcut repository kaynakları ve proje belgeleri üzerinden hazırlanmıştır. Bu oturumda Unreal Build Tool, Editor veya PIE çalıştırılmadı.

## Canonical sahiplik kararları

| Alan | Mevcut sahip / kaynak | Faz planı kararı |
|---|---|---|
| Fiziksel ağaç/kütük oynanışı | `AHarvestableActor`, `ACarvableLog`, `AForestCharacter` | Ana fiziksel kütük akışını koru; alternatif `ALogActor` yolunu otomatik olarak birleştirme. |
| Kütük yapı desteği | `StructureLink` ve Wood yapı mantığı | Yeni tahkimat için adapter ekle; ikinci, bağımsız destek otoritesi yaratma. |
| Sparse 3B kazı verisi | `UExcavationVolumeSubsystem`, `FExcavationVolumeSnapshot`, `IExcavationBaseDensitySource` — `plan/phase-00-baseline` dalı | Bu veri çekirdeğini denetleyip yeniden kullan. İkinci density map oluşturma. |
| Dünya simülasyonu hücreleri | `UChunkState` ve `WorldCore` / W1–W6 kodu | 2B/çevresel hücre state'ini 3B kazı density verisiyle aynı şey sayma; birleştirme adapter'ı kullan. |
| Scan / Hit Analyzer | `WorldScanSystem` taslak PR #4–#5 | Kaynakları gözden geçirip ortak Scan sözleşmesine uyumlandır; ana dala merge edildiğini varsayma. |
| Kütük Snap | `LogSnapSystem` taslak PR #3 | Mevcut kütük kurallarını koru ve generic placement provider arkasına adapt et. |
| Generic WorldSnap | `Plugins/WorldSnap`, `WorldSnapScanBridge` taslak PR #6 | Yalnızca aday seçimini yapar; trace, çarpışma onayı veya yapısal geçerliliğin sahibi değildir. |
| Yerleştirme / karakter girişi | `AForestCharacter` ve var olan placement fonksiyonları | Her şeyi bir anda yeniden yazma; önce mevcut akışa ince bir adapter ekle, sonra mekanik bazında geçir. |
| Survival | `USurvivalComponent` | Mevcut oyuncu statlarını koru; kazı/yeraltı etkilerini adapter üzerinden ekle. |
| Dünya kayıtları | `WorldPersistence` ve mevcut ilgili subsystem'ler | Kazı delta'sı + fiziksel aktör + bağlantı state'inin tek başına mevcut world save içinde zaten bulunduğunu varsayma; kapsamı E2E testle kanıtla. |
| Landscape Builder | Mevcut ana dalda canonical builder bulunması doğrulanmadı | Yeni bir sınıflandırma haritası ve editör/runtime iş akışı olarak ekle; scanner ile karıştırma. |
| Smooth surface meshing/collision | Kazı veri API'si tek başına mesh/collision üretmiyor | Ayrı arayüz/adapter ekle; önce tek küçük test alanını uçtan uca tamamla. |
| Kazı tahkimatı | Mevcut kütük inşasına benzer parçalar var, voxel tahkimat entegrasyonu kanıtlanmadı | Yeni yerleştirilebilir actor türlerini var olan placement ve destek sistemlerine adapte et. |

## Bulunan kritik entegrasyon riskleri

1. **Kodun varlığı, akışın varlığı değildir.** Repo içinde birden fazla sistem veya veri tipi bulunabilir; çağrı zincirinin uçtan uca çalıştığı ayrıca gösterilmelidir.
2. **Kütük/tahta kanonik kimliği:** `ACarvableLog` ile eski/alternatif `ALogActor` aynı nesne modeli gibi kullanılmamalıdır.
3. **Kazı data core vs render:** Sparse density delta'sı görünür smooth mesh veya güncellenmiş collision değildir. Bu ara katman henüz kurulmalıdır.
4. **Scan vs placement:** Scanner hedefi/normalini verir; Snap aday seçer; Placement çakışma ve commit kurallarını doğrular; destek grafiği ise yük ilişkisini yönetir.
5. **Dünya çevre hücreleri vs voxel hücreleri:** Su, nem, toprak besini, biyokütle için kullanılan state doğrudan 3B kazı geometrisinin yerini tutmaz.
6. **Persistence kapsamı:** World/chunk kayıtlarının fiziksel kütükleri, taşları, tahkimat parçalarını ve ilişkilerini sakladığı varsayılamaz.
7. **Test kanıtı:** Eski test kayıtları daha sonra hazırlanan Scan/Snap/voxel/tahkimat PR'larının derlendiğini veya oyun içinde çalıştığını kanıtlamaz.

## Faz 0 çıkış koşulları

- Tek bir canonical voxel kazı veri sahibi seçildi.
- Tek canonical fiziksel kütük/yapı sistemi belirlendi.
- Scan, classification, meshing, collision, placement, support, survival ve persistence arayüz sınırları tanımlandı.
- Ana dal / taslak PR / yeni geliştirme durumları kod belgelerinde birbirinden ayrıldı.
- Faz 1'de yalnızca ortak veri tipleri, profil ayarları ve interface contracts eklenir; henüz oynanışın başarılı olduğu iddia edilmez.

## Doğrulama sınırı

Bu belge statik repository okuması ve mimari sahiplik kararlarını kaydeder. **UE 5.8 UBT derleme, Editor/PIE ve runtime doğrulaması yapılmış değildir.** Bu nedenle Faz 1 tamamlandığında gerçek build/test çalıştırılana kadar derlenebilirlik durumu ayrıca açık tutulmalıdır.
