# MyProject2 — 14 Fazlık Uygulama Omurgası

**Hedef:** Faz 0–14 boyunca bütün mekanik kodlarını mevcut MyProject2 çekirdeğiyle uyumlu, modüler ve birbirine bağlanabilir biçimde kurmak. Kullanıcıya ara demo paketi verilmez; tek Windows demo bütün fazlar tamamlandıktan sonra hazırlanır.  
**Motor:** Unreal Engine 5.8  
**Ana yaklaşım:** Önce sistemleri tasarlayıp kodlamak ve entegrasyon sözleşmelerini kurmak; kapsamlı hata avı, bütün katalog incelemesi, regresyon ve performans analizi Faz 14'te toplu yapılır.

## Çalışma prensibi

1. **Kod üretimi ana ilerleme ölçüsüdür.** 485 mekanik satırının her biri için ayrı editör kanıtı çıkarılması sonraki kodlamanın ön koşulu değildir. Mekanikler sahip fazlarında uygulanır; mevcut modüller bulunup tekrar kullanılır, eksik olanlar ortak mimariye uygun geliştirilir.
2. **Katmanlar ve sözleşmeler önce gelir.** Her modül kendi sorumluluğunu taşır; diğer modüllere kararlı C++ arayüzleri, ortak kimlikler ve açık işlem sonuçları üzerinden bağlanır. Aynı envanter, dünya hücresi, yapı grafı veya üretim stoğu iki farklı sistemde kopya otorite olarak tutulmaz.
3. **Geliştirme sırasında yalnızca hafif kontroller.** Yeni kodun yerel tutarlılığını korumak için gereken statik/syntax kontrolleri ve küçük hedefli kontroller yapılır. 485 mekanik için kapsamlı kanıt toplama ve uçtan uca hata avı her adımda tekrarlanmaz.
4. **UE testleri geliştirmeyi bekletmez.** Unreal Engine 5.8 build ve oyun testleri kullanıcının Claude Code/UE ortamında çalıştırılabilir. Bu çalışma ortamında UE mevcut olmadığı için sırf motor testi beklenerek kod aşaması durdurulmaz; çalıştırılmamış motor testleri başarılı diye de raporlanmaz.
5. **Belirsizlikleri uydurmayız.** Eksik gereksinim ya da risk açık bir backlog maddesi olur ve sahibi olan fazda çözülür. Örneğin RAID kaynağındaki 69 başlık/63 detay farkına altı hayalî mekanik eklenmez.
6. **Toplu derin analiz Faz 14'tedir.** Bütün fazlar kodlandıktan sonra 485 mekanik, katmanlar arası çağrı zincirleri, çifte sahiplik, kaynak/miktar bilançosu, save/load, hata/rollback, performans ve regresyon birlikte gözden geçirilir. Bulunan kusurlar yayın kapısı geçmeden düzeltilir.
7. **Kullanıcıya tek teslimat.** Ara faz paketleri yalnızca dahili geliştirme/doğrulama içindir. Tek kullanıcıya açık Windows demo, son kabul turundan sonra teslim edilir.

## Katman mimarisi

- **L0 — Ortak çekirdek:** Kanonik ID'ler, world/time/seed, durum sahipliği, olay kimliği, transaction/rollback, ortak sonuç tipleri ve sürümlü kayıt sınırları. Ana sözleşmeler Faz 2'de sabitlenir; daha önce yapılan prototipler bunlarla uyumlu olacak şekilde adaptörlenir.
- **L1 — Alan sistemleri:** Kazı/terrain, kütük, taş, çubuk, materyal, su-toprak, tarım/hayvan, bölge, lojistik, üretim ve raid kendi veri modeli ve iş kurallarına sahip olur.
- **L2 — Entegrasyon adaptörleri:** Alan sistemlerini birbirine bağlar. Örnekler: fiziksel öğe↔envanter, materyal↔world cell, ortak yapı grafı, üretim↔stok, raid↔yapı hasarı. Adaptörler otoriter state sahibi değil; doğrulanmış dönüşüm/işlem köprüsüdür.
- **L3 — Oyun akışı/orkestrasyon:** Oyuncu girdisi, olay zamanlayıcıları, baskın akışı ve üretim kuyrukları. Aynı işi yapan ikinci bir orkestratör oluşturulmaz.
- **L4 — Son sertleştirme:** Faz 14'te çapraz faz analizleri, kapsamlı hata/kenar durum taraması, regresyon, kalıcılık ve performans; ardından tek final paket.

Yeni bir sistem eklenirken önce mevcut sorumlusu aranır. Aynı işi yapan eski sistem varsa paralel otorite oluşturmak yerine tek bir kanonik sistem seçilir ve uyum adaptörü yazılır.

## Fazlar

| Faz | Başlık | Uygulama çıktısı | Faz içi ilerleme ölçütü |
|---|---|---|---|
| 0 | Baseline ve katalog eşleştirmesi | Kanonik 485 satır, dokuz aile eşleştirmesi, statik kaynak triage ve entegrasyon riskleri | ID ve eşleştirmeler tutarlı; açık sorunlar doğru fazlara aktarılmış |
| 1 | Kazı veri temeli | Sparse yerel 3B kazı delta modeli, sabit koordinat sözleşmesi, geri alınabilir kazı işlemi ve backend sınırı | Kuyu/tünel/oda için aynı veri modeli mümkün; geçersiz işlem durumu değiştirmez |
| 2 | Core contract ve kalıcı dünya verisi | Kimlik/sahiplik, event/idempotency, sparse delta, sürümlü kayıt sözleşmeleri | Modüller ortak kontratları kullanır; ikinci state otoritesi oluşmaz |
| 3 | Kazılabilir arazi | Runtime hacim sorgusu, mesh/collision bağlama katmanı, chunk seam ve kalıcı delta | Kazı verisi ile üretilen görünür geometri arasında açık adaptör ve deterministik state |
| 4 | Yeraltı yapısı ve tehlikeleri | Destek/çökme, toprak-kaya profili, su girişi ve tuzaklar | Aynı dünya/hacim kontratı üzerine bağlanır |
| 5 | Kütük sistemini sağlamlaştırma | Mevcut `ACarvableLog` akışı, yapı tipleri, hasar ve geri kazanım | Mevcut çalışan çekirdek korunur; yeni sistemlerle tek sorumluluk sınırı belirlenir |
| 6 | Taş, çubuk ve envanter entegrasyonu | Fiziksel öğe→taşıma/stok→işleme→inşa→kurtarma | Kaynak tüketimi/spawn/geri alma tek bir işlem sözleşmesine bağlanır |
| 7 | Yapı grafı ve materyal simülasyonu | Kanonik yapı grafı; yangın/nem/darbe/çürüme adaptörleri | Wood/stone/stick etkileri ortak bağlantı ve sahiplik katmanına bağlanır |
| 8 | World State | Su, toprak, nem, biyokütle, yangın, çürüme ve kimya | Ortak zaman/seed/world state üzerinden çalışır |
| 9 | Tarım ve hayvancılık | Ekim→büyüme→hasat; yem→hayvan→ürün/gübre→toprak | Hasat, stok, yem ve çıktı ayrı stok adaları oluşturmaz |
| 10 | Bölge ve lojistik | Bölgesel kaynak/risk; patika, yol, köprü, rampa, kızak, halat/makara | Rota ve taşıma sistemleri world/structure state'e adaptörle bağlanır |
| 11 | Oyuncu üretim zincirleri | Kütük→testere→tahta/kereste/yan ürün; stok, kapasite, kuyruk ve güç | Üretim girdi/çıktıları ortak kaynak muhasebesinden geçer |
| 12 | Raid Director | Tek spawn sahibi, scheduler, hedef, navigasyon ve ortak yapı hasarı | Baskın orchestration'ı tek akıştan geçer; dünya/yapı etkisi ortak arayüzü kullanır |
| 13 | Bölge hikâyesi ve atmosfer | En az üç mekanikçe ayrışan bölge; harabe/iz/nesne temelli anlatı | Çevresel anlatı bölge verisi ve oyuncu keşif döngüsüne bağlanır |
| 14 | Bütünsel analiz, hata ayıklama ve içerik kilidi | 485 mekanik toplu inceleme, entegrasyon/regresyon, save/load, kaynak bilançosu, performans ve blocker raporu | Katman sınırları ve çapraz sistem akışları analiz edilir; kritik kusurlar kapatılır |
| 15 | Tek final Windows demo | Tek paket, README, kontroller, test kayıtları, performans ve bilinen sorunlar | Temiz açılış, 30–60 dakika oynama, iki save/load turu ve kabul checklist'i |

### Faz 2 — uygulanan çekirdek sözleşme

- `UWorldPersistenceSubsystem`, dünya kimliği, deterministik kaydedilmiş seed, monotonik dünya revizyonu ve kazı alt sisteminin commit/rollback/reset olaylarını ortaklaştırır. `UExcavationVolumeSubsystem` kazı hücrelerinin tek otoriter sahibi olmaya devam eder.
- `ApplySphereEditOnce`, `ApplyCapsuleEditOnce` ve `ApplyBoxEditOnce`, çağıranın sağladığı `FGuid` komut kimliğiyle tekrar denemelerin aynı edit'i ikinci kez uygulamasını engeller. Son 100.000 komut kimliği ve son 100.000 olay anahtarı kayıt zarfında tutulur; bu sınırlı geçmiş penceresinin dışındaki kimlikler kalıcı idempotency garantisi kapsamında değildir.
- `UWorldCoreSaveGame` sürümlü manifest, dünya ID/seed/revizyonu, kazı sparse snapshot'ı ve sınırlı idempotency/event defterlerini taşır. Yüklemede manifest ve defterler önce doğrulanır; sparse snapshot'ın doğrulanması ve tek adımda değiştirilmesi mevcut `ImportSnapshot` yoluna bırakılır. Başarılı yükleme kaydedilmiş revizyonu korur.
- **Sınır:** Bu, genel kayıt zarfının Faz 2 temelidir ve şu anda kazı sparse delta'sını kaydeder. Terrain mesh/collision, envanter ve diğer alan sistemlerinin kendi sahipliğindeki kayıtları bu aşamada tamamlanmış sayılmaz; ilgili fazlarda aynı zarf genişletilecektir.
- `Scripts/tests/test_world_persistence_contract.py` CI'a bağlı statik kontrat kontrolüdür. Unreal Engine 5.8 C++ derleme veya oyun otomasyonu bu ortamda çalıştırılmadı; motor doğrulaması ve bütüncül çapraz sistem audit'i Faz 14'e kadar açık kalır.

### Faz 3 — runtime arazi geometri adaptörü (uygulama durumu)

- AExcavationTerrainChunkActor, mevcut UExcavationVolumeSubsystem density örneklemesinden Marching Tetrahedra yüzeyi üretir ve aynı dinamik mesh bileşenini complex-as-simple fizik çarpışması için kullanır. Kazı hücresi/yoğunluğu için ikinci bir otorite oluşturmaz.
- UExcavationTerrainChunkSubsystem, mevcut 2B simülasyon chunk store'unun yerine geçmez. Sadece 3B görsel/çarpışma chunk aktörlerini kaydeder, değişiklik event'lerini etkilenmiş chunk'lara yönlendirir ve snapshot yüklemesi/reset sonrası geometriyi yeniden üretir. Böylece mevcut UWorldChunkSubsystem / UWorldStateSubsystem sahipliği korunur.
- Otomatik renderer streaming varsayılan olarak oyuncunun etrafında 3D chunk'ları küçük bir bütçeyle üretir; başlangıç ayarları yatay yarıçap 1, oyuncu chunk'ının 4 kat altı ve her tick'te en fazla 1 yeni chunk'tır. TerrainChunkActorClass, projeye özel Blueprint chunk alt sınıfı üzerinden malzeme/profil özelleştirmesini mümkün kılar.
- UExcavationLandscapeDensitySource, yüklü ALandscapeProxy yükseklik alanlarını temel density sağlayıcısına bağlar; yüklü Landscape bulunmazsa prototip düz yüzey sağlayıcısı kullanılır. Proje tarafından kaydedilen aktif custom provider her zaman bu fallback kaynağından önceliklidir.
- Malzeme sözleşmesi artık SampleBaseMaterialIdAtWorld API'sini tanımlar. Landscape sağlayıcısı GetPhysicalMaterialAtLocation sonucunu, UExcavationTerrainChunkSubsystem.LandscapeMaterialMappings ile kararlı ID'ye çevirebilir; ID, AExcavationTerrainChunkActor.StratifiedMaterialLayers içindeki eşleşen malzeme slotuna aktarılır. Daha katmanlı/dinamik stratigrafi gereken projeler aynı arayüzü kendi provider'ında uygulayabilir. ID 0 varsayılan SurfaceMaterial slotudur; ID 1–31 eşleştirilebilir.
- Dinamik mesh üçgenlerinde material-ID attribute etkinleştirilir. Mesh/collision üretimi ve temizliği sonrası UNavigationSystemV1::UpdateComponentInNavOctree çağrılır.
- Her chunk örneklemesi için sonlu density ve geçerli koordinat kontrolü yapılır; 64 hücre/eksen ve 200.000 triangle varsayılan üst sınırı aşılırsa bozuk/aşırı mesh collision'a commit edilmez. Geliştirme sürümü tam chunk mesh rebuild kullanır; daha ayrıntılı per-chunk incrementality/performance ayarı Faz 14 toplu analizine bırakılmıştır.
- Açık entegrasyon noktası: Landscape'in kendi render/collision yüzeyiyle gerçek runtime hand-off (delik/görünürlük/çarpışma yetkilisi) henüz tamamlanmadı. Mevcut density provider Landscape yüksekliğini okur ve ayrı voxel mesh üretir; orijinal Landscape yüzeyinin oyukta kaybolduğunu henüz garanti etmez. Ayrıca UE 5.8 build/runtime kabulü bekliyor. Bu iki madde kapanmadan Faz 3 tamamlanmış sayılmaz.

### Faz 4 — yeraltı desteği, çökme, su ve tuzak katmanı (uygulama durumu)

- AUndergroundSupportActor, ABuildingNodeActor'dan türetilir ve mevcut UStructuralIntegritySubsystem yük/çökme grafına katılır; ikinci bir taşıyıcı grafı açmaz. Ahşap direk/brace, taş sütun ve güçlendirilmiş kiriş profilleri kapasite, ağırlık ve nem dayanımıyla ayrılır. Varsayılan görünür mesh Unreal temel küpüdür; projeye özel mesh Blueprint alt sınıfıyla değiştirilebilir.
- UUndergroundHazardSubsystem, kazı event'leri ve ortak density örneklemesi üzerinden boşluk hücrelerini izler. Açıklık eşiğini aşan, yakında çalışan desteği bulunmayan tavan hücresi için küçük ve sınırlı bir carve işlemi yapar. Çökme re-entrancy guard, tick başına collapse bütçesi ve tick başına reconciliation/stability bütçeleriyle sınırlandırılır.
- Hidroloji, UWorldStateSubsystem'in mevcut 2B yüzey hücresindeki su hacmini veya eşik üzerindeki toprak nemini ölçülü biçimde azaltıp 3B cave-cell water pocket'a aktarır. Kuru yüzey/zeminden su üretilmez; aynı yüzey hücresinden bir hydrology tick içinde yalnızca bir kez sızıntı yapılır. Mağara cepleri açık alt hücreye sınırlı hacim aktarımıyla yerçekimine göre akar.
- AUndergroundTrapActor mevcut AStoneTrapActor hasar/overlap akışını kullanır; üstündeki boşluk için minimum cavity-clearance doğrulaması yapabilir ve Blueprint çağrısıyla belirli hacimde su cebi varsa tetiklenebilir.
- Faz 2'nin ortak UWorldCoreSaveGame zarfı schema 2'ye yükseltildi. Faz 4 desteğin kimlik/konum/tip/dayanıklılığını, 3B su ceplerini ve underground seepage nedeniyle değişen 2B kaynak hücrelerinin son su/nem değerlerini kaydedip yükleyebilir. Önceki schema 1 kayıtları boş Phase 4 snapshot varsayılanıyla okunabilir.
- **Doğrulama sınırı:** Python kontrat testi CI'a bağlıdır ve başarılıdır; bu Unreal Engine 5.8 C++ derlemesi veya runtime sonucu değildir. Collapse eşiği/malzeme gerilme parametreleri ve uzun oyun oturumu/akış dengesi Faz 14 çapraz sistem analizinde kalibre edilecektir.

## Kazı teknolojisi seçimini nasıl ele alacağız

Faz 1'de bütün performans/benchmark bürokrasisini tamamlamadan ortak veri sözleşmesini ve değiştirilebilir backend sınırını kuracağız. İlk uygulama seyrek yerel 3B hücre/density delta yaklaşımıdır; A/B Dynamic Mesh CSG alternatifi not edilir ancak ilk kod katmanını bloke etmez. Nihai optimizasyon/alternatif seçimi, gerçek kısıtlar görüldükten sonra Faz 14 bütünsel analizinde yapılabilir. Kuyu, yatay tünel, oda ve chunk-sınırı gereksinimleri baştan veri modelinin parçasıdır; yalnızca heightmap deformasyonuna indirgenmez.

## Kanıt ve durum bildirimi

U0–U6 etiketleri kataloğun mevcut bilgi durumunu anlatır; geliştirme sırasını tek başına bloke etmez. Statik kontrol, UE automation ve görsel/uzun oturum kabulü birbirinden ayrı tutulur. Çalıştırılmamış bir test hiçbir zaman geçmiş olarak yazılmaz; fakat bekleyen test, uygun kod katmanının geliştirilmesini durdurmaz.

## Tek kullanıcı teslimatı

Faz 0–14 tamamlandıktan ve Faz 14 bütünsel analizindeki kritik kusurlar giderildikten sonra Faz 15 başlar: tek Windows packaged build, temiz başlatma, 30–60 dakikalık oynanış, iki save/load çevrimi, kontroller/README, sonuç kayıtları, performans ve bilinen sınırlamalar raporu. Ara fazlarda kullanıcıya paket teslim edilmez.


### 2026-10-14 — Faz 5: mevcut kütük ve onarım akışının sertleştirilmesi

- İkinci bir kütük sahibi oluşturulmadı; mevcut `ACarvableLog` ve `StructureLink` kanonik kaldı. Onarım önkoşulu artık yalnızca uzunluğu değil kesit boyutlarını ve split sayısını da karşılaştırır; taşınan/hareket eden, yanmakta olan veya zaten eklemleri kesilmiş stok onarım adayı olamaz.
- `ReplaceDamaged`, eski eklem kayıtlarını yenileme parçasına kopyalarken işlemi yedek mesh ile korur. Herhangi bir kesim yeniden üretilemezse geometri, eklem aralıkları ve yerleşim geri yüklenir; kısmi kesim başarılı onarım olarak bildirilmez.
- Onarım tamamlandığında mevcut `StructureLink::Settle` akışı yeniden çalıştırılır; destek grafı ve önbelleğe alınan yapı türleri eski geometride kalmaz. Mevcut `MechanicsDemo` carving/repair delegate bağlantıları korunur.
- `Scripts/tests/test_carvable_log_contract.py` statik entegrasyon sözleşmesi eklendi ve Python CI runner'a bağlandı. Bu testler source contract'larını doğrular; Unreal Engine 5.8 C++ derlemesi veya oyun içi görsel kanıt yerine geçmez. Faz 5 görsel/runtime kabul kanıtı final kabul turunda tamamlanacak; bütünsel çapraz-faz audit Faz 14'tedir.

## Faz 6 — Envanter transfer, fiziksel kaynak ve storage bağlantısı (uygulama durumu)

- Oyuncunun stack stoğunun sahibi artık `UInventoryComponent`; `AForestCharacter` eski `TMap<EForestItem,int32>` kopyasını tutmaz. Mevcut `GetItemCount`/`AddItem`/`RemoveItem` çağrıları bu tek kaynağa yönlendirilir. `ACarvableLog` kütükleri ve taşınan gövdeler fiziksel aktör olarak kalır; paralel bir kütük/çubuk envanteri açılmaz.
- `ForestItems::GetInventoryID` ve item ağırlık sözleşmesi mevcut oyun enum'unu `InventoryCore` kimliğine eşler. Çubuk için `SM_GB_Stick` greybox mesh'i eklenmiştir. Yalnızca serbest, ham çubuk stack stoğa alınır; işlenmiş/yapıya bağlanmış çubuklar dünya aktörü olarak korunur.
- `TransferItem`, kaynak ve hedef slotlarının tam kopyasını alır, iki inventory'de de geçici event yayınını bastırır ve sadece iki taraf başarıyla güncellenince event yayınlar. Herhangi bir başarısızlık iki slot düzenini de geri yükler. `ExchangeItems` işleme/tarif dönüşümünü aynı envanter içinde atomik yapar; kendi girdisini kendi çıktısına çeviren no-op/çoğaltma tarifleri reddedilir.
- `CanDropItem` envanter miktarıyla `bCanBeDropped` politikasını spawn öncesi doğrular. `DropInventoryItem` genel stack kaynaklarında pickup yığını üretir; taş ve çubukta her sayım bir gerçek fiziksel aktöre dönüştürülür. Bütün aktörler spawn olduktan sonra bir defada stok düşülür; spawn/debit başarısız olursa geçici aktörler temizlenir. Pickup tarafında envantere tam miktar eklenemezse kaynak aktörü yok edilmez.
- `AInventoryStorageActor` kendi stack stoğunu aynı `UInventoryComponent` ve `TransferItem` işlemiyle tutar. Benzersiz `StorageKey` şartı ve deterministik sıralı `FInventoryStorageSnapshot` ile depolar ortak save zarfına girer.
- `UWorldCoreSaveGame` schema 3, oyuncu ve storage `FInventoryItem` snapshot'larını kaydeder. Load öncesi player/storage kapasitesi, depo anahtarları, item kimliği/miktarı ve domain snapshot doğrulamaları yapılır; önceki schema 1 ve 2 kayıtları okunabilir. Fiziksel dünya aktörlerinin tamamını persistence'a eklemek bu fazın dışında kalır.
- `Scripts/tests/test_inventory_transfer_contract.py` ve `Scripts/tests/test_phase6_inventory_integration.py` Python runner'a bağlıdır. Bu kontrol statik sözleşme denetimidir; UE 5.8 C++ build/Automation kanıtı değildir. CI #675 ve #691 test sözleşmesi uyuşmazlıkları yakaladı; assertion ve storage preflight beklentisi düzeltildi. Güncel dal head'i için Python tooling CI #696 başarılı (#38530520494). Bu run statik Python/tooling testidir; UE 5.8 C++ build/Automation veya oyun içi E2E kanıtı değildir.
- Faz 6'nın **uygulama katmanı tamamlandı**; tracker statik CI #696'yı kanıt olarak kaydeder. UE 5.8 E2E kabulü (depo transferi, işleme, fiziksel drop/pickup, build/salvage, iki save/load turu) ve çapraz katman bilançosu henüz çalıştırılmadı; bunlar son kabul/Faz 14'e kalır ve kodlama sırasını bloke etmez. 485 mekanik çapraz-sistem denetimi yine Faz 14'tedir.

## Faz 7 — tek malzeme durumu ve yapı etkileri (uygulama durumu)

- `ACarvableLog::Damage` mevcut ahşap bütünlüğü olmaya devam eder. Nemden ve zamanla çürümeden gelen aşınma bu değere eklenir; yeni bir paralel sağlamlık/yapı hesabı oluşturulmaz. Hasar eşikleri mevcut `StructureLink::Settle` çağrı zincirini kullanır.
- `UBurnSubsystem` yanma/ısınma ve yakındaki kütüklere ısı yayılımının tek sahibi kalır. Yeni malzeme adaptörü ikinci bir `EBurnState` tutmaz; nem, tutuşma eşiğini ve mevcut yakıt sayacının süresini etkiler.
- `UWoodMaterialSubsystem`, ortak `UWorldStateSubsystem` saat/hava durumundan günlük bir malzeme aralığı alır, yağmur gökyüzü görünürlüğünü kontrol eder ve en çok `MaxLogsProcessedPerTick` kütüğe uygular. Kütükler sabit ada sırasıyla işlenir; uzun yakalama aralığı 365 günle sınırlandırılır.
- İsteğe bağlı `UMaterialDefinition` profili su emilimi/hasar/çürüme katsayılarını sağlar; profil atanmamışsa muhafazakâr ahşap varsayılanları kullanılır. `MoistureContent` ve `MaterialAgeDays` mevcut aktör üzerindeki durum alanlarıdır; genel `UWorldCoreSaveGame` henüz dinamik dünya aktörlerini yeniden oluşturmaz. Tam aktör snapshot/kalıcılık ve dünya hücresi nemiyle birleştirme Faz 14 çapraz sistem denetiminde ele alınacaktır.
- `Scripts/tests/test_wood_material_integration_contract.py` canonical integrity/fire/structure owner'larını ve bütçeli hava entegrasyonunu kontrol eder. Python tooling CI #737 başarılıdır (run `38531310496`); Unreal Engine derleme/Automation veya görsel oyun testi yerine geçmez.



## Faz 8 — World State, ekoloji bağlantısı ve chunk kayıtları (uygulama durumu)

- UWorldStateSubsystem mevcut su, toprak, kimya, biyokütle yangını ve çürüme bileşenlerini tek chunk pipeline'ında orkestre eder; paralel dünya hücresi veya ikinci yangın sahibi oluşturulmaz. İşlem sırası su → toprak → kimya → çapraz alan nem/toksin etkisi → hücre biyokütlesi yangını → çürümedir.
- Toprak nemi, mevcut biyokütle MoistureContent alanına üstel yaklaşım ile aktarılır. Hücre toksin yoğunluğu, mevcut biyokütlenin Health değerini ayarlanabilir bir katsayıyla etkiler. FireSimulation aynı biyokütle nemini ateşleme ve yayılım kararında kullanır; yapı/kütük yanması ise Faz 7 sınırında UBurnSubsystem sahipliğinde kalır.
- Rüzgâr yönü USimulationState ortak hava durumuna deterministic günlük RNG ile eklenmiştir; UWorldStateSubsystem bu yönü shared weather'dan okur ve hücre yangını yayılımına aktarır. Günlük rüzgâr/yağış üretimi seed tabanlı kalır.
- Chunk başına LastObservedWorldDay ve AccumulatedDeltaDays, gözlemlenen zamanı tekrar tekrar borca eklemeyi engeller. Simülasyon adımı en fazla bir oyun günü; biriktirilen pasif/uzun süreli catch-up en fazla 365 gün olarak sınırlıdır. Tarihsel yağış yeniden kurma döngüsü de en fazla 365 günü işler. Bu sınırlar frame hitch'lerini önler; çok eski pasif geçmiş tam fiziksel tekrar oynatma garantisi vermez.
- FChunkStateSnapshot ve FWorldStateSnapshot kayıt değerleri eklendi. Hücre su/toprak/biyokütle/kimya/terrain değerleri, chunk koordinatı/metadata, saat, hava, rüzgâr yönü ve seed doğrulandıktan sonra chunk store yeni bir map olarak hazırlanıp tek adımda değiştirilir. Snapshot üst sınırı 16.384 chunk ve 1.048.576 cell'dir.
- UWorldCoreSaveGame schema 4 içinde bHasWorldState ve chunk/cell snapshot taşır. Schema 1–3 kayıtlarının okunma yolu korunur; eski kayıtlar world-state alanı taşımadığı için yeni snapshot import edilmez. Domain runtime sahipliği yine UWorldStateSubsystem/UChunkStateStore üzerindedir.
- Scripts/tests/test_phase8_world_state_integration_contract.py, tek pipeline sırasını, shared wind, moisture/toxin coupling, bounded lazy catch-up ve schema-4 snapshot sözleşmesini denetler. Bu test ve Python CI, UE 5.8 C++ derlemesi veya gerçek save/load Automation testi yerine geçmez; motor kabulü ve daha derin çapraz-domain rollback/performance analizi Faz 14'te kalır.

- Statik Python tooling CI **#794 başarılı** (run [38534211068](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38534211068)); yeni Faz 8 entegrasyon sözleşmesindeki 7 test ve mevcut Python/katalog testleri geçti. Bu, Unreal Engine 5.8 derleme veya oyun içi save/load Automation kanıtı değildir. Eski kayıt şemalarının motor içi migration testi ve çoklu-domain import rollback/performance doğrulaması Faz 14'te kalır.


## Faz 9 — tarım, yem, hayvan üretimi ve gübre döngüsü (uygulama durumu)

- ACropFieldActor tohum ekimi için PlantCropFromInventory API'siyle mevcut UInventoryComponent stokundan tohum düşürür; ekim girdisi ve UPlantDefinition doğrulanmadan stok değişmez. Aynı tarla doluyken ikinci ekim engellenir.
- Tarla günlük büyüme hesabında mümkün olduğunda UWorldStateSubsystem hücresindeki canonical su/nem ve N/P/K/pH değerlerini okur. Yağış ve temel toprak simülasyonunu hücreye ikinci defa uygulamaz. Bitki su kullanımı ve büyüme kaynaklı nutrient uptake yine aynı dünya hücresine yazılır; chunk zamanlanmış simülasyona alınır.
- HarvestToFeedStorage mevcut UHarvestComponent sonucunu kapasite kontrolü başarılı olan UFeedStorageComponent içine koyar. Yem kapasitesi/kimliği/kalitesi geçerli değilse hasat ilerletilmez; başarısız hasatta geçici feed katkısı geri alınır. UFeedStorageComponent için miktar/kapasite ve finite-input korumaları eklendi.
- Mevcut AAnimalActor yerel feed storage'ından yem tüketmeye devam eder. Tüketilen feed ve protein miktarı UManureComponent içinde sınırlı gübre/nitrojen stoğu oluşturur. CollectProductsToInventory süt/yumurta/yün için sayılabilir tam birimleri mevcut inventory'ye eklemeden ilgili kaynak üretim stoğunu azaltmaz. Ürün ItemID'leri inventory'de tanımlı olmalıdır; envanterde yer yoksa ürün hayvan bileşeninde tutulur.
- ApplyManureToSoil geçerli UWorldStateSubsystem hücresini aldıktan sonra manure stoğundan oranlı miktar çeker ve hücrenin azot/fosfor/potasyum değerlerini günceller. UFeedCycleComponent ikinci bir TMap feed stoğu tutmaz; aynı actor üzerindeki UFeedStorageComponent için adaptördür. FeedCycle üzerinde kimliği belirtilmemiş eski UpdateDaily(consumption, production) çağrısı yalnızca belirli feed'ler üzerinden stok tüketebilir; kimlikli üretim UpdateDailyForFeed/AddHarvestedFeed API'leriyle verilmelidir.
- Scripts/tests/test_phase9_farming_integration_contract.py sekiz statik sözleşme kontrolüyle runner'a bağlandı. CI #837 (run 38535234996) başarılıdır; Faz 9 statik sözleşme testleri ve diğer Python/katalog kontrolleri geçti. UE 5.8 compile, gerçek SaveGame round-trip, hayvan/bitki dinamik aktörlerinin yeniden oluşturulması ve bütünsel kaynak dengesi bu kontrolün kapsamı değildir; motor, actor persistence ve uzun oturum kabulü Faz 14'te kalır.


## Faz 10 — bölgeler, altyapı ve rota maliyeti (uygulama durumu)

- \`URegionLogisticsSubsystem\` bölge koordinatını kanonik \`FWorldChunkSettings::WorldToChunk\` ve negatif koordinatlarda doğru çalışan floor-division ile üretir. Varsayılan bölge 8×8 world chunk'tır; boyut \`ChunksPerRegion\` ile değiştirilebilir.
- Bölge kaynağı/tehlike özeti ayrı bir resource store değildir. \`QueryRegionSurvey\`, mevcut \`UWorldStateSubsystem/UChunkStateStore\` hücrelerinden biyokütle toplamı, N/P/K toplamı, yüzey suyu ve ateş/toksin/su bileşenli çevresel risk değerini sorgu anında hesaplar. Kayıtların içindeki hücre durumu tek kaynak olmaya devam eder.
- Tek rota grafı \`Trail/Road/Bridge/Ramp/SledRoute/Ropeway\` bağlantı tiplerini taşır. Link maliyeti bağlantı türü temel hızı, mesafeyi, 0–1 bakım/dayanım durumunu, 0–1 yerel riski ve bağlantı orta noktasının World State riskini kullanır. Payload, link'in \`MaxPayloadKg\` kapasitesini aşarsa bağlantı kullanılamaz; kapalı düğüm/link rotaya katılmaz.
- Node/link kayıtları sabit \`FName\` anahtarlarından deterministic GUID üretir. Paylaşılan kavşak için komşu bağlantılar aynı start/end node key'ini kullanır. Ağın üst sınırı 2.048 düğüm / 8.192 bağlantıdır; bozuk, yinelenen, geçersiz değerli veya dangling-link snapshot kabul edilmez. Import doğrulama sonrasında yeni dizileri kurup canlı grafı değiştirir.
- Mevcut \`UBridgeComponent::BuildBridge\` başarılı olduğunda, yalnızca \`LogisticsLinkKey\` tanımlıysa köprü rotası kaydeder; \`AZiplineActor\` da aynı şartla BeginPlay'de halat/zipline uçlarını kaydeder. Bu kaydı kapatmak veya anahtar vermemek eski fiziksel mekanikleri değiştirmez. Uçları Blueprint'te değiştiren sistemler \`RefreshLogisticsRoute\` çağırmalıdır. Road/ramp/sled-route tipleri aynı subsystem API'si üzerinden mevcut actor/Blueprint akışından eklenebilir; bunlar için varsayılan dünya çapında otomatik tarama yapılmaz.
- Ortak \`UWorldCoreSaveGame\` schema 5, \`FRegionLogisticsSnapshot\` ve \`bHasRegionLogistics\` alanını taşır. Schema 1–4 kayıtları okunmaya devam eder; eski kayıt region-logistics snapshot'ı taşımadığı için mevcut grafı boş snapshot'la silmeye çalışmaz. Altyapı değişikliği \`OnLogisticsChanged\` üzerinden world revision'ı artırır; load import'u revision suppression içinde yapılır.
- \`Scripts/tests/test_phase10_region_logistics_contract.py\` veri sahipliği, maliyet faktörleri, snapshot doğrulaması, schema geriye uyumluluğu ve bridge/zipline bağlarını statik olarak denetler. Python tooling CI #879 başarılıdır (run [38536422646](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38536422646)); Faz 10 sözleşme testleri ve mevcut Python/katalog kontrolleri geçti. Unreal Engine 5.8 build/Automation, gerçek rota senaryosu ve schema 1–5 SaveGame round-trip bu ortamda çalıştırılmış değildir; Faz 14 toplu motor kabulinde kalır.


## Faz 11 — üretim zincirleri, kuyruk, kapasite ve güç kapıları (uygulama durumu)

- \`UProductionRecipeAsset\` üretim kimliği, girdi/çıktı satırları, süre, watt gereksinimi ve işçi sayısı için doğrulanan veri tanımı sağlar. \`UProductionChainComponent\` bir istasyonun FIFO kuyruğunu, ilerlemesini, blok durumlarını ve iş geçmişini taşır. Mevcut \`ACraftingBench\` bu bileşeni içerir; eski \`FBenchRecipe\` kayıtları da yeni envanter işlemi üzerinden kuyruğa aktarılır.
- Kuyruğa alma girdileri mevcut \`UInventoryComponent::ExchangeItemBatches\` atomik işlemiyle hemen ayırır. Bu işlem birden fazla girdiyi topluca önkontrol eder, envanter yuvalarının tam snapshot'ını saklar, geçici değişikliklerde event yayınını bastırır ve başarısızlıkta slotları aynen geri yükler. İş iptali ayrılan girdileri aynı giriş deposuna atomik olarak iade eder; iade sığmıyorsa iptal reddedilir.
- Üretim FIFO başındaki işte güç (\`AvailablePowerWatts\`) ve işçi (\`AvailableWorkers\`) koşullarını kontrol eder. Koşul yoksa zaman ilerlemesi durur. Üretim süresi dolduğunda çıktı, mevcut output inventory'ye atomik eklenmeye çalışılır; kapasite veya ağırlık yetersizliği \`BlockedOutput\` oluşturur ve output commit başarılı olana kadar ürün job snapshot'ında kalır. Tick başına tek output commit, uzun frame sonrasında kuyruk patlamasını sınırlar. Kuyruk, batch ve snapshot üst sınırları vardır.
- Yerleşik sawmill örneği \`HalfLog × 1 → Plank × 2\`, 5 saniye, 100 W ve 1 işçi kullanır. Diğer makineler \`UProductionRecipeAsset\` veya \`QueueConfiguredRecipe\` ile tanımlanabilir. Bu iki ürün ID'si ve diğer tarif girdileri/çıktıları mevcut inventory item registry/definition'larında tanımlı olmalıdır.
- Depoda bu faz öncesinde ortak bir güç üretim/dağıtım sahibi bulunmadığından watt ve işçi kullanılabilirliği \`SetAvailablePowerWatts\` / \`SetAvailableWorkers\` API'leriyle dış istasyon/Blueprint adaptöründen verilir. Güç eksikliğinin üretimi durdurması kodlandı; gerçek generator/grid bağlantısı ve enerji dengesi tamamlandı kabul edilmez. Gerçek fiziksel \`ACarvableLog\` kesimini stackable \`HalfLog\` öğesine otomatik dönüştürme de bu fazda otomatikleştirilmedi; fiziksel kesim mevcut kütük mekaniğinde kalır.
- Ortak \`UWorldCoreSaveGame\` schema 6, her istasyonun sabit anahtarlı üretim snapshot'ını (iş ID'si, ayrılan girdiler, çıktı yükü, ilerleme, güç/işçi gereksinimi, durum ve pause flag) kaydeder. Stok, hâlâ kendi canonical \`UInventoryComponent\`/player veya \`AInventoryStorageActor\` sahibinde kaydedilir; ikinci stok ledger'ı oluşturulmaz. Aktif kuyruk için hem giriş hem çıkış inventarisinin core save tarafından kaydedilen \`AForestCharacter\` veya \`AInventoryStorageActor\` üzerinde olması zorunludur; koşul sağlanmıyorsa geçerli job/stock ayrışmasını önlemek adına save reddedilir. Schema 1–5 eski save okuma yolu korunur.
- \`Scripts/tests/test_phase11_production_chain_contract.py\`, atomik çoklu işlem, girdi rezervasyonu, iptal/refund, güç/işçi/çıktı blokları, kapasite ve save snapshot sözleşmelerini statik olarak test eder. Python tooling CI sonucu en son commit için aşağıda ayrıca işlenecek. Unreal Engine 5.8 derlemesi, gerçek makine rota/üretim E2E'si, güç sağlayıcı bağlantısı ve save/load turları bu ortamda çalıştırılmış değildir; bunlar Faz 14 bütünsel kabulinde kalır.
