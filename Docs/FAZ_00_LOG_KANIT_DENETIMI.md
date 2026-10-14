# Faz 0 — LOG sisteminin kaynak kanıtı ve kapsam denetimi

**Dal:** `plan/phase-00-baseline`  
**Kapsam:** Mekanik kataloğundaki LOG-001–LOG-076  
**Yöntem:** C++ kaynak akışı + mevcut otomatik test raporu + açık entegrasyon boşlukları  
**Önemli:** Bu, kaynak statik denetimidir. Unreal Editor içinde görsel onay anlamına gelmez ve 76 mekanik için U5/U6 tamamlandı iddiası değildir.

## Otoriter sistem sınırı

Şu an oyuncunun ana kütük etkileşim akışı `AForestCharacter` + `ACarvableLog` + `AHarvestableActor` + `StructureLink` etrafında dönüyor. `MechanicsDemo` bu akışın büyük bölümünü otomasyonla senaryolaştırıyor. Envanter klasöründeki `ALogActor` ayrı bir fiziksel sınıf ve mevcut kütükle kimlik/state eşdeğerliği kanıtlanmamış. Bu yüzden kaynak adı benzerliğinden dolayı iki sistem birleştirilmiş sayılmaz.

Kanıt bağlantıları:
- [ForestCharacter.cpp](../Source/MyProject2/ForestCharacter.cpp) — taşıma, bırakma, yerleştirme, araç kullanımı ve birincil etkileşim.
- [HarvestableActor.cpp](../Source/MyProject2/HarvestableActor.cpp) — ağaç darbesi, ağaç düşmesi, gövde bölme, taş kırma ve parça üretimi.
- [CarvableLog.cpp](../Source/MyProject2/CarvableLog.cpp) — fiziksel kütük, kesim/oyma, uç şekillendirme, taşıma durumu, hasar/yanma ve yerleştirme.
- [MechanicsDemo.cpp](../Source/MyProject2/MechanicsDemo.cpp) — otomatik regresyon senaryosu ve kabul raporları.
- [HaulCart.cpp](../Source/MyProject2/HaulCart.cpp) — el arabası/kızak/raf kapasitesi, yükleme/indirme ve hareket.
- [TEST_DURUMU.md](./TEST_DURUMU.md) — kaynak raporunda 101 mekanik kontrolü ve 12 Wood testi kaydı. Bu oturumda UE 5.8 altında tekrar çalıştırılmadı.

## LOG ID bazlı triage

| Katalog ID | Kaynak kanıtı | Kaynak denetimi sonucu | Sonraki kanıt / engel |
|---|---|---|---|
| LOG-001 | `AHarvestableActor::ApplyHit`, `StartFall`, `FinishFall`; `AMechanicsDemo::Hit` | **Kod yolu var.** Darbe sağlığı sıfırlanınca ağaç düşme durumuna girer; düşüş sonunda gövde üretilir. | UE 5.8 canlı kontrol ve mevcut ağaç türlerinin her biri için test. |
| LOG-002 | `AHarvestableActor::SplitLog` | **Kod yolu var.** Büyük gövde iki bölüme; daha küçük aşama taşınabilir `ACarvableLog` parçalarına dönüşür. | Kısa ağaç/tek kalan parça sınırlarını regression testte ayrı adlandır. |
| LOG-003 | `AHarvestableActor::LogsFromHeight`, `FinishFall` | **Formül var.** Ağacın yüksekliği üretilen kütük sayısını etkiliyor. | Farklı boyutlardan beklenen log sayısı için sınır testleri ayrı kanıtlanmalı. |
| LOG-004, LOG-011 | `AForestCharacter::StartCarry`, `GetCarryLoad`, `ACarvableLog::BeginCarry` | **Kod yolu var.** Omuz kapasitesi parça hacminden hesaplanıyor; yük sınırı aşılırsa alma reddediliyor. | Kayıt/yükleme ve tüm parça boylarında oyuncu doğrulaması. |
| LOG-005, LOG-006 | `AForestCharacter::DropOne`, `DropCarried`, `ACarvableLog::PlaceAt` | **Kod yolu var.** Omuzdaki kütük dünyaya bırakılıyor; yerdeki kütük fizik/çarpışmaya geri dönüyor. | Düşme/çarpışma, uç durum ve oyuncu engeline yakın bırakma testi. |
| LOG-007, LOG-009, LOG-010 | `ACarvableLog::Push`, fizik bileşeni ve `NotifyHit` | **Kısmi.** Serbest kütük itilip fiziksel çarpışma/hasar alabiliyor. Bu, her zeminde doğru yuvarlanma/sürtünme kalibrasyonunu kanıtlamaz. | Farklı eğim/yüzey/engel matrisinde ölçülen hareket testi. |
| LOG-008 | `AForestCharacter::StartRopeDrag`, `AHarvestableActor::BeginDrag/DragTo/EndDrag` | **Kod yolu var.** Gövdeye ip bağlanıp uç noktadan sürükleniyor. | Halat gerilimi/kopma/engeller ve save/load henüz bu kanıt kapsamına dahil değil. |
| LOG-012 | `UpdateCarryPlacementAt`, `PlaceCarried` | **Kod yolu var.** Görüş iziyle çaprazlama, dikme, kiriş, payanda, döşeme, eğim ve bazı hedefler seçiliyor. | Yerleştirme türlerinin her biri için negatif/sınır testini isimli testlere ayır. |
| LOG-013, LOG-066, LOG-067 | `AHaulCart::CanLoad/Load/Unload`, `GetCapacity`; `AForestCharacter::PlaceCarried` | **Kod yolu var.** El arabası/kızak/raf farklı hacim kapasitelerine sahip; parça yükleme ve indirme tanımlı. | Rota, eğim, devrilme, yükle beraber kalıcılık ve dünya sınırları doğrulanmalı. |
| LOG-014 | `StartRopeDrag` bir gövdeyi kısa mesafede çekiyor; zipline/uzun hat akışı kanıtı yok. | **Kanıtlanmadı.** Gövde sürükleme, yük taşıyan zipline sistemi değildir. | Ayrı hat ankrajı, hat kapasitesi, kanca/bağlama ve yük hareket testi. |
| LOG-015, LOG-076 | İncelenen ana kütük akışında su üstünde yüzdürme/rafta taşıma otoritesi belirlenmedi. | **Kanıtlanmadı.** | Su fiziği ve sal/şamandıra altyapısı tasarlanıp ayrı kanıtlanmalı. |
| LOG-016–LOG-023 | `ACarvableLog::CrossCut/Split/ShapeEnd`, `AForestCharacter::ApplyCarveHit`; `MechanicsDemo::RunCarveStep/RunShaping` | **Kod/test raporunda yol var.** Enine kesim, boyuna yarma, uç düzeltme ve uç şekilleri ana kütük akışında bulunuyor. | Her katalog ID'sini ayrı otomasyon kimliği ve test çıktısına bağla; sadece birleşik demo sonucu satır bazında yeterli değil. |
| LOG-024–LOG-029 | `ACarvableLog::Notch/CutProfile/CutDovetail/CutGroove` | **Kod yolu var.** Farklı çentik/oluk geometrileri dinamik mesh üzerinden kesiliyor; joint cut bilgisi tutuluyor. | Katalogdaki her geometri için ölçü toleransı, doğru yüz/orientasyon ve save/load testi. |
| LOG-030–LOG-031 | `ACarvableLog::ShapeEnd` | **Kod yolu var.** Sivri uç/pah şekillendirme ve trim mantığı bulunuyor. | Malzeme hacmi korunumu ve modül çizgisi testleri ayrı ID kanıtına bağlanmalı. |
| LOG-032, LOG-069, LOG-070 | `AForestCharacter::CraftFirewood/Craft`, `ACampfire`, `ACarvableLog` yanma metotları | **Kısmi.** Kütükler boyuta dayalı yakıt olarak ateşe verilebiliyor; ateş sistemi var. Firewood tarifinin ve fiziksel odun parçasının aynı kaynak hesabında olup olmadığı ayrı doğrulanmalı. | Yakıt üretimi→ateş tüketimi→kayıt/yükleme ve kaynak miktarı korunumu. |
| LOG-033–LOG-052 | `PlaceCarried`, `StructureLink::Settle`, `MechanicsDemo::RunFrameStage/AddBraceScript/AddPlatformScript/AddCabinScript/AddRecognitionScript` | **Kısmi/geniş test raporu var.** Temel çerçevesi, direk/kiriş/payanda/platform/döşeme/kabin ve bazı tanıma senaryoları var. Her çatı, merdiven, korkuluk, kasa veya geçici iskele çeşidinin tamamı aynı seviyede kanıtlanmış değil. | Her yapı türünü ayrı ID ve test haritası/oyun içi kanıtıyla işaretle; olmayanları “kısmi” bırak. |
| LOG-053 | `ComputeBuildTransform`, `UpdateCarryPlacementAt` | **Kod yolu var.** Snap/hizalama mantığı var. | Tolerans, yüzey/rotasyon ve yanlış hedef testleri. |
| LOG-054–LOG-059 | `StructureLink::Settle`, `AForestCharacter::SettleStructure`; MechanicsDemo collapse/brace senaryoları | **Kod yolu ve test raporu var.** Destek kontrolü ve kısmi çöküş senaryoları mevcut. | Karma taş/çubuk/odun için aynı yapı otoritesi ve büyük yapı maliyeti henüz kanıtlanmadı. |
| LOG-060, LOG-063 | `ACarvableLog::CanReplace/ReplaceDamaged`, `AddDamage/SetDamage` | **Kod yolu var.** Hasarlı parça değiştirme ve bağlantı kesme davranışları var. | Bağlantıların tamamında uçtan uca onarım, kesik durum ve save/load testi. |
| LOG-061 | `StructureDamageResolver` ayrı modülde eski `ALogActor/AStoneActor/AStickActor` sınıflarıyla çalışıyor. | **Entegrasyon engeli.** Bu resolver'ın ana `ACarvableLog` yapısına hasarı kanıtlanmadı. | Tek `IStructuralDamageTarget`/adapter ya da kanonik yapı graph kararı; çift hasar uygulanmadığını kanıtla. |
| LOG-062 | `ACarvableLog::ApplyHeat/Ignite/StepBurn`, `ACampfire` ve yanma demo senaryosu | **Kod/test raporunda yol var.** Isı, yakıt ve yanma durumu mevcut. | Görsel alev/ışık/yanık geçişi yalnızca editör/oyun testiyle doğrulanabilir. |
| LOG-064 | `ReplaceDamaged` hasarlı parçayı serbest bırakıyor; ateş/yakıt akışı mevcut. | **Kısmi.** Onarımdan çıkan hasarlı parçanın elde taşınması kanıtlı; genel yapı sökümünden malzeme iade oranı aynı şey değil. | Söküm/salvage oranını ve hasar/çentik kaybı politikasını ürün sözleşmesiyle tanımla. |
| LOG-065 | `AHaulCart` içindeki `EHaulKind::Rack` ve yüklü parça dizisi | **Kısmi.** Fiziksel raf/istif hedefi var; kalıcı depo stoğu veya envanterle ortak sahiplik kanıtı değildir. | Kapasite, item kimliği, erişim, dünya save/load ve diğer stok sahipleriyle tutarlılık testi. |
| LOG-068 | `StartRopeDrag` | **Kanıtlanmadı / farklı kapsam.** Bu, uzun hatlı yük taşıma sistemi değil, tek gövdeyi oyuncuya bağlayan sürükleme ipi. | LOG-014 ile aynı zipline/taşıma-hattı tasarım kapısına bağlı. |
| LOG-071–LOG-075 | İncelenen ana kütük sınıfı ve taşıma akışında tezgâh/makine üretimi, tamamlanmış tuzak/fırlatma veya genel geçici köprüleme için uçtan uca zincir gösterilmedi. | **Kanıtlanmadı veya başka sistemde aranmalı.** | Her özelliğin varlığı yalnızca katalog adı/klasörle kabul edilmez; gerçek çağrı zinciri ve test aranır. |

## LOG denetiminden çıkan mimari kararlar

1. **Kanonik kütük:** Mevcut oynanışın kütüğü `ACarvableLog` olarak korunur; ikinci log sınıfına sessiz geçiş yapılmaz.
2. **Taşıma hatları ayrı özelliktir:** Oyuncunun gövdeyi ip ile sürüklemesi, zipline/halatla uzun mesafe lojistiğine tamamlandı demek değildir.
3. **Fiziksel depo farklı kavramdır:** `AHaulCart` içindeki yükleme, ortak kalıcı stok/raf/depo ile birleşmiş sayılmaz.
4. **Test granülaritesi:** `MechanicsDemo` birleşik regresyon için değerlidir ancak 76 katalog satırının her biri kendi test kimliği ve kanıtına bağlanmalıdır.
5. **Görsel doğrulama:** Headless testler çizgi yönlendirme, mesh/collision görünümü, elde duruş ve alev görünümünü kanıtlamaz.

## Karar

LOG sistemi “baştan yapılacak” değildir; mevcut fiziksel kütük sistemi güçlü bir korunacak çekirdektir. Satır eşleştirme tablosundaki 59 kayıt U2 (yerel davranış izi), 17 kayıt U0 (bu incelemede uygulanmış davranış zinciri bulunamadı veya ayrı özellik gerekli) seviyesinde kaldı. Hiçbir satır U4/U5/U6 olarak yükseltilmedi. Özellikle LOG-014, LOG-015, LOG-061, LOG-068, LOG-072–LOG-076 için ayrı entegrasyon/geliştirme işi gerekiyor.
