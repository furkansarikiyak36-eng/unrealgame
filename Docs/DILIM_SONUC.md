# Dilim sonucu

Üreten: `Scripts/probe_showcase.sh` (Scripts/record_dilim.py). Plan: [DILIM_SOTF.md](DILIM_SOTF.md).

- Tarih: 2026-10-25 07:47
- Dal: claude/sotf-expansion-build, commit: 8f054db (kirli çalışma kopyası)
- Çalışan istasyonlar: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
- Toplam: PASS 9, KISMEN 13, FAIL 1, BLOCKED 101

PASS: ölçen istasyon çalıştı, ilgili yerleştirmelerde XX yok ve ölçüt yalnız yerleştirmeyse. KISMEN: yerleştirme tamam, ölçütün geri kalanı (sayım, destek, yürüme...) henüz ölçülmüyor. FAIL: XX var. BLOCKED: ölçen istasyon yok ya da çalışmadı. Unreal içinde koşulmayan hiçbir satır PASS sayılmaz.

## Istasyonlar

| # | İstasyon | OK | XX |
|---|---|---|---|
| 1 | Kesim zinciri: 600 300 150 75, yarım, çeyrek, düzlenmiş, kal | 12 | 0 |
| 2 | Yatay kütük duvar (6 sıra), kapı ve pencere açıklığı | 12 | 0 |
| 3 | Kütük kulübe: dört duvar, köşe geçmeli (6 sıra, 2.25 m oda) | 12 | 0 |
| 4 | Platform: direk, kiriş, payanda, döşeme; rampa ve ikinci kat | 24 | 0 |
| 5 | Eğik çatı: alçak ve yüksek duvar, üstünde çatı kirişleri | 16 | 0 |
| 6 | Savunma duvarı (palisat): yan yana dikili kütükler | 8 | 0 |
| 7 | 45° çapraz sıralar: +45, üstüne 4 sıra, en üstte -45 | 6 | 1 |
| 8 | Basamak (yaklaşık merdiven): 300, 150, 75 üst üste | 3 | 0 |
| 9 | Bank ve masa: kısa kütük ayaklar, üstünde tahta | 10 | 0 |
| 10 | Taşıma ve depolama: el arabası, kızak, kütük rafı | 14 | 0 |
| 11 | Modül: 4 x 300 duvar, ortadan 2 x 2 açıklık -> 4 adet 75 cm  | 7 | 0 |
| 12 | Yüz snap noktaları: ön, arka, iki uç (her 75 cm'de) | 12 | 0 |
| 13 | Alt yüz: kirişin altına paralel kütük ve destek direği | 5 | 0 |
| 14 | Kütük ev: 600'lük kütükler, 12 sıra köşe geçmeli, kapı, penc | 48 | 0 |

## Kabul ID'leri

| ID | Davranış | Sonuç | Kanıt |
|---|---|---|---|
| STK-01 | Dal/çubuk toplama | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-02 | Çubuğu yere dikme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-03 | Çubuğu sivriltme → kazık | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-04 | Kazığı taşla güçlendirme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-05 | Çubuk çit hattı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-06 | Çit parçasını değiştirme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-07 | Çubuğu yakıt yapma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-08 | Çubuktan ışık (meşale) | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-01 | Ağaç kes, log düşsün | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-02 | 3/4 boyda kesme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-03 | Yarıya kesme | **KISMEN** | istasyon 1: 1 yerleştirme istenen gibi; eksik: parça sayısı ve hacim kontrolü |
| LOG-04 | Çeyrek boy | **KISMEN** | istasyon 1: 1 yerleştirme istenen gibi; eksik: parça sayısı ve hacim kontrolü |
| LOG-05 | Boyuna yarma | **KISMEN** | istasyon 1: 2 yerleştirme istenen gibi; eksik: parça sayısı ve hacim kontrolü |
| LOG-06 | Kalas | **KISMEN** | istasyon 1: 2 yerleştirme istenen gibi; eksik: parça sayısı ve hacim kontrolü |
| LOG-07 | Kalası kısaltma | **KISMEN** | istasyon 1: 2 yerleştirme istenen gibi; eksik: parça sayısı ve hacim kontrolü |
| LOG-08 | Log → kazık (ucu sivri) | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-09 | Kesim yerini görme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-10 | Kesim yönünü seçme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-11 | Yapıya uygun boy hazırlama, malzeme doğru tüketilir | **KISMEN** | istasyon 1: 12 yerleştirme istenen gibi; eksik: malzeme tüketimi / hacim kontrolü |
| LOG-12 | Log fırlatma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-13 | Bağlantı yüzeyi işleme | **KISMEN** | istasyon 3,14: 34 yerleştirme istenen gibi; eksik: çentik/yuva geometrisi kontrolü |
| LOG-14 | İşlenmiş parça bağımsız ve taşınabilir | **KISMEN** | istasyon 1: 12 yerleştirme istenen gibi; eksik: taşınabilirlik kontrolü |
| ASM-01 | Yatay yerleştirme | **PASS** | istasyon 1: 12 yerleştirme istenen gibi |
| ASM-02 | Dikey (direk) | **PASS** | istasyon 4,6: 16 yerleştirme istenen gibi |
| ASM-03 | Mevcut loga bağlanma, noktalar görünür | **PASS** | istasyon 12,13: 17 yerleştirme istenen gibi |
| ASM-04 | Köşe | **KISMEN** | istasyon 3,14: 32 yerleştirme istenen gibi; eksik: destek bağlantısı kontrolü |
| ASM-05 | Üst üste duvar | **KISMEN** | istasyon 2,14: 42 yerleştirme istenen gibi; eksik: destek bağlantısı kontrolü |
| ASM-06 | Açılı yerleştirme | **FAIL** | -45 (dik çapraz): köşe geçmesi  (istenen: köşe çaprazı) |
| ASM-07 | Kiriş | **PASS** | istasyon 4: 4 yerleştirme istenen gibi |
| ASM-08 | Payanda | **PASS** | istasyon 4: 2 yerleştirme istenen gibi |
| ASM-09 | Döşeme | **PASS** | istasyon 4,9: 11 yerleştirme istenen gibi |
| ASM-10 | Çatı | **PASS** | istasyon 5,14: 22 yerleştirme istenen gibi |
| ASM-11 | Basamak / merdiven | **KISMEN** | istasyon 8: 3 yerleştirme istenen gibi; eksik: oyuncu çıkabiliyor mu (yürüme) kontrolü |
| ASM-12 | Rampa | **PASS** | istasyon 4: 1 yerleştirme istenen gibi |
| ASM-13 | Korkuluk | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-14 | Halat köprü | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-15 | Yapıdan parça sökme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-16 | Yerleştirme önizlemesi | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-17 | Geçersiz yerleşimi reddetme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-01 | Destek ilişkisi kurma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-02 | Yükü aşağı aktarma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-03 | Desteksiz parçayı belirleme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-04 | Payandanın etkisi | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-05 | Direği kaldırma: yalnız bağlı parçalar düşer | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-06 | Kirişi kaldırma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-07 | Açıklık açma sonrası kontrol | **KISMEN** | istasyon 2,11: 9 yerleştirme istenen gibi; eksik: üst sıraların desteği kontrolü |
| SUP-08 | Parça hasarı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-09 | Onarım | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-10 | Parçayı değiştirme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-11 | Çöküş, ilgisiz bölüm yerinde | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-12 | Yeniden bağlantı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-01 | Taşı yerden alma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-02 | Taşı bırakma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-03 | Taş istifleme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-04 | Taş duvar | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-05 | Taş zemin | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-06 | Taş sütun | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-07 | Taş kiriş | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-08/09 | Taş duvarda kapı/pencere | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-10 | Taş bağlantı grafiği | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-11 | Taşı kaldırınca yeniden değerlendirme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-12 | Log eşiğinin taş temele oturması | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-13 | Taşa hasar | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-14 | Taş şömine / ateş halkası | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-15 | Taşı araçla taşıma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-01–03 | Logu al, çoklu taşı, bırak | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-04 | Yük hareketi etkiler | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-05–07 | Kızak/el arabası yükle, al, it | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-08 | Araçta log + taş | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-09 | Su üstünde kızak | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-10 | Halatlı nakliye | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STO-01 | Log rafı | **BLOCKED** | istasyon 10 çalıştı ama bu ID için yerleştirme satırı bulunamadı |
| STO-02 | Çubuk deposu | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STO-03 | Taş yığını | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STO-04–06 | Al/koy, kapasite, çift sayım yok | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-01 | Sivri log duvarı (palisat) | **KISMEN** | istasyon 6: 8 yerleştirme istenen gibi; eksik: çarpışma boşluğu kontrolü |
| DEF-02 | Çubuk kazık | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-03 | Kazığı taşla güçlendirme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-04/05 | Çubuk çit, yıkılması | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-06–08 | Tuzak tetikleme, sıfırlama, düşen tuzak | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-09 | Yangın tuzağı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-10 | Alarm | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-11 | Tel + patlayıcı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-01/02 | Ateş kurma, besleme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-03 | Logdan yakacak | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-04 | Duvar meşalesi | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-05 | Kafatası lambası | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-06 | Taş şömine | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-07 | Kurutma rafı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-08 | Yağmur suyu | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-09 | Saksı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-10 | Mobilya | **PASS** | istasyon 9: 10 yerleştirme istenen gibi |
| STK-09 | Dikili çubuklar yakınsa çit bağlantısı; uzak olan diziye gir | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STK-10 | Kazık kaldırılınca ona bağlı taş takviyesi ne olur | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-12 | Yarım duvarın **dış yüzüne** dışa bakan kazık (yüz ve yön ön | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-13 | Palisat kapısı + çubukla kilitleme | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| DEF-14 | Palisat devamlılığı: bir kütük sökülünce boşluk savunma sını | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-15 | Kesim önizlemesi ile gerçek kesim aynı düzlemde | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| LOG-16 | İnşa sırasında otomatik boy kesimi (tam log → açıklığa uygun | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-18 | Dolu bağlantı noktası ya da başka parçanın içinden geçen yer | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-19 | Önizlemedeki konum ve yön = konan konum ve yön (aynı karar) | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-20 | İki uçlu yerleşim: açı kameradan değil, iki dayanma noktasın | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-21 | Yatan logun bir ucunu kaldır, altına destek koy (eğimli taşı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-22 | Merdiven = yan taşıyıcı + aralıklı basamak; rampa = taşıyıcı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-23 | A-çatı: iki eğim mahyada **kabul edilmiş bağlantı** ile bulu | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-24 | Çok katlı: üst döşeme alt kirişe, üst direk alt kata; alt di | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| ASM-25 | Halat köprü rolleri: ankraj, halat, taşıyıcı, yürüme yüzeyi  | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-13 | Üstünde durmak ≠ bağlı olmak: taşın üstünde duran log, taş s | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-16 | Taş şömine bir **tarif** (gereken taş sayısı + tamamlanma);  | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-17 | Taş kolon ve taş kiriş destek grafiğinde ayrı rol; kolon sök | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| STN-18 | Taş şekil tanıma (duvar / kolon / zemin), ahşaptaki `Describ | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-11 | Kızakta taş ve çubuk (yalnız log değil) | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| CAR-12 | Zipline ile log/taş taşıma | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-11 | Kamp ateşini taşla güçlendirme (halka) | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UTIL-12 | Duvar saksısı ve dikili saksı (iki yerleşim biçimi) | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| FUNC-01 | Elle dizilmiş taş çevreleme + içinde ateş → şömine işlevi (ı | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| FUNC-02 | İşlevli yapıdan bir parça sökülünce işlev yeniden değerlendi | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| FUNC-03 | Tarif ile elle kurulan aynı işleve ulaşır (`AStoneFireplace` | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| FUNC-04 | Tanınan şekil işlevini koşula bağlar: "Merdiven" adı verilen | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| FUNC-05 | Taşıyıcı + aralıklı basamak = merdiven; taşıyıcı + sürekli y | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| SUP-14 | Payanda doğru yerdeyse bir destek direği sökülebilir, yapı a | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UI-01 | Hedef belli: bakılan nesne/yüz işleme uygun mu | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UI-02 | Olası dönüşüm görünür: kesim çizgisi, bağlantı oku/noktası,  | **BLOCKED** | bu ID'yi ölçen istasyon yok |
| UI-03 | Sonuç anlaşılır: yerleşince/kesilince dünya gerçekten değişi | **BLOCKED** | bu ID'yi ölçen istasyon yok |
