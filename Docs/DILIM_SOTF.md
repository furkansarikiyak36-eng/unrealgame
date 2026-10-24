# Sons of the Forest Referans Dilimi — Log + Taş

**Amaç:** Sons of the Forest'ın tümünü kopyalamak değil; fiziksel kütük, taş ve elle montaj oynanışını kendi sistemimiz için gözlemlenebilir bir kabul hedefi yapmak.

**Kapsam dışı:** Combat, düşman AI, hikâye/görev, rehber kitabı ve hayalet yapı şablonları. Bizim inşa yaklaşımımız serbest/elle montajdır.

> **Durum notu:** “Mevcut durum” sütunu şu anki proje değerlendirmesinden aktarılmıştır; Unreal Engine 5.8 içinde bu belgeyle yeniden test edilmiş olduğu anlamına gelmez. Kodun bulunması, oynanışın çalıştığını veya kaydedildiğini tek başına kanıtlamaz.

## 1. Oynanış hedefi ve istasyonlar

Durumlar: **Var** = mevcut olduğu bildirildi; **Kısmen** = eksik veya yalnızca kısmen bağlanmış; **Test yok** = kod/aktör olabilir ancak oynanış kabul testi yok; **Yok** = istenen davranış eksik.

| ID | Oynanış hedefi | Mevcut bildirilen durum | İstasyon / kontrol | Kabul ölçütü |
|---|---|---|---|---|
| LOG-01 | Ağacı kes; fiziksel kütük düşsün | Var | Ağaç-kesim istasyonu | Kesim, düşüş ve tekrar etkileşim aynı dünyada çalışır; kütük kaybolmaz/çoğalmaz |
| LOG-02 | Kütüğü yarım/çeyrek/tahta gibi işle | Var; nokta göstergesi eklendi | Kütük işleme istasyonu | İşlem seçimi doğru parçayı ve beklenen ölçüyü üretir |
| CARRY-01 | Kütük/taş taşı; çoklu taşıma ve araç kullan | Var; hacme göre çoklu taşıma ve taşıma istasyonu bildirildi | Taşıma istasyonu | Taşınan, bırakılan ve araçtaki adetler tutarlıdır |
| BUILD-01 | Kütükleri elle üst üste koy; köşeyi geçir | Var | Kütük duvar/köşe istasyonu | Parçalar destek ilişkisi kurar; küçük konum değişimleriyle bağlantı bozulmaz |
| BUILD-02 | Duvarı keserek kapı/pencere açıklığı oluştur | Var | Duvar açıklığı kontrolü | Açıklık sonrası parça durumu ve kalan destekler doğru güncellenir |
| BUILD-03 | Direk, kiriş, payanda, platform ve döşeme kur | Var | Ahşap montaj istasyonu | Montaj parçaları fiziksel bağlantı ve destek kurallarını izler |
| ROOF-01 | Eğik çatı kirişi ve mahya oluştur | Kısmen; mahya eksik | Çatı istasyonu | İki eğim ortak mahyada buluşur; parçalar doğru destek alır |
| ACCESS-01 | Merdiven/basamak kur | Kısmen | Erişim istasyonu | Parçalar doğru yükseltiye bağlanır ve karakter kullanılabilir yüzeyde hareket eder |
| DEF-01 | Palisat/sivri kazık duvar kur | Var | Savunma montaj istasyonu | Parçalar bir hat boyunca birleştirilebilir; destek ve çarpışma tutarlı |
| SUPPORT-01 | Desteksiz parçanın çökmesi | Var; yeni uyarı bildirildi | Destek/çökme istasyonu | Destek kaldırılınca yalnızca destek zinciri bozulan parçalar yeniden değerlendirilir |
| AUTOCUT-01 | İki direk arasına uygun kütükleri otomatik boyda kes | Yok | Otomatik boy kesme kontrolü | Açıklık ölçülür; üretilen parça hedef aralığa oturur. **İlk dilimin geçiş şartı değil** |
| STONE-01 | Taşı elde et, taşı ve bırak | Var; bağlantı yeni yapıldı | Gevşek taş istasyonu | Kaynak, taşınan öğe ve dünyadaki aktör adetleri tutarlı |
| STONE-02 | Taş temel/duvar/zemin/sütun ör | Kısmen; yerleştirme var, test yok | Taş duvar/zemin/sütun istasyonu | Her parça yerleşir; yük ve destek kontrolü tekrar üretilebilir |
| FIRE-01 | Taştan ateş çukuru/ocak oluştur | Kısmen; FireStone kontrol edilmedi | Ateş/ocak kontrolü | Parça yerleşimi, ateş durumu ve söndürme davranışı doğrulanır |
| STORAGE-01 | Kütükleri raf/yığın üzerinde depola | Kısmen; taşıma istasyonu var | Depolama kontrolü | Depoya ekleme/çıkarma adet ve fiziksel temsili çift saymaz |
| DAMAGE-01 | Yapıya hasar ver ve onar | Kod var; oynanış testi yok | Hasar/onarım kontrolü | Sağlık değeri, hasar durumu, onarım ve destek etkisi tutarlı |
| SAVE-01 | Dünyayı kaydet, yeni dünya oturumunda yükle | Test edilmedi; en yüksek risk | Kayıt-yükleme test istasyonu | Bölüm 3'teki bütün zorunlu durumlar round-trip olur |
| DIG-SAVE-01 | Kazılmış toprağı kaydet ve geri yükle | Test edilmedi | Kazı-kalıcılık kontrolü | Density override'ları geri gelir; mesh/collision ve destek yeniden doğrulanır |

### İstasyon kuralı

Her ID, `probe_showcase` içinde bir istasyona veya tekrarlanabilir bir Play kontrolüne bağlanır. Bir istasyonun “var” olması, kabul testini geçtiği anlamına gelmez. Her koşuda sonuç **PASS / FAIL / BLOCKED** ve kısa kanıt (test çıktısı veya ekran görüntüsü + kullanılan dal/commit) olarak kaydedilir. Gerçek Unreal testi çalıştırılmadıysa sonuç **BLOCKED — UE doğrulaması yapılmadı** olmalıdır; test adı bulunması PASS sayılmaz.

## 2. Dilim için geçici sahiplik tablosu

Bu kararlar tüm oyunun mimarisini baştan tasarlama amacı taşımaz. İlk round-trip testiyle gerçek oynanış zincirinde kanıtlanacak, gerekirse düzeltilecektir.

| Alan | Mevcut aday / gözlem | Bu dilimdeki karar |
|---|---|---|
| Oyuncu envanteri | Kaynak incelemesine göre kullanılan `AForestCharacter::Inventory`; alternatif `UInventoryComponent` bağlanmamış görünüyor | Testte önce oyunun gerçekten kullandığı envanteri izleyin. İki envanteri paralel güncellemeyin. |
| Kayıt-yükleme | Faz 9 dalında `UWorldSaveGame : ULogSaveGame`; ayrıca `WorldSaveSubsystem/UWorldPersistence` ve `InventorySaveSubsystem` mevcut | Kanonik kayıt otoritesi henüz **kanıtlanmadı**. Test, oyuncunun kullandığı gerçek Save/Load giriş noktasından çalışmalı. |
| Aktif kütük oynanışı | `ACarvableLog` aktif kesim/kütük akışında; `ALogActor` eski kayıt/bridge kapsamına girmiş; `LogConstructionActor` yapı aktörü | Aktif akışın aktörü round-trip testinde özellikle bulunmalı. `ALogActor` kaydının başarılı olması `ACarvableLog` kalıcılığını kanıtlamaz. |
| Gevşek taş | `AStoneActor`; ayrıca envanter öğesi ve `HarvestableActor` içinde gevşek taş temsilleri bulunabiliyor | Dünyadaki taş, eldeki öğe ve hasat içindeki taş miktarını ayrı ölçün; aynı taşı iki kez üretmeyin. |
| Taş yapı | `AStoneMasonryActor` | Taş duvar/temel ayrı aktör olarak kaydedilip geri kuruluyor mu, bağımsız kontrol edin. “Taş aktörü kaydoldu” genellemesi yapmayın. |
| Kazılabilir arazi | `UExcavationVolumeSubsystem` sparse density snapshot adayı; chunk/world persistence ayrı kapsamlar taşıyabilir | Kazı verisi ve yeniden oluşturulan mesh/collision ayrı kontrollerdir. Hücre snapshot'ı tek başına görsel/oynanış round-trip'i kanıtlamaz. |

**Sahiplik kararını kapatma ölçütü:** Bir sistem ancak gerçek oynanış yolunda tek kaynak olarak kullanıldığı ve kayıt-yükleme testiyle doğrulandığı zaman bu dilimde “kanonik” ilan edilir. Bu belge tek başına hiçbir sınıfı silme veya devre dışı bırakma yetkisi vermez.

## 3. İlk kayıt-yükleme testinin kapsamı

### Test öncesi kurulum

1. Deneme için boş/yeni bir save slotu seçin; slot adı ve map kimliğini test çıktısına yazın.
2. Oyuncunun gerçek envanterinde sayısı bilinen kütük/taş öğeleri bulundurun.
3. Dünyaya en az bir **aktif oynanış kütüğü** (`ACarvableLog`), bir gevşek taş (`AStoneActor`) ve bir taş yapı parçası (`AStoneMasonryActor`) yerleştirin.
4. Birkaç kütükle köşe/duvar bağlantısı oluşturun. Desteği ve transformu gözlemlenebilir bir parça seçin.
5. Kazı sistemi gerçekten map'te etkinse küçük, koordinatı belirli bir toprak alanını kazın. Etkin değilse kazı testi **BLOCKED** kaydedilir; sahte bir başarılı sonuç yazılmaz.

### Kaydetmeden önce kaydedilecek ölçümler

- Oyuncu: transform, envanter öğe/adet çiftleri ve test kapsamındaki survival durumları.
- Kütükler: gerçek sınıf adı, adet, transform ve destek/bağlantı bilgisi.
- Taşlar: gevşek taş ve taş yapı aktörlerinin sınıfı, adedi, transformu ve mevcut yapısal/sağlık durumu.
- Arazi: kazılmış bölgenin koordinatları ve seçilen örnek noktalardaki density/zemin durumu.
- Kayıt: kullanılan Save giriş noktası, slot adı, şema sürümü ve dönen başarı/hata sonucu.

### Yükleme ve karşılaştırma

1. Kaydetme başarılı raporlandıktan sonra PIE oturumunu bitirin; dünyayı temiz bir oturumda yeniden açıp aynı slotu gerçek oyun Load yolu üzerinden yükleyin. Mümkünse ayrıca uygulama yeniden başlatılarak kontrol edin.
2. Önce/sonra ölçümlerini karşılaştırın. Aktör sayısı, sınıf, transform, envanter adetleri, yapı bağlantıları ve kazı durumu uyumlu olmalıdır.
3. Arazi mesh/collision yeniden kuruluyorsa yapı desteğini doğrulama tamamlanmadan otomatik olarak geçerli saymayın.
4. Aynı slotu ikinci kez yükleyip **çift aktör/çift öğe** oluşmadığını kontrol edin.
5. Aktör türü kaydedilemiyorsa test **FAIL** veya desteklenmeyen durum açıkça raporlanarak **BLOCKED** olur; bu aktörün yok sayılması test başarısı değildir.

### İlk geçiş kapıları

- [ ] Kayıt başarılı sonucu doğru ve aynı slot açılabilir.
- [ ] Aktif `ACarvableLog` round-trip olur; yalnızca eski `ALogActor` kaydı yeterli değildir.
- [ ] `AStoneActor` ve `AStoneMasonryActor` ayrı ayrı round-trip olur.
- [ ] Oyuncu envanteri/adetleri değişmez; dünyadaki ve envanterdeki öğeler çift sayılmaz.
- [ ] Yapı bağlantıları geri gelir ve destekler güvenli biçimde yeniden doğrulanır.
- [ ] Kazı snapshot'ı, mesh/collision tekrar üretimiyle birlikte doğrulanır veya kazı kapsamı açıkça BLOCKED işaretlenir.
- [ ] İkinci yüklemede veri çoğalması veya sessiz kayıp oluşmaz.
- [ ] Başarısız yükleme, yarım uygulanmış bir dünyayı başarı gibi bırakmaz.

**Kritik not:** Faz 9 teknik belgesi, mevcut legacy log payload'ının `ALogActor` kapsadığını ve bütün aktif `ACarvableLog`/ağaç/taşıma aktörlerini kapsamadığını açıkça belirtir. Bu nedenle testin ilk beklenen çıktısı PASS olmak zorunda değildir; öncelikli amaç gerçek kapsamı görünür kılmaktır.

## 3A. Kayıt kodunun statik inceleme sonucu

İnceleme dalı: `feature/phase-09-vertical-slice-stone-persistence`. Bu bölüm **kaynak kodu incelemesidir; Unreal çalıştırma testi değildir**.

- `Phase7_Integration/SaveGameSubsystem.cpp` içindeki `CaptureLogActors` / `StageLogs` yalnızca `ALogActor` örneklerini tarar ve yeniden oluşturur. `ACarvableLog` kapsamda değildir.
- Aynı dosyadaki `CaptureStoneActors` / `StageStoneActors` yalnızca `AStoneActor` sınıfını işler. `AStoneMasonryActor` kayıt/geri yükleme yoluna dahil edilmemiştir.
- `ACarvableLog` mevcut biçimiyle çalışma anında `UDynamicMesh` üzerinde kesim/yarma/oyuk işlemleri üretir. Sadece transform veya kaynak Static Mesh'i kaydetmek, oyuncunun kestiği biçimi tek başına geri getirmez. Kalıcılık tasarımı geometriyi saklamalı ya da aynı sonucu üreten doğrulanabilir bir kesim geçmişini yeniden oynatmalıdır.
- Kod `UWorldSaveGame::SaveSchemaVersion = 3` yazar ve yükleyicide 2/3 sürümlerini kabul eder; `Docs/PHASE_09_VERTICAL_SLICE.md` içindeki “version-2” açıklaması bu nedenle kodla tutarsızdır. Şema değişikliğinden önce belge/gerçek sürüm tekleştirilmeli.
- `Scripts/run_phase9_tests.sh` 31 otomasyon testi adını ve log sonuçlarını kontrol eder. Bu listede voxel snapshot ve taş kayıt doğrulama testleri var; ancak `ACarvableLog` veya `AStoneMasonryActor` için gerçek dünya aktörü round-trip testi yok. Test adının bulunması, çalıştırmanın geçtiğini veya PIE davranışının doğrulandığını kanıtlamaz.

**Sonuç:** Mevcut koddan yalnızca “SaveWorld çağrısı başarılı” sonucu almak dilimi kapatmaz. Aktif kütük ve taş yapı sınıfı kapsam dışı kaldığı için ilk kabul denemesi şu an **PASS kabul edilemez**; bu aktörler için kalıcılık adapter'ı ve round-trip doğrulaması eklenmelidir.

## 4. İlk çalışma sırası

1. Bu listedeki her istasyonun gerçekten mevcut olduğunu ve doğru aktörü sınadığını kontrol edin.
2. Kayıt-yükleme testini gerçek oyuncu Save/Load giriş noktasından çalıştırıp **ilk başarısız round-trip durumunu** belirleyin.
3. Önce yalnızca o kırık yolu düzeltin; envanter, kayıt veya log sistemlerini topluca yeniden yazmayın.
4. Round-trip kapıları geçtikten sonra taş duvar/zemin/sütun testine, sonra otomatik boy kesmeye geçin.
5. 20–30 dakikalık oyun akışını ancak test istasyonları PASS olduktan sonra tamamlanmış sayın: kes → işle → taşı → taş temel + kütük kulübe kur → kaydet → oturumu yenile → yükle → yapıyı kontrol et.

