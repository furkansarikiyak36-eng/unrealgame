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

## Faz 6 — Envanter transfer işlem sınırı

- `UInventoryComponent::TransferItem` mevcut envanter sistemine eklenir; ayrı bir inventory/physical-item mimarisi açmaz. Aynı kaynak-hedef, geçersiz kimlik/miktar ve yetersiz stok reddedilir.
- Transfer, kaynak yığınının kanonik `FInventoryItem` payload'ını korur. Hedef kapasitesi önce kontrol edilir; kaynak ancak sonrasında düşülür. Hedef ekleme beklenmedik biçimde başarısız olursa kaynak stoğu geri eklenmeye çalışılır ve rollback başarısızlığı açıkça loglanır.
- `DropItem` içindeki `bCanBeDropped` politikası değiştirilmez; transfer API'si dünya aktörü spawn etmez ve fiziksel item üretimini taklit etmez.
- `Scripts/tests/test_inventory_transfer_contract.py` statik sözleşme kontrolleri Python runner'a bağlandı. Bunlar C++ derlemesi veya UE Automation testi değildir. Fiziksel taş/çubuk aktörleri, storage, processing, construction/salvage ve save/load uçtan uca bağlantıları Faz 6 içinde devam edecek; genel 485 mekanik denetimi Faz 14'tedir.
