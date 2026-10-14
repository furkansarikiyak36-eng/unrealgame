# MyProject2 — Durum raporu (14 Ekim 2026)

Unreal Engine 5.8, C++, greybox (sanatsız) hayatta kalma oyunu. Bu rapor **şu an oyunda neyin çalıştığını** anlatır.

Her madde üç düzeyden birinde:
- ✅ **Oynanır**: oyunda tuşu var ve otomatik testten geçiyor.
- 🟡 **Kodu var**: derleniyor, ama oyunda girişi yok ya da oyunda denenmedi.
- ❌ **Yok.**

Görsel taraf (çizgiler, alev, ışık, HUD, yeni aktörlerin görünüşü) testlerde doğrulanmaz. Bunlar oyunda gözle denenmeli, çoğu henüz denenmedi.

## 1. Özet

| Referans oyun (ondan alınan) | Durum |
|---|---|
| **Sons of the Forest**: kütük, taş, çubukla her şey | ✅ Kütük sistemi tam. ✅ Fiziksel taş ve çubuk, çubuk yapıları. 🟡 Taşla duvar örme (kodu var, oyuncu yolunda denenmedi). |
| **7 Days to Die**: günlük üsse baskın, az voxel, beton | ✅ Her 3. gece baskın, gittikçe kalabalık; düşman yapıyı yıkıyor. ❌ Voxel kazı, toprak sıkıştırma, beton: en sona bırakıldı (kazının kodu var, oyuna bağlı değil). |
| **Frostpunk + The Long Dark**: bölge zorluğu, günlük hayatta kalma | ✅ Gün/gece, soğuk iklim, vücut ısısı, donma, ateş/çatı/giysiyle ısınma. Üç bölge. ❌ Günlük "seçim" ekranı yok; seçim yakıt, yemek ve baskın baskısından doğuyor. |
| **Çiftlik oyunları**: tarım, hayvancılık, deri | ✅ Tarla, ekim, büyüme, hasat. Yumurta, et, deri, yün. Deri giysi, ip. 🟡 Hayvan besleme yok (koyun yün vermiyor). |
| **Death Stranding**: zor dünya, köprü ve iple medenileştirme | ✅ Nehir (yavaşlatır, ıslatır), yar, ip hattı (rota ağına kayıtlı), kirişle köprü, HUD'da altyapı sayısı. |
| **Subnautica**: kalıntılarla hikâye | ✅ 5 not, yanık kulübe, okunan notlar günlüğe giriyor. |

**Testler:** mekanik 119/119, Wood birim 12/12, envanter kapasite 5/5, çubuk tarif 3/3, taş tuzağı 4/4, taş yerleştirme 4/5. Web Claude'un 32 adımlık Python sözleşme testleri de geçiyor.

## 2. Nasıl çalıştırılır

- **Haritalar:** `Demo_WoodChain` (asıl oyun ve test haritası, 70 × 70 m), `Greybox_Mechanics`.
- **Normal oyun:** `Demo_WoodChain`'de Play. Açılışta senaryo dünyayı kurar: nehir, yar, yanık kulübe, notlar, yabani tahıl, hayvanlar, taşlar. Saat sabah 08:00, 1. gün.
- **Rehberli demo:** P (admin) ardından L. Görev listesi HUD'da çıkar.
- **Otomatik test:** editör kapalıyken `bash Scripts/run_tests.sh`. Önce derler, sonra Python sözleşmelerini, 5 Unreal birim test grubunu ve 119 mekanik kontrolü çalıştırır. Test sırasında senaryo kurulmaz.

## 3. Kontroller

| Tuş | İş |
|---|---|
| WASD, Shift, Space | Hareket, koşma (dayanıklılık harcar), zıplama |
| Sol tık | Vur (balta: ağaç, kesim, oyma, düşman, hayvan) / taşınanı koy / yapım modunda yap |
| E | Al (kütük omza, küçük taş ve çubuk çantaya, yapı taşı ele) · arabadan indir · tarlada ek / hasat et · hayvandan ürün topla · not oku · su iç · ip ucunda ipe bin |
| R | Taşınanı yatır → dik → eğik (oyma sırasında: alet değiştir) |
| Q / tekerlek | Seçim değiştir: oyma derinliği, eğim açısı, yapım listesi |
| F (basılı) | Çakmak: bakılan odunu ya da ateş yerini ısıtır |
| C | Taş koy (7 taş halka = ateş yeri) |
| I | Yapım: diken, çit, meşale, tarla, deri giysi, ip, ip hattı |
| 8 | Ye (pişmiş et, yumurta, tahıl, süt) |
| X | İt (serbest kütük, araba, kızak) |
| O | Duvardaki kütükten 2 modüllük açıklık kes |
| V | Parçayı geçici (iskele) işaretle |
| T, B, 3, Tab | Mala, inşa modu (taş blok vb.), taş blok yap, çanta |
| P (admin) + … | G malzeme, H oda, J kes, N aç, M hasar, K oyma kütüğü, L demo, Y yapıya hasar, Z baskın başlat, U araba/kızak/sehpa koy |

## 4. Oynanır sistemler

### 4.1 Kütük (Sons of the Forest) ✅
- **Ağaç:** Ağaç kesilir, devrilir. Gövde kesilerek kütüklere ayrılır. Kütük sayısı ağacın boyundan çıkar (boyun %85'i, en çok 4 kütük). Devrilen ağaç 3 çubuk bırakır.
- **Modül ızgarası:** 65 cm, tam kütük 6 modül (390 cm). Bütün kesimler modül çizgilerinde.
- **Kesim ve oyma (10 alet):** çentik, enine kesim, boyuna yarma, oval çentik, düz çentik, sivri uç, pah, uç düzeltme, kırlangıç, oluk.
  - Yalnızca üst/alt yüzden kesilir.
  - Oyma derinliği çapın 1/6, 1/3 ya da 1/2'si.
  - Her kesim iki vuruş ister; önce kırmızı kesikli çizgi önizlemesi.
- **Taşıma:** Omuzda hacme göre: 1 tam kütük = 2 yarım = 6 tane 1 modüllük parça. El arabası, kızak ve sehpa ile taşıma ve depolama (yalnız admin U ile çıkıyor). Kütük itilebilir. Sert çarpma kütüğe hasar verir.
- **Yerleştirme:**
  - Yere yatır, direk dik, direğin tepesine kiriş, 45° payanda.
  - Döşeme tahtası (düz yüz üstte).
  - Çaprazlama: eyer çentiği kendiliğinden açılır.
  - Eğik parça (15–75°).
  - Yatan kirişin üstüne direk.
- **Yapı ve destek:**
  - Destek grafiği ve çöküş zinciri.
  - Kiriş açıklığı: destekler arası en çok 4,5 modül, taşma en çok 2 modül; aşarsa kiriş ortadan kırılır.
  - Hasar eşikleri: %60'ta eklemler gevşer, %85'te parça hiçbir şey taşımaz.
  - Hasarlı parça aynı boyda sağlam parçayla değiştirilir.
  - Duvardan açıklık kesilebilir.
- **Yapı tanıma (16 tür):** yatay/dikey duvar, kazık duvarı, savunma duvarı, geçit, kapı kasası, pencere kasası, platform, tezgâh, çatı iskeleti, çatı kaplaması, rampa, merdiven, köprü, korkuluk, iskele. Bakılan parçanın HUD'unda adı çıkar.

### 4.2 Taş ve çubuk (Sons of the Forest) ✅ / 🟡
- ✅ **Kaya:** Kırılınca 1 yapı taşı (40 kg) ve küçük taşlar düşer. Küçük taş çantaya girer, yapı taşı iki elle taşınır.
- 🟡 **Taş yerleştirme:** Web Claude'un yüz/yuva seçimli önizlemesiyle yapı taşı konur. Bu akış otomatik testte değil, oyunda denenmedi. Çakışma kontrolü testi kalıyor (§7).
- ✅ **Ateş yeri:** Küçük taşlar C ile tek tek konur, 7 taş halka ateş yeri olur.
- ✅ **Çubuk yapıları (I):**
  - Diken (4 çubuk): düşmana 20 can/sn.
  - Çit (6 çubuk): yolu kapatır, düşman kırar.
  - Meşale (2 çubuk): ışık, 3 m'de +12°C.

### 4.3 Ateş ✅
- **Yanma durumu:** Her odun parçası önce ısınır, sonra tutuşur, yanar ve yanık olur. Kalın kütük tam ısıyla 14 sn'de tutuşur.
- **Yakıt:** Hacme göre; tam kütük yaklaşık 10 dk yanar. Hasar yakıt bittiğinde %100'e varır.
- **Yayılma:** Yanan parça 50 cm içindeki odunu ısıtır. Aynı anda en çok 12 parça yanar.
- **Yakma:** F ile çakmak ya da yanan bir odunla. Odun atmak tek başına ateşi yakmaz.
- **Yapıya etkisi:** Yanan direk eklemleri gevşeyince kirişi düşürür.

### 4.4 Günlük döngü ve hayatta kalma (Frostpunk / The Long Dark) ✅
- **Saat:** Web Claude'un ortak simülasyon saati kullanılıyor; 1 oyun saati = 60 sn, 1 gün = 24 dk. Gece 20:00–06:00.
- **Sıcaklık:**
  - Günlük en düşük/en yüksek değerler, soğuk iklim için 14°C aşağı kaydırılır.
  - Bölge farkı: Kayalık −4, Nehir kıyısı −2.
  - Isıtanlar: çatı +8, ateş +28'e kadar (5 m'de sıfıra iner), meşale +12.
  - Islanmak −8.
- **Vücut ısısı:** 12°C'nin altında düşer, deri giysi bu kaybı azaltır. Sıfırda donma, can kaybı.
- **Diğer ihtiyaçlar:** Açlık, susuzluk, can, dayanıklılık.
- **Yemek ve su:** 8 tuşuyla yemek, nehirden su.
- **HUD:** gün ve saat, bölge, sıcaklık ve katkıları, sıradaki baskın gecesi, altyapı sayısı, not sayısı, vücut ısısı çubuğu.

### 4.5 Baskın ve savunma (7 Days to Die) ✅
- **Zaman:** Her 3. gece 22:00'de baskın; 18:00'de uyarı, şafakta (06:00) biter.
- **Büyüklük:** Toplam 3 + gün sayısı düşman, aynı anda en çok 2 + gün/3.
- **Altyapı:** Web Claude'un baskın yöneticisi (dalga, bütçe, doğma yeri); düşmanı biz tanımlıyoruz.
- **Düşman:** Oyuncuya ya da en yakın yapıya yürür, yolundaki kütüğü ya da çiti yıpratır, yapıyı çökertebilir.
- **Düşmana hasar verenler:** sivri kazık, çubuk diken, çarpan kütük, balta (25).
- 🟡 **Eksikler:** Zorluk oyuncunun ilerlemesine bağlı değil, yalnızca güne bağlı. Düşman görseli kırmızı bir silindir. Haritada gezinme ağı yok, düşman düz çizgide yürüyor.

### 4.6 Tarım ve hayvancılık ✅ / 🟡
- ✅ **Tohum:** Yabani tahıldan (nehrin öte yakası).
- ✅ **Tarla:** I → Tarla (2 çubuk), E ile ekim. Ekin günlerle büyür (greybox tahıl yaklaşık 5–6 günde olur). Hasat en az 3 tahıl ve tohum geri verir.
- ✅ **Hayvanlar:** Tavuk yumurtlar. Kesilen koyun 4 et, 2 deri, 1 yün verir.
- ✅ **Yemek:** Ateş yanında yenen et pişer, daha çok doyurur.
- ✅ **Deri ve yünden yapılanlar:**
  - Deri giysi: 3 deri; parça başına %40 yalıtım, en çok %80.
  - İp: 2 yün ya da 1 deri.
- 🟡 **Besleme:** Hayvanı besleme bağlantısı yok, koyun bu yüzden yün vermiyor. Süt (inek) haritada yok.

### 4.7 Dünya, bölgeler, altyapı (Death Stranding) ✅
- **Bölgeler:**
  - Kayalık (batı): yar, taşlar.
  - Orman (orta): ağaçlar, yanık kulübe.
  - Nehir kıyısı (doğu): nehir, tarım ve hayvanlar.
- **Nehir:** Haritayı boydan geçer. Suda %45 hız ve −8°C; köprüde kuru kalınır.
- **Yar:** 4,5 m, ipsiz çıkılmaz.
- **İp hattı:** 1 ip, 2–30 m. Her ip rota ağına (web Claude'un lojistik sistemi) kayıt olur. Ucunda E ile kayılır ya da tırmanılır.

### 4.8 Hikâye (Subnautica) ✅
- **Notlar:** Önceki yerleşimcilerin 5 notu: ateş, nehir ve köprü, yar ve ip, üçüncü gece, yanık kulübe.
- **Yanık kulübe:** Yanmış kütüklerden bir kalıntı.
- **Günlük:** Okunan not günlüğe girer, HUD'da kaç tanesinin bulunduğu görünür.

## 5. Kodu var, oyunda yok 🟡

Web Claude'un dalından gelen, derlenen ve kendi birim/sözleşme testleri olan ama oyuncunun kullanamadığı sistemler:

- **Kazı ve yeraltı:** seyrek 3B kazı hacmi, arazi parçası mesh ve çarpışması, yeraltı desteği, göçük, yeraltı suyu, yeraltı tuzakları. Oyuncunun kazı aleti yok; en sona bırakıldı.
- **Üretim zinciri:** tarif, iş kuyruğu, güç/işçi bekleme, iptal ve iade. Örnek tarif: yarım kütük → 2 kalas. Haritada istasyon yok.
- **Kayıt/yükleme:** ortak kayıt zarfı (dünya, kazı, lojistik, üretim kuyruğu, envanter). Fiziksel kütükler, yapılar, hayvanlar ve bitkiler kaydedilmiyor; oyunda kaydetme yok.
- **Dünya ve malzeme simülasyonu:** hücre bazlı su, toprak, besin, kimya, biyokütle, çürüme, malzeme profilleri. Tarım bunun toprak verisini kullanıyor; geri kalanı oyuna yansımıyor.
- **Envanter arayüzü bileşenleri:** hızlı erişim, tarif keşfi, ürün bileşenleri (zırh, tüketilebilir, onarım aleti, fırlatılan silah).
- **Paketin eski sınıfları:** `ALogActor` (ikinci kütük sistemi), çubuk mobilyaları, taş kemer/sütun/harç/şömine, yağmur toplayıcı vb.

## 6. Testler

| Grup | Sonuç | Ne doğrular |
|---|---|---|
| Mekanik (`AMechanicsDemo`, oyun içinde, grafiksiz) | 119/119 | Kütük, yapı, ateş, hasar, taşıma, eğik parça, açıklık, araba, baskın, gün döngüsü, taş/çubuk, tarım, nehir, ip, notlar |
| Wood birim | 12/12 | Destek hesabı, yapı tanıma (tezgâh dahil) |
| Envanter kapasite | 5/5 | Kapasite, geri alma, geçersiz eşya |
| Çubuk tarif işlemi | 3/3 | Tarif işleminin bütünlüğü |
| Taş tuzağı | 4/4 | Tuzak hasarı |
| Taş yerleştirme | 4/5 | Yüz/yuva seçimi, bağlantı; **çakışma kontrolü kalıyor** |
| Python sözleşmeleri (web Claude) | 32 adım geçti | Kaynak kodda beklenen yapılar, katalog tutarlılığı |

## 7. Bilinen sorunlar ve eksikler

1. **Taş yerleştirme çakışma testi:** Bir engelin üstüne konan taş reddedilmiyor; çakışma sorgusu engeli görmüyor.
2. **Görsel denetim:** Oyunda gözle denenmedi (§ başı).
3. **Test sırasında senaryo yok:** Senaryo yerleşimi otomatik testte kurulmuyor. Grafiksiz bir oyun açılışında hatasız kurulduğu doğrulandı, ama nesnelerin yerleri oyunda görülmedi.
4. **Ses:** Hiçbir mekanikte ses yok.
5. **Taş ve paket sınıfları:** Taş duvarın oyuncu akışı, hayvan besleme, düşman yol bulma ve kayıt/yükleme yok.
6. **İkinci kütük sistemi:** `ALogActor` ve eski yapı sistemleri (`StructuralIntegritySubsystem`, `LogConstructionActor`) duruyor, kullanılmıyor.
7. **GitHub:** Bu durum yerel `integrate/web-plan` dalında. GitHub `main` dalı Faz 7–14'te kaldı; web Claude'un dalı ise derlenmeyen hâliyle duruyor.

## 8. Kod

- **Toplam:** 724 C++ dosyası, yaklaşık 43 bin satır.
- **Bizim çekirdek:** 60 dosya, yaklaşık 16,7 bin satır. Wood paketi yaklaşık 4,2 bin satır.
- **Paket ve web Claude:** 69 alt klasör.
- **Ana sınıflar:**
  - Kütük: `ACarvableLog`.
  - Oyuncu ve arayüz: `AForestCharacter`, `AForestHUD`.
  - Yapı: `StructureLink`.
  - Ateş ve gün: `UBurnSubsystem`, `ACampfire`, `UDayLoopSubsystem`.
  - Baskın: `ARaidEnemy`, `RaidKit`.
  - Taşıma ve kaynaklar: `AHaulCart`, `AFireStone`, `LooseResources`, `AStickStructure`.
  - Tarım: `FarmKit`.
  - Dünya ve hikâye: `ARiverStrip`, `ACliffBlock`, `ARopeLine`, `ARemnantNote`, `UScenarioSubsystem`.
  - Test: `AMechanicsDemo`.
- **Belgeler (`Docs/`):** `BASLA.md` (giriş), `KARARLAR.md`, `PROJE_GUNLUGU.md` (her işin kaydı), `KOD_HARITASI.md`, `TEST_DURUMU.md`, web Claude'un katalog ve denetim dosyaları.

## 9. Sıradaki işler (öneri)

1. Oyunda bir tur oynayıp görsel hataları toplamak: nehir, yar, notlar, HUD, alev, düşman.
2. Taş yerleştirme çakışma testini düzeltmek, taş duvarı oyuncu yolunda test etmek.
3. Hayvan besleme (tahılla), sonra süt.
4. Ses (balta, ateş, düşman, rüzgâr).
5. Kayıt/yükleme: fiziksel kütük ve yapıların kaydı.
6. En son: voxel kazı, toprak sıkıştırma, beton.
