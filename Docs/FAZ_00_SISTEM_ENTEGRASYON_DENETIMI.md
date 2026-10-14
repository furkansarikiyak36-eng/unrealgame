# Faz 0 — Sistem entegrasyonu kaynak denetimi

**Durum:** Kaynak statik denetimi tamamlandı; Faz 0 hâlâ açık.  
**Bu belge neyi kanıtlar:** İncelenen kaynakların somut davranış ve sahiplik sınırlarını.  
**Neyi kanıtlamaz:** Unreal Engine 5.8 derlemesini, oyun içi görsel doğrulamayı, bütün çağrı noktalarını veya 485 katalog satırının tek tek tamamlandığını.

## 1. Fiziksel kaynak ↔ envanter

Kaynaklar:
- [PhysicalToInventoryBridge.cpp](../Source/MyProject2/InventoryBridge/PhysicalToInventoryBridge.cpp)
- [InventoryToPhysicalBridge.cpp](../Source/MyProject2/InventoryBridge/InventoryToPhysicalBridge.cpp)
- [InventoryComponent.cpp](../Source/MyProject2/InventoryCore/InventoryComponent.cpp)
- [InventoryItem.h](../Source/MyProject2/InventoryCore/InventoryItem.h)
- [InventorySaveSubsystem.cpp](../Source/MyProject2/InventoryCore/InventorySaveSubsystem.cpp)

Gözlenen gerçekler:
- Fizikselden envantere köprü yalnızca `ALogActor`, `AStoneActor`, `AStickActor` alıyor ve `FName ItemID` üzerinden tek bir genel item ekliyor. Aktörün transformu, fiziksel hasarı, şekli, bağlantısı veya yapı durumu item payload'ına aktarılmıyor.
- Oynanışın kütük sistemi `ACarvableLog`; bu köprü onunla doğrudan çalışmıyor. Dolayısıyla aynı kaynak görünse bile iki log sınıfı tek ve kayıpsız sistem sayılmaz.
- Envanterden fiziksele dönüşte önce `SpawnActor`, sonra `RemoveItem` var; RemoveItem başarısızsa yeni aktör yok ediliyor. Bu, işlemin bir bölümünü doğru sırada yapıyor. Ancak yaratılan aktör sınıfı köprü ayarına göre seçiliyor ve fiziksel nesneye özgü kesim/darbe/bağlantı state'i generic item üzerinden geri gelmiyor.
- `UInventoryItemData` yalnızca `WorldMesh` tutuyor; fiziksel sınıfa ve state serileştirmesine bağlayan genel bir factory sözleşmesi bu tipte görünmüyor.
- `UInventoryComponent::DropItem` sadece `RemoveItem` çağırıyor. İncelenen metodun kendisi fiziksel dünya pickup'ı üretmiyor. Tam kullanıcı eylemindeki bütün çağrı noktaları ayrıca aranmalı; bu bulgu tek başına tüm UI akışının eşya kaybettiğini kanıtlamaz.
- `UInventorySaveSubsystem` item array'ini ve quick slotları saklıyor; fiziksel dünya aktörleri bu save nesnesinin kapsamına alınmıyor.

**Karar:** Faz 6'dan önce “drop/pickup/transfer/craft” işlemleri ayrık işlem sözleşmelerine sahip olmalı. Büyük kütük gibi geometri/state taşıyan fiziksel kaynaklar, tasarım izin vermeden stack item'a indirgenmemeli. Başarı, iptal, dolu envanter, sınıf yok, spawn başarısızlığı ve save/load için miktar korunumu testleri gerekli.

## 2. Raid Director ↔ fiziksel yapı

Kaynaklar:
- [RaidDirectorSubsystem.cpp](../Source/MyProject2/RaidCore/RaidDirectorSubsystem.cpp)
- [SpawnDecisionPipeline.cpp](../Source/MyProject2/RaidPhaseR5_Spawn/SpawnDecisionPipeline.cpp)
- [StructureDamageResolver.cpp](../Source/MyProject2/RaidPhaseR8_Defense/StructureDamageResolver.cpp)

Gözlenen gerçekler:
- `RaidDirectorSubsystem::StartRaid` event lock alıyor, context snapshot topluyor, zorluk/plan belirliyor, dalga state machine'i başlatıyor; bunlar somut orkestrasyon davranışları.
- `CaptureContext` oyuncu konumunu snapshot alıyor ancak `Level=1` atıyor ve dünya çarpanını `1.f` sabitliyor. Bu nedenle gerçek oyuncu ilerlemesi/oyun saatiyle zorluk eşleşmesi bu koddan doğrulanmıyor.
- `RaidDirectorSubsystem::ProcessSpawns` kendi içinde doğrudan `SpawnActor` çağırıp bütçe/popülasyon sayaçlarını güncelliyor.
- Ayrı `USpawnDecisionPipeline::ExecuteSpawnDecision` de konum arayıp aktör üretiyor, bütçe/popülasyonu güncelliyor. İncelenen director kodu bu pipeline'ı çağırmıyor; tek canonical spawn yolu kanıtı yok.
- `StructureDamageResolver::ApplyDamageToBlock` `ALogActor`, `AStoneActor`, `AStickActor` türlerine dallanıyor. Ana fiziksel `ACarvableLog` akışı için doğrudan hasar adaptörü görünmüyor.
- `UpdateWorldAfterBlockDestroyed` bir yorumdan ibaret; nav/rota/yapı bağımlılıklarını fiilen yenilemiyor.

**Karar:** Raid için tek spawn otoritesi seçilecek ve tüm limit/bütçe/counter kontrolleri aynı yolun parçası olacak. Yapı hasarı ortak bir arayüz/adapter üzerinden authoritative yapı state'ine uygulanmalı. Yıkım; destek çökmesi, dünya olayı ve navigasyon yenilemesini tetiklemeli.

## 3. Dünya simülasyonu ↔ kalıcılık ↔ fiziksel geometri

Kaynaklar:
- [ChunkState.h](../Source/MyProject2/WorldCore/ChunkState.h)
- [WorldPersistence.cpp](../Source/MyProject2/WorldPhaseW6_Persistence/WorldPersistence.cpp)
- [WaterSimulation.cpp](../Source/MyProject2/WorldPhaseW4_WaterSoil/WaterSimulation.cpp)
- [SoilSimulation.cpp](../Source/MyProject2/WorldPhaseW4_WaterSoil/SoilSimulation.cpp)
- [WorldEventSubsystem.cpp](../Source/MyProject2/WorldPhaseW3_Event/WorldEventSubsystem.cpp)

Gözlenen gerçekler:
- Chunk hücresinde terrain, water, chemistry, biomass, toprak besinleri/nem ve flags saklanıyor. Bu, yoğunluk tabanlı bir 3B kazı hacminin kendisi değil.
- `WorldPersistence::SerializeWorldToBytes` dünya saati, hava, seed ve `ChunkStateStore` payload'ını yazıyor; `DeserializeWorldFromBytes` bunu geri yüklüyor ve `SynchronizeAfterLoad` çağırıyor.
- Bu serialization metotları canlı `ACarvableLog`, `AHaulCart`, yapı aktörleri veya fiziksel item aktörlerini dolaşarak serileştirmiyor. Başka bir alt sistem bunları saklıyor olabilir; repo içinde ayrı bir uçtan uca kanıt bulunana kadar kapsam bilinmiyor.
- Su simülasyonu tek `UChunkState` alıyor. Hücreler içinde su akışı ve su seviyesi eventi var; komşu chunk sınırından fiziksel akışın doğru birleşmesi bu fonksiyonda gösterilmiyor.
- Toprak simülasyonu su→nem, buharlaşma/perkolasyon ve N/P/K kaybını değiştiriyor. Bunlar lokal hücre modelinin varlığını kanıtlar, ancak fiziksel kazılmış tünelin drenaj/geometrik bağlantısını kanıtlamaz.
- `UWorldEventSubsystem` eventleri kuyruğa alıyor ve frame başına sınırlı sayıda işliyor; event queue bulunması bir event'in her sistemde sonuç ürettiği anlamına gelmez.
- Bu kaynaklarda çukur/tünel/oda collision mesh'ini üretip tekrar yükleyen bir runtime hacim pipeline'ı görünmüyor.

**Karar:** Faz 1'de en az iki kazı teknolojisi karşılaştırılacak. Faz 2'de chunk hücre state'i ile runtime excavation geometry delta'sının sahibi ve kayıt formatı açıkça tanımlanacak. Sadece world-state save testi, fiziksel geometri save/load kabul kapısı yerine kullanılamaz.

## 4. Tarım ↔ hayvan ↔ yem ↔ gübre

Kaynaklar:
- [HarvestComponent.cpp](../Source/MyProject2/PhaseT5_Harvest/HarvestComponent.cpp)
- [AnimalActor.cpp](../Source/MyProject2/PhaseT6_Animal/AnimalActor.cpp)
- [FeedCycleComponent.cpp](../Source/MyProject2/PhaseT9_Integration/FeedCycleComponent.cpp)
- [FertilizerComponent.cpp](../Source/MyProject2/PhaseT9_Integration/FertilizerComponent.cpp)
- [ManureComponent.cpp](../Source/MyProject2/PhaseT9_Integration/ManureComponent.cpp)

Gözlenen gerçekler:
- `HarvestComponent::Harvest` bitki biyokütlesinden yield döndürüyor ve bitki state'ini sıfırlıyor; kendisi stok/enventere ekleme yapmıyor.
- `AAnimalActor::UpdateDaily`, `AAnimalActor` üzerindeki `FeedStorageComponent` üzerinden yem seçiyor ve tüketiyor; büyüme, enerji, sağlık, gebelik ve süt/yumurta/yün üretimini günlük güncelleme içinde hesaplıyor.
- Ayrı `UFeedCycleComponent` kendi `FeedStock` map'ini saklıyor ve `AddHarvestedFeed/ConsumeForAnimals/UpdateDaily` metotlarını tanımlıyor. İncelenen hayvan akışında bu map yerine `FeedStorageComponent` kullanılıyor; ikisi arasında aynı authoritative stock olduğunu gösteren bağlantı bu incelemede bulunmadı.
- `UFertilizerComponent::ApplyManure` owner'daki `USoilNutrientComponent` azotunu güncelliyor ve `UManureComponent::CollectManure` çağırıyor; bunun stok yönü ve çağrı zamanlaması E2E testle doğrulanmalı.
- `UManureComponent` manure miktarı ve nitrogen state'i tutuyor; ayrı bileşen bulunması ürün artığı/hayvan atığının otomatik toprak döngüsüne bağlandığını kanıtlamıyor.

**Karar:** Faz 9 öncesi bir canonical stok sahibi seçilmeli veya `FeedCycleComponent` açıkça bir `FeedStorageComponent` adapter'ına bağlanmalı. Harvest yield'i stokta muhasebeleştirilmeli; hayvan tüketimi aynı stoktan düşmeli; hayvan ürünü ve manure miktar/besin bilançosuyla toprağa geri girmeli. Gün atlatma, kapasite doluluğu ve save/load testleri gerekli.

## 5. Materyal state ↔ chunk biomassa

Kaynaklar:
- [MaterialChunkIntegration.cpp](../Source/MyProject2/MaterialPhaseM6_Integration/MaterialChunkIntegration.cpp)
- [MaterialPersistenceIntegration.cpp](../Source/MyProject2/MaterialPhaseM6_Integration/MaterialPersistenceIntegration.cpp)

Gözlenen gerçekler:
- Chunk biomass, material interaction ve exponential decay modeliyle güncelleniyor; bazı material state alanları extension payload'larına serialize ediliyor.
- Material registry, species/material ID'sini bulamazsa Wood profilini döndürebiliyor veya yeni bir Wood fallback oluşturuyor. Bu, eksik/yanlış ID'yi görünmez kılabilir.
- Bu chunk biomass hesapları, aynı yangın/hasar olayının fiziksel actor'a ve world cell'e bir kez uygulandığını göstermez. İki temsil arasında event ownership/authoritative state sözleşmesi açık olmalı.

**Karar:** Bilinmeyen material ID için sessiz Wood fallback yerine raporlanabilir karar (fail/skip/explicit fallback policy) kullanılsın. Fiziksel aktör ile chunk hücresi aynı materyalin iki kopyasıysa hangisinin authoritative olduğu, adaptör sınırı ve etki uygulama idempotency'si belgelenmeli.

## Faz 0'a etkisi

Bu statik incelemeyle repo içinde somut olarak **entegrasyon engelleri** doğrulandı; sistemlerin varlığı, aralarında uçtan uca akış olduğu anlamına gelmiyor. Faz 0, katalog satırlarını bu kaynak yollarına ve belirli testlere bağlamak için açık kalır. Kod/derleme/oyun içi kanıtı henüz olmayan maddeler U5/U6 seviyesine yükseltilmez.


## 6. Oyuncu envanteri ve modüler Inventory stack'i

Kaynaklar:
- [ForestCharacter.cpp](../Source/MyProject2/ForestCharacter.cpp)
- [ForestCharacter.h](../Source/MyProject2/ForestCharacter.h)
- [InventoryComponent.cpp](../Source/MyProject2/InventoryCore/InventoryComponent.cpp)
- [InventoryUIComponent.cpp](../Source/MyProject2/InventoryUI/InventoryUIComponent.cpp)
- [CraftingUIComponent.cpp](../Source/MyProject2/InventoryUI/CraftingUIComponent.cpp)
- [InventorySaveSubsystem.cpp](../Source/MyProject2/InventoryCore/InventorySaveSubsystem.cpp)
- [FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv](./FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv)

Gözlenen gerçekler:
- `AForestCharacter` kendi `TMap<EForestItem, int32> Inventory` stoğunu ve 24 slot/20 stack kurallarını tutuyor; pickup/drop/craft fonksiyonlarının ana akışı bu temsil üzerinde.
- `AForestCharacter` constructor'ı `USurvivalComponent`, kamera ve preview bileşenlerini oluşturuyor; incelenen C++ constructor'ında `UInventoryComponent`, `UInventoryUIComponent`, `UCraftingUIComponent` veya `UQuickAccessComponent` oluşturulmuyor.
- Paralel modüler `UInventoryComponent`, kategori ve ağırlık kontrolü, `InventoryUIComponent`, `CraftingUIComponent`, `RecipeValidator` ve `InventorySaveSubsystem` mevcut.
- Crafting UI tarafındaki `ConfirmCraft` consumed ingredient'ları bir map'te birleştiriyor, kaldırma veya çıktı ekleme başarısız olursa rollback deniyor; ama bu sistemin oynanabilir karakterdeki legacy inventory ile aynı stok otoritesini kullandığı gösterilmedi.
- `UInventoryComponent::DropItem` yalnızca `RemoveItem` çağırıyor. Ayrı inventory-to-physical bridge generic `ALogActor/AStoneActor/AStickActor` sınıflarını spawn ediyor; ana oynanıştaki `ACarvableLog` dönüşümü yok.
- Blueprint tarafında bu modüler bileşenlerin karaktere eklenmiş olma olasılığı dışlanamaz; bu oturumda ilgili asset wiring/editör başlangıcı doğrulanmadığı için entegrasyon "yok" diye kesinlenmiyor, **kanıtlanmamış** olarak işaretleniyor.

**Karar:** Faz 6'dan önce tek canonical inventory authority seçilmeli. Legacy `AForestCharacter` map'i ile modüler `UInventoryComponent` arasında iki yönlü sync yazmak yerine tek owner veya açık adapter tercih edilmeli. Pickup, craft, drop, quick access, physical bridge ve save/load aynı stok kaynağını kullanmalı. Testler full inventory, invalid spawn, craft rollback, cancel ve round-trip resource conservation koşullarını kapsamalı.


## 7. Çubuk craft işlemi — atomiklik düzeltmesi (kaynak kodu güncellendi)

İncelenen `UCraftingComponent::Craft` yolu için şu değişiklikler çalışma dalına eklendi:

- `UStickInventoryComponent::CanApplyCraftTransaction` aynı öğenin birden çok ingredient satırındaki miktarlarını toplar; var olan sayıyı yalnızca ayrı satırlarla karşılaştırmaz.
- Çıktı kapasitesi girdiler düşüldükten sonraki toplam üzerinden kontrol edilir; tüketilen girdilerin açtığı kapasite çıktı için kullanılabilir.
- `ApplyCraftTransaction` eski `Bundles` dizisinin snapshot'ını saklar. Beklenmeyen herhangi bir input removal/output add hatasında snapshot geri yüklenir.
- `UCraftingComponent` gerekli aletin mevcut olmasını kontrol eder; `RequiredShape` dolu tarifleri, mevcut stok biçimi şekil saklamadığı için yanlışlıkla geçerli saymak yerine reddeder.
- `Source/MyProject2/Tests/StickCraftingTransactionTests.cpp` içine üç Unreal Automation Test eklendi: duplicate ingredient preflight, kapasite sınırı/başarısızlıkta korunma ve başarılı işlem bilançosu.
- `Scripts/run_tests.sh` yeni test grubunu ayrı logla çalıştıracak; `Scripts/verify_test_logs.py` üç testin doğru suite prefix'i altında raporlanmasını ve minimum üç başarılı sonuç olmasını zorunlu kılıyor.

**Doğrulama sınırı:** Python tooling testleri yeni log doğrulama yollarını kontrol edebilir, fakat bu C++/UObject testleri ancak UE 5.8 Windows hostunda derlenip çalıştırılınca geçerli sayılır. Bu ortamda Unreal build henüz çalıştırılmadı; P0-027 bu yüzden “düzeltildi” değil, **kod + otomasyon testi eklendi; motor doğrulaması bekliyor** durumundadır.
