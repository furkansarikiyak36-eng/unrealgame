# Sons of the Forest esinli mekanik genişletme planı

**Dal:** feature/sotf-mechanics-expansion  
**Taban:** test/chatgpt-world-snap  
**Motor hedefi:** Unreal Engine 5.8 / C++  
**Durum:** kapsam planı ve ilk iki uçlu yerleşim dilimi; UBT/PIE doğrulaması bekliyor.

## 1. Referans ilkesi ve kaynaklar

Sons of the Forest burada davranış referansıdır; oyun içi kodunun birebir kopyalandığı varsayılmaz. Esas alınan davranış kümeleri: doğrudan/serbest montaj, farklı kütük boyları, kesikli kesim ve yerleşim göstergeleri, yarma ve kalas, boş/uygun snap arama, iki uçlu eğimli kütükler, kiriş-payanda, döşeme/çatı/kapı/pencere, merdiven/rampa, savunma duvarı ve gate, tuzaklar, halat köprüsü/zipline, depolama ve yardımcı yapılar.

Kaynaklar:
- Kütükler, kesim ve malzeme çeşitleri: https://sonsoftheforest.wiki.gg/wiki/Logs
- Guide Book: blueprint ve tek tek montaj farkı: https://sonsoftheforest.wiki.gg/wiki/Guide_Book
- Serbest inşa, kütük yarma, duvar, kapı/pencere, payanda ve merdiven/rampa: https://sonsoftheforest.fandom.com/wiki/Custom_Building
- İnşa yapıları, halat köprü, tuzak ve yardımcılar: https://eip.gg/sotf/guides/construction/
- Depolama, planter, rain catcher ve tuzak tarifleri: https://sonsoftheforest.wiki.gg/wiki/Prefab_Buildings

## 2. Depodaki mevcut temel

Kaynak kodda zaten bulunan ana parçalar:
- ACarvableLog: dinamik kesim/yarma, oyma araçları, modül çizgileri, parça üretme, bağlantı yüzeyi işleme.
- ForestCharacter: eldeki kütüğü taşıma/yerleştirme, yüz/uc snap kararları, kesim göstergesi, kesikli yerleşim önizlemesi, yatay/dikey/kiriş/payanda/döşeme/köşe/eğimli yerleşim yolları.
- LogSnapSystem ve WorldSnap: kütük yüz/uç adayları ve genel aday sıralama.
- StructureLink/WoodStructure: ahşap bağlantı ve destek değerlendirmesi, yapı türü tanıma, destek kaybından sonra yeniden değerlendirme.
- StonePlacementComponent/StoneConnectionGraph: taş yüz/yuva yerleşimi ve taş bağlantıları; henüz ahşap destek otoritesinde birleşmiyor.
- Campfire, FireStone ve stick/trap/storage/utility sınıfları: bir kısmı var; çoğunun oyuncu akışına bağlanma ve gerçek oyun testi kapsamı ayrı.
- SaveGameSubsystem, WorldSaveSubsystem ve WorldPersistenceSubsystem: birden fazla kayıt yolu var; tüm aktif parçaların/bağlantıların tek bir doğrulanmış sözleşmeyle kaydedildiği kanıtlanmış değil.

## 3. Bu daldaki ilk gerçek kod kapsamı

1. Yeni WorldSnapTwoAnchorSolver: iki uçlu sabit uzunluklu parça için konum, yön, pitch, anchor mesafesi, anchor kullanılabilirliği ve yük taşıma durumunu doğrular.
2. ForestCharacter eğimli kütük yolunda zemin ucu + direk tepesi iki-anchor solver'ını kullanır. Açı kameradan değil uç geometrisinden türetilir.
3. StructureLink içindeki LengthModules üst sınırı 6'dan 8'e çıkarılır; böylece canonical 600 cm kütük 75 cm modül sisteminde sekiz modülle temsil edilebilir.
4. Kütük başına görsel alev sayısındaki üst sınır 8 modüle çıkarılır.
5. 390 cm = 6 modül diyen eski açıklamalar düzeltilir. Canonical 300 cm = 4 modül, 600 cm = 8 modüldür; 390 cm legacy ölçüdür.
6. İki-anchor otomasyon testleri ve headless test koşucusu bağlantısı eklenir.

Bu ilk dilim bütün Sons of the Forest mekaniklerini uyguladığı anlamına gelmez. Kalan davranışlar aşağıdaki aşamalı kapsamdır.

## 4. Her mekanik aynı ortak zinciri kullanmalı

1. **World scan:** raycast hedefi alır; sınırlı yerel overlap/shape query yakın zemin, kütük ucu/yüzü, direk tepesi, kiriş ucu, kalas, taş yüzü ve boş yuvaları aday olarak getirir.
2. **Sınıflandırma:** ham hit bilgisi semantik hedef türünden ayrılır. Tek trace, tüm yüzeyin devamlı/sağlam/kazılabilir olduğunu kanıtlamaz.
3. **Snap sağlayıcıları:** kütük, direk, kiriş, kalas, zemin ve taş aynı generic point/anchor sözleşmesini uygular.
4. **Uyumluluk:** yalnız geometrik olarak en yakın nokta seçilmez; hedef türü, boşluk, yüz normali, eksen, erişilebilirlik, collision hacmi ve destek durumu kontrol edilir.
5. **Yerleşim:** tek yüz/tek uç veya iki uç solver'ı final transform'u hesaplar. Açı gerekiyorsa iki endpoint geometrisinden çıkarılır.
6. **Preview = commit:** aynı karar/transform fonksiyonu kullanılır; commit anında hedef revision, yuva doluluğu, collision ve destek yeniden doğrulanır.
7. **Atomik işlem:** spawn, malzeme tüketimi ve bağlantı kaydı tek işlem sayılır; başarısızlık yarım yapı ya da kayıp kaynak bırakmaz.
8. **Destek ve kalıcılık:** parça/anchor ID'leri destek grafiğine girer; destek kaybında ilgili alt grafik değerlendirilir; yükleme sonrası yüzey/collision güncel olduğunda bağlantılar yeniden doğrulanır.

## 5. Mekanik ailesi ve hedef davranışlar

### A. Kütük kesme ve dönüşüm
- 75 cm modül standardı; 75/150/300/600 cm temel boy ailesi.
- Kesikli kesim işareti ile gerçek kesim düzlemi aynı hesaplamadan üretilir.
- Enine kesim, boyuna yarma, çeyrekleme, kalas çıkarma, sivri uç, pah ve bağlantı çentiği ortak dönüşüm sözleşmesi kullanır.
- Her çıkan parça bağımsız ID alır; hacim/malzeme kaydı ve spawn tek transaction olur.
- Uygun boy otomatik çıkarılacaksa kesilecek parça sayısı, artan parçalar ve malzeme tüketimi başarılı commit sonrası güncellenir.

Kabul: CUT-01 preview düzlemi = gerçek kesim; CUT-02 600→300+300 ve 300→150+150; CUT-03 yarma/kalas parçalarının bağımsızlığı; CUT-04 hacim ve rollback; GRID-01 300 cm=4 modül ve 600 cm=8 modül.

### B. Snap ve tarama
- Raycast oyuncunun baktığı yüzü seçer; yakın bölgedeki diğer uygun slotlar sınırlı sorguyla aday olabilir.
- Kütük Top/Bottom/Front/Back/EndA/EndB; direk, kiriş, kalas, taş, zemin ve boş slot sağlayıcıları.
- Dolu/yanlış yüz/çakışan hacim/eski revision geçerli aday sayılmaz.
- Kütükler 75 cm grid kullanır. Kalas aralığı configurable lattice ile yönetilir; ilk kabul hedefi 20 cm'dir ama gerçek mesh genişliği/collision testinden geçmeden sabit varsayılmamalıdır.

Kabul: SNAP-01 deterministik sıra; SNAP-02 dolu/yanlış slot reddi; SNAP-03 en yakın geçerli aday seçimi; SNAP-04 preview/commit parity; SCAN-01 sınırlı yerel aday bütçesi.

### C. İki uçlu yerleşim
- Her uç ayrı anchor ID ve türüne sahiptir: zemin + direk tepesi, kiriş ucu + direk, kütük üstü + zemin, platform kenarı + kiriş.
- İki uç da kullanılabilir/yük taşıyabilir olmalı; aynı socket iki kez kullanılamaz.
- Parça boyu ile anchor mesafesi tolerans dışında ise ölçeklemek yerine reddet.
- Solver local X bounds'i dikkate alır; uçların yön vektörü pitch ve transform'u belirler.
- Bu daldaki ilk bağlantı zemin ucu + ankrajlı direk tepesi senaryosudur.

Kabul: END-01 iki yatay uç; END-02 eğimli 600 cm kütük; END-03 uzunluk/mesafe uyuşmazlığı reddi; END-04 desteksiz anchor reddi; END-05 preview ve commit aynı transform.

### D. Ahşap montaj
- Yatay duvar, dikey palisat, temel, platform, döşeme, kiriş, payanda, çatı, mahya, kapı/pencere, merdiven, rampa ve korkuluk aynı yerleşim zincirini kullanır.
- Döşeme kalasları düzenli aralıkla yerleşir ve çakışma kontrolü yapar.
- Açıklık çıkarıldığında kalan parça/support bağlantıları tutarlı kalır.
- Çatı mahya kirişi tek bir yüzün en yakın snap'ine düşmez; iki uç kısıtı kullanır.
- Payanda alternatif yük yolu oluşturur; ara direk ancak destek grafiği tekrar hesaplandıktan sonra kaldırılabilir.
- Merdiven: yan taşıyıcı + aralıklı basamak. Rampa: devamlı yürünebilir yüzey. Yalnızca yapı adına bakarak traversal işlevi atama.

Kabul: ASM-06 çapraz köşe; ASM-10 mahya; FLOOR-01 kalas lattice; STAIR-01 karakter traversal; RAMP-01 continuous surface; SUP-01 payanda sonrası destek grafiği.

### E. Savunma ve tuzaklar
- Kütük üstü sivriltme, palisat aralık/continuity, kapı/gate, çubuk kazığı, taşla takviye, dışa bakan spike.
- Tuzak lifecycle: armed → triggered → cooldown/reset veya consumed.
- İlk aile: küçük hayvan/fish trap, stick spike, spring/fly-swatter tipi, alarm ve ip/ateşli tuzak. Ortak tetik altyapısı, ayrı malzeme/etki profilleri.
- Çarpışma, gate açıklığı, karakter geçişi, düşman hasarı ve reset koşulları ayrı kabul edilir.

Kabul: DEF-01 palisat devamlılığı; DEF-02 spike yüz/yön; DEF-03 gate açıldıktan sonra kalan duvar desteği; TRAP-01 tek tetik tek olay; TRAP-02 deterministik reset.

### F. Taş ve karma malzeme
- Taş duvar/kolon/kiriş/döşeme rolleri ayrılır.
- StoneConnectionGraph ile StructureLink arasında tek authoritative support değerlendirmesi kurulur; çift sahipli bağlantı olmaz.
- Taş temel üstü kütük, taş kolonun tuttuğu kiriş ve taşın kaldırılmasından doğan destek kaybı test edilir.
- Kırılma/taşıma/kurma durumları açık lifecycle state olarak temsil edilir.

Kabul: STONE-01 taş üstü ahşap destek; STONE-02 kolon/kiriş rolü; STONE-03 taş kaldırınca cascade; STONE-04 karma yapı save/load revalidation.

### G. Halat, taşıma ve depolama
- Halat köprüsü iki ankraj, halat/taşıyıcı ve yürüme yüzeyi bağlantıları olan yapı tipidir.
- Zipline, kızak ve el arabası log/taş/çubuk sahipliğini atomik olarak değiştirir.
- Depolama kapasitesi fiziksel parça ID'leriyle çalışır; aynı parçanın araçta, yerde ve rafta çift sayılması önlenir.

Kabul: LOGISTICS-01 iki uçlu halat; LOGISTICS-02 inventory↔physical transfer çift sayım yapmaz; STORAGE-01 kapasite aşımı rollback olur.

### H. Yapı tanıma ve yardımcı işlevler
- Duvar/platform/çatı/köprü/merdiven/rampa/iskele geometri + bağlantı grafiğinden tanınır.
- İşlev isimden ayrı doğrulanır: merdiven yürünebilir, şömine gereken taş/ateş/boşluk koşulunu sağlar.
- Mevcut kurutma rafı, yağmur suyu, planter, depo ve tezgâh sınıfları ortak yerleşim/support/save yoluna bağlanır.
- Blueprint/prefab sistemi varsa el montajından ayrı kalır ama ortak materyal sahipliği ve kalıcılık sözleşmesini kullanır.

Kabul: FUNC-01 elle şömine; FUNC-02 parça sökülünce işlev yeniden hesaplanır; FUNC-03 elle kurulum/tarif eşdeğerliği; FUNC-04 ad tek başına işlev kanıtı değildir.

### I. Hasar, kalıcılık ve performans
- Stable piece ID, tanım, transform, ölçü, malzeme, hasar/yanma, bağlantı/anchor ID, lifecycle state kaydedilir.
- Restore önce şema/referans doğrular, sonra staged restore yapar. Terrain/collision güncel olmadan support aktif sayılmaz.
- Hasar/yangın/destek değişimi graph revision'ını artırır; collapse olayı iki defa yürütülmez.
- Global actor scan ve yüzlerce trace her karede kabul edilmez; lokal tarama/cache/olay bazlı destek değerlendirmesi ölçülür.

Kabul: SAVE-01 restart sonrası parça kimliği/ölçüsü; SAVE-02 invalid save canlı dünyayı kısmen silmez; PERF-01 10/100/1000 parçada ölçüm; PERF-02 Unreal Insights olmadan optimizasyon iddiası yok.

## 6. Faz sırası

| Faz | Kapsam | Çıkış kapısı |
|---|---|---|
| 0 | Dal baz çizgisi, 75 cm/canonical mesh uyumu, eski ölçülerin envanteri | Ölçü/asset sözleşmesi açık |
| 1 | WorldScan sağlayıcıları ve lokal tarama bütçesi | Ham gözlem ve semantik hedef ayrılır |
| 2 | Snap filtresi, çakışma, boş slot, revision, deterministic tie-break | Geçersiz aday onaylanmaz |
| 3 | Kesim çizgisi, kesim ızgarası, hacim/ID transaction | CUT ve GRID testleri |
| 4 | Tek uç/iki uç solver, zemin+direk ilk bağlantı | END testleri ve preview/commit parity |
| 5 | Döşeme lattice, basamak, rampa, çatı/mahya | Floor/stair/ramp/roof testleri |
| 6 | Support graph, strut, hasar/yangın ve collapse | Destek regresyon testleri |
| 7 | Taş-ahşap ortak support otoritesi | Karma malzeme testleri |
| 8 | Palisat/gate, tuzaklar, halat köprüsü, zipline, depo ve yardımcılar | Defense/trap/logistics/storage testleri |
| 9 | Save/load, revision, atomic restore ve profiling | UBT/Automation/PIE/Insights kapıları |

## 7. Test kapısı ve sınırlar

Yeni test adları:
- WorldSnap.TwoAnchor.LevelPlacement
- WorldSnap.TwoAnchor.SlopePitch
- WorldSnap.TwoAnchor.LengthMismatch
- WorldSnap.TwoAnchor.NonLoadBearing
- WorldSnap.TwoAnchor.SameAnchor
- WorldSnap.TwoAnchor.PitchRange

Bunlara mevcut WorldSnap resolver testleri eklenir; toplam beklenen otomasyon sayısı 9'dur. Test kaynağının mevcut olması çalıştırıldığı anlamına gelmez. Bu dalın kodu henüz Unreal Engine 5.8 UBT/Automation/PIE ortamında çalıştırılmadı; PASS olarak raporlanmamalıdır.

## 8. Merge koşulları

- UE 5.8 Editor target derlemesi başarılı.
- WorldSnap otomasyonları 9/9 geçer.
- Gerçek meshlerle zemin + ankrajlı direk eğimli yerleşimi PIE'de doğrulanır.
- Preview ve commit aynı transform'u üretir.
- 600 cm kütük yapı grafiğinde 8 modül olarak doğrulanır.
- Kaynak test logları, commit SHA ve UE sürümüyle kaydedilir.

Bu dal ana dala birleştirilmiş değildir; diğer mekanikler plan kapsamındadır, bu commit yalnızca ilk uygulama dilimini kurar.
