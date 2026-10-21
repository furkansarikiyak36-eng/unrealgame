# Faz 0–12 — Mekanik Yetenek Haritası: Oyuncu ne yapabilir?

**Dal:** `plan/phase-00-baseline`  
**Katalog:** `Docs/MEKANIK_KATALOG_TAKIP_CSV.csv` — 485 mekanik kaydı  
**Sınır:** Bu doküman C++/veri sözleşmeleri kaynak incelemesine dayanır. Bu oturumda Unreal Engine 5.8 derlemesi, Editor veya oyun içi test çalıştırılmadı. “Kaynak yolu mevcut” ile “oyuncu akışında tamamlandı ve kanıtlandı” aynı şey değildir.

## Okuma kılavuzu

Katalogdaki U seviyeleri özellik tamamlama yüzdesi değildir; kaynakta ne kadar kanıt görüldüğünü anlatır.

- **U2 — Yerel davranış izi:** Fonksiyon/algoritma/işlem yolu kodda görülüyor. Tüm oyuncu akışı, entegrasyon ve motor testi otomatik olarak kanıtlanmış sayılmaz.
- **U1 — Model/veri izi:** Veri tipi, asset, arayüz veya alt sistem var. Çalışır oyun davranışının bütünü kanıtlanmış sayılmaz.
- **U0 — Gösterilmemiş:** Bu incelemede uçtan uca davranış yolu gösterilememiş; tamamlandı diye sunulmamalı.

Kanonik katalogdaki mevcut sayım: **485 toplam = 333 U2, 88 U1, 64 U0.** Her satırın isimli davranış tanımı, kod kanıtı, test durumu ve bilinen sınırı [485 satırlık mekanik kataloğunda](./MEKANIK_KATALOG_TAKIP_CSV.csv) bulunur.

## Oyunun mevcut mekanik döngüsü

Oyunun amaçlanan ana akışı şöyle kuruluyor:

1. **Doğadan kaynak çıkar:** ağaç kes, gövdeyi böl, taş ve çubuk topla; malzemenin fiziksel dünya durumunu koru.
2. **Kaynağı taşı ve hazırla:** kütüğü el/omuz ile taşı, sürükle veya araçla götür; stack'lenebilir parçaları envanter/depolara aktar; kes, yar, oy, şekillendir ve tarifle işle.
3. **Yapı kur:** kütükleri, taşları veya çubukları yerleştir; bağlantı/eklem kur; destek yükünü ve düşen parçaları yönet; yapı parçası ekle, hasar ver, onar veya sök.
4. **Dünyayı yönet:** su, toprak, nem, besin, biyokütle, yangın, çürüme ve hava durumunun dünya hücrelerine etkisini simüle et; ürün yetiştir, hayvanı besle, ürün ve gübre döngüsü kur.
5. **Altyapı ve üretim kur:** bölge kaynak/risk verisini sorgula; rota/bağlantı ekle; istasyonda girdi rezerve et, kuyrukta üret, kapasite/güç/işçi beklemesini ele al.
6. **Tehditle karşılaş:** baskın olayını başlat, dalga spawn bütçesini işle, düşman navigasyonunu çalıştır; savunma parçasının ve köprünün hasarının dünyaya etkisini uygula.
7. **Kaydı geri yükle:** ortak SaveGame zarfındaki alan snapshot'larını yükle. Tüm fiziksel aktörlerin ve bütün gameplay state'lerinin aynı kayda girip geri oluşturulması henüz kanıtlanmış değildir.

Bu akışın çekirdek bölümleri kaynakta mevcut olsa da tüm döngünün tek bir gerçek UE oyun oturumunda baştan sona çalıştığı henüz doğrulanmış değildir.

## Sistem bazında: kabiliyetler, girdiler ve sınırlar

### 1. LOG — Ağaç, fiziksel kütük ve kütükle yapı kurma (76 mekanik)

**Oyuncunun yapması hedeflenen/yerel kod yolu görülen işler**
- Baltayla ağaca vurup düşürmek; ağaç boyuna göre gövde üretmek ve gövdeyi parçalara bölmek.
- Kütüğü el/omuzda kaldırmak, bırakmak, itmek, yuvarlamak veya kısa mesafede sürüklemek.
- Kütüğü boyuna/enine kesmek; uç şekillendirme, sivri uç, pah, çentik ve kırlangıç gibi işleme adımları.
- Parçaları direk, kiriş, duvar, köşe, iskele, destek, çit veya savunma parçaları olarak yerleştirmek.
- Destekleri değerlendirmek; bazı destek kaldırma senaryolarında kirişin düşmesini/kırılmasını; hasar, ateş ve onarım davranışlarını çalıştırmak.
- El arabası/kızak benzeri taşıma sınıflarında yük kapasitesi, yükleme/indirme ve hareket akışı.

**Şu anki sınır**
- Ana oynanıştaki `ACarvableLog`, envanter köprülerindeki `ALogActor` ile aynı sınıf/state sahibi değildir. Fiziksel kütüğü genel bir stack item'a çevirince transform, hasar, kesim/şekil ve bağlantı state'i korunmuş sayılmaz.
- Zipline/uzun hat, sal/su üstü taşıma ve genel kurtarma gibi bazı katalog yetenekleri tam akış olarak gösterilmemiş.
- Depodaki `Docs/TEST_DURUMU.md` 101 MechanicsDemo kontrolü ve 12 Wood testi kaydı içeriyor; bu testler bu turda tekrar çalıştırılmadı ve tüm LOG-001–LOG-076 için ayrı kabul kanıtı sayılmaz.

**Örnek katalog:** `LOG-001` ağaç kesme; `LOG-004–LOG-012` kaldırma/taşıma/bırakma; `LOG-013` taşıma aracı; `LOG-014/015` uzun hat ve su üstü taşıma (uçtan uca kanıtlanmamış); `LOG-059` yapısal çöküş; `LOG-064` genel malzeme geri kazanımı (uçtan uca gösterilmemiş).

### 2. STONE — Taş çıkarma, işleme, yerleştirme ve taş yapı (96 mekanik)

**Kodda bulunan mekanik aileleri**
- Kaya yatağı ve taş aktörleri; madencilik/çıkarma, tanım/şekil saklama ve fiziksel taş davranışı.
- Taşı elde taşıma ve kızakla taşıma için bileşenler.
- Taş yerleştirme önizlemesi, yüz/socket seçimi, çarpışma boşluğu ve yapısal bağlantı kurma/işgal edilmiş socket'i reddetme akışları.
- Yapı taşı üretme, taş duvar/kolon/kiriş ve destek ilişkileri, taş hasarı/kırılma ve bazı çöküş bileşenleri.
- Taş tuzakları/engeller, şömine/yemek pişirme, depolama ve bazı fonksiyonel öğeler (ör. elektrik conduit modülü) için ayrı modüller.

**Şu anki sınır**
- Taş oyuncu akışının tamamı (topla → elde taşı → yerleştir/fırlat → darbe → tekrar topla → kayıt/yükle) bu turda uçtan uca kanıtlanmadı.
- `CrushBoulder` kaynakta istenen bütün parçaları spawn etmeden taşı yok etmeyecek şekilde rollback temizliği içeriyor; fakat gerçek UE spawn-failure senaryosu çalıştırılmadı.
- `StoneWorkingComponent` içindeki `HollowGroove`, `EngraveJoint` ve `SmoothSurface` gerçek geometri işlemi yapmadığı için `false` döndürüyor. Bu, başarılı görünen no-op'tan daha doğru bir ret davranışıdır; fakat özellikler hâlâ yapılmış değildir.
- Taş yapı ile `ACarvableLog` ana kütük yapı grafının tam ortak yük/çöküş/SaveGame sahipliği kanıtlanmış değildir.

### 3. STICK — Çubuk, işleme, üretim ve hafif savunma (76 mekanik)

**Kodda bulunan mekanik aileleri**
- Çubuk toplama, envanter/stack yaklaşımı, kırma ve farklı şekillere işleme.
- Tarif üzerinden çubuk üretimi; birden çok girdiyi toplu uygulayan `ApplyCraftTransaction` işlem yolu.
- Kazık, kazık duvarı, hasır çit, güçlendirme, mızrak/yay/ok gibi doğaçlama ekipmanlarla ilişkili bileşenler.
- Meşale, tuzak mekanizmaları/alarm teli, mobilya, raf ve depolama gibi çevresel modüller.
- Çubuk dayanıklılığı/nem/esneme/yıkım ve geri kazanım bileşenleri.

**Şu anki sınır**
- Şekle bağlı tarifler `RequiredShape` belirtiyorsa generic inventory yalnızca tip/adet tutuyorsa işlem fail-closed reddediliyor; shape-aware gerçek envanter bağlantısı kanıtlanmamış.
- Parçalama kodunda başarısız spawn'da parçaları temizleyip kaynak çubuğu koruma yolu mevcut; UE hata enjeksiyonu henüz çalıştırılmadı.
- Sökümün tahmin ettiği malzeme miktarının doğru depoya/enventer sahibine tam bir kez işlendiği her akış için kanıtlanmış değil.
- `WaterCollector::Tick` hâlâ gerçek hava/yağış kaynağını okumuyor; yağmur toplayıcı için su üretimi tamamlanmış özellik sayılamaz.

### 4. INVENTORY — Toplama, stack, taşıma sınırı, transfer, drop ve depolama (32 mekanik)

**Oyuncunun kodla ilişkilendirilmiş yapabildiği işler**
- Yerdeki pickup ile etkileşme; stack'lenebilir öğeleri karakterin `CoreInventory` bileşenine ekleme/çıkarma.
- Item ID/definition üzerinden stack, slot kapasitesi ve genel inventory bileşeninde ağırlık sınırı kontrolü.
- Envanteri kategoriye göre filtreleme, hızlı erişim slotları ve crafting UI için bileşenler.
- Kaynak/hedef envanterlerini ve çoklu girdi/çıktı partilerini tek işlem gibi güncellemeyi amaçlayan `TransferItem` / `ExchangeItemBatches` rollback sözleşmeleri.
- Dünyaya stack item bırakmak için aktör spawn etme ve ancak başarılı spawn'dan sonra envanterden düşme mantığı; depolama aktörü ve storage snapshot'ları.

**Şu anki sınır**
- Kütük/taş/çubuk fiziksel aktörleri ile stack item arasındaki dönüşüm genel ve kayıpsız bir factory/state-payload sözleşmesi değil.
- Generic `UInventoryComponent::DropItem` tek başına aktör spawn etmez; bunun için oyuncu/drop component veya açık physical bridge çağrılmalıdır.
- Inventory UI, quick access, fiziksel drop ve SaveGame'in tüm gerçek oyuncu akışında aynı inventory sahibiyle çalışması UE 5.8 oturumunda doğrulanmadı.
- Kayıt kapsamındaki inventory'ler ile dünyadaki tüm fiziksel kaynaklar eşdeğer değildir.

### 5. EXCAVATION + UNDERGROUND — 3B kazı ve yeraltı tehlikeleri (Faz 1–4)

Bu işler eski 485 mekanik ailelerinin adlarından ibaret değildir; Faz 1–4 uygulama katmanlarında eklenen mimari kabiliyetlerdir.

**Kodda bulunan kabiliyetler**
- Sparse 3B kazı/hacim delta modeli; sphere/capsule/box edit girişleri ve komut kimliğiyle tekrarlı çağrıyı engelleme.
- Runtime chunk mesh üretimi, mesh/collision bağlama, etkilenen chunk'ları güncelleme ve snapshot sonrası geometri yenileme.
- Yeraltı desteği, boşluk/tavan hücresi takibi ve bütçeli çökme; yüzey world-state su/nem verisinden sınırlı mağara su ceplerine seepage akışı.
- Su cebi ve yeraltı destek snapshot'ları; ortak SaveGame zarfında kazı/tehlike kayıt alanları.
- Tuzak actor'ının mevcut taş tuzağı/hasar akışından yararlanması.

**Şu anki sınır**
- Landscape'in kendi yüzeyiyle runtime voxel mesh hand-off'i—oyukta eski yüzeyin görünürlük/çarpışma yetkisinin gerçekten kalkması—henüz kanıtlanmış değil.
- Tünel/oda/kuyu + chunk sınırı + çarpışma + su + Save/Load tek UE 5.8 dünyasında bu oturumda çalıştırılmadı.
- 3B kazı geometrisi ile 2B dünya hücre hidrojisinin her kenar senaryosunda tek su/kazı sahibi olarak korunduğu henüz kabul edilmedi.

### 6. WORLD + MATERIAL — Dünya hücresi, çevre ve madde tepkileri (35 + 38 mekanik)

**World State tarafındaki kabiliyetler**
- Chunk/cell tabanlı yükseklik/deformasyon, yüzey suyu, toprak nemi ve N/P/K, kimya/pH/toksin/radyoaktivite, biyokütle ve hücre yangını state alanları.
- Su → toprak → kimya → çapraz nem/toksin → biyokütle yangını → çürüme sıralı world-state pipeline'ı.
- Ortak hava, dünya saati ve seed; günlük deterministik rüzgâr yönü; sınırlı zaman catch-up; doğrulanan chunk/cell snapshot import/export.
- Region survey'in çevresel kaynak/risk özetini bu kanonik hücrelerden sorgulaması.

**Material tarafındaki kabiliyetler**
- Ahşap/taş/toprak vb. malzemeler için definition/property profilleri.
- Malzeme etkileşimi ve chunk biyokütlesi tarafında zaman/nem/yangın/çürüme bağlantıları.
- Kütük hasarı ve yangın sistemine adaptör yolları.

**Şu anki sınır**
- Bütün malzeme türlerinin fiziksel aktörleri, 2B hücre biyokütlesi ve yapı grafı üzerinden aynı tek malzeme state'ini kullandığı doğrulanmış değil.
- Bilinmeyen cell MaterialID, `MaterialChunkIntegration` içinde Wood profiline sessizce düşebiliyor; yanlış kimlik doğru bir malzeme olarak işlem görebilir.
- Yağış/su toplayıcı, 2B su hücresi ve 3B yeraltı su cepleri arasında tüm kaynak/kayıp dengesi uçtan uca ölçülmedi.
- Runtime world payload'ı fiziksel aktör persistence'ının yerine geçmez.

### 7. FARMING — Bitki, tarla, hayvan, ürün ve gübre (57 mekanik)

**Kodda bulunan mekanik aileleri**
- Saat/gün adımlı simülasyon ve deterministic seed/RNG.
- Bitki/tarla tanımları, tohum/ekim, büyüme aşamaları, su ve besin stres hesapları, hasat ve kalite/yield bileşenleri.
- Hayvan actor'ları; günlük büyüme/beslenme/enerji/sağlık akışları; aynı aktör üzerindeki `UFeedStorageComponent` ile yem tüketimi.
- Süt/yumurta/yün üretim depoları, üreme/gebelik/doğum bileşenleri.
- Gübre oluşturma/toplama ve world-state toprak besinlerine uygulama için adaptör yolları.
- `UFeedCycleComponent` artık aynı sahibin `UFeedStorageComponent` deposuna yönlenen adaptör; bağımsız stok sahibi olarak ele alınmamalı.

**Şu anki sınır**
- Bitki, hayvan, yem ve dünya hücreleri aynı stok/state denklemi ve tek kalıcı aktör kaydı altında çalışıyor diye varsayılmamalı. Günlük kaynak bilançosu ve Save/Load round-trip gerekiyor.
- Irrigation/sulama için gerçek kaynak-miktar-aralık bağlantısı katalogda gösterilmemiş.
- Ürünlerin inventory'e aktarımı, kapasite doluluğu ve hayvan/tarla aktörlerinin yeniden yüklenmesi gerçek UE oyun akışında doğrulanmadı.

### 8. REGION + LOGISTICS — Bölge kaynak/risk sorgusu, rotalar ve altyapı (Faz 10)

Bu fazda eklenen ana kabiliyetler:
- `WorldToChunk` üzerinden bölge koordinatı üretmek ve `UWorldStateSubsystem/UChunkStateStore` hücrelerinden bölgesel kaynak/risk survey'i hesaplamak.
- Trail/Road/Bridge/Ramp/SledRoute/Ropeway gibi link tipleri taşıyan düğüm/bağlantı grafı.
- Mesafe/tür hızı/bağlantı kondisyonu/risk/ortam riski/yük kapasitesi üzerinden traversal cost; geçersiz veya kapasite dışı rotaları reddetme.
- Köprü ve zipline actor'larının tanımlı sabit anahtarla route graph'a kayıt olması.
- Köprü log parçası yok edilince ona bağlı lojistik bağlantının kapatılması için adapter.
- Lojistik snapshot kaydı ve doğrulamayla geri yükleme.

**Şu anki sınır**
- Köprü spawn'ı kısmen başarısız olduğunda önceki parçalar temizlenmiyor; all-or-nothing rollback gerekli.
- Köprü/zipline rotaları için otomatik dünya çapı tarama yok. Actor/Blueprint'in tanımlı anahtarla kayıt yolunu çağırması gerekli.
- Route graph ile gerçek fiziksel geçiş/yükleme/araç hareketi tüm link tiplerinde tek bir E2E zincirinde kanıtlanmadı.
- Route graph rota metadata'sını kaydeder; bütün fiziksel köprü/yol aktörlerini otomatik kaydeden evrensel actor snapshot değildir.

### 9. PRODUCTION — Tarif, FIFO kuyruk, güç/işçi kapısı ve çıktı (Faz 11)

**Kodda bulunan işler**
- Recipe asset doğrulama: girdi/çıktı, süre, power ve worker gereksinimlerini kontrol etme.
- Bir üretim istasyonunda FIFO job kuyruğu, progress, pause/resume/cancel, job geçmişi ve sınırlı kuyruk/batch.
- Girdileri `ExchangeItemBatches` ile kuyruk başlarken ayırma; iptalde aynı stok sahibine iade.
- Güç veya işçi yetersizse job'u bekletme; çıktı deposu doluysa `BlockedOutput` durumunda ürünü job snapshot'ında tutma.
- İstasyona ait kuyruk snapshot'ını ortak `UWorldCoreSaveGame` zarfında saklama.

**Şu anki sınır**
- Repo içinde ortak generator/power-grid sahibi ve canlı enerji üretim-dengeleme bağlantısı kanıtlanmadı. `AvailablePowerWatts` dış girdidir.
- Gerçek fiziksel kütük kesiminin otomatik biçimde recipe inventory'deki `HalfLog` girdisine dönüştüğü bir bağlantı kabul edilmedi.
- Gerçek makine akışı, dolu output, iptal/refund ve Save/Load turu UE 5.8'de çalıştırılmadı.

### 10. RAID — Baskın, dalga, spawn, düşman yolu ve savunma hasarı (63 katalog mekanik)

**Kodda bulunan işler**
- Event ID/aktif olay kilidi; başlatma öncesi plan, stage, enemy group/member/class doğrulaması.
- Horde planından dalga sırası; event genelinde tüketilen spawn bütçesi, dalga kapasitesi/population counter ve sınırlı spawn/query bütçeleri.
- Opsiyonel otomatik scheduler ve lifecycle (Preparing/Active/Intermission/Completed/Cancelled) geri bildirimleri.
- Director tarafından üretilen aktörleri event bazında takip etme; iptalde yalnızca kendi event aktörlerini temizleme.
- Navmesh varsa bütçeli path query/cache; navmesh yoksa greybox collision-swept hareket yolu.
- Yapı hasarı resolver'ı, yapı bounds değişince nav dirty-area güncelleme ve köprü log parçası yok olduğunda ilgili route'u devre dışı bırakma.

**Şu anki sınır**
- Otomatik scheduler varsayılan olarak kapalıdır; etkinleşmesi için açık API çağrısı/konfigürasyon gerekir.
- `CaptureContext` her oyuncu için `Level=1`, dünya çarpanı için `1.0` kullanıyor; gerçek progression, survival süreleri ve dünya saati zorluk hesabına bağlanmış değil.
- Hasar resolver'ı `ALogActor/AStoneActor/AStickActor` türlerine dallanıyor; ana `ACarvableLog` yapı akışı için ortak hasar adapter'ı yok.
- `USpawnDecisionPipeline` ayrı kod olarak duruyor; aktif Director'ın incelediğimiz `ProcessSpawns` yolu doğrudan spawn ediyor. Tek gerçek spawn yolu ve kullanılmayan paralel mimarinin amacı açıkça belgelenmeli.
- Raid feedback ve structure/navigation etkileri için gerçek oyun içi başlangıçtan iptal/bitirişe E2E kanıtı yok.

### 11. CORE — Birleşik state ve sözleşme parçaları (12 mekanik)

**Kodda bulunan yapı taşları**
- Malzeme türü, yapı durumu ve connection-type enum'ları.
- Log/stone/stick için ayrı actor-state/connection grafiklerinin yanında genel `IStructuralNode` / `UStructuralGraph` API izleri.
- Raid feedback/event bus ve dünya event subsystem'i.
- Ortak SaveGame manifesti: world ID, seed, revision, schema version ve olay/komut kimlik defterleri.

**Şu anki sınır**
- Ortak enum/interface bulunması bütün aktör ailelerinin aynı grafı, aynı state machine'i veya aynı persistence mantığını kullandığı anlamına gelmez.
- Yapı aileleri arası tek authority, event ordering/idempotency ve world/actor save kapsama sınırları hâlâ çapraz denetim gerektirir.

## Save/Load: tam olarak neyi kaydetmeyi beklemeliyiz?

Ortak `UWorldCoreSaveGame` zarfının mevcut kapsamı kodda şu kayıt ailelerini içeriyor:
- Kazı hacmi ve yeraltı tehlike snapshot'ı.
- WorldState chunk/cell durumu ve dünya zamanı/havası/seed.
- Bölge lojistik node/link snapshot'ı.
- Üretim istasyonlarının job kuyruğu snapshot'ları.
- Oyuncu stack envanteri ve `AInventoryStorageActor` envanter snapshot'ları.
- Komut/event idempotency defterleri.

Buna karşılık bu zarfın fiziksel ağaç/kütük/taş/çubuk/yapı aktörleri, bitkiler ve hayvanlar için genel yeniden oluşturma sistemi olduğunu varsaymıyoruz. Ayrı `USaveGameSubsystem` log snapshot'ı yalnızca `ALogActor` ailesini ele alıyor; onu kütük, yapı ve dünya persistence'ının tamamı gibi sunmak yanlış olur. Ayrıca yükleme başarısızlığında bütün alanları tek transaction'da geri çeviren ortak rollback kabulü henüz sağlanmış değil.

## Sonuç: “oynanabilir özellik” demek için kabul eşiği

Bir mekanik şu dört düzeyle raporlanmalı:

1. **Kod izi:** İlgili sınıf/API var mı?
2. **İşlem:** Başarılı, başarısız ve kısmi başarısızlıkta state doğru mu?
3. **Oyuncu bağlantısı:** Input/AI/Blueprint gerçek oyun akışına bağlanmış mı?
4. **Motor kanıtı:** UE 5.8 build, named automation ve gerektiğinde görsel/E2E test geçti mi?

Yalnızca ilk düzeyde olan bir özelliğe “oyunda çalışıyor” denmez. Bu yetenek haritası kaynak kodda görülen kabiliyetleri ve açık sınırları ayırır; satır bazındaki ayrıntı ve kanıt durumu için 485 satırlık mekanik kataloğu ana referans olmaya devam eder.

İlişkili denetim: [Faz 0–12 statik çapraz sistem denetimi](./FAZ_00_12_STATIK_CAPRAZ_DENETIMI.md).
