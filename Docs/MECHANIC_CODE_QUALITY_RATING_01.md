# Mekanik Kod Kalitesi Puan Kartı 01

**Tarih:** 23 Ekim 2026  
**Kapsam:** Ana daldaki mevcut mekaniğin temsilci kaynak dosyaları ve modules/mechanics-pre-unreal dalındaki taşınabilir C++ prototipleri.  
**Bu belge neyi puanlar:** Algoritma ve model derinliği, veri/state güvenliği, sorumluluk ayrımı, hata yolları, algoritmik maliyet ve kodun sürdürülebilirliği.

**Bu, test/derleme/PIE doğrulama puanı değildir.** Yıldızlar kodun nasıl yazıldığına ve hangi davranışı gerçekten modellediğine bakılarak verilmiştir. Bir modülün Unreal adaptörünün henüz bulunmaması, o işlev için çalışan kodun yerine yalnızca sözleşme varsa kod puanını düşürür; test sonuçları ayrıca puanlanmamıştır.

## Yıldız ölçeği

- ★☆☆☆☆ — algoritma yok denecek kadar az; çoğunlukla iskelet veya yer tutucu.
- ★★☆☆☆ — temel davranış var; model basit, önemli yollar eksik veya state güvenliği zayıf.
- ★★★☆☆ — anlamlı algoritma ve sorumluluk ayrımı mevcut; kayda değer teknik borç bulunuyor.
- ★★★★☆ — ayrıntılı model, tutarlı veri akışı ve iyi ayrıştırılmış kod; sınırlı ama belirgin ölçek/kenar durum eksikleri.
- ★★★★★ — derin ve tutarlı algoritma, iyi ölçeklenme, atomik state değişimi ve sürdürülebilir tasarım.

## 1. Yeni World Mechanics modülleri — kod kalitesi

| Mekanik / modül | Kod puanı | Kod düzeyindeki gerekçe |
|---|---:|---|
| CoreContracts | ★★★½☆ | Ortak ID, revizyon, Result/error, koordinat/AABB ve temel doğrulama tipleri doğru bir ortak dil kuruyor. Fakat bazı taşınabilir kodların yoğun tek satırlı yazımı okunabilirliği düşürüyor; kimlik ve snapshot sözleşmeleri de henüz tam kapsamlı değil. |
| RuntimeProfiles | ★★★☆☆ | LOW_MID/HIGH_END için spacing, chunk ve bütçe değerleri tek yerde toplanmış; spacing × hücre sayısı ile chunk kenarı arasında tutarlılık kontrolü var. Session lock yeni profilin iş bütçelerini tam karşılaştırmıyor; aynı grid geometrisine sahip farklı kuyruk/iş bütçeleri, kilitlenmiş profille eşdeğer sayılabiliyor. |
| WorldScan | ★★½☆☆ | Ray, shape sweep, overlap ve surface sample için ayrı istek/sınır kontrolleri iyi bir kontrat. Ancak sağlayıcı fonksiyonları varsayılan olarak Unsupported dönüyor; henüz gerçek dünya sorgusu yok. Overlap için profil kontrolü diğer query yollarıyla tutarlı değil, hit doğrulaması kaynak revizyonunu zorunlu kılmıyor ve ShapeSweep için ignore filtresi Ray kadar kapsamlı değil. |
| WorldClassification | ★★½☆☆ | Kanıt önceliği, manuel AABB override, sınıflandırma revizyonu ve dirty-cell kurucusu anlamlı. Fakat UpsertCells her küçük güncellemede tüm cells_ haritasını kopyalıyor; bu yerel değişikliği toplam harita büyüklüğüne bağlıyor. maxCellCount argümanı ayrıca toplam harita kapasitesini dolaylı olarak sınırlandırıyor. |
| TerrainVoxel | ★★★½☆ | Sparse density override, sınırlı küre/kutu/dikey kapsül düzenlemesi, beklenen revizyon, duplicate operation ID, receipt ve rollback gibi güçlü temel kararlar var. Eksikler: base-density kaynağı/world kimliği snapshot kimliğine bağlanmıyor; varsayılan base source sağlayıcısı yoksa yoğunluğu tamamen katı sayabiliyor; geniş snapshot/stres yükü için üst seviye maliyet yönetimi gerekli. |
| MeshCollision / CPU mesher | ★★★☆☆ | Marching Tetrahedra, edge cache, normal/UV/material verisi, triangle budget ve çıktı doğrulaması gerçekten uygulanmış. Buna karşın her edge için std::map kullanılması, gradientlerin çok kez hesaplanması ve giriş sample sayısı için sert üst bütçenin olmaması pahalı olabiliyor. Material seçimi tetrahedron içindeki ilk katı örneğe bağlı; geçiş katmanlarında fiziksel/material sürekliliği daha gelişmiş olabilir. |
| Excavation | ★★½☆☆ | Scan sonucu, hedef kimliği, mesafe, edit offset ve classification revision kontrol edilip density transaction çağrılıyor. Ancak RawHit.sourceRevision güncel yüzey/target revizyonuyla kıyaslanmıyor; gerçek tool/capability sözleşmesi ve tek bir tutarlı event çıkışı eksik. |
| PlacementSnap | ★★★☆☆ | Saf ve deterministik aday filtreleme/sıralama, explicit socket önceliği, mesafe/ID tie-break ve commit revision kontrolü iyi bir çekirdek. Ancak bu yalnızca adayları sıralıyor: aday üretimi, gerçek oriented full-volume overlap, Actor spawn, kaynak düşümü ve başarısız commit telafisi burada yok. Dot eşiklerinin anlamlı aralıkta olup olmadığı da kontrol edilmiyor. |
| Shoring | ★★½☆☆ | Siper geometrisinden deterministik istasyon/dikme/payanda/kaplama listesi çıkarıyor; Proposed → Validated → Installed → NeedsRevalidation yaşam döngüsü anlaşılır. Ancak gerçek arazi ankrajı, yüzeyden yükseklik/normal örneklemesi ve eksik kaplama çözümü yok; herhangi bir terrain revision değişimi bütün planları geçersizleştirebiliyor. Install/Revalidate işlemlerinin parça kayıtlarına uygulaması tam atomik değil. |
| StructuralIntegrity | ★★½☆☆ | ID kontrolü, destek DAG'ı, cycle reddi, hasar/onarım ve collapse latch anlaşılır. Kritik algoritma açığı: destek veren parçanın kapasitesi bağımlıların her birine tek tek kıyaslanıyor; çok sayıda bağımlının toplam talebi destekleyicide birikmiyor. Bu nedenle ortak taşıyıcı gerçekte aşırı yüklenmiş olsa bile her kol ayrı ayrı destekli sayılabilir. |
| WorldEnvironment | ★★½☆☆ | Timestamp/revision taşıyan, opsiyonel kanalları finite/range açısından kontrol eden veri modeli düzenli. Ancak bu bir çevre simülatörü değil; su, toprak, hava ve ışık için hesaplayıcı/provider davranışları henüz bu modülde bulunmuyor. |
| Survival modülü | ★★½☆☆ | Eylem maliyetlerini tabloya bağlamak, ortam örneğini doğrulamak ve maruziyeti ayrı state'te tutmak iyi bir ayrım. Update içinde deltaSeconds maxStepSeconds ile kesiliyor fakat kalan süre dilimlere ayrılarak işlenmiyor; uzun frame/pause süresinin bir bölümü kayboluyor. undergroundDepthM alanı henüz hesapta kullanılmıyor. |
| Persistence / density snapshot codec | ★★★☆☆ | Sürüm, boyut limitleri, deterministik kayıt sıralaması, duplicate kontrolü ve checksum içeren binary codec dikkatli yazılmış. Ancak sparse delta'nın hangi base-density kaynağı/world/profile'a ait olduğu taşınmıyor; farklı baseline ile aynı override'lar farklı dünya üretir. Whole-world participant ve migration davranışı bu koddan tek başına çıkmıyor. |
| Modüllerin genel C++ stili | ★★½☆☆ | Kalıcı modül sahipliği ve bağımlılık sınırları iyi düşünülmüş. Buna karşılık birçok .cpp dosyasında çok sayıda kontrol ve state mutasyonu tek satırlara sıkıştırılmış; inceleme, hata ayıklama ve yeni kural ekleme maliyetini yükseltiyor. |

### Yeni modüller için bileşik kod puanı

- **Mimari kararlar:** ★★★★☆
- **Gerçek algoritma kapsamı:** ★★★☆☆
- **State/işlem güvenliği:** ★★★☆☆
- **Ölçeklenebilirlik yaklaşımı:** ★★★☆☆
- **Bakım ve okunabilirlik:** ★★½☆☆
- **Genel kaynak kod olgunluğu:** **★★★☆☆**

Bu puan bir doğrulama notu değildir. Modüller sadece spesifikasyon değil; density transaction, mesher, snap resolver, shoring lifecycle ve snapshot codec gibi gerçek algoritmalar içeriyor. Genel yıldızı aşağı çeken başlıca konular veri/state atomikliği, ölçeksiz tam-map işlemleri, eksik maliyet sınırları ve sıkıştırılmış kod stili.

## 2. Ana daldaki mevcut mekaniğin kod değerlendirmesi

| Mekanik / temsilci kaynak | Kod puanı | Kod düzeyindeki gerekçe |
|---|---:|---|
| Ahşap bağlantı ve destek mantığı | ★★★★☆ | [WoodStructureLogic.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/Wood/WoodStructureLogic.cpp) uç-uç/uç-gövde/gövde-gövde ilişkilerini, çentikleri, yönelimi ve destek yayılımını açık kurallarla modelleyen gerçek geometri mantığı taşıyor. Bununla birlikte destek grafiğinin tamamını yeniden değerlendirme ve çiftli parçaları tarama maliyeti büyüyen yapılar için yükseliyor; model gerçek mühendislik yük çözümü değil, oyun kuralı yaklaşımı. |
| Ahşap yapı tanıma (duvar/platform/açıklık/köprü vb.) | ★★★½☆ | [WoodStructureAnalysis.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/Wood/WoodStructureAnalysis.cpp) düğüm/kenar grafiğinden birden çok yapı türü çıkarıyor ve belirsiz sonuçlarda temkinli. Ancak çok sayıda dikey parçanın komşuluğu ve grup yayılımı için O(n²) eşleşmeler, çiftli extent hesapları ve sabit toleranslar var; büyük yapı kümelerinde spatial index ve yerel yeniden analiz avantajlı olur. |
| Kütük kesme/yarma geometrisi | ★★★½☆ | [WoodCutLibrary.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/Wood/WoodCutLibrary.cpp) ile [WoodCutterComponent.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/Wood/WoodCutterComponent.cpp) kesimi hesaplama ile Actor/transform uygulamasını kısmen ayırıyor; modüler dilimler ve geometri değişimi var. Saf kesim hesapları, gerçek dünya state'ine yazan bölümden daha güçlü. |
| Kütük kesiminin world-state commit'i | ★★☆☆☆ | WoodCutterComponent::Finish yeni parça aktörlerini tek tek spawn ediyor; spawn veya bileşen bulunamama durumunda continue edip yine de ana kütüğü küçültüyor ve başarı döndürebiliyor. Bu işlem atomik değil: başarısız bir ara spawn malzemenin/parçanın kaybolmasına yol açabilir; bileşensiz oluşan yeni aktör de temizlenmiyor. Önce bütün çıktıları stage et, hepsi başarılıysa commit et; aksi durumda spawn edilenleri yok edip eski parçayı aynen koru. |
| Ahşap fiziksel sisteminin bütünü | ★★★½☆ | Çeşitli kesitler, uç biçimleri, çentikler, yaşam döngüsü, fizik, destek ilişkisi ve yapı tanıma anlamlı bir domain modeli oluşturuyor. Puanı sınırlayan en büyük konu kesim commit'inin atomik olmaması ve büyüyen yapılar için tam çiftli tarama maliyeti. |
| Ateş ve yanma | ★★½☆☆ | [BurnSubsystem.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/BurnSubsystem.cpp) aktif yanan parçaları izliyor, ısı/yanma durumlarını ayırıyor ve alt adım kullanıyor. Fakat her yanan parça için dünyadaki tüm kütükler ve kamp ateşleri tekrar taranıyor; ACampfire::Burn da kendi Tick'inde tüm kütükleri dolaşıyor. Birden çok ateş ve kütük olduğunda aynı dünya taramaları tekrar edilir. Advance ayrıca DeltaTime çok büyükse tüm alt adımları aynı çağrıda sınırsız döngüyle işleyebilir. |
| Raid Director / dalga orkestrasyonu | ★★½☆☆ | [RaidDirectorSubsystem.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/RaidCore/RaidDirectorSubsystem.cpp) bütçe, spawn limiti, dalga state machine, zaman ve olay geri bildirimi gibi sorumlulukları ayrı bileşenlere dağıtıyor. Ancak orkestrasyonun kendisi aşırı sıkıştırılmış; oyuncu bağlamında Level=1 ve bIsAlive=true varsayılıyor, spawn hedefi ilk yaşayan oyuncu, hedef seçme/saldırı rotası derinliği kodda sınırlı. |
| Raid hedef puanlama | ★★☆☆☆ | [TargetScorer.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/RaidPhaseR6_Targeting/TargetScorer.cpp) mesafe ve görünürlük hesabı yapıyor. Ancak WeightThreat ve WeightAccessibility hedefe özgü bir ölçümden gelmiyor; her hedefe aynı sabit ekleniyor. Böylece ağırlık adları görünmesine rağmen seçim ağırlıklı olarak mesafe/görünürlüğe dayanıyor. Bu puanlama mekanizması gerçek hedef seçimi seviyesine ulaşmıyor. |
| Su akış çözücüsü | ★★☆☆☆ | [WaterFlowSolver.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/WorldPhaseW4_WaterSoil/WaterFlowSolver.cpp) akış transferini önce delta haritasına yazıp sonra uygulayarak aynı adım içinde sırayla güncellenme yanlılığını azaltmaya çalışıyor; bu doğru bir başlangıç. Ancak yalnızca 2B gridde +X/+Y komşu çiftlerini işler, Level doğrudan Volume'dan atanıyor ve arazi yüksekliği/hidrolik yük kullanılmıyor. Rates.Add aynı hücrenin değerini her komşu için değiştirerek toplam akış hızını biriktirmek yerine son transfer ile ezebiliyor. |
| Chunk/world simülasyon zamanlayıcısı | ★★★☆☆ | [WorldStateSubsystem.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/WorldCore/WorldStateSubsystem.cpp) aktif/pasif/scheduled chunk, döner cursor, simülasyon bütçesi ve yüklenmeyen chunk için lazy catch-up yaklaşımı içeriyor. Ancak Tick sırasında chunk listesi toplanıp yeniden sıralanıyor; büyük haritalarda sürekli O(n log n) maliyet doğurabilir. GetCellAtWorld, veri okur gibi görünmesine rağmen hücreyi dirty işaretliyor; salt erişimle mutasyonun ayrılması daha doğru. |
| Dünya kaydetme/yükleme | ★★★☆☆ | [WorldPersistence.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/WorldPhaseW6_Persistence/WorldPersistence.cpp) sürüm, magic, boyut üst sınırı, sıkıştırma, deterministik anahtar sırası ve yükleme öncesi kayıtları parse etme gibi sağlamlık önlemleri taşıyor. Ancak parse sonrasında canlı store temizlenip parçalar sırayla yeniden yaratılıyor; yeniden kurulum ortasında bir hata olursa kısmi dünya kalabilir. Dosyaya kaydetme de geçici dosya + atomik rename modelini kullanmıyor. |
| Envanter stack/weight/slot mantığı | ★★½☆☆ | [InventoryComponent.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/InventoryCore/InventoryComponent.cpp) stack kapasitesi, ağırlık ve slot için preflight yapıyor; ekleme/çıkarma sonuç türleri de var. Ancak RestoreFromSavedItems önce ClearAll çağırıp sonra kayıtları doğruluyor; yükleme hatasında önceki envanter de kayboluyor. DropItem bu sınıfta yalnızca RemoveItem çağırıyor; dünya drop aktörü başka bir orchestration katmanında güvenilir biçimde oluşturulmadıkça item kaybı riski var. |
| Bitki büyümesi ve crop–soil bağlantısı | ★★½☆☆ | [PlantGrowthComponent.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/PhaseT2_Plant/PlantGrowthComponent.cpp) GDD, biyokütle, stres ve büyüme evresi gibi temel kavramları kullanıyor; [CropFieldActor.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/PhaseT2_Plant/CropFieldActor.cpp) toprak/su/nutrient/PH bileşenlerini günlük simülasyona bağlıyor. Fakat günlük soil update içinde çok sayıda sabit parametre veriliyor; besin/su/PH etkilerinin bir kısmı birbirine çarpan basit uygunluk katsayıları. Biyolojik model halen oyun düzeyi basitleştirilmiş bir model. |
| Hayvan beslenme, büyüme, üretim ve üreme | ★★½☆☆ | [AnimalActor.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/PhaseT6_Animal/AnimalActor.cpp), [AnimalReproductionComponent.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/PhaseT8_Reproduction/AnimalReproductionComponent.cpp) ve üretim bileşenleri ayrı sorumluluklara bölünmüş. Ancak AnimalActor::UpdateDaily birçok alt sistemin akışını tek merkezde birleştiriyor; üreme ilerlemesi olasılıkla birikiyor ve eşik geçilince ayrıca oranla zar atılıyor, bu iki aşamanın olasılık anlamı tasarımsal olarak netleştirilmeli. Bazı üretim bileşenleri birkaç satırlık doğrusal çarpanlardan ibaret. |
| Material M1–M6 / etkileşim kuralı sistemi | ★★★☆☆ | [MaterialInteractionSubsystem.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/MaterialPhaseM2_Interaction/MaterialInteractionSubsystem.cpp) kural seçimini, materyal sınıfını ve Fire/Water/Impact/Time davranışlarını ayırıyor; genişletilebilir kural matrisi iyi bir tasarım. Fakat etkileşim alt sistemi hasarı hesaplayıp olay yayınlıyor; hedef state'inin nasıl değişeceği başka abonelere kalıyor. Aynı materyal davranışının sahibini ve event sonuçlarını tek sözleşmede tutmak önemli. |
| Temel hayatta kalma (health/stamina/hunger/thirst) | ★★½☆☆ | [SurvivalComponent.cpp](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Source/MyProject2/SurvivalComponent.cpp) anlaşılır lineer stamina, ihtiyaç tükenmesi, grace period ve ölüm mantığı içeriyor. Bu kod yalın ve takip edilebilir; fakat etkiler çoğunlukla lineer sabit oranlardan oluşuyor ve eylem maliyetleri/çevresel maruziyet diğer yeni modüllerle henüz ortak modelde birleşmiyor. |

## 3. En belirgin kod kusurları — öncelik sırası

### P0 — state ve kaynak kaybını önle

1. **WoodCutterComponent::Finish'i atomik yap.** Tüm yeni parçaları önce oluştur ve doğrula; tek parça oluşturulamazsa spawn edilenleri geri al, ana kütüğü değiştirme.
2. **Inventory RestoreFromSavedItems'i staged restore yap.** Eski envanteri temizlemeden önce yükü ayrı buffer'da doğrula; commit başarısızsa eski state korunmalı.
3. **WorldPersistence restore'unu gerçekten transaction yap.** Pending chunk'ları doğrulamak iyi; canlı Store'u temizledikten sonraki kurulum da ya staged store swap ya da rollback ile güvence altına alınmalı.
4. **StructuralIntegrity yük biriktirmesini uygula.** Aynı support node'a gelen tüm aktif bağımlı yükleri hesapla; her kenarı bağımsız kapasite kontrolüyle destek var sayma.

### P1 — yanlış simülasyon sonuçlarını düzelt

5. **WaterFlowSolver::Rates toplamlarını biriktirsin.** Rates.Add yerine mevcut akışa göre topla; net akış işaretini (giren/çıkan) bir kuralla tanımla. Terrain/head modeli ve 2D/3D kapsamı açık olmalı.
6. **Raid hedef skorunu gerçek özelliklerden hesapla.** Threat ve accessibility her aktör için ayrı ölçülmeli. CaptureContext gerçek oyuncu seviyesini, hayatta olma durumunu ve relevant survival metriklerini kullanmalı; target selector director'a bağlanmalı.
7. **SurvivalExposure delta süresini kaybetmesin.** deltaSeconds 0.25 s'den büyükse süreyi dilimlerle işle veya biriktir; yalnızca ilk dilimi hesaplamak toplam maruziyeti eksik bırakır.
8. **Excavation güncel kaynak revizyonunu kontrol etsin.** Gözlem revizyonu değişmişse eski yüzeydeki kazı isteği commit edilmemeli.

### P2 — ölçeklenebilirlik ve sürdürülebilirlik

9. **Ateşin dünya taramasını uzamsal sorguya dönüştür.** Her yanan kütüğün bütün kütükleri/kamp ateşlerini dolaşması yerine heat radius içindeki adayları spatial hash/grid veya world query ile getir.
10. **WoodStructure pairwise hesaplarını sınırla.** Büyük yapı analizinde tüm parçaları birbirine kıyaslama; dirty bağlantı çevresini spatial hash/broad phase ile daralt.
11. **ClassificationMap'in full map copy işini kaldır.** Doğrulama sırasında touched key listesi oluşturup commit/rollback'i yalnızca o key'lerde uygula; batch ve total storage limitlerini ayrı tanımla.
12. **Mesher'ın tüm maliyetini bütçele.** Sample count limiti, gradient cache, daha düşük maliyetli edge key/cache ve materyal geçiş politikası ekle.
13. **Yeni modül kod biçimini normalleştir.** Her kontrol/state değişimini okunabilir bloklara ayır, uzun fonksiyonları parse/preflight/stage/commit alt adımlarına böl.

## 4. Kod kalitesi bileşik puanı

| Kod alanı | Puan |
|---|---:|
| Domain derinliği en iyi olan mevcut alan: ahşap bağlantı ve fiziksel montaj | ★★★★☆ |
| En iyi yapılandırılmış yeni modül kararı: sahiplik/sorumluluk ayrımı | ★★★★☆ |
| Yeni modüllerin mevcut algoritmik uygulama seviyesi | ★★★☆☆ |
| State mutasyonlarının atomikliği | ★★½☆☆ |
| Büyüyen dünyada algoritmik maliyet kontrolü | ★★½☆☆ |
| Genel okunabilirlik/bakım kolaylığı | ★★½☆☆ |
| Proje genelinde temsilci kaynak dosyalar için kod kalitesi | **★★★☆☆** |

## 5. Sonuç

Kod açısından en güçlü bölüm, fiziksel kütük sisteminin geometrik ilişkileri, parça rolleri ve destek yayılımı için geliştirilmiş kurallı domain modelidir. Yeni modüller de yalnızca doküman değil: density edit, rollback, CPU mesher, snap resolver, shoring plan ve persistence codec gibi gerçek kod var.

Ancak projenin birçok alanında bir sonraki seviye, yeni özellik eklemekten çok **işlem güvenliği ve daha derin algoritma** gerektiriyor. Kesim, envanter geri yükleme ve dünya geri yükleme işlemleri atomic olmalı; su akışında toplamlar doğru hesaplanmalı; yapısal bütünlük yükleri biriktirmeli; raid targeting oyuncu/hedef state'ine göre çalışmalı; global aktör taramaları yerel sorgulara dönüşmeli.

Bu puan kartı bir temsilci statik kaynak incelemesidir; tüm kaynak dosyaların eksiksiz satır satır denetlendiği anlamına gelmez. Üstte verilen dosya bağlantıları, puanlara dayanak olan kod örneklerini açar.
