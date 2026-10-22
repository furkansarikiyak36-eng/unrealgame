# Smooth Voxel + Kazı Tahkimatı + Hayatta Kalma Tasarım Raporu

**Tarih:** 22 Ekim 2026  
**Hedef:** Unreal Engine 5.8 / C++  
**Tür:** Mimari ve oynanış tasarım raporu; uygulama/test kanıtı değildir.  
**Ana fikir:** Oyuncu smooth-voxel zemini kazdığında oluşan gerçek kazı yüzeylerine fiziksel yerleştirme parçaları (direk, kaplama tahtası, payanda, merdiven vb.) yerleştirebilir. Bu parçalar voxel geometrisinin parçası yapılmaz; araziyle temas ve yük ilişkisi kuran ayrı oyun nesneleri olarak kalır.

> **Kapsam notu:** Görseldeki 4 m kazı derinliği ve 1,5 m aralık, prototipte kullanılabilecek referans parametreleridir. Bunlar gerçek dünyadaki güvenli kazı tahkimatı için mühendislik hesabı veya uygulama talimatı sayılmaz. Oyundaki dayanım modeli de gerçek geoteknik hesabın yerine geçmez.

---

## Mekanik kataloğu — kısa grup özeti

Bu bölüm raporun hızlı başvuru tablosudur. Durum etiketleri, mekanik fikrinin tamamlandığını değil repo içindeki mevcut kanıt düzeyini gösterir.

| Grup | Kısa öz rapor | Ana bağımlılık |
|---|---|---|
| A. Dünya tarama ve sınıflandırma | Dünyayı algılar; hangi alanın voxel, statik veya belirsiz olduğunu belirler. | World Scanner, Analyzer, Landscape Builder |
| B. Smooth voxel arazi | Kazı verisini değiştirir; yüzey ve collision'ı günceller. | Sparse density, chunk, mesher, collision |
| C. Yerleştirme ve snap | Nesneyi geçerli yüzey/bağlantı noktasına oturtur. | Scan sonucu, snap provider, overlap |
| D. Kazı tahkimatı | Dikme, kaplama ve payandaları kazılmış hacme yerleştirir; destek kaybını izler. | Voxel yüzeyi, shoring planner, yapı grafiği |
| E. Yapı ve hasar | Yapı desteğini, hasarı, onarımı ve çöküşü yönetir. | Structure graph, material state, fizik |
| F. Hayatta kalma ve çevre | Açlık, susuzluk, dayanıklılık, sıcaklık, ıslanma ve su etkilerini yönetir. | Survival, zaman/hava, world state |
| G. Kaynak, envanter ve üretim | Fiziksel kaynakları toplama, taşıma, işleme ve üretime aktarır. | Fiziksel aktörler, envanter, crafting |
| H. Ateş ve savunma | Yanma, ısı, tuzak ve düşman baskısını yönetir. | Fire state, trap, raid, structure damage |
| I. Tarım ve hayvanlar | Bitki ve hayvan üretimini günlük dünya döngüsüne bağlar. | Crop/animal state, yem, stok, toprak |
| J. Keşif ve kayıt | Keşif bulgularını ve dünya değişikliklerini saklar. | Journal, persistence, chunk/actor state |
| K. Test ve performans | Mekanikleri doğrular; frame, mesh, collision ve kayıt maliyetlerini ölçer. | Otomasyon, UE 5.8, Unreal Insights |

### A. Dünya tarama ve sınıflandırma

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Raycast | Kameranın/isteğin gördüğü ilk uygun çarpışma hedefini bulur. | Scan taslak PR |
| Shape Trace | Küre, kutu veya kapsül hacmini bir yol boyunca tarar. | Scan taslak PR |
| Overlap Query | Bir alan içindeki çarpışan bileşenleri toplar. | Scan taslak PR |
| Yüzey örnekleme | Küçük bir grid ile yükseklik, normal ve eğim değişimini ölçer. | Scan taslak PR |
| Bounds / fizik sorgusu | Nesne sınırlarını ve anlık fizik durumunu alır. | Scan taslak PR |
| Hit Analyzer / Classification | Hedefi tür, yüzey, hareket ve yetenek adaylarına ayırır. | Scan taslak PR |
| Landscape Builder | Mevcut dünyayı analiz eder, sınıflandırma haritası çıkarır. | Yeni iş; uygulanacak |
| Voxel/statik ayrımı | Kazılabilir zemin, sabit kaya/dağ, geçiş ve belirsiz alanları ayırır. | Yeni tasarım |

**Grup özeti:** Scan dünyayı sorgular; Landscape Builder ise sorgu ve kaynak verilerinden bölgesel karar üretir. Her ikisi de bütün dünyayı her karede taramamalıdır.

### B. Smooth voxel arazi

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Sparse 3D density/delta | Değişen kazı hücrelerini temel arazi verisinden ayrı saklar. | Başlangıç veri çekirdeği taslak dalda |
| Kazı/doldurma işlemleri | Sphere, capsule veya box hacmini değiştirir. | Başlangıç API'si taslak dalda |
| Chunk yönetimi | Dünya hücrelerini bölgelere ayırır; etkilenen alanları bulur. | Kısmen mevcut |
| Smooth mesh üretimi | Yoğunluk verisinden yumuşak üçgen yüzey oluşturur. | Tamamlanması gerekiyor |
| Collision yenileme | Yeni yüzeyin fiziksel çarpışmasını günceller. | Tamamlanması gerekiyor |
| Chunk sınırı dikişi | Komşu chunk'larda delik, kopuk normal ve çarpışma boşluğu oluşmasını önler. | Tamamlanması ve test edilmesi gerekiyor |
| Kazı kalıcılığı | Density delta/snapshot'ı kaydeder ve yükler. | Veri sözleşmesi var; uçtan uca entegrasyon eksik |

**Grup özeti:** Voxel çekirdeği kazının veri otoritesidir. Mesh, collision ve save/load aynı kazı sonucunu göstermeden sistem tamamlanmış sayılamaz.

### C. Yerleştirme ve snap

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Snap point üretimi | Kütük/tahta gibi parçalar için izin verilen bağlantı noktalarını üretir. | Kütük snap taslak PR |
| Snap resolver | En yakın uyumlu adayı normal, mesafe ve eksen kurallarıyla seçer. | Taslak PR |
| Ghost preview | Yerleştirilecek transformu ve geçersizlik nedenini gösterir. | Var olan placement akışı; entegrasyon doğrulanmalı |
| Overlap kontrolü | Parçanın dolu hacimle çakışıp çakışmadığını kontrol eder. | Yerel sorgu yaklaşımı taslakta |
| Commit doğrulama | Onay anında çakışma ve destek koşullarını yeniden kontrol eder. | Tasarım/PR; UE 5.8 testi gerekiyor |
| 20 cm plank ızgarası | Tahta için düzenli snap noktaları sağlar. | Yeni provider olarak eklenmeli |

**Grup özeti:** Scan hedefi bulur; Snap nokta seçer; Placement çakışma ve destek kurallarını doğrulayıp nesneyi yerleştirir.

### D. Kazı ve tahkimat

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Kazı hacmi | Voxel zeminde kuyu, hendek, tünel veya oda açar. | Veri çekirdeği var; gerçek mesh/collision hattı eksik |
| Tahkimat planner'ı | Kazı ölçülerine göre dikme, kaplama ve payanda yerleşimini planlar. | Yeni tasarım |
| Dikey dikme | Kaplama ve payanda bağlantıları için ana dikey taşıyıcı parça. | Yerleştirilebilir actor olarak yapılacak |
| Kaplama tahtası | Kazılmış yan yüzeyi örten ayrı fiziksel parça. | Yerleştirilebilir actor olarak yapılacak |
| Yatay payanda | Karşılıklı destekler arasında bağlantı oluşturur. | Yerleştirilebilir actor olarak yapılacak |
| Yüzeye anchor/snap | Tahkimat parçasını voxel yüzeyi ve diğer parçalara hizalar. | Scan/Snap entegrasyonu gerektirir |
| Eksik parça raporu | Planlanan tahkimatta eksik veya geçersiz bağlantıları gösterir. | Yeni tasarım |
| Kazı sonrası revalidation | Voxel değişince komşu tahkimatı yeniden kontrol eder. | Yeni entegrasyon |

**Grup özeti:** Tahkimat voxel mesh'i değildir. Fiziksel aktörlerden oluşur; kazı yüzeyi değişince yerleşim ve destek geçerliliği tekrar değerlendirilir.

### E. Yapı, destek ve hasar

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Yapı grafiği | Kütük/parçalar arasındaki temas ve destek ilişkilerini belirler. | Ana dalda temel kod mevcut |
| Kiriş/direk/payanda | Yapı geometrisini kurar ve yük yolları oluşturur. | Kısmen mevcut; yeni tahkimat tipleri ayrı |
| Hasar ve gevşeme | Hasar arttıkça bağlantı veya taşıma kabiliyetini düşürür. | Ana dalda temel kütük kuralları mevcut |
| Onarım/değişim | Hasarlı parçayı onarır veya değiştirir. | Kütük sistemi için temel mevcut |
| Destek kaybı/çökme | Kritik bağlantı kaybında bağlı parçaları yeniden değerlendirir. | Temel sistem mevcut; tahkimata bağlanmalı |
| Malzeme dayanımı | Ahşap/taş gibi malzemelerin hasar ve çevresel tepkisini belirler. | Kısmi kod; sistemler arası entegrasyon doğrulanmalı |

**Grup özeti:** Tahkimat yerleşimiyle yapı grafiği aynı bağlantı kimliklerini veya aynı deterministik temas kurallarını kullanmalıdır; yoksa görselde bağlı görünen parçalar simülasyonda bağlı olmayabilir.

### F. Hayatta kalma ve çevre

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Sağlık/açlık/susuzluk | Oyuncunun temel hayatta kalma kaynaklarını izler. | Survival kodu mevcut; uçtan uca doğrulanmalı |
| Dayanıklılık | Kazı, ağır taşıma ve inşa sırasında tüketilir. | Temel stamina var; işlere göre maliyet ayarlanmalı |
| Sıcaklık/ıslanma | Hava, bölge ve ıslaklıkla oyuncunun ısı durumunu değiştirir. | Genişletilmiş dalda kod parçaları mevcut |
| Barınak/ateş ısısı | Çatı ve ateş ortam sıcaklığını etkiler. | Temel kurallar var; entegrasyon testi gerekir |
| Yeraltı erişimi | Merdiven, halat, platform ve giriş/çıkış yolları sağlar. | Kısmen tasarım/önceki mekanik kodları |
| Su girişi/birikmesi | Açılmış hacimde su ve nem etkilerini yönetir. | World state var; voxel hacmiyle entegrasyon eksik |
| Karanlık/ışık | Yeraltında meşale ve görünürlük ihtiyacını doğurur. | Tasarlanacak/oyun içi doğrulama gerekir |
| Hava kalitesi/oksijen | Opsiyonel yeraltı maruziyeti mekaniğidir. | Sonraki faz; ilk prototip zorunluluğu değil |

**Grup özeti:** İlk survival entegrasyonu stamina, sıcaklık/ıslanma, ışık, su ve kaçış yolu üzerine kurulur. Oksijen/gaz gibi ek simülasyonlar temel kazı döngüsü doğrulandıktan sonra eklenir.

### G. Kaynak, envanter ve üretim

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Ağaç/kaya toplama | Dünyadaki ham kaynağı fiziksel kaynaklara dönüştürür. | Ana dalda temel kod mevcut |
| Kütük kesme/oyma | Standart modül noktalarından kütük ve yapı parçaları üretir. | Ana dalda mevcut |
| Fiziksel taşıma | Büyük kütük ve taşları hacim/kullanıma göre taşır. | Temel kütük akışı mevcut |
| Envanter | Küçük/stack edilebilir öğeleri ve hızlı erişimi yönetir. | Kod mevcut; fiziksel kaynak köprüsü eksik |
| Crafting | Kaynak tüketerek parça/alet üretir. | Kod mevcut; ana oynanışa entegrasyon doğrulanmalı |
| Üretim/iş istasyonu | Hammaddeyi işlenmiş kaynak ve bileşene dönüştürür. | Modüller var; tam oynanış bağlantısı eksik |
| Kaynak muhasebesi | Başarısız yerleştirme/üretimde kaynak kaybını önler. | Uçtan uca işlem sözleşmeleri gerekli |

**Grup özeti:** Fiziksel kütük/taş gibi nesneler ile genel envanter öğeleri tek bir sistemmiş gibi ele alınmamalı. Transfer, üretim ve kayıt aynı miktar korunumu kurallarını kullanmalıdır.

### H. Ateş, savunma ve düşmanlar

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Kamp ateşi/yakıt | Odun hacmine göre yanma süresi ve ısı üretir. | Ana dalda temel kod mevcut |
| Ahşap yanması | Tutuşma, yanma, kömürleşme ve yakına yayılım. | Ana dalda temel kod mevcut |
| Tuzak | Temas/trigger ile oyuncu veya düşmana etki uygular. | Genişletilmiş dalda taslak |
| Kazık/çit savunması | Geçişi sınırlar ve düşmana zarar verebilir. | Kısmi kod/taslak |
| Raid yönetimi | Düşman saldırısı, hedef seçimi ve dalga planını yönetir. | Temel davranış var; spawn/navigation entegrasyonu eksik |
| Yapıya saldırı | Düşman hasarını yapı parçasına aktarır. | Bazı sınıflar için var; tüm ana aktörlere adaptör gerekir |

**Grup özeti:** Yangın ve temel raid kodu ile yeni tuzak/defans taslakları aynı hasar ve yapı durumu sözleşmesine bağlanmalıdır.

### I. Tarım ve hayvanlar

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Tarla/tohum | Ekim alanı açar ve büyümeyi yönetir. | Genişletilmiş dalda kod mevcut |
| Bitki büyümesi/hasat | Zamanla ürün ve tohum üretir. | Kod mevcut; stok bağlantısı doğrulanmalı |
| Hayvan günlük döngüsü | Hayvanın sağlık, büyüme ve ürün durumunu günceller. | Genişletilmiş dalda kod mevcut |
| Yem tüketimi | Hayvan stoğundan kaynak tüketir. | Birden fazla stok sistemi bulunduğundan entegrasyon kontrolü gerekir |
| Hayvansal ürünler | Türüne göre yumurta, süt, yün, et/deri gibi ürünler sağlar. | Kısmi kod; E2E doğrulama gerekli |
| Gübre/toprak | Hayvan ve tarım kaynaklarını toprak besinlerine bağlar. | Modüller var; bilanço testi gerekli |

**Grup özeti:** Hasat, yem, hayvan ürünü ve gübre aynı canonical stok ve günlük simülasyon akışına bağlanmalı; ayrı bileşenlerin birbirini gerçekten çağırdığı test edilmelidir.

### J. Keşif ve kayıt

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Keşif/notlar | Dünyadaki notları bulur ve günlüğe kaydeder. | Genişletilmiş dalda kod var |
| Dünya zamanı/hava | Gün, gece ve hava durumunu dünya state'inde tutar. | Kod mevcut; sistem entegrasyonu doğrulanmalı |
| Chunk persistence | Hücre ve dünya verilerini saklar. | Temel kayıt altyapısı var |
| Voxel persistence | Kazı delta/snapshot'ını yeniden yükler. | Başlangıç sözleşmesi var; runtime geometriye bağlanmalı |
| Fiziksel aktör kaydı | Kütük, taş, tahkimat, hasar ve bağlantı durumunu korur. | Tam kapsama ve E2E test gerekli |
| Güvenli yükleme sırası | Önce terrain/collision, sonra aktör/anchor ve sistem state'ini kurar. | Yeni entegrasyon kuralı |

**Grup özeti:** Hücre state'ini kaydetmek fiziksel dünyayı kaydetmekle aynı şey değildir. Voxel, tahkimat ve bağlantılar tek yükleme senaryosunda doğrulanmalıdır.

### K. Test ve performans

| Mekanik | Kısa açıklama | Durum |
|---|---|---|
| Mekanik demo/test | Temel oyun döngüsünü otomatik veya rehberli yürütür. | Ana dalda mevcut |
| Scan/Snap testleri | Aday seçimi, normal uyumu ve deterministik sonucu kontrol eder. | PR taslaklarında test tanımı var |
| Kazı kabul testleri | Kuyu, tünel, oda, seam, rollback ve reload senaryoları. | Planlandı; UE içinde doğrulanmalı |
| CPU/frametime profili | Scan, mesh, collision ve worst-frame maliyetini ayırır. | Ölçüm yapılmalı |
| Test raporu/kanıt | Build, otomasyon, log ve ekran kaydını saklar. | Bazı altyapı var; yeni alanlara genişletilmeli |

**Grup özeti:** Kaynak kodu veya test tanımı tek başına çalıştığını kanıtlamaz. Her yeni mekanik için UE 5.8 build, otomasyon ve gereken yerde Editor/PIE doğrulaması gerekir.

---

## 1. Karar özeti

Sistem dört ayrı otoriteden oluşur:

1. **Landscape Builder:** Mevcut dünyayı inceler ve bölgeleri sınıflandırır.
2. **Smooth Voxel Core:** Değiştirilebilir arazi hacminin yoğunluk/SDF ve kazı delta verisinin sahibi olur.
3. **World Scan & Placement:** Güncel voxel yüzeyi veya statik geometri üzerinde hedef, yüzey, çarpışma ve snap adaylarını bulur.
4. **Structure & Survival Simulation:** Yerleştirilen tahkimatın bağlantılarını, yükünü, dayanımını, çökmesini ve oyuncunun hayatta kalma durumuna etkisini yönetir.

Temel karar: **Kazı arazi verisini değiştirir; tahkimat kazılmış yüzey üzerine/ içine yerleştirilen ayrı fiziksel aktörlerden oluşur.** Bir destek tahtasını değiştirmek voxel yoğunluğunu değiştirmemeli; voxel kazısı ise etkilenen desteklerin geçerliliğini yeniden kontrol etmelidir.

## 2. Referans kazı sahnesi

Görseldeki örnek oyun içi bir “tahkimat yerleştirme modülü” olarak ele alınır:

- Kazı yüzeyi: 3B voxel hacmi ile oluşmuş çukur veya hendek.
- Referans derinlik: 4 m.
- Referans payanda aralığı: kazı boyunca 1,5 m merkezden merkeze (O.C.).
- Dikey ahşap dikmeler: yan yüzeylerde ana taşıyıcı elemanlar.
- Yatay duvar kaplama tahtaları (lagging/waling): kazılmış toprağa bakan yüzü tutan elemanlar.
- Yatay enine payandalar (struts): karşılıklı kenarlar arasında basınç/mesnet ilişkisi oluşturan elemanlar.
- Alt yüzey ve kazı tabanı: voxel zemin; gerektiğinde geçiş, platform veya zemin kaplaması da yerleştirilebilir.
- Kesikli/ghost hacim: planlanan kazı hacmi veya geçici önizleme; gerçek voxel yoğunluğu değil.
- Katman renkleri: toprak, gevşek taş, sağlam kaya ve bilinmeyen bölge gibi yüzey sınıfları.

Payanda aralığı, eleman kesiti, ahşap kalitesi ve destek yüksekliği oyun tasarım parametreleridir; sabit mühendislik gerçekleri gibi kullanılmamalıdır.

---

## 3. Sistem blokları

### BLOK 01 — Landscape Builder (yeni araç)

**Görev:** Dünyayı ilk seferde veya editör aracıyla parça parça analiz eder.

- Landscape, statik mesh, component/actor bounds, collision kanal yanıtı ve fiziksel materyal/katman verilerini toplar.
- Actor/component tag ve manuel bölge işaretlerini okuyabilir.
- World'u ayarlanabilir analiz hücrelerine/bölgelere böler.
- Yükseklik, normal, eğim ve yüzey devamlılığı gibi geometri verilerini toplar.
- Çıktı olarak bir sınıflandırma haritası ve her kararın kaynağını üretir.
- Her karede tüm dünyayı taramaz; editörde batch, oyun sırasında ise yalnızca kirli/değişen bölgelerde çalışır.

**Sınıflar:**

- `EditableVoxel`: kazılabilir arazi.
- `StaticGeometry`: değiştirilmeyecek kaya/dağ/mesh.
- `Transition`: voxel ile statik yüzey arasındaki birleşim.
- `Water`: su hacmi/yüzeyi; gerçek voxel kazısıyla bağlantısı ayrı kurulmalı.
- `Unknown`: yeterli kanıt olmayan bölge; sessizce dönüştürülmez.

**Önemli:** Eğim tek başına karar vermez. Düz kaya statik kalabilir; eğimli toprak kazılabilir olabilir. Açık tag/manuel override, otomatik geometri çıkarımından daha yüksek önceliklidir.

### BLOK 02 — Smooth Voxel Core (veri otoritesi)

- Sparse 3D density/SDF veya uygun 3B hücre verisi.
- Dünya koordinatı → cell → chunk adresleme.
- Sphere, capsule ve box tabanlı kazı/doldurma operasyonları.
- Her işlem için transaction kimliği, değişen hücre listesi ve güvenli rollback.
- Temel arazi yoğunluğu + yerel kazı delta katmanı.
- Chunk komşuluğu ve seam yenileme listesi.
- Sürümlü snapshot/delta dışa aktarma ve içe aktarma.

Voxel Core gerçek araziyi tanımlar. Mesh, collision, Scan ve tahkimat aktörleri bu verinin üstündeki sistemlerdir; yoğunluk verisinin yerine geçmez.

### BLOK 03 — Voxel Mesh / Collision Builder

- Density/SDF hücrelerinden smooth yüzey mesh'i üretir.
- Yalnızca değişmiş chunk'ları ve gerekli komşuları yeniler.
- Chunk sınırlarında delik, normal kopması veya üst üste gelen yüzey kalmadığını doğrular.
- Collision mesh güncellemesini render mesh güncellemesinden ayrı ve izlenebilir bir iş olarak yönetir.
- Büyük işleri tek kareye yüklemek yerine kuyruğa alır ve bütçeler.
- Kazıdan sonra fizik sorgularının güncel collision'a erişmesini sağlar.

**İlk prototipteki zorunlu kabul:** Görünen yüzey, collision yüzeyi ve kayıtlı voxel delta aynı kazı sonucunu göstermelidir.

### BLOK 04 — World Scan & Hit Analyzer

Altı sorgu tipini ihtiyaç oldukça kullanır:

1. Raycast.
2. Sphere/box/capsule shape trace.
3. Sphere/box overlap.
4. Yüzey grid örnekleme.
5. Actor/component bounds.
6. Anlık fizik durum sorgusu.

Analiz çıktısı: nesne tipi, temas konumu, normal, mesafe, yüzey profili, fizik durumu ve destek/snap/kazı/hasat/taşıma/etkileşim aday yetenekleri.

Scanner tek hit ile bütün bir duvarın sağlamlığını, yüzeyin devamlılığını veya yeraltı hacmini bilemez. Tahkimat yerleştirme gerektiğinde birden çok yüzey örneği ve mevcut voxel hücre verisini de talep eder.

### BLOK 05 — Surface Anchor & Snap Resolver

**Görev:** Bir tahkimat parçasının voxel kazı yüzeyine uygun transform ile yerleşmesini sağlamak.

- Raycast/Scan mevcut yüzey noktasını ve normalini bulur.
- Yüzey örneklemesi parçanın temas edeceği alanın eğimini ve devamlılığını değerlendirir.
- Adaylar parça türü, izin verilen yüz, yön, eksen, mesafe, derinlik ve hizalama kurallarına göre filtrelenir.
- Ghost preview, nihai transform ve onay işlemi aynı resolver sonucunu kullanır.
- Preview sonucu cache edilebilir; gerçek commit sırasında geçerlilik, yüzey sürümü ve çakışma tekrar kontrol edilir.
- Yüzey kazıyla değişmişse eski snap noktası geçersiz olur ve preview yenilenir.

**Tahtalar için not:** 20 cm gibi bir plank ızgarası, Scan'in özelliği değil, snap point provider'ın kuralıdır. İstenilen aralık ayrı yapılandırılır.

### BLOK 06 — Placeable Excavation Shoring

Tahkimat elemanları voxel mesh'ine birleştirilmez; bağımsız actor/component olarak kalır.

**Parçalar:**

- Dikey timber upright / dikme.
- Yatay kaplama tahtası / lagging.
- Yatay enine payanda / strut.
- Alt taban/oturma parçası.
- Geçici platform ve merdiven/iniş-çıkış elemanı.
- İsteğe bağlı kama, bağlama ve tamir parçaları.

**Yerleştirme aşamaları:**

1. Oyuncu voxel yüzeyini tarar ve kazı/tahkimat modunu seçer.
2. Tahkimat planı hendek ekseni, genişliği, derinliği ve segment aralığı üzerinden oluşturulur.
3. Ghost parçalar yüzey normallerine ve plana göre gösterilir.
4. Çakışma, yerleştirme sınırı, zemine oturma ve bağlantı kuralları kontrol edilir.
5. Onayla fiziksel parçalar oluşturulur ve yapısal grafiğe kaydedilir.
6. Kazı geometrisi yeniden değişirse o bölgeye bağlı parçalar yeniden doğrulanır.

### BLOK 07 — Shoring Layout Planner

- Kazı hattı boyunca yerleştirme istasyonları üretir.
- Varsayılan referans aralığı 1,5 m O.C.; tasarımcı tarafından değiştirilebilir.
- Her istasyonda karşılıklı dikmelerin ve enine payandanın konumunu hesaplar.
- Kaplama tahtalarını yerel voxel yüzeyine hizalar.
- Kazı derinliği/eni segment sınırlarını aşınca ek dikme/payanda isteyebilir.
- Eksik parçaları, geçersiz bağlantıları ve çakışmaları görsel olarak raporlar.
- Otomatik yerleştirme ile oyuncunun tek tek yerleştirmesi ayrı modlar olabilir.

İlk sürümde otomatik planner tek başına fiziksel yapı inşa etmemeli; aday yerleşimi ve eksik parçaları sunmalı. Oyuncu onayıyla inşa veya geliştirici test modu kullanılabilir.

### BLOK 08 — Structure Graph, Support & Failure

- Her dikme, kaplama ve payanda bir fiziksel yapı bileşeni olarak kayıt edilir.
- Bağlantılar explicit socket/anchor kimlikleri veya test edilmiş geometri-temas kurallarıyla takip edilir.
- Kaplama tahtası → dikme → enine payanda → karşı dikme bağlantı zinciri kurulur.
- Destek hesabı kazı derinliği, hendek eni, payanda açıklığı, malzeme profili, nem, hasar ve parça durumunu kullanır.
- Bir eleman sökülür/kırılırsa bağlı tahkimat tekrar değerlendirilir.
- Destek kaybı olduğunda oyun kuralına göre bükülme, gevşeme, kısmi çökme veya tam göçük olayı tetiklenir.
- Çökme alanı Scan/collision ve oyuncu erişimi için yenilenir.

İlk prototipte yük kapasitesi deterministik ve basitleştirilmiş bir oyun modeli olabilir. Bu skorlar gerçek zemin mekaniği veya gerçek kazı güvenliği hesabı değildir.

### BLOK 09 — Excavation Events & Ground State

- Yeni kazı hacmi oluşturma ve voxel hücrelerini değiştirme.
- Kazı tabanı ve yan duvarlarını tekrar analiz etme.
- Zemin malzemesi: soil, loose rock, hard rock veya unknown gibi gameplay sınıfları.
- Yerel nem/su durumu.
- Yüzey desteği ve desteklenmemiş açıklık.
- Çökme riski için yerel skor/eşik.
- Kazı alanına bağlı tahkimat elemanlarını invalidation/recheck kuyruğuna alma.
- Gerekirse voxel hacmine bağlı düşen toprak/debris olayları.

Voxel'in değişebilir olması, her yüzeyin otomatik çökmesi gerektiği anlamına gelmez. Çökme ayrı ve açık bir simülasyon kararıdır.

---

## 4. Hayatta kalma mekanikleri

### BLOK 10 — Temel Survival State

- Sağlık, dayanıklılık, açlık ve susuzluk.
- Yeme/içme ve kaynak tüketimi.
- Kazı, ağır taşıma ve inşa için dayanıklılık tüketimi.
- Yorgunluk veya iyileşme hızı.
- Sıcaklık ve ıslanma etkileri.
- Hasar kaynakları ve ölüm/yeniden doğma akışı.

Repo'da `SurvivalComponent` ve genişletilmiş survival kodu bulunur; bütün yeni çevre etkilerinin kazı sistemiyle uçtan uca bağlı olduğu kabul edilmemelidir.

### BLOK 11 — Temperature, Wetness & Shelter

- Ortam sıcaklığı bölge/hava/zaman verisinden gelir.
- Islanma soğuma cezası oluşturabilir.
- Çatı/barınak ve ateş sıcaklık etkisi sağlar.
- Yeraltı/derin kazı alanının sıcaklığı ayrı bir bölge profili kullanabilir.
- Uzun süre ıslak/soğuk kalma sağlık veya dayanıklılığı etkileyebilir.
- Tahkimat tek başına ısıtma veya su yalıtımı sağlamaz; bunun için ayrı kaplama, drenaj ve barınak kuralları gerekir.

### BLOK 12 — Underground Access & Exposure

Bu blok yeni oynanış tasarımıdır; mevcut kodda tamamlandığı varsayılmamalıdır.

- Kazıya güvenli giriş/çıkış noktası.
- Merdiven, halat ve geçici platform yerleştirme.
- Oyuncunun düşme mesafesi ve dar alan hareketi.
- Karanlıkta meşale/ışık ihtiyacı.
- Su birikmesi ve tabanda ilerlemenin zorlaşması.
- Uzun süre yeraltında kalma için isteğe bağlı hava kalitesi/oksijen veya gaz sistemi.
- Kaçış yolu kapanması ve kurtarma davranışı.

Hava/oksijen simülasyonu ilk prototipte zorunlu değildir. Önce ışık, erişim, su, sıcaklık ve göçük etkileri test edilip daha sonra genişletilmesi önerilir.

### BLOK 13 — Water Ingress & Drainage

- Su seviyesi ve çevresel nem bilgisi world/chunk state ile ilişkilendirilir.
- Kazı yerel su tabakasına ulaştığında sızıntı veya birikme olayı doğabilir.
- Tahkimatın su geçirmez olduğu otomatik varsayılmaz.
- Drenaj kanalı, su tahliyesi veya pompa gibi ayrı sistemler eklenebilir.
- Su derinliği hareket, dayanıklılık ve ıslanma üzerinden oyuncuya etki eder.
- Chunk sınırından su sürekliliği ve kayıt/yükleme tutarlılığı test edilir.

Mevcut su/toprak veri modülleri, kazılmış voxel hacminin otomatik biçimde hidrolik olarak bağlandığını tek başına kanıtlamaz.

### BLOK 14 — Shoring Craft, Materials & Repair

- Odun/kütük toplama, parçayı işleme ve tahtaya dönüştürme.
- Parça türüne göre envanter veya fiziksel taşıma.
- Dikme, kaplama ve payanda yapımı için kaynak tüketimi.
- Tahkimatın dayanıklılık/hasar durumu.
- Sökme ve yeniden kullanma.
- Hasarlı tahtayı/dikmeyi değiştirme.
- Oyuncunun araç, stok ve erişim gereksinimleri.
- Yerleştirme esnasında kaynak tüketimini transaction mantığıyla yapmak: başarısız yerleştirme kaynak kaybettirmemeli.

Kütüklerin fiziksel şekli ve durumu, yalnızca genel envanter öğesine indirgenmemelidir. Kesilmiş tahta gibi standardize edilmiş parçalar envanterde taşınabilir; uzun dikme ve payandalar fiziksel olarak taşınabilir.

### BLOK 15 — Risk Feedback & Player Readability

- Scan sonucu yerleştirilebilir/yerleştirilemez bilgisini verir.
- Ghost preview geçerli veya geçersiz durumu anlaşılır gösterir.
- Geçersizlik sebebi görünür olmalı: destek noktası yok, yüzey çok eğimli, çakışma var, parça eksik, açıklık fazla veya voxel yüzeyi yeniden değişmiş.
- Tahkimat yerleştirilmeden önce risk uyarısı gösterilebilir.
- Çökme riski için basit görsel/işitsel uyarılar olabilir.
- Tahkimat grafiği ve zemin durumu sürümü önizlemede cache key olarak kullanılır.

---

## 5. Tarama → kazı → tahkimat → survival iş akışı

### Örnek: Oyuncu 4 m derinliğinde hendek açıyor

1. **Scan:** Başlangıç yüzeyini bulur; temas noktası, normal ve hedef bölgeyi döndürür.
2. **Landscape sınıflandırması:** Alanın `EditableVoxel` olduğu doğrulanır. Statik kayaya denk geliyorsa mevcut kurala göre kazı reddedilir veya ayrı davranış uygulanır.
3. **Voxel edit:** Planlanan hacme ait density delta tek transaction olarak uygulanır.
4. **Mesh/Collision:** Etkilenen chunk'lar yenilenir; yeni hendek yüzeyleri collision ile eşleştirilir.
5. **Shoring planner:** Hendek ekseni, genişliği ve derinliğini alır; 1,5 m O.C. referanslı yerleşim istasyonları hesaplar.
6. **Snap/placement:** Dikmeler ve kaplama tahtaları yeni yüzeylere oturtulur; payandalar karşılıklı dikmeler arasında konumlanır.
7. **Support graph:** Eleman bağlantıları ve basitleştirilmiş destek durumu kaydedilir.
8. **Survival:** Kazı/taşıma dayanıklılık tüketir; ıslanma, soğuk, ışık ve su durumu oyuncuya etki edebilir.
9. **Dünya değişikliği:** Sonraki kazı bir dikmenin tabanını veya temas yüzeyini bozarsa bağlı tahkimat geçersizleşir ve yeniden kontrol edilir.
10. **Persistence:** Voxel delta, tahkimat aktörleri, hasar/bağlantılar ve survival/world state'i birlikte kaydedilir.

Bu akışta hiçbir sistem diğerinin yerine geçmez: Scan bulur; Voxel Core araziyi değiştirir; Mesh/Collision günceller; Snap yerleştirir; Structure Graph desteği yönetir; Survival etkileri hesaplar.

---

## 6. Save/Load kapsamı

Kalıcı kayıt sadece kazı delta'sını saklamakla bitmez. En az şu veriler birlikte ele alınmalıdır:

- Voxel density delta ve sürüm numarası.
- Kirli/yenilenen chunk indeksleri veya yeniden üretilebilir referansları.
- Yerleştirilen tahkimat parçalarının türü, GUID'i, transformu, malzeme/hasar durumu.
- Bağlantı/snap ilişkileri ya da bunları yeniden kurmak için deterministik anchor kimlikleri.
- Kazı/tahkimat bölgesi parametreleri ve açık risk/olay durumu gerekiyorsa.
- Su, nem, sıcaklık ve dünya zamanının ilgili state'i.
- Oyuncu envanteri, taşıdığı kaynaklar ve survival değerleri.

Yükleme sırası: world/chunk state → voxel delta → voxel mesh/collision → placeable actors → yapı grafiğini tekrar kurma/doğrulama → survival/world simülasyonunu yeniden senkronize etme. Geometri hazır olmadan tahkimat yüzey referansları kesin kabul edilmemelidir.

---

## 7. Performans ve CPU kuralları

- Landscape Builder taraması editör/batch veya kontrollü arka plan işidir; her Tick tüm dünya taranmaz.
- Normal nişan için bir raycast; placement için yerel overlap ve gerekiyorsa küçük yüzey örnekleme kullanılır.
- Yüzey örnekleme 3×3/5×5 gibi küçük grid ile başlar; daha yoğun grid yalnızca açıkça istenirse çalışır.
- Voxel edit hücreleri, sonra mesh, ardından collision ve etkilenen tahkimat kontrolleri için kuyruk oluşturur.
- Her değişiklik bütün dünyayı değil, etkilenen chunk ve doğrudan komşuları kirli işaretler.
- Support graph cache'i voxel/chunk sürümü veya yapı graph sürümü değiştiğinde geçersizleşir.
- Commit sırasında preview cache'ine güvenilmez; hedef, collision, anchor ve destek yeniden doğrulanır.
- Unreal Insights/istatistiklerle Game Thread maliyeti, mesh üretim süresi, collision güncellemesi, en kötü kare sıçraması ve aktif chunk sayısı ölçülür.

İlk kabul hedefi bir performans iddiası değil, ölçüm kapısıdır: küçük kuyu, chunk sınırını geçen tünel, çok chunk'lı kazı ve tahkimatın ardışık yerleştirilmesi aynı donanımda profillenir.

---

## 8. Test senaryoları ve kabul koşulları

1. **Sınıflandırma:** Çimenli Landscape voxel, işaretli kaya statik, belirsiz alan bilinmeyen kalmalı.
2. **Smooth surface:** Küçük kazı sonrası mesh/collision uyuşmalı; oyuncu görünür zeminden düşmemeli veya görünmez duvara çarpmamalı.
3. **Chunk seam:** Tünel iki chunk sınırını geçerken yüzeyde delik/çıkıntı olmamalı.
4. **Temas/snap:** Dikme eğimli voxel yüzeye yanlış açıyla yapışmamalı; kaplama tahtası yüzeyde kabul edilebilir temas kurmalı.
5. **Hendek planı:** Dikey dikmeler, kaplama ve 1,5 m O.C. referanslı payandalar uygun konumlarda gösterilmeli; ölçüler ayarlanabilmeli.
6. **Çakışma:** Aynı hacme iki parça yerleşmesi engellenmeli veya yalnızca açıkça izin verilmiş bağlantılar kabul edilmeli.
7. **Destek kaybı:** Bir payanda/dikme söküldüğünde bağlı yapının geçerliliği yeniden hesaplanmalı.
8. **Yeniden kazı:** Oyuncu mevcut desteğin yanındaki voxel'i kazınca bağlı elemanlar yeniden doğrulanmalı; geçersiz destek sessizce geçerli kalmamalı.
9. **Survival:** Kazı/taşıma stamina tüketmeli; soğuk/ıslaklık/ışık/su etkileri ayrı testlerle doğrulanmalı.
10. **Save/load:** Kaydı kapatıp yeniden yüklemede voxel, mesh, collision, tahtalar, bağlantılar, hasar ve survival state'i tutarlı kalmalı.
11. **Failure safety:** Yerleştirme başarısız olursa kaynak tüketimi geri alınmalı; bozuk save veya eksik anchor dünya state'ini kısmen bozmasın.
12. **Profiling:** Her senaryoda trace sayısı, etkilenen chunk, mesh/collision süresi ve worst-frame spike kaydedilmeli.

---

## 9. Mevcut repo durumuyla ayrım

Bu rapor yeni tasarım ve entegrasyon sözleşmesidir; aşağıdaki maddeler hazır çalışıyor iddiasında değildir.

| Parça | Mevcut kanıt / durum |
|---|---|
| Fiziksel kütük kesme, taşıma, yerleştirme, yapı desteği ve yangın temeli | Ana dalda mevcut kod ve tarihli test kayıtları var |
| SurvivalComponent ve genişletilmiş hayatta kalma mekanikleri | Kod parçaları var; bütün sistem bağlantıları ayrı doğrulama ister |
| World Scanner + Hit Analyzer | Açık taslak PR #4 |
| Altı tarama yöntemli sorgu planı | Açık taslak PR #5 |
| Log snap resolver ve preview iyileştirmesi | Açık taslak PR #3 |
| WorldSnap runtime eklentisi ve scan-to-snap köprüsü | Açık taslak PR #6 |
| Sparse 3D excavation data core | `plan/phase-00-baseline` kolunda taslak; mevcut belgeler mesh/collision üretmediğini belirtiyor |
| Landscape Builder ve classification preview | Bu raporda tasarlanan yeni iş; henüz uygulanmış özellik değil |
| Smooth voxel mesh ve collision pipeline | Tamamlanmış/UE 5.8 içinde doğrulanmış değil |
| Voxel yüzeye yerleşen hendek tahkimatı | Yeni tasarım; henüz uygulanmış özellik değil |
| Kazı–tahkimat–survival uçtan uca entegrasyonu | Henüz kanıtlanmış değil |

### İlgili mevcut tasarım ve kod belgeleri

- [Faz 1 kazı teknolojisi spike](https://github.com/furkansarikiyak36-eng/unrealgame/blob/plan/phase-00-baseline/Docs/FAZ_01_KAZI_TECHNOLOJI_SPIKE.md)
- [World Scan & Analysis](https://github.com/furkansarikiyak36-eng/unrealgame/blob/feature/global-world-scan-analysis-v01/Docs/GlobalWorldScanAnalysis.md)
- [Altı tarama yöntemi](https://github.com/furkansarikiyak36-eng/unrealgame/blob/feature/global-world-scan-six-methods-v01/Docs/GlobalWorldScanAnalysis.md)
- [Log Placement & Snap System](https://github.com/furkansarikiyak36-eng/unrealgame/blob/feature/log-snap-system-v01/Docs/LogPlacementSnapSystem.md)
- [Ana dalın entegrasyon kararları](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Docs/KARARLAR.md)

---

## 10. Önerilen uygulama sırası

**Faz A — Arazi sınıflandırma prototipi:** Landscape/Static Rock/Unknown ayrımı ve görsel önizleme.

**Faz B — Tek smooth voxel alanı:** Küçük bir test alanında kazı → smooth mesh → collision → save/load.

**Faz C — Surface Anchor & Placement:** Voxel yüzeyine bir dikme ve bir kaplama tahtası yerleştirme; preview/commit ve çakışma kontrolü.

**Faz D — Hendek tahkimat planner'ı:** Karşılıklı dikmeler, kaplama ve değiştirilebilir aralıkla payandalar; önce ghost plan, sonra gerçek actor spawn.

**Faz E — Destek ve göçük:** Bağlantı grafiği, destek geçerliliği, hasar ve yeniden kazı sonrası revalidation.

**Faz F — Survival entegrasyonu:** Stamina, ışık, sıcaklık/ıslanma, su birikmesi ve güvenli giriş-çıkış. Hava/oksijen gibi daha pahalı özellikler sonraki faza bırakılabilir.

**Faz G — Persistence ve performans kapısı:** Tüm actor/state/voxel verilerini beraber kaydet-yükle ve UE 5.8 içinde profille.

**Nihai tasarım kararı:** Smooth voxel arazi dinamik ve kazılabilir zemini temsil eder. Tahkimat, merdiven, platform ve diğer inşa parçaları bu yüzeylere snap/anchor kurallarıyla yerleşen ayrı fiziksel nesnelerdir. Kazı dünyayı günceller; yapı grafiği desteği yeniden değerlendirir; survival sistemi değişen ortamı oyuncuya yansıtır. Böylece kazılabilir arazi, yerleştirme ve hayatta kalma tek bir veri yığınına dönüşmeden birlikte çalışır.
