# Faz 0 — Baseline ve Gerçek Durum Denetimi

**Tarih:** 14 Ekim 2026  
**Repo:** `furkansarikiyak36-eng/unrealgame`  
**Dal:** `plan/phase-00-baseline`  
**Durum:** Devam ediyor — bu belge ilk statik kaynak denetimini kaydeder; tüm 485 katalog satırının incelendiği iddiasında değildir.

## Amaç

Mekanikleri sınıf/fonksiyon adlarıyla değil, gerçek oynanış akışı ve kanıtla değerlendirmek. Faz 1'e geçiş kapısı, 485 satırlık katalogda her gereksinimin sahibi, çağrı zinciri, mevcut durum, test ve açık riski kayıt altına almaktır.

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
- These are infrastructure changes only. **No Unreal Engine build or editor run has been performed in this environment**, so the shell runner still needs validation on the actual Windows + UE 5.8 host. The 101/12 minimums are taken from the existing repository report and should be changed only with an explicit baseline decision.

## Faz 0 çıkış kriterleri

Faz 0 tamamlanmış sayılmadan önce:

- [ ] Katalogdaki 485 satırın her birinde kalıcı ID bulunmalı.
- [ ] Her satır için kaynak dosyası/metot ve çağrı zinciri doldurulmalı; yoksa açıkça “bulunamadı” yazılmalı.
- [ ] Her satır U0–U6 durumlarından biriyle, kanıtı gösterilerek etiketlenmeli.
- [ ] Otomatik test kapsamı ile editor test kapsamı ayrılmalı.
- [ ] Çakışan authoritative state sahipleri karara bağlanmalı.
- [ ] Yeni fail-closed test runner, Windows + UE 5.8 ana makinesinde gerçek derleme/test ile doğrulanmalı; süreç çıkış kodları ve log ayrıştırması beklenen sonucu vermeli.
- [ ] Gerçek UE 5.8 baseline logları oluşturulmalı; bu çalışma ortamından henüz koşturulmuş gibi gösterilmemeli.
- [ ] Kazı spike'ının karşılaştıracağı teknoloji seçenekleri ve ölçümler onaylanmalı.

**Sıradaki çalışma:** önce CSV katalog satırlarını repo dosyaları ve çağrı zincirlerine bağlamak; ardından U0 olarak kalan en yüksek riskli sistemlerden başlayarak gerçek bir davranış ve test kanıtı elde etmek. Faz 1 kazı spike'ına ancak Phase 0 çıkış kapısı geçildikten sonra başlanır.


## 14 Ekim 2026 — katalog kapısı ve denetleyici

Eklenen denetleyici: `Scripts/audit_mechanic_catalog.py`; bağımsız test paketi: `Scripts/tests/test_audit_mechanic_catalog.py`.

Yerel katalog çıktısında 485 satır, 485 benzersiz ID, 0 yinelenen ID ve 485 adet `U0 — denetim gerekli` bulunuyor. Bu sonuç katalog şemasının düzgün olduğunu, ancak mekaniklerin kanıtlanmadığını gösterir. Bu nedenle Faz 0 **aktif kalır**; tüm satırlar için kaynak sahibi, çağrı zinciri, otomatik test kanıtı, editör kanıtı ve blocker bilgisi tamamlanmadan Faz 1 çıkış kapısı açılmaz.

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
