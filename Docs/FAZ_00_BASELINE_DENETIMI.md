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

## Faz 0 çıkış kriterleri

Faz 0 tamamlanmış sayılmadan önce:

- [ ] Katalogdaki 485 satırın her birinde kalıcı ID bulunmalı.
- [ ] Her satır için kaynak dosyası/metot ve çağrı zinciri doldurulmalı; yoksa açıkça “bulunamadı” yazılmalı.
- [ ] Her satır U0–U6 durumlarından biriyle, kanıtı gösterilerek etiketlenmeli.
- [ ] Otomatik test kapsamı ile editor test kapsamı ayrılmalı.
- [ ] Çakışan authoritative state sahipleri karara bağlanmalı.
- [ ] `Scripts/run_tests.sh` kullanıcının UE 5.8 ortamında çalıştırılıp yeni baseline kayıt edilmeli; bu çalışma ortamından henüz koşturulmuş gibi gösterilmemeli.
- [ ] Kazı spike'ının karşılaştıracağı teknoloji seçenekleri ve ölçümler onaylanmalı.

**Sıradaki çalışma:** önce CSV katalog satırlarını repo dosyaları ve çağrı zincirlerine bağlamak; ardından U0 olarak kalan en yüksek riskli sistemlerden başlayarak gerçek bir davranış ve test kanıtı elde etmek. Faz 1 kazı spike'ına ancak Phase 0 çıkış kapısı geçildikten sonra başlanır.
