# Faz 0–12 — Statik çapraz sistem denetimi

**Denetim tarihi:** 14 Ekim 2026  
**İncelenen kaynak head'i:** `58dcc674209c2649d606291787cc0dd2cd08206c`  
**Kapsam:** Faz 0–12 kodu, entegrasyon sözleşmeleri, ortak veri sahipliği, çağrı/işlem sınırları, kayıt kapsamı ve mevcut kanıt belgeleri.  
**Kapsam dışı:** Unreal Engine derlemesi, UHT, Editor/Blueprint görsel doğrulaması, oyun içi Automation ve uzun oturum. Bu denetimde UE çalıştırılmadı.

## Karar özeti

**Faz 0–12'nin tümünü “birbiriyle uyumlu ve kabul edilmiş” olarak işaretlemek için yeterli kanıt yok.** Ortak çekirdeğin bazı ana bağlantıları tutarlı hâle getirilmiş; buna karşılık birkaç önemli akış hâlâ ayrı sahiplikte, eksik bağlanmış veya hata durumunda atomik değil. Bu rapor bir kaynak düzeyi ön denetimidir; Faz 14'ün 485 mekaniğin tümüne uygulanacak kapsamlı kabulü değildir.

Mevcut head için GitHub Python tooling CI #1011 başarılı görünmektedir: [run #1011](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38539954356). Bu koşum Python/shell araçlarını, kaynak sözleşme testlerini ve katalog denetimlerini kapsar; Unreal Engine 5.8 derlemesi veya oyun içi davranış kanıtı değildir.

## Uyumlu görünen bağlantılar

1. **Oyuncu stack envanteri:** `AForestCharacter` artık `UInventoryComponent` olan `CoreInventory` örneğini oluşturuyor; stack'li kaynak işlemleri `GetItemCount/AddItem/RemoveItem` adaptörleri üzerinden bu kaynağa gidiyor. Taşınan büyük kütük gibi fiziksel nesneler stack'e zorla indirgenmiyor. Kaynaklar: [ForestCharacter.cpp](../Source/MyProject2/ForestCharacter.cpp), [InventoryComponent.cpp](../Source/MyProject2/InventoryCore/InventoryComponent.cpp). Bu, eski bulgu matrisindeki “karakterde iki stack inventory aynı anda otorite” ifadesini güncelleme ihtiyacını doğuruyor; fiziksel nesne dönüşüm köprülerinin kapsamı ise hâlâ ayrı bir konu.

2. **Dünya hücresi ve bölge sorgusu:** Bölgesel kaynak/risk sorguları ayrı bir kaynak deposundan değil, `UWorldStateSubsystem/UChunkStateStore` verisinden türetiliyor. Lojistik düğüm/bağlantı snapshot'ı ayrı kaydedilse de bölgesel kaynak alanı ikinci otorite olarak tutulmuyor. Kaynak: [RegionLogisticsSubsystem.cpp](../Source/MyProject2/WorldCore/RegionLogisticsSubsystem.cpp).

3. **Tarım–yem köprüsü:** `UFeedCycleComponent` artık kendi bağımsız stok haritasını işletmek yerine aynı actor üzerindeki `UFeedStorageComponent` üzerinden ekleme/tüketim yapıyor. Önceden bunu ayrı bir stok sahibi gibi tarif eden bulgu satırları yeniden sınıflandırılmalı. Kaynak: [FeedCycleComponent.cpp](../Source/MyProject2/PhaseT9_Integration/FeedCycleComponent.cpp).

4. **Üretim–envanter işlemi:** Üretim kuyruğu canonical `UInventoryComponent::ExchangeItemBatches` işlemiyle girdileri rezerve edip çıktıları bırakıyor; aktif job için input/output envanterlerinin kaydedilebilir oyuncu veya storage sahibi üzerinde olması kontrol ediliyor. Bunun gerçek istasyon, güç sağlayıcısı ve save/load turunda çalıştığı henüz motor içinde kanıtlanmış değil. Kaynak: [ProductionChainComponent.cpp](../Source/MyProject2/Phase11_Production/ProductionChainComponent.cpp), [WorldPersistenceSubsystem.cpp](../Source/MyProject2/WorldCore/WorldPersistenceSubsystem.cpp).

5. **Baskın dalga üretimi:** Aktif `URaidDirectorSubsystem` kod yolu, dalga ve event toplam spawn bütçesini sınırlandırıyor; üretilen düşmanları event sahipliğiyle izliyor ve iptal sırasında yalnızca bu event'in yönettiği aktörleri temizliyor. Bununla birlikte oyuncu bağlamı, fiziksel yapı hasarı ve alternatif spawn pipeline sınıfının çağrı noktaları tamamen kabul edilmiş değil. Kaynak: [RaidDirectorSubsystem.cpp](../Source/MyProject2/RaidCore/RaidDirectorSubsystem.cpp).

## Açık bulgular

| ID | Önem | Gözlenen durum | Gerekli sonuç |
|---|---|---|---|
| AUD-01 | **Kritik** | Core save zarfı kazı, yeraltı tehlikeleri, dünya hücreleri, lojistik, üretim kuyruğu, oyuncu stack stoğu ve depoları kapsıyor; fiziksel `ACarvableLog`, taş/çubuk aktörleri, inşa edilmiş yapılar ve tarım/hayvan aktörleri için genel bir aktör snapshot/yeniden oluşturma zinciri görünmüyor. Ayrıca eski `USaveGameSubsystem::LoadWorld` yalnızca `ALogActor` kaydediyor; yüklemede mevcut logları önce yok ediyor, spawn başarısız olunca devam ediyor ve sonunda yine `true` döndürüyor. | Tek bir koordineli kayıt kapsamı/politikası belirlenmeli. Yükleme öncesi tam preflight yapılmalı; başarısız aktör üretiminde mevcut dünya korunmalı veya oluşturulan aktörler rollback edilmeli. “Load başarılı” ancak tüm zorunlu snapshot'lar geri yüklendiğinde dönmeli. |
| AUD-02 | **Kritik** | `UWorldPersistenceSubsystem::LoadCoreStateFromSlot` alan snapshot'larını sırayla içe alıyor. Sonraki lojistik/üretim içe aktarması başarısız olursa envanter/üretim için bazı rollback yolları var; daha önce içe alınmış excavation, hazard, world-state ve lojistik snapshot'larının tamamını tek işlem olarak eski hâline döndüren ortak rollback yolu görünmüyor. | Bütün domain snapshot'ları staging alanda hazırlanıp doğrulanmalı; toplu commit veya tüm-domain rollback garantisi verilmeli. Hata enjeksiyonu ile doğrulanmalı. |
| AUD-03 | **Kritik** | `UPhysicalToInventoryBridge` yalnızca `ALogActor/AStoneActor/AStickActor` alıp bir adet genel item ekledikten sonra fiziksel aktörü yok edebiliyor; transform, hasar, şekil ve yapı bağlantısı genel item'a aktarılmıyor. Ana oynanabilir kütük tipi `ACarvableLog` ile bu köprü arasında eşleme yok. | Kayıplı dönüşüm, kimliği/state'i korunarak uygulanmalı veya fiziksel aktörün dönüşümü açıkça reddedilmeli. Köprü kullanımı gerçek oyuncu akışında izlenmeli. |
| AUD-04 | **Kritik** | `UStructureDamageResolver::ApplyDamageToBlock` yalnızca `ALogActor/AStoneActor/AStickActor` türlerine hasar veriyor; ana fiziksel yapı akışındaki `ACarvableLog` için adaptör yok. Yıkım sonrası mevcut kod nav alanını işaretliyor ve ilgili `ALogActor` üzerindeki köprü bağlantısını kapatabiliyor, fakat bütün yapı/support grafı sahipleri için ortak yıkım bildirimi kanıtlanmıyor. | Bir ortak yapısal hasar/yıkım sözleşmesi gerekli. Aynı hasar tek kez işlenmeli; destek, malzeme, navigasyon ve lojistik bağımlılıkları ortak olay üzerinden güncellenmeli. |
| AUD-05 | **Yüksek** | `UBridgeComponent::BuildBridge` döngüsü bazı log aktörlerini oluşturup sonraki spawn başarısız olduğunda `false` döndürebiliyor; önceden oluşturulan parçalar rollback edilmiyor. Rota yalnızca bütün parça spawn'ları başarılı olursa kaydediliyor. | Köprü inşası all-or-nothing olmalı: başarısız denemede üretilen bütün parçalar temizlenmeli veya açık ve eksiksiz bir partial-build modeli tanımlanmalı. |
| AUD-06 | **Yüksek** | `URaidDirectorSubsystem::CaptureContext` her oyuncuya `Level=1` atıyor ve `WorldMultiplier=1` sabitliyor. Dolayısıyla gerçek oyuncu ilerlemesi/survival süresi ve dünya saatiyle zorluk hesabı bağlanmış görünmüyor. | Kanonik oyuncu/survival ve dünya saati sağlayıcısına bağlanmalı; event-start snapshot mı canlı yenileme mi olduğu açıkça belirlenmeli. |
| AUD-07 | **Yüksek** | `UMaterialChunkIntegration` geçerli bir `CandidateID` bulunamazsa sessizce Wood profiline, o da yoksa geçici bir Wood tanımına geri dönebiliyor. Bilinmeyen materyal ID'si bu yüzden yanlış materyal davranışıyla işlenebilir. | Bilinmeyen ID için belirlenmiş politika (reddet/atla/belgeli fallback), tanılama ve test eklenmeli; sessiz yanlış eşleştirme kaldırılmalı veya açıkça sınırlandırılmalı. |
| AUD-08 | **Yüksek** | `AWaterCollector::Tick` şu anda yalnızca “hava sisteminden yağış girdisiyle değiştir” yorumunu çalıştırıyor; gerçek yağış-toplayıcı bağlantısı yok. | Collector, ortak hava/yağış ve yakalama alanına bağlanmalı; kuru havada su üretmediği, taşma ve kayıt davranışı doğrulanmalı. |
| AUD-09 | **Yüksek** | `UInventoryToPhysicalBridge` aktör sınıfını bridge ayarından seçip yalnızca verilen `ItemID` stoğunu düşüyor; item ID ile seçilen fiziksel sınıf/definition arasında bu fonksiyonda tip uyumu doğrulaması veya tam item payload'ı aktarımı görünmüyor. | `ItemID ↔ fiziksel sınıf/definition` eşlemesi doğrulanmalı; mismatch spawn öncesinde reddedilmeli. |
| AUD-10 | **Yüksek** | `UProductionChainComponent` güç ve işçi koşullarını kontrol ediyor, ancak repoda ortak generator/power-grid sahibine canlı bağlantı kanıtı yok; güç, dış sistemin verdiği `AvailablePowerWatts` değeri. | Bu, eksik entegrasyon olarak açıkça kalmalı; güç sağlayıcısı/gride bağlanmadan enerji ekonomisi tamamlandı denmemeli. |
| AUD-11 | **Orta** | `HollowGroove`, `EngraveJoint` ve `SmoothSurface` şu anda `false` döndürerek işlevin mevcut olmadığını doğru biçimde reddediyor. Sessiz başarılı no-op problemi azaltılmış olsa da özelliklerin davranışı henüz yok. | Gerçek geometri/bağlantı değişimi uygulanmalı veya ilgili katalog satırları desteklenmiyor olarak açıkça işaretlenmeli. |
| AUD-12 | **Orta** | Raid kaynağının üst başlığı 69 alt mekanik belirtirken ayrıntılı listede 63 isim bulunuyor. Mevcut katalog denetimi 485 satırın şema/eşleme tutarlılığını doğrular; bu gereksinim uyuşmazlığını tek başına çözmez. | Kaynak sahibiyle 63/69 farkı kapatılmalı; altı eksik gereksinim uydurulmamalı. |

### Bulgu matrisiyle belge uyumsuzluğu

`Docs/FAZ_00_BULGU_MATRISI.csv` içindeki bazı durumlar Faz 6–12 sonrasındaki kaynakla yeniden uzlaştırılmalı. Örnekler:

- **P0-022:** Matris hâlâ karakterdeki eski `TMap<EForestItem,int32>` envanterini açık problem olarak tarif ediyor; güncel `ForestCharacter.h/.cpp` `CoreInventory` kullanan geçişi içeriyor. Fiziksel item dönüşümü ve SaveGame kapsamı yine açık kalıyor.
- **P0-019 / P0-007:** Eski bağımsız FeedCycle stok sahibi anlatımı güncel `FeedCycleComponent` davranışıyla uyuşmuyor; bileşen artık `UFeedStorageComponent` adaptörü.
- **P0-006 / P0-018:** “Yıkım hook'u boş” ifadesi artık tümüyle doğru değil; mevcut hook nav dirty area güncelliyor. Ancak `ACarvableLog` hasar adaptörü ve ortak yapı grafı entegrasyonu hâlâ çözülmemiş.
- **P0-026:** Stick breaking ve stone crushing kaynaklarında kısmi spawn başarısızlığında parçaları temizleme/rollback korumaları bulunuyor. Bu nedenle matris satırının güncel uygulama ve kalan riskle yeniden eşleştirilmesi gerekiyor.

Matris satırları gerçek bulguları kaybetmeden “çözüldü”, “kısmen çözüldü”, “açık” ve “motor testi bekliyor” diye güncellenmeli; eski bir gözlem, yeni kodu okumadan mevcut kusur olarak raporlanmamalı.

## Sıralı düzeltme önerisi

1. **Save/load ve fiziksel dünya kapsamı:** hangi aktörlerin hangi sahibiyle kaydedileceğini belirle; eski log save yolundaki destructive load/yanlış başarı sonucunu ele al; tüm-domain rollback sözleşmesini kur.
2. **Fiziksel nesne, yapı ve raid:** item–actor eşleme/kimlik kaybını kontrol et; raid hasarını ana fiziksel yapı ve destek grafına bağla.
3. **İşlem atomikliği:** köprü spawn rollback'ini düzelt; spawn-failure enjeksiyonları ekle.
4. **Eksik ortak girdiler:** raid zorluk bağlamı, bilinmeyen materyal ID politikası, yağış-toplayıcı ve güç sağlayıcısı ilişkisini karara bağla.
5. **Kanıt ve doküman senkronizasyonu:** Faz 0 bulgu matrisi, ADR ve takip tablosunu güncel kaynağa göre uzlaştır; her kalan madde için açık kabul ölçütü tanımla.

## Kabul sınırı

Bu denetimden sonra **kaynak düzeyindeki tutarsızlıklar daha görünür hâle getirildi**, ancak Faz 0–12 kabul edildi denemez. Özellikle kaynakta görülen Save/Load atomikliği ve fiziksel aktör kapsamı çözülmeden; yapı hasarı–destek grafı bağı kurulmadan; köprü kısmi üretim rollback'i sağlanmadan kaynak bütünlüğü ve uçtan uca uyumluluk garanti edilmiş olmaz.

Unreal Engine 5.8 derlemesi veya oyun testi bu turda çalıştırılmadı; bunlar ayrı kabul kanıtı olarak kalmalıdır.
