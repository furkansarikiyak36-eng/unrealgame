# Sons of the Forest dilimi — log, taş, çubuk: plan v2

> Genişletilmiş mekanik kapsamı ve uygulama sırası: [SOTF_MECHANICS_EXPANSION_PLAN.md](SOTF_MECHANICS_EXPANSION_PLAN.md). Bu ek plan, mevcut kabul matrisini ve önceki ölçüm durumlarını silmeden genişletir.

**Amaç:** SotF'ın serbest (elle) inşa tarafındaki tekil oyuncu davranışlarını kendi sistemimizde ölçülebilir kabul hedefleri yapmak. Referans bir hedef listesi, zorunlu özellik listesi değil: her ID sonradan "alıyoruz / almıyoruz" diye işaretlenir. Kapsam dışı: combat, düşman AI, hikâye, rehber kitabı ve hayalet şablonları (bizde montaj elle).

**Kabul aracı:** `Scripts/probe_showcase.sh`. MechanicsTest'in Play oturumunda geçici bir `ALogShowcase` kurar; istasyonları oyuncunun gerçek koduyla (`PlaceForShowcase` vb.) çalıştırır; harita kaydedilmez. Her kontrol bir ID'ye bağlanır ve **PASS / FAIL / BLOCKED** sonucu, tarih ve commit ile `Docs/DILIM_SONUC.md`'ye yazılır. Unreal içinde koşulmayan satır PASS sayılmaz. Şu anki ölçüm: 14 istasyon, 178/180 yerleştirme.

## Durum kodları (kod taramasına göre, 2026-10-25)

| Kod | Anlamı |
|---|---|
| **Bağlı** | Oyuncu kodundan (`ForestCharacter`, `ForestGameMode`) ya da vitrinden kullanılıyor |
| **Sınıf var** | Sınıfı yazılmış ama oyuncuya ve vitrine bağlı değil: sadece kendi dosyasında duruyor |
| **Kısmen** | Bağlı ama davranışın bir kısmı eksik |
| **Yok** | Kod yok |
| **?** | Emin değilim; A aşamasında ölçülecek |

> Not: "Sınıf var" olanlar bu planın en ucuz kazancı; yeni kod yazmadan önce mevcut sınıfı oyuncuya bağlayıp istasyonda sınamak gerekiyor. Dikkat: odun taşıma için gerçek sınıf `AHaulCart` (el arabası, kızak ve raf türleri); `ALogCart`, `ALogSled`, `ALogStorageRack` eski ve bağlı değil, bağlanmayacak. "?" satırları 2026-10-25'te kod taramasıyla netleştirildi; kalanlar bir Play ölçümü gerektiriyor (A.4).

## 1. Çubuk ve kazık — STK

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| STK-01 | Dal/çubuk toplama | `LooseResources::SpawnStick`, `AStickActor` | Bağlı |
| STK-02 | Çubuğu yere dikme | `AStickStructure`, çubuk modu (`CraftStake`: çubuk dikilir) | Bağlı |
| STK-03 | Çubuğu sivriltme → kazık | `AStakeActor` (eski sınıf) bağlı değil; çubuk modunda dikilen çubuk sivriltilmiyor | Kısmen |
| STK-04 | Kazığı taşla güçlendirme | — | Yok |
| STK-05 | Çubuk çit hattı | Çubuk modunda tek tek dikme; hat/aralık kuralı yok | Kısmen |
| STK-06 | Çit parçasını değiştirme | — | Yok |
| STK-07 | Çubuğu yakıt yapma | `ACampfire` (yakıt) | Bağlı |
| STK-08 | Çubuktan ışık (meşale) | `ATorchActor`, `AWallTorchActor` | Sınıf var |

## 2. Log işleme — LOG

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| LOG-01 | Ağaç kes, log düşsün | `UTreeFellingSubsystem::ChopFrom`, `RawLogsForHeight` | Bağlı; istasyon yok, haritada ağaç yok |
| LOG-02 | 3/4 boyda kesme | `RawLogShapes` yalnız ikiye böler (L → 2×L/2) | Yok |
| LOG-03 | Yarıya kesme | Ortaya vur → enine kes | Bağlı |
| LOG-04 | Çeyrek boy | İki kez ikiye bölerek (600 → 150) | Kısmen |
| LOG-05 | Boyuna yarma | Uca vur → yarım, sonra çeyrek | Bağlı |
| LOG-06 | Kalas | Yana vur ×3 (Hewn1 → Hewn2 → Kalas), kalas → 4 tahta | Bağlı |
| LOG-07 | Kalası kısaltma | Kalasa ortadan vur → enine (profil korunur) | Bağlı |
| LOG-08 | Log → kazık (ucu sivri) | Serbest oymada `Point` aracı var; ham log (`RawLogShapes`) için yok | Kısmen |
| LOG-09 | Kesim yerini görme | HUD vuruş noktası + renk + işlem adı | Bağlı |
| LOG-10 | Kesim yönünü seçme | Vuruş yerinden otomatik (orta/uç/yan), [R] oluk | Bağlı |
| LOG-11 | Yapıya uygun boy hazırlama, malzeme doğru tüketilir | Parça sayısı/hacim kontrolü yok | Test yok |
| LOG-12 | Log fırlatma | `AThrownWeaponActor` (silah için) | Yok |
| LOG-13 | Bağlantı yüzeyi işleme | Otomatik: `CutSeat`, köşe çentiği, döşeme yuvası | Bağlı |
| LOG-14 | İşlenmiş parça bağımsız ve taşınabilir | Çıkan parçalar `ACarvableLog` | Bağlı; test yok |

## 3. Elle montaj — ASM

| ID | Davranış | Bizde | Durum | Vitrin |
|---|---|---|---|---|
| ASM-01 | Yatay yerleştirme | Lay | Bağlı | 1 |
| ASM-02 | Dikey (direk) | Stand, [R] | Bağlı | 4, 6 |
| ASM-03 | Mevcut loga bağlanma, noktalar görünür | Yüz soketleri + işaretler, [G] açı/yığın modu | Bağlı | 12, 13 |
| ASM-04 | Köşe | Corner, `CornerBite` | Bağlı | 3, 14 |
| ASM-05 | Üst üste duvar | Stack | Bağlı | 2, 14 |
| ASM-06 | Açılı yerleştirme | Slope (tekerlek ile açı), 45° | Bağlı | 5, 7 |
| ASM-07 | Kiriş | Beam | Bağlı | 4 |
| ASM-08 | Payanda | Brace | Bağlı | 4 |
| ASM-09 | Döşeme | Floor (30 cm aralık, yuvalı) | Bağlı | 4, 9 |
| ASM-10 | Çatı | Slope kirişleri; kaplama ve mahya kirişi yok | Kısmen | 5, 14 |
| ASM-11 | Basamak / merdiven | Basamak dizimi; gerçek merdiven yok | Kısmen | 8 |
| ASM-12 | Rampa | Platform istasyonunda rampa | Bağlı | 4 |
| ASM-13 | Korkuluk | — | Yok | — |
| ASM-14 | Halat köprü | `ARopeLine` (köprü sayacı HUD'da) | Kısmen ? | — |
| ASM-15 | Yapıdan parça sökme | Sabit logu omuza al → `SettleStructure` | Bağlı; test yok | — |
| ASM-16 | Yerleştirme önizlemesi | Hayalet + destek rengi + etiket | Bağlı | — |
| ASM-17 | Geçersiz yerleşimi reddetme | Uygun nokta yoksa reddedilir; **desteksiz olan uyarıyla kabul edilir** (bilinçli: SotF de kurdurup düşürür) | Kısmen | — |

## 4. Destek ve bozulma — SUP

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| SUP-01 | Destek ilişkisi kurma | `StructureLink` (Stacked, modül başına destek) | Bağlı |
| SUP-02 | Yükü aşağı aktarma | Var/yok desteği; yük miktarı hesabı yok | Kısmen |
| SUP-03 | Desteksiz parçayı belirleme | `WouldBeSupported`, önizleme rengi | Bağlı |
| SUP-04 | Payandanın etkisi | `StructureLink`te payandaya özel kural yok; payanda log olarak sayılıyor, ayrı destek etkisi ölçülmedi | Kısmen |
| SUP-05 | Direği kaldırma: yalnız bağlı parçalar düşer | `SettleStructure` | Bağlı; test yok |
| SUP-06 | Kirişi kaldırma | aynı | Bağlı; test yok |
| SUP-07 | Açıklık açma sonrası kontrol | `CutOpening` + düşen parçalar | Bağlı; test yok |
| SUP-08 | Parça hasarı | `ACarvableLog::AddDamage` (yalnız admin tuşu) | Kısmen |
| SUP-09 | Onarım | `CarvableLog::CanReplace` + `ECarryPlace::Repair`: hasarlı parçanın yerine yenisi konur, eskisi omuza gelir | Bağlı |
| SUP-10 | Parçayı değiştirme | aynı (Repair) | Bağlı |
| SUP-11 | Çöküş, ilgisiz bölüm yerinde | `SettleStructure` | Bağlı; test yok |
| SUP-12 | Yeniden bağlantı | `SettleStructure` → `StructureLink::Settle` | Bağlı |

## 5. Taş — STN

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| STN-01 | Taşı yerden alma | `AStoneActor`, `StoneCarry`, haritadaki taşlar (`SpawnStoneFromMesh`) | Bağlı (2026-10-25) |
| STN-02 | Taşı bırakma | `StoneCarry::Drop` | Bağlı |
| STN-03 | Taş istifleme | `StonePlacement` (yüz seçimi, onay) | Bağlı; test yok |
| STN-04 | Taş duvar | `StonePlacement`; `UStoneWallComponent` bağlı değil | Kısmen |
| STN-05 | Taş zemin | `StonePlacement` | Kısmen ? |
| STN-06 | Taş sütun | `StonePlacement` (dikey yüz) | Kısmen ? |
| STN-07 | Taş kiriş | — | Yok |
| STN-08/09 | Taş duvarda kapı/pencere | — | Yok |
| STN-10 | Taş bağlantı grafiği | `UStoneConnectionGraph` taş aktöründe var ama `StructureLink` ve oyuncu kodu okumuyor | Sınıf var |
| STN-11 | Taşı kaldırınca yeniden değerlendirme | ? | ? |
| STN-12 | Log eşiğinin taş temele oturması | **`StructureLink` taşı hiç tanımıyor** (0 başvuru): taş temel üstündeki log destek sayılır mı ölçülmedi | Yok (kilit risk) |
| STN-13 | Taşa hasar | `AStoneActor::ApplyDamage` | Kısmen |
| STN-14 | Taş şömine / ateş halkası | `AFireStone` bağlı; `AStoneFireplace` sınıf var | Kısmen |
| STN-15 | Taşı araçla taşıma | `AHaulCart` yalnız odun taşıyor; `AStoneSled` eski sınıf bağlı değil | Yok |

## 6. Taşıma ve depolama — CAR / STO

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| CAR-01–03 | Logu al, çoklu taşı, bırak | Omuz yığını, hacim sınırı | Bağlı |
| CAR-04 | Yük hareketi etkiler | Yük hıza etki ediyor: hız = 340 − 120 × yük | Bağlı |
| CAR-05–07 | Kızak/el arabası yükle, al, it | `AHaulCart` | Bağlı (vitrin 10) |
| CAR-08 | Araçta log + taş | `AHaulCart` yalnız odun hacmi alıyor | Yok |
| CAR-09 | Su üstünde kızak | `ARaftActor` ayrı | Yok |
| CAR-10 | Halatlı nakliye | `ARopeLine`, `AZiplineActor` (bağlı değil) | Kısmen |
| STO-01 | Log rafı | `AHaulCart` (raf türü, 8 omuz yükü); `ALogStorageRack` eski sınıf | Bağlı (vitrin 10) |
| STO-02 | Çubuk deposu | `AStickStorageRack` | Sınıf var |
| STO-03 | Taş yığını | `AStoneStoragePile` | Sınıf var |
| STO-04–06 | Al/koy, kapasite, çift sayım yok | — | Test yok |

## 7. Savunma — DEF

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| DEF-01 | Sivri log duvarı (palisat) | Dikili log + `Point` | Bağlı (vitrin 6) |
| DEF-02 | Çubuk kazık | `AStakeActor` | Sınıf var |
| DEF-03 | Kazığı taşla güçlendirme | — | Yok |
| DEF-04/05 | Çubuk çit, yıkılması | Çubuk modu | Kısmen ? |
| DEF-06–08 | Tuzak tetikleme, sıfırlama, düşen tuzak | `ATrapActor`, `ASpringTrap`, `AStoneTrapActor` | Sınıf var |
| DEF-09 | Yangın tuzağı | — | Yok |
| DEF-10 | Alarm | `ATripWireAlarm` | Sınıf var |
| DEF-11 | Tel + patlayıcı | — | Yok |

## 8. Ateş ve yardımcılar — UTIL

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| UTIL-01/02 | Ateş kurma, besleme | `ACampfire`, `AFireStone` | Bağlı |
| UTIL-03 | Logdan yakacak | `CraftFirewood`: 1 log → 4 yakacak | Bağlı |
| UTIL-04 | Duvar meşalesi | `AWallTorchActor` | Sınıf var |
| UTIL-05 | Kafatası lambası | — | Yok (almıyoruz) |
| UTIL-06 | Taş şömine | `AStoneFireplace` | Sınıf var |
| UTIL-07 | Kurutma rafı | `ADryingRack` | Sınıf var |
| UTIL-08 | Yağmur suyu | `AWaterCollector` | Sınıf var |
| UTIL-09 | Saksı | `APlantPot` | Sınıf var |
| UTIL-10 | Mobilya | Bank/masa (vitrin 9), `AStickChair`, `AStickBed` | Kısmen |

## 9. Uygulama sırası

### A — Ölçüm altyapısı: **yapıldı (2026-10-25)**
1. `ALogShowcase::BuildStation(int32)` + `StationCount()`: tek istasyon koşturulur.
2. `Scripts/probe_showcase.sh [istasyon…]` (örn. `... 3 5 14`; argümansız hepsi): MechanicsTest'te geçici vitrinle koşar, sonucu `Scripts/record_dilim.py` ile **`Docs/DILIM_SONUC.md`**'ye yazar: her kabul ID'si için PASS / KISMEN / FAIL / BLOCKED, ölçen istasyonlar, tarih, dal ve commit. PASS yalnız ölçütü gerçekten yerleştirme olan ID'lerde; sayım/destek/yürüme gibi geri kalanı ölçülmeyenler KISMEN ("eksik: ..." yazar).
3. Kabul ID → istasyon eşlemesi `Scripts/record_dilim.py` içindeki `STATIONS` tablosunda; yeni istasyon/kontrol eklenince buraya satır eklenir.
4. "?" satırları kod taramasıyla netleştirildi; kalan 5 tanesi (taş zemin/sütun, taşı sökme, halat köprü, çubuk çit) Play ölçümü gerektiriyor ve E/G aşamalarında kapanır.

İlk ölçüm (178/180 yerleştirme): **PASS 8, KISMEN 13, FAIL 2** (mahya kirişi, -45 dik çapraz), **BLOCKED 69** (ölçen istasyon henüz yok).

### B — Paket 1: malzemeyi dönüştür (LOG, STK-01..03)
- Kesim zinciri istasyonuna kontrol: her işlemde parça sayısı + toplam hacim korunur, çıkan mesh adı doğru, 600/300/150/75 hepsi (LOG-03..07, 11, 14).
- LOG-01 istasyonu: haritaya 2–3 "Fellable" ağaç, `ChopFrom` ile kes, log sayısı/boyu, ikinci vuruşta çoğalma yok.
- Karar ver ve gerekirse yaz: LOG-02 (3/4 kesim) ve LOG-08 (ham loga sivri uç). İkisi de `RawLogShapes`'e küçük ekleme + mesh gerekiyorsa asset (ChatGPT görevi).
- STK-03: `AStakeActor`'ı bağla (dikili çubuğa balta → kazık).

### C — Paket 2–3: taşı ve kur (CAR-01..07, ASM-01..12)
Çoğu bağlı ve vitrinde. Yalnız kabul kontrolleri: taşıma adetleri (CAR), destek bağlantısı var mı ve 5 cm kaydırmada korunuyor mu (ASM-04/05), çatı mahya kirişi hatası (ASM-10, bilinen tek XX), basamaktan oyuncu çıkabiliyor mu (ASM-11).

### D — Paket 4: ayakta kalma (SUP-01..07, 11)
Yeni istasyon "çökme": iki ayrı yapı yan yana (direk + kiriş + log). Birinin direğini sök: yalnız onun parçaları düşer, öteki yerinde kalır (SUP-05, 11). Kirişi sök (SUP-06). Açıklık sonrası üst sıralar destekli mi (SUP-07). Hasar → kırılma → yeniden hesap (SUP-08).

### E — Paket 5: taş + ahşap (STN-01..12)
Yeni istasyonlar: taş al/bırak (STN-01/02), 2 sıra taş duvar + 2×2 zemin + 3 taşlık sütun (STN-03..06, 10), **taş temel üzerine log eşiği** (STN-12, dilimin kilidi; `StructureLink` taşı tanımıyorsa ilk iş bu), taşı sök → yeniden değerlendirme (STN-11). Sonra ateş halkası (STN-14).

### F — Paket 6: kaydet ve yükle (en yüksek risk)
1. Esas sistem: `SaveGameSubsystem`. Diğer ikisi bu dilimde kullanılmaz, silinmez.
2. Giriş noktası: F5 kaydet, F9 yükle, slot `Dilim`. Şu an oyuncunun hiçbir kayıt yolu yok.
3. Kayıt istasyonu: küçük sahne → ölç → kaydet → haritayı yeniden aç → yükle → karşılaştır → ikinci yükleme (çoğalma yok). İlk sonuç FAIL beklenir.
4. Adaptörler: `ACarvableLog` (önce hazır şekiller: mesh + dönüşüm + sabit mi), `AStoneActor`, `AStoneMasonryActor`, oyuncu envanteri. `feature/phase-09-*` dalında taş kaydı var ama dal birleştirilemez (bizden 925 dosya farklı): yalnız o kısım elle alınır.
5. Sonra: serbest oyulmuş loglar (kesim geçmişini yeniden oynatma), kazı (yoğunluk anlık görüntüsü + mesh/collision).

### G — Paket 7: savunma ve yardımcılar (DEF, UTIL, STO)
"Sınıf var" olanları tek tek oyuncuya bağla ve her birine küçük bir istasyon: log rafı, taş yığını, çubuk deposu (STO, çift sayım testi), yaylı tuzak + alarm (DEF-06/07/10), taş şömine, kurutma rafı, meşale (UTIL). Önce sınıfın gerçekten çalışıp çalışmadığı ölçülür; çalışmayan sınıf ya düzeltilir ya da "almıyoruz" işaretlenir.

### H — Oynanış koşusu
MechanicsTest'te 20–30 dakika, elle: ağaç kes → işle → taşı → taş temel + log kulübe → F5 → haritayı yeniden aç → F9 → yapıyı kontrol et. B–F PASS olmadan başlamaz.

## 10. İş bölümü ve kurallar

| İş | Kim |
|---|---|
| Ölçüm, istasyonlar, Play testleri, ekran görüntüleri, sınıfları oyuncuya bağlama | Claude |
| Kayıt adaptörleri, kesim geçmişi, `RawLogShapes` eklemeleri (3/4, sivri uç), otomatik boy kesme | ChatGPT, dar görevle: dosya, fonksiyon, girdi/çıktı ve kabul ID'si yazılı |
| Bir dal ancak `probe_showcase` sonucu düşmüyorsa ve ilgili ID PASS ise birleşir | — |

**Bu dilimde değiştirmeyeceklerimiz:**
- İki envanter birleştirilmez. `AForestCharacter::Inventory` gerçek olanı.
- `ALogActor`, `LogConstructionActor`, `WorldSaveSubsystem` silinmez.
- `Modules/` taşınabilir C++ paketi Unreal'a taşınmaz.

## 11. Ek (v3): "parça yerleştirmekten geometri oluşturmaya"

Kaynak: SotF serbest inşa incelemesi (çubuk, savunma, taş, kesim, snap, iki uçlu montaj, destek, işlevsel yapılar, lojistik). Ana fikir: oyuncu malzemeyi işleyip elle birleştirir; sistem ortaya çıkan **şeklin işlevini tanır**. Hazır tarif (şömine gibi) ayrı yoldur.

### 11.1 Bizde zaten olan temel (beklenenden güçlü)
- **Şekil tanıma (ahşap):** `StructureLink::DescribeFeaturesOf` + `EWoodFeatureKind`: yatay/dikey duvar, sivri kazık duvarı, savunma duvarı, geçit, kapı kasası, pencere kasası, platform, çatı iskeleti/kaplaması, rampa, merdiven, köprü, korkuluk, iskele, tezgâh. HUD odaklanılan parçanın ait olduğu yapıyı yazıyor. Eksik: bu tanımaların her biri için kabul istasyonu yok.
- **Şekil tanıma (taş, ateş):** `AFireStone::TryForm`, halkadaki taş sayısı yetince kamp ateşi kurar. Bu, "hazır tarif olmadan işlevsel sonuç" örneği.
- **Önizleme = gerçek yerleşim:** Önizleme ve koyma aynı kararı kullanıyor (`GetLogPlacementResult`, `GetCarryPlacementTransform`). Kanıtlayan test yok.
- **Niyet sırası:** Önce hangi yüz/uç (üst, ön/arka, alt, uç), sonra [G] modu (düz/45/90/yığın), en son yakınlık. "En yakın nokta" kuralı değil.

### 11.2 Yeni kabul ID'leri

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| STK-09 | Dikili çubuklar yakınsa çit bağlantısı; uzak olan diziye girmez (mesafe + devamlılık) | — | Yok |
| STK-10 | Kazık kaldırılınca ona bağlı taş takviyesi ne olur | — | Yok |
| DEF-12 | Yarım duvarın **dış yüzüne** dışa bakan kazık (yüz ve yön önemli) | Yüz soketleri var, kazık türü yok | Yok |
| DEF-13 | Palisat kapısı + çubukla kilitleme | `AOpeningBuckActor`/`ADoorActor` var, palisata bağlı değil | Sınıf var |
| DEF-14 | Palisat devamlılığı: bir kütük sökülünce boşluk savunma sınırını bozar | `EWoodFeatureKind::StakeWall` tanıma | Kısmen |
| LOG-15 | Kesim önizlemesi ile gerçek kesim aynı düzlemde | HUD noktası = vuruş noktası; serbest oymada `GetCutGuide` | Kısmen; test yok |
| LOG-16 | İnşa sırasında otomatik boy kesimi (tam log → açıklığa uygun parça) | — | Yok (AUTOCUT-01 ile aynı) |
| ASM-18 | Dolu bağlantı noktası ya da başka parçanın içinden geçen yerleşim reddedilir | Kısmi (`IsSpanFree`, çentik yeri) | Kısmen |
| ASM-19 | Önizlemedeki konum ve yön = konan konum ve yön (aynı karar) | Kodda aynı fonksiyon | Test yok |
| ASM-20 | İki uçlu yerleşim: açı kameradan değil, iki dayanma noktasından (direk tepesi / log üstü / zemin) | Çatı kirişi en yakın yüksek loga göre açı alıyor; direk tepesi ve zemin ucu yok | Kısmen |
| ASM-21 | Yatan logun bir ucunu kaldır, altına destek koy (eğimli taşıyıcı) | — | Yok |
| ASM-22 | Merdiven = yan taşıyıcı + aralıklı basamak; rampa = taşıyıcı + sürekli yüzey (ayrı tanım) | `EWoodFeatureKind::Stairs/Ramp` tanıma var | Kısmen |
| ASM-23 | A-çatı: iki eğim mahyada **kabul edilmiş bağlantı** ile buluşur (kesişmek yetmez) | Mahya kirişi artık yerleşiyor (2026-10-25) | Kısmen |
| ASM-24 | Çok katlı: üst döşeme alt kirişe, üst direk alt kata; alt direk sökülünce zincir yeniden değerlendirilir | Platform istasyonu ikinci kat | Kısmen; çökme testi yok |
| ASM-25 | Halat köprü rolleri: ankraj, halat, taşıyıcı, yürüme yüzeyi ayrı | `ARopeLine` | Kısmen |
| SUP-13 | Üstünde durmak ≠ bağlı olmak: taşın üstünde duran log, taş sökülünce bağlantısı yoksa düşer | `StructureLink` taşı tanımıyor | Yok (STN-12 ile birlikte) |
| STN-16 | Taş şömine bir **tarif** (gereken taş sayısı + tamamlanma); gelişigüzel taş duvar şömine olmaz | `AStoneFireplace` sınıf var; `AFireStone` halkası bağlı | Kısmen |
| STN-17 | Taş kolon ve taş kiriş destek grafiğinde ayrı rol; kolon sökülünce kiriş yeniden değerlendirilir | — | Yok |
| STN-18 | Taş şekil tanıma (duvar / kolon / zemin), ahşaptaki `DescribeFeaturesOf` gibi | — | Yok |
| CAR-11 | Kızakta taş ve çubuk (yalnız log değil) | `AHaulCart` yalnız odun | Yok |
| CAR-12 | Zipline ile log/taş taşıma | `AZiplineActor` bağlı değil | Sınıf var |
| UTIL-11 | Kamp ateşini taşla güçlendirme (halka) | `AFireStone::TryForm` | Bağlı; test yok |
| UTIL-12 | Duvar saksısı ve dikili saksı (iki yerleşim biçimi) | `APlantPot` | Sınıf var |

### 11.3 Sıraya etkisi
- **E (taş + ahşap)** genişler: `StructureLink`'in taşı tanıması (STN-12, SUP-13), taş kolon/kiriş rolleri (STN-17) ve taş şekil tanıma (STN-18). Hepsi aynı işin parçası: taşı destek grafiğine sokmak. Dilimin en büyük kod işi bu; ChatGPT'ye tek, net görev olarak verilebilir.
- **C (taşı ve kur)** içine: ASM-19 (önizleme = gerçek) ve ASM-18 (çakışma reddi) testleri ucuz, hemen eklenir.
- **D (çökme)** içine: ASM-24 (çok katlı zincir) ve DEF-14 (palisat boşluğu).
- **Yeni küçük paket (C ile D arası): iki uçlu yerleşim.** ASM-20 → ASM-21 → ASM-22. Merdiven, rampa ve eğimli taşıyıcıların ortak temeli.
- **Şekil tanıma istasyonu (yeni):** her `EWoodFeatureKind` için küçük bir yapı kur; HUD/`DescribeFeaturesOf` doğru adı veriyor mu (PASS/FAIL). Bizde var olan en güçlü mekaniği ölçülebilir yapar.
- Kapsam dışı (şablon): çizilebilir savunma duvarı, hazır yapı planları, effigy, kafatası lambası.

## 12. Ek (v4): malzeme → geometri → işlev

Kaynak metnin devamı. Üç katman ayrı tutulur:

| Katman | Ne | Bizde |
|---|---|---|
| **A. Malzeme** | Tek parça: boy, profil, kesik mi, hangi işlemlere açık | `ACarvableLog` + `RawLogShapes` (log), `AStoneActor` (taş), `AStickActor` (çubuk) |
| **B. Geometri** | Parçaların birbirine göre düzeni ve bağlantıları | `StructureLink` destek grafiği (yalnız ahşap), soket/snap kararları |
| **C. İşlev** | Düzenin kullanımı: ateş yanar, üstünden geçilir, alan kapanır, tuzak çalışır | Ad olarak tanıma var (`DescribeFeaturesOf`, `EWoodFeatureKind`); işlev olarak yalnız ateş halkası (`AFireStone::TryForm` → `ACampfire`) |

**Kural:** Şekil oluştu diye işlev kendiliğinden açılmaz; işlevin kendi koşulları vardır (merdivende yürünebilirlik, şöminede ateşin çevrelenmesi, köprünün ayakta kalması). Tarif (hazır yapı) ile elle kurulan aynı işleve ulaşabilmeli; bir parça sökülünce şekil ve işlev yeniden değerlendirilir.

### 12.1 Yeni kabul ID'leri

| ID | Davranış | Bizde | Durum |
|---|---|---|---|
| FUNC-01 | Elle dizilmiş taş çevreleme + içinde ateş → şömine işlevi (ısı/pişirme), tarif seçmeden | Ateş halkası var; çevreleme/şömine tanıma yok | Yok |
| FUNC-02 | İşlevli yapıdan bir parça sökülünce işlev yeniden değerlendirilir (ör. halkadan taş alınca ateş) | ? | ? |
| FUNC-03 | Tarif ile elle kurulan aynı işleve ulaşır (`AStoneFireplace` tarifi ↔ elle şömine) | `AStoneFireplace` bağlı değil | Sınıf var |
| FUNC-04 | Tanınan şekil işlevini koşula bağlar: "Merdiven" adı verilen yapıda oyuncu gerçekten çıkabiliyor; "Köprü" ayakta | Ad var, koşul testi yok | Kısmen |
| FUNC-05 | Taşıyıcı + aralıklı basamak = merdiven; taşıyıcı + sürekli yüzey = rampa (aynı iki uçlu temel, ayrı tanım) | `EWoodFeatureKind::Stairs/Ramp` | Kısmen |
| SUP-14 | Payanda doğru yerdeyse bir destek direği sökülebilir, yapı ayakta kalır | Payanda log olarak sayılıyor; bu senaryo ölçülmedi | ? |
| UI-01 | Hedef belli: bakılan nesne/yüz işleme uygun mu | HUD etiketi, yüz adı | Bağlı |
| UI-02 | Olası dönüşüm görünür: kesim çizgisi, bağlantı oku/noktası, destek göstergesi | Vuruş noktası (ham log), kesim kılavuzu (serbest oyma), soket işaretleri, destek rengi; bağlantı yönü oku yok | Kısmen |
| UI-03 | Sonuç anlaşılır: yerleşince/kesilince dünya gerçekten değişir, mesaj ne olduğunu söyler | Mesajlar ("köşe geçmesi oturdu", "desteksiz, düşebilir") | Bağlı |

ASM-21 (bir ucu kaldır, altına destek koy) bu bölümün çekirdeği: geometri değişimi + yeni destek + yeni bağlantı yüzeyi. İki uçlu yerleşim paketinde (§11.3) ilk iş.

### 12.2 Sıraya etkisi
- Şekil tanıma istasyonu (§11.3) iki aşamalı olur: önce **ad** doğru mu (B → C adı), sonra **işlev koşulu** (FUNC-04: yürü, dur, yan).
- Taş paketi (E) sonunda: FUNC-01/02/03 şömine üçlüsü. Taşın destek grafiğine girmesinden sonra gelir; aynı ChatGPT görevinin devamı.
- UI-02 eksiği (bağlantı yönü oku) iki uçlu yerleşim paketine eklenir: ASM-20/21'de oyuncu hangi ucun nereye oturacağını görmeli.

## 13. ChatGPT "kapsamlı eksik analizi" (2026-10-25) — kararlar

`MANTIKLAR.md` üzerine yazılan değerlendirme okundu. Karar:

**Alıyoruz (plana bağlı, sıra değişmez):**
- İki uçlu yerleşim **atomik**: iki uçtan biri geçersizse hiç konmaz; açı iki ankrajdan; önizleme = onay (END-01..05 → ASM-20/21 paketine).
- Ahşap + taş **ortak destek grafiği**, bağlantı türü ile "değiyor" ayrı (SUP-13, STN-12, STN-17 ile aynı iş). İlk sürümde yük hesabı yok; kural tabanlı.
- **Şekil / işlev / destek** üç ayrı doğrulama (FUNC-04 ile aynı).
- Ateş halkası gerçekten halka mı: taşların ateş merkezine göre açısal dağılımı kontrol edilsin; 7 taşın bir yana yığılması sayılmasın. Ucuz, hemen yapılabilir.
- Mesh adı yerine **dönüşüm tablosu** (mesh → çalışma zamanı sınıfı → tanım); ad yalnız yedek + bilinmeyen için uyarı. İlk adım tablo; tam `PieceDefinition` veri varlıkları sonra.
- Kayıt şemasında kalıcı parça kimliği, bağlantılar ve sürüm; yüklemede önce tüm parçalar + bağlantılar, **sonra** destek değerlendirmesi (yükleme sırasında çökme olmasın).

**Sonra (dilim sonrası):** merkezi aday puanlama ("ilk tutan kural" yerine), `TopologyChangeSet`, hasar türlerinin ayrılması (görsel/yapısal/yanma), taşıma araçlarında kaynak etiketi, zipline, korkuluk ailesi, kesim için malzeme muhasebesi işlemi (transaction).

**Düzeltmeler / almıyoruz:**
- Omuz kapasitesi "300 modül boyu" değil: **bir 300 cm logun hacmi**.
- 65 cm modül eski bir tasarımdı; esas olan 75 cm.
- Döşeme aralığı belirsiz değil: kullanıcı kararı **merkezden merkeze 30 cm, daha fazla olmaz** (geniş tahtada tahta genişliği). 20 cm bir kural değil.
- Önerilen 9 yeni hizmet/sınıf (Construction Transaction Manager, Persistence Service, Pattern/Function Definition varlıkları...) bir kerede kurulmayacak: her biri ilgili kabul ID'si FAIL verince, o işi çözmek için eklenir.
