# Faz 0 — Baseline ve Gerçek Durum Denetimi

**Tarih:** 14 Ekim 2026  
**Repo:** `furkansarikiyak36-eng/unrealgame`  
**Dal:** `plan/phase-00-baseline`  
**Durum:** Faz 0 baseline kapısı tamamlandı — kaynak triage 485/485 satıra senkronlandı; özellik ve test açıkları sahip fazlara aktarıldı.

## Amaç

Mekanikleri sınıf/fonksiyon adlarıyla değil, gerçek oynanış akışı ve kanıtla değerlendirmek. Faz 1'e geçiş kapısı, 485 satırlık katalog kimliklerinin aile eşleştirmeleriyle bire bir tutması; U0–U2 statik kaynak triage durumu, kanıt izi ve blocker/not alanlarının ana kataloğa aktarılmasıdır. Satır bazlı UE otomasyonu ve görsel kabul, özellikleri uygulayan fazlarda üretilir; bunlar bütün kod çalışmalarını başlatmadan önce tamamlanması gereken küresel bir kapı değildir.

## İncelenen kaynaklar

- `TOTAL_PLAN.md`
- `Docs/DURUM_RAPORU.md`
- `Docs/KARARLAR.md`
- `Docs/KOD_HARITASI.md`
- `Docs/TEST_DURUMU.md`
- `Source/MyProject2/CarvableLog.h`
- `Source/MyProject2/ForestCharacter.h`
- Inventory bridge/component, Raid Director/defense/spawn, Farming feed integration, Material chunk integration, World chunk manager/persistence kaynakları
- Ana kaynak dizini ve faz klasörleri
- Son görünen commit: `a1a0b0c6d3dd499cf3221f415b46e8073c5a32e2` — "Phases 7-14: slopes, openings, grooves, carts, stone ring, tree log count, raiders, push and impact damage"

## Kanıtlı baseline

1. Proje dosyası Unreal Engine **5.8** ile ilişkilendirilmiş.
2. Mevcut oynanış kütüğü `ACarvableLog`; oyuncu etkileşimleri `AForestCharacter` içinde. Log, fizik, yanma ve yapı kontrolleri için `MechanicsDemo` ve Wood testleri bulunuyor.
3. `Docs/TEST_DURUMU.md` otomatik üretilmiş raporunda **101 mekanik kontrolü** ve **12 Wood testi** geçtiğini bildiriyor. Bu rapor bu oturumda Unreal Engine altında yeniden çalıştırılmadı; bu sayılar kaynak raporundan alınmıştır.
4. `WorldCore`, `WorldPhaseW1_Chunk`, `WorldPhaseW2_Lazy`, `WorldPhaseW3_Event`, `WorldPhaseW4_WaterSoil`, `WorldPhaseW5_FireDecayChem` ve `WorldPhaseW6_Persistence` kaynak klasörleri var.
5. Inventory, Stone, Stick, Raid, Farming ve Material için faz klasörleri ve C++ kaynakları var. Dizin ve sınıf bulunması, bunların tümünün oyuncu akışına entegre olduğunu kanıtlamaz.
6. `WorldPersistence` chunk state'i sürüm ve doğrulama kontrolleriyle serileştiriyor; incelediğimiz kayıt şeması arazi, su, toprak, kimya ve biyokütle alanlarını içeriyor. Bu tek başına fiziksel aktörlerin tamamının ya da kazı geometrisinin kalıcı olduğunu kanıtlamıyor.
7. Kaynak ağacında bu denetimde özel bir runtime kazı/voxel hacim sistemi bulunamadı. Yatay tünel ve oda, mevcut heightmap verisinin tek başına sağlayabildiği bir yetenek değildir; bu özellik teknoloji spike'ı gerektirir.

## İlk yüksek riskli bulgular

### A. Mevcut kütük ile Inventory köprüsü aynı nesne modelini kullanmıyor

`InventoryBridge/PhysicalToInventoryBridge.cpp`, `ALogActor`, `AStoneActor` ve `AStickActor` türlerini envanter öğesine çeviriyor. Mevcut oynanış kütüğü ise `ACarvableLog`. Bu köprü doğrudan `ACarvableLog` kabul etmiyor; fiziksel kütüğün kesim/çentik/hasar durumunu koruyarak envantere alıp geri çıkarma davranışı kanıtlanmış değil.

**Karar:** İkinci bir kütük otoritesi ekleme. Envantere çevrilecek fiziksel öğe için kimlik, hacim, şekil, kesim, hasar ve dünya transformu sahipliği tasarlanmalı. Büyük fiziksel kütükleri soyut item yığınına çevirmek ancak tasarım açıkça izin veriyorsa yapılmalı.

### B. Envanterden dünyaya geri dönüş ve yere bırakma incelenmeli

`InventoryToPhysicalBridge.cpp` spawn başarılı olduktan sonra öğeyi tüketiyor; bu iyi bir başlangıç işlem sırası. Buna karşılık `InventoryComponent::DropItem` yalnızca `RemoveItem` çağırıyor ve bu fonksiyonda fiziksel aktör üretimi görünmüyor.

**Karar:** Drop, consume, craft ve spawn işlemleri ayrı tanımlanmalı. Her işlem atomik olmalı: öğe yalnızca hedef nesne/çıktı başarıyla oluşturulduğunda tüketilmeli; başarısızlıkta kayıp veya çoğalma olmamalı.

### C. Raid Director'da orkestrasyon var, canlı dünya entegrasyonunda açık uçlar mevcut

`RaidDirectorSubsystem` olay başlatma, zorluk, plan, dalga, kapasite ve spawn akışını çağırıyor. İncelenen kodda oyuncu seviyesi `1` atanıyor ve oyuncu konumları olay başında snapshot alınıyor. `StructureDamageResolver`, `ALogActor`, `AStoneActor`, `AStickActor` üzerinden hasar uyguluyor; mevcut `ACarvableLog` için doğrudan yol bu resolver'da görünmüyor. `UpdateWorldAfterBlockDestroyed` gövdesi yalnızca entegrasyon yorumu içeriyor.

**Karar:** Baskın sınıflarını tamamlandı işaretleme. Gerçek oyuncu/oyun state'inden zorluk, canlı hedef güncellemesi, düşmanın `ACarvableLog` ve ortak yapı durumuna hasarı, yok edilen engel sonrası rota güncellemesi uçtan uca kanıtlanmalı.

### C2. Raid spawn işleme yolu iki ayrı biçimde tanımlı

`RaidDirectorSubsystem::ProcessSpawns` doğrudan düşman sınıfını spawn ediyor ve budget/population counter güncelliyor. Ayrıca `RaidPhaseR5_Spawn/SpawnDecisionPipeline.cpp` içinde spawn doğrulama ve aynı türde budget/counter güncellemesi yapan ayrı bir `ExecuteSpawnDecision` var. İncelenen ana director çağrı zincirinde bu pipeline'ın kullanıldığı gösterilmedi.

**Karar:** Tek spawn karar yolunu seç. Ya director pipeline'ı zorunlu çağırmalı ya da pipeline kaldırılmadan önce bütün çağrıları ve test bağımlılıkları kanıtlanmalı. Spawn reddi/konum yokluğu/limit doluluğu/bütçe tüketimi/population count senaryoları aynı yol üzerinden test edilmeli.

### D. Farming feed hesabı var, üretim zinciri sahipliği kanıtlanmadı

`FeedCycleComponent` yerel bir stok haritası üzerinden yem ekleme/tüketme ve günlük toplam güncellemesi yapıyor. Bu dosya tek başına hasat çıktısının gerçek stoktan gelip hayvana ulaştığını veya gübrenin gerçek dünya toprağına döndüğünü kanıtlamaz.

**Karar:** Hasat, yem, hayvan tüketimi, ürün, gübre ve toprak besini aynı kaynak muhasebesiyle bağlanmalı; kütle/kaynak dengesi testleri eklenmeli.

### E. Material–World ilişkisi hücre odaklı; fiziksel aktörlerle ortak otorite kanıtlanmalı

`MaterialChunkIntegration`, chunk hücrelerinin biyokütle/yanma/bozunma durumuna malzeme etkileşimi uygular. Bu dosya mevcut ahşap aktörlerinin yanma ve hasar durumuyla aynı authoritative state'i kullandığını tek başına kanıtlamaz. Malzeme profili seçiminin `SpeciesID` üzerinden yapılması da veri sözleşmesiyle doğrulanmalı; eksik profilin sessizce Wood profiline düşmesi yanlış materyal atamasını maskeleyebilir.

**Karar:** Fiziksel Actor malzeme durumu ile chunk hücre materyali birbirinden bağımsız kopyalar hâlinde tutulmamalı. Hangi varlığın hangi etkileşimi işleyeceği açıkça belirlenmeli.

### F. Birden fazla yapısal modelin sahipliği kararlaştırılmalı

Depoda `ACarvableLog` ve Wood analiz kodunun yanında `BuildingNodeActor`, `StructuralIntegritySubsystem`, `LogConstructionActor`, taş/çubuk yapıları ve Material/World entegrasyonları var. Hepsinin tek bir yapı grafına katıldığı bu ilk denetimle kanıtlanmadı.

**Karar:** Faz 2/7 için kanonik yapı düğümü/bağlantı sözleşmesi tasarlanmalı. Çalışan kütük zinciri regresyon testleri korunmadan eski sistemi silme veya ikinci bir destek grafını etkinleştirme yapılmamalı.

## İlk durum tablosu

| Sistem | Mevcut kanıt | Başlangıç değerlendirmesi | İlk açık kapı |
|---|---|---|---|
| Kütük | Oynanış aktörü, kesim/yerleştirme/yanma/hasar kodu; raporlanan 101 kontrol | En güçlü çalışan çekirdek; görsel ve bütün katalog kapsamı doğrulanmalı | 485 katalog satırıyla tek tek eşleştir; editörde görsel kabul |
| Taş | Mining, placement, structural, processing, functional, environment, logistics klasörleri | Modül var; bütün akışların entegre olduğu kanıtlanmadı | Fiziksel taş → taşıma → yapı → hasar → söküm/kazanım |
| Çubuk | Core, processing, crafting, fire, defense, traps, furniture, environment, extensions klasörleri | Modül var; bütün akışların entegre olduğu kanıtlanmadı | Toplama → işleme → kullanım → geri kazanım testi |
| Inventory/Crafting | Slot/stack/weight, recipe matcher/validator, save subsystem ve bridge kodları | Yerel mantık mevcut; mevcut `ACarvableLog` ile kimlik/durum korunumu riski | Drop, pickup, craft, spawn işlemlerinde kayıp/çoğalma testi |
| Raid | Director, scheduler, planner, budget, spawn, defense, wave-state kodları | Orkestrasyon önemli ölçüde mevcut; dünya entegrasyonu ve AI döngüsü ayrıca doğrulanmalı | Canlı oyuncu hedefi, gerçek yapı hasarı, rota güncellemesi ve abort |
| Farming | Plant, soil, animal, production, reproduction, integration klasörleri | Çok sayıda model var; dünya/stok bağlantısı doğrulanmadı | Hasat → yem → tüketim → ürün/gübre → toprak E2E |
| World State | Chunk, lazy, event, water/soil, fire/decay/chemistry, persistence klasörleri | Kalıcı chunk state altyapısı var; voxel hacim kazısı yok/kanıtlanmadı | Save/load determinism, chunk seam, gerçek su akışı ve kazı spike'ı |
| Material | Definition, interaction, instance, composite, plaster/metal, integration klasörleri | Profil/etkileşim altyapısı var; fiziksel aktörlerle tek otorite belirsiz | Ahşap/taş Actor'ü ile chunk state arasında tek hasar/yanma akışı |
| Core | World time, chunk, event, inventory save ve farklı yapı modelleri | Kurucu sınıflar var; sahiplik sözleşmeleri kesinleştirilmeli | Kimlik, olay semantiği, RNG, zaman ve persistence kontratları |

## Phase 0 implementation performed on the plan branch

These changes have been made on `plan/phase-00-baseline`; they are not yet merged into `main`:

- `Scripts/verify_test_logs.py` fails closed when the MechanicsDemo summary is missing, any failure is reported, no individual Wood automation results are parseable, a Wood test fails, results conflict/duplicate, or the report falls below the recorded regression floor (**101 MechanicsDemo checks and 12 unique Wood tests**).
- `Scripts/tests/test_verify_test_logs.py` covers valid logs, failed mechanics checks, missing mechanics summary, failed Wood tests, missing Wood results, and both minimum-count regressions.
- `Scripts/run_tests.sh` captures the build exit code, fails on missing logs or non-zero Unreal process exit, runs log verification before regenerating the report, and retains build/test logs. It also supports `UE_ENGINE_ROOT` to avoid a hard-coded install path.
- These are infrastructure changes only. **No Unreal Engine build/editor run was performed by this assistant.** As requested, UE-specific automation is delegated to the project's Claude Code/UE host workflow; source implementation and platform-independent regression checks continue without waiting for this environment to have Unreal installed. A feature is not reported as engine-tested until that workflow supplies a real result. The 101/12 minimums remain inherited from the existing repository report.

## Faz 0 çıkış kriterleri — sonuç

- [x] Kanonik katalogdaki 485 satırın kalıcı ID'leri mevcut; yinelenen ID yok.
- [x] Dokuz aile eşleştirmesi ana katalogla bire bir uyumlu: toplam 485/485.
- [x] Kaynak triage durumları ve satır-bazlı kaynak izi, test/editör bekleyenleri, blocker/notlar ana katalogla senkronlandı.
- [x] U0–U2 seviyeleri doğru şekilde korunuyor; doğrulanmayan mekanikler tamamlandı olarak yükseltilmedi.
- [x] Inventory ownership ADR ve kritik bulgu matrisi mevcut; daha sonraki fazlara ait entegrasyon riskleri açık tutuluyor.
- [x] UE-specific test runner, yalnızca uygun Claude Code/Unreal host ortamında çalıştırılacak; bu ortamda motor testi yapılmadığı başarı olarak gösterilmiyor.
- [x] Kazı teknolojisi için A/B adayları ve ölçüm alanları tanımlandı; gerçek benchmark gelmeden nihai seçim yapılmıyor.

**Faz 0 sonucu:** Baseline/source-triage kapısı geçildi. Bu, 485 mekaniğin tamamlandığı anlamına gelmez. Açık U0 satırları ve sistem entegrasyon bulguları kendilerine ait uygulama fazlarında kapatılacak. **Faz 1 kaynak kod çalışması artık başlayabilir;** A/B kazı teknolojisinin nihai seçimi ise ölçüm kanıtına bağlıdır.

**Sıradaki çalışma:** Faz 1'de karşılaştırılabilir iki kazı temsilini ve deterministik ölçüm/validasyon girişlerini kurmak; bunu yaparken Faz 2'nin kanonik dünya kimliği, sparse delta ve kayıt sözleşmeleriyle çelişmemek.



## 14 Ekim 2026 — katalog kapısı ve denetleyici

Eklenen denetleyici: `Scripts/audit_mechanic_catalog.py`; bağımsız test paketi: `Scripts/tests/test_audit_mechanic_catalog.py`.

14 Ekim 2026 senkronundan sonra ana katalogda 485 satır ve 485 benzersiz ID var; yinelenen ID, zorunlu metadata hatası ve kanıt alanı boşluğu 0. Kaynak triage dağılımı **333 U2, 88 U1, 64 U0**. Bu seviyeler mekaniklerin oyunda tamamlandığını değil kaynak izi ve belirsizlik durumunu gösterir. Faz 0 baseline kapısı bu nedenle geçildi; 64 U0 gereksinim uydurulmadan açık kalır ve ilgili teslim fazlarında ele alınır.

Denetim komutu:

```bash
python Scripts/audit_mechanic_catalog.py path/to/MEKANIK_KATALOG_TAKIP_CSV.csv --expected-count 485 --json-out Saved/TestRuns/catalog_audit.json
python Scripts/audit_mechanic_catalog.py path/to/MEKANIK_KATALOG_TAKIP_CSV.csv --expected-count 485 --strict
```

İlk komut keşif sırasında boşlukları raporlar; ikinci komut boşluk varsa non-zero çıkışla kabul kapısını kapalı tutar. Katalog-denetleyici için 9 birim test izole Python ortamında çalıştırıldı ve geçti. Testler yerel denetim kopyasında yapıldı; GitHub Actions sonucu henüz görünür değil. GitHub Actions sonucu görünmeden CI geçişi; Unreal Engine 5.8 hostunda build/automation yapılmadan da motor geçişi iddia edilmez.


## Ek kaynak denetimleri — 14 Ekim 2026

İlk LOG denetimi: [FAZ_00_LOG_KANIT_DENETIMI.md](./FAZ_00_LOG_KANIT_DENETIMI.md). LOG-001–076 için kaynak yolu, kısmi destek ve bu incelemede bulunmayan özellikler ayrılmıştır. CSV eşleştirmesi 59 U2 / 17 U0 olarak sınıflandırılmıştır; bu kaynak taraması editör/oyun içi doğrulama yerine geçmez.

Sistem entegrasyonu taraması: [FAZ_00_SISTEM_ENTEGRASYON_DENETIMI.md](./FAZ_00_SISTEM_ENTEGRASYON_DENETIMI.md). En önemli engeller:
- fiziksel kaynak köprüsünün `ACarvableLog` yerine farklı `ALogActor` türünü kabul etmesi;
- envanter `DropItem` metodunun kendi başına fiziksel aktör oluşturmaması;
- dünya kayıt metotlarının world/chunk state'i kaydetmesi ama bu metotlarda canlı yapı/araç/oyuncu aktörlerini kapsamaması;
- Raid Director'ın doğrudan spawn yolu ile ayrı SpawnDecisionPipeline arasında canonical sahiplik göstermemesi;
- raid structural resolver'ın `ACarvableLog` adaptörünü ve yıkım sonrası rota yenilemesini içermemesi;
- `AAnimalActor` güncellemesinin `FeedStorageComponent`, ayrı `FeedCycleComponent`'in ise başka bir stok haritası kullanması;
- material ID çözümlenemezse Wood fallback'e gidilebilmesi;
- runtime 3B kazı geometrisi/çarpışma delta'sı için bu kaynaklarda uçtan uca pipeline bulunmaması.

Bu bulgular kodu doğrudan yeniden yazmak için değil, fazlar arası sahiplik sözleşmesini netleştirmek ve yanlışlıkla birden fazla otoriter sistem üretmemek için kaydedildi. Kaynakları bulunmayan ya da çağrı zinciri kanıtlanmamış mekanikler U0 olarak kalır. Faz 0 kapısı, 485 satırın her biri için ID bazlı durum + kanıt/test/engel eşleştirmesi tamamlanana kadar açıktır.
 

## Satır bazlı kapsam ilerlemesi

Satır bazlı eşleştirme dosyaları dokuz katalog ailesinin tamamını kapsıyor: LOG 76, INVENTORY 32, RAID 63, WORLD 35, MATERIAL 38, FARMING 57, CORE 12, STONE 96, STICK 76 — toplam **485/485**. Statik kaynak triage dağılımı U2=333, U1=88, U0=64; U3–U6=0. Kaynak haritalama tamamlandı; tüm satırlara özgü test kimliği/sonucu, editör kanıtı ve gerektiğinde kayıt/performans kanıtı eklenmesi hâlâ açık.

`Scripts/run_python_tests.sh`, dokuz sistem eşleştirme CSV'sini beklenen satır sayılarıyla denetler. Son GitHub Actions Python tooling run #88 başarılıdır. Bu denetim şema/kimlik/metaveri ve Python araç testidir; UE 5.8 derlemesi, editör kabulü veya paket testi yerine geçmez.


## Envanter sahipliği engeli

`AForestCharacter` içinde eski `EForestItem` tabanlı 24 slot/20 stack stoğu var; ayrıca generic `UInventoryComponent`, kategori/ağırlık kuralları, ayrı UI/crafting ve save subsystem bulunuyor. Oyuncu karakteri constructor'ında generic komponentlerin yaratıldığı görünmüyor; Blueprint eklemesi de bu oturumda doğrulanmadı. Dolayısıyla çift envanter sahibi olasılığı bir blocker olarak kaydedildi; `Docs/FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv` her INV satırının kaynak/kanıt durumunu takip ediyor. Bu sahiplik kararı çözülmeden Faz 6 entegrasyonuna başlanmamalı.


## Taş/çubuk eşleştirmesi ve ilk kod düzeltmesi

- [Taş/çubuk statik denetimi](./FAZ_00_STONE_STICK_DENETIMI.md) ve iki CSV eşleştirmesi, son 172 satırı kapsar. Böylece 485/485 gereksinim kaynak triage dosyasına girildi; kaynak seviyeleri U2=333, U1=88, U0=64, U3–U6=0.
- P0-027 (çubuk crafting atomikliği) için kaynak değişikliği yapıldı: duplicate girdiler birleştiriliyor; kapasite tüketim sonrası hesaplanıyor; hata durumunda stok snapshot'ı geri yükleniyor. Üç Unreal Automation Test ve yeni fail-closed log doğrulama yolu eklendi.
- Bu yeni C++ testleri bu ortamda UE 5.8 altında derlenip çalıştırılmadı. Python CI, doğrulayıcı ve katalog araçlarını sınar; C++ kabulü için Windows/UE host çıktısı gerekir.


## Faz 1 hazırlığı — kazı teknolojisi ölçüm kapısı

- [Faz 1 kazı spike şartnamesi](./FAZ_01_KAZI_TECHNOLOJI_SPIKE.md) ve [boş ölçüm matrisi](./FAZ_01_KAZI_SPIKE_SABLONU.csv) oluşturuldu.
- Karşılaştırılacak adaylar: **A — sparse 3B density/SDF delta + yüzey çıkarımı** ve **B — Dynamic Mesh üzerinde lokal CSG/boolean kesileri**. Bunlar adaydır; hiçbirini proje için doğru teknoloji diye henüz seçmedik.
- Sabit benchmark kümesi: 2×2 m / 5 m kuyu, 2×2 m / 12 m tünel (chunk sınırını geçen), 6×5×3 m oda, kesişen tünel, collision, save/close/load, kısmi işlem rollback.
- Ölçüm matrisi 25 cm ve 50 cm çözünürlükleriyle dört koşu ister. Game-thread/mesh/collision süreleri, worst spike, RAM ve save boyutu kanıtla yazılmadan teknoloji kararı verilmez.
- `Scripts/validate_excavation_spike.py` ve testleri, matrisin dört A/B×çözünürlük koşusunu tutmasını ve “Seçildi/Tamamlandı” gibi kararların bütün ölçüm/kanıt alanları dolmadan yazılmamasını kontrol edecek şekilde eklendi.
- Bu yalnızca Faz 1 hazırlığıdır. Faz 0 henüz UE 5.8 C++ build/automation ve diğer kabul kanıtlarıyla kapanmadı; bu ortamda geometri/collision benchmark çalıştırılmadı.


## Yeni kaynak düzeltmesi — taş yerleştirme

P0-023 için `UStonePlacementComponent` yüz/socket seçimi ve placement lifecycle davranışını uygulayacak şekilde güncellendi; üç Unreal Automation Test yazıldı. Log parserı ve UE host test runner'ı artık `MyProject2.StonePlacement` suite'ini zorunlu doğrulamaya dahil ediyor. Bu C++ kaynaklar UE 5.8 hostunda henüz derlenip çalıştırılmadığından bulgu “In Progress” kalır; yüz/socket preview ve attach/iptal davranışı oyun içinde gözlenmeden U5/U6 kanıtı verilmez.


## Yeni kaynak ilerlemesi — fiziksel taş yerleştirme

`AForestCharacter` gerçek fiziksel `AStoneActor` pickup/taşıma/yerleştirme akışını input kontrollerine bağladı. Taş önizlemesi aynı fiziksel aktörü korur; iptal state/transform/attach bilgisini geri yüklemeye çalışır. Yerleştirmeden önce component overlap kontrolü çalışır. Yapısal hedef bir graph sahibiyse, seçilen kaynak socket'i yakındaki boş hedef taşı socket'iyle eşleşmeli ve karşılıklı `UStructuralGraph::ConnectTo` kaydı alınmalıdır; aksi halde taş kurulmuş sayılmaz.

Beş test `Source/MyProject2/Tests/StonePlacementTests.cpp` içine eklendi: yaşam döngüsü, yüz/socket transformu, carry rollback, çarpışma engeli ve reciprocal socket graph bağlantısı/dolu socket reddi. Windows/UE 5.8 çalıştırıcısı bu suite'i ayrı logda zorunlu doğruluyor. **C++ build ve test sonucu hâlâ beklemede**; Python CI sonucu bu kabulün yerine geçmez.


## Taş yerleştirme graph sahipliği güncellemesi

Kaynak/face veya socket adı artık hedef AStoneActor'ın yakın, doğru yöne bakan ve boş socket'iyle eşleşmeden yapısal commit yapılmasına izin vermez. UStructuralGraph::ConnectTo iki uçta da kayıt oluşturur; state transition başarısızsa yeni bağlantı geri alınır. Hedefteki doluluk bilgisi shared DataAsset'ten değil actor graph'ından okunur. Bu adapter yalnızca AStoneActor hedefini kapsıyor; eski ahşap yapı graflarıyla genel bağlantı henüz yapılmadı.

Beş Unreal Automation Test source dosyasına eklendi; Unreal Engine 5.8 build/test çalıştırması henüz yok.


## Taş tuzağı — hasar dispatch düzeltmesi

P0-025 için StoneTrapActor'ın trigger volume overlap olayı bağlandı. Trigger bilinen hedef tipleri için doğrudan receiver metodunu çağırıyor; bilinmeyen hedefler için Unreal'ın standart ApplyDamage yoluna dönüyor. Cooldown süresince Rearm çağrısı tuzağı yeniden armed yapmıyor. İki C++ Automation Test eklendi (hasar/cooldown ve null-victim güvenliği). UE 5.8 build/automation ve gerçek overlap/physics doğrulaması henüz yok; P0-025 In Progress kalır.


### Stone Trap güncellemesi

P0-025 trigger volume'u Pawn ve PhysicsBody kanallarında overlap yapıyor. Bu, yalnızca karakter overlap'ini değil, fiziksel taş/kütük gövdelerini de aday tetikleyici olarak içerir. StoneTrapTests.cpp query-only/overlap ayarlarını, hedef hasarını, cooldown ve null-victim durumunu test ediyor; gerçek overlap ve fizik davranışı UE 5.8 runtime kabulü bekliyor.


### Stone Trap test kapsamı güncellemesi

P0-025 için test suite iki senaryodan dört senaryoya genişledi: düşman hasarı + cooldown, null victim, fiziksel taş canı ve kütük normalized damage. Scripts/verify_test_logs.py minimum dört benzersiz MyProject2.StoneTrap sonucu olmadan testi kabul etmiyor. Motor doğrulaması beklemede.




## 14 Ekim 2026 — kanonik katalog ve CI kabul noktası

- Kaynak gereksinim snapshot'ı `Docs/MEKANIK_KATALOG_TAKIP_CSV.csv` altında repo'ya eklendi; 485 satır ve 485 benzersiz Mechanic_ID içeriyor.
- `Scripts/validate_catalog_mapping.py` ve 8 birim testi, ana katalog ile LOG/INVENTORY/RAID/WORLD/MATERIAL/FARMING/CORE/STONE/STICK eşleştirme CSV'lerinin kimlik ve sistem kapsamını bire bir uzlaştırıyor.
- Python CI run #38515014058 başarılıdır. Bu kontrol kodun UE C++ derlemesini veya oyun içi davranışını sınamaz.
- RAID kaynağındaki 69 başlık / 63 detay sayımı P0-001 olarak açık kalıyor. Sayı kararını kaynak gereksinim sahibi doğrulamadan yeni mekanik ID'leri oluşturulmayacak.
- Faz 0 kapanışı için 485 satırın kimlik/aile eşleştirmesi ve U0–U2 kaynak triage kaydı gerekiyordu; bunlar tamamlandı ve ana kataloğa senkronlandı. U0 feature work, row-level automation/editor proof, RAID başlık sayısı kararı ve envanter/yapı/world entegrasyon sorunları ilgili uygulama fazlarında açık blocker olarak sürer. UE 5.8 test çıktıları Claude Code üzerinden ilgili kabul kapılarında alınır; burada motor testi çalıştırılmamış olması kod geliştirmeyi durdurmaz.


## Envanter tek-otorite kararı (ADR-001)

- AForestCharacter::Inventory mevcut oynanabilir odun/yapı döngüsünün otoriter stok kaydı olarak kalır; UInventoryComponent ailesinin karaktere bağlı olduğu kanıtlanmamıştır.
- Depoda incelenen player/Inventory Blueprint'i bulunmadığı için UI, quick-access ve SaveSubsystem bağlantıları varsayılmayacak.
- Geçiş için Docs/FAZ_00_INVENTORY_OWNERSHIP_ADR.md içinde ID mapping, physical actor state, işlemsel transfer, save/load ve UE 5.8 kabul kapıları tanımlanmıştır. P0-022 açık kalır.
- Inventory kapasite sınırı ve rollback düzeltmesi ile iki motor Automation Test'i eklendi. Python CI güncel C++ dosyasını derlemez veya çalıştırmaz; UE 5.8 host kapısı bekliyor.
