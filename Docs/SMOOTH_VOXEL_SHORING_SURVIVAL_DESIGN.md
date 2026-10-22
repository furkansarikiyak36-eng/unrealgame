# Smooth Voxel + Kazı Tahkimatı + Hayatta Kalma Tasarım Raporu

**Tarih:** 22 Ekim 2026  
**Hedef:** Unreal Engine 5.8 / C++  
**Tür:** Mimari ve oynanış tasarım raporu; uygulama/test kanıtı değildir.  
**Ana fikir:** Oyuncu smooth-voxel zemini kazdığında oluşan gerçek kazı yüzeylerine fiziksel yerleştirme parçaları (direk, kaplama tahtası, payanda, merdiven vb.) yerleştirebilir. Bu parçalar voxel geometrisinin parçası yapılmaz; araziyle temas ve yük ilişkisi kuran ayrı oyun nesneleri olarak kalır.

> **Kapsam notu:** Görseldeki 4 m kazı derinliği ve 1,5 m aralık, prototipte kullanılabilecek referans parametreleridir. Bunlar gerçek dünyadaki güvenli kazı tahkimatı için mühendislik hesabı veya uygulama talimatı sayılmaz. Oyundaki dayanım modeli de gerçek geoteknik hesabın yerine geçmez.

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
