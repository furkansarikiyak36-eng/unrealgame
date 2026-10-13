# MyProject2 — Total Plan (13 Ekim 2026)

Bu dosya `KUTUK_SISTEMI_PLANI.md` (Faz 1–6) üzerine devam eder. Faz 1–6 uygulandı ve testli; burası **Faz 7 ve sonrasını** ve yeni kararları içerir. Kaynak: 76 maddelik kütük mekanik kataloğu + `DURUM_RAPORU.md` + 13 Ekim görüşmesi.

## Kurallar (değişmez)

- `ACarvableLog` ana sistem; Wood paketinden yalnızca mantık (`WoodStructureLogic`, `WoodStructureAnalysis`) kullanılır.
- Her mekanik için `AMechanicsDemo` otomatik testine kontrol eklenir; Wood birim testleri bozulmaz.
- Tasarım ilkesi: serbest kullanım, sabit kesim noktaları, tarif yok.
- Greybox: mesh'ler `Scripts/build_greybox.py` ile üretilir.
- Kod zip/patch ile paylaşılır; derleme ve testler kullanıcı tarafında (`Scripts/run_tests.sh`).

## Yeni kararlar

1. **Ateş bir yerleştirme modu değil, odunun özelliğidir.** Log, kazık, tahta, yarım/çeyrek parça temelde odundur ve yanabilir. R'daki "ateş" modu kalkar; kütüğe tıklayınca campfire görseli çıkması yanlıştı.
2. **Yanma parçanın kendi durumudur;** yanan parçanın çevresinde ateş simülasyonu (alev, ışık, ısı yayılımı, yakıt tüketimi) taşınır.
3. **Ateş yeri taşlarla kurulur.** Taş halkası otomatik oluşmaz; oyuncu taşları toplayıp yerleştirir.
4. **İki ateşleme kaynağı da eklenir:** (a) mevcut ateşten alev alma (yanan parçayı elde taşıma), (b) ayrı çakmak/çıra aleti.

## Mevcut durum özeti

76 maddenin kabaca: ~43 tam çalışıyor ve testli, ~12 kısmi, ~21 yok (ayrıntı `DURUM_RAPORU.md`). Son test: 74/74 mekanik, 11/11 Wood birim testi. Görsel taraf oyunda gözle doğrulanmadı.

## Fazlar

Her faz kendi başına oynanabilir sonuç verir ve kendi testini ekler.

### Faz 7 — Kısmi olanları kapat (küçük-orta)

Katalog: 45–47, 49–52 (çatı, merdiven, rampa, korkuluk, pencere kasası, iskele, savunma yapıları).

- Eğik parça yerleştirme (45° ve serbest açı).
- Platform üstüne direk dikme.
- Parçayı "geçici" işaretleme tuşu (`SetTemporary` kodda var, tuş eksik).
- **Test:** Her yapı türü oyuncu yoluyla kurulur ve yapı tanıma doğru adı verir (çatı iskeleti, merdiven, rampa, korkuluk, iskele).

### Faz 8 — Yanma sistemi: ateş parçanın özelliği (orta)

Katalog: 62, 70 (genişletilir). Bug: kütüğe tıklayınca campfire görseli çıkıyor.

**8a. Yanma durumu**
- `ACarvableLog` yanma durumu taşır: `Sağlam → Tutuşuyor → Yanıyor → Yanık`.
- Yanan parçanın çevresinde alev ve ışık görünür, parça boyutuna göre ölçeklenir (mevcut greybox koni alev ve ışık buraya taşınır).
- **Yakıt** parçanın hacmidir; mevcut formül korunur (tam kütük ~593 sn, yarım ~297, çeyrek ~148, üst sınır 900 sn). Yakıt bitince parça "Yanık" olur; mevcut onarım/değiştirme çalışmaya devam eder.
- **Isı yayılımı:** `ACampfire`'daki mantık (160 cm yakınındaki odun hasar alır) yanan her parçaya genelleştirilir.
- **Parçaya göre fark:** tutuşma hızı ve yanma süresi parça türüne göre ayarlanır (ince parça çabuk tutuşur ve kısa yanar, kalın kütük geç tutuşur ve uzun yanar).
- **Yapıya etkisi:** mevcut hasar eşikleri (≥%60 eklem gevşer, ≥%85 hiçbir şey taşımaz) yanan direk/duvarı kendiliğinden çökertir.
- Aynı anda yanan parça sayısına üst sınır konur (performans). Gerçek akışkan simülasyonu yapılmaz; yayılma, yakıt ve hasar oyun mantığı olarak kurulur.

**8b. Ateş yeri (taş halka)**
- Taş kayadan kazma ile alınır, taşınır, oyuncu tek tek halka olarak yerleştirir (yaklaşık 6–8 taş, snap'li daire; sayı denemeyle ayarlanır).
- Halka tamamlanınca ateş yeri aktif olur. Halka yanma kuralını değiştirmez; yayılmayı sınırlar ve ateşi güvenli tutar.
- `ACampfire::Build` ile otomatik taş oluşturma kalkar; `ACampfire` içindeki yakıt kodu yanma sistemine taşınır.
- R döngüsü: yatır ↔ dik (ateş modu çıkar).

**8c. Ateşleme kaynakları (ikisi de)**
- **Alev alma:** yanan parça elde taşınabilir (meşale gibi); yanabilir bir parçaya temas ettirince tutuşturur. Mevcut ateşten alınır.
- **Çakmak/çıra aleti:** yeni alet; bakılan parçayı tutuşturur, süre parçanın tutuşma hızına bağlıdır.

- **Test:** (1) Taşsız yere odun atılınca ateş kurulmaz. (2) Halka tamamlanınca ateş yeri aktif olur. (3) Çakmakla bir parça tutuşur, yakıt hacimle orantılı azalır, bitince "Yanık" olur. (4) Yanan parçadan alınan alevle başka parça tutuşur. (5) Yanan direk kirişi düşürür. (6) Mevcut yangın/onarım testleri (74/74) bozulmaz.

### Faz 9 — Ham madde zinciri ve karar temizliği (küçük)

Katalog: 3.

- **Gövde boyu:** `HarvestableActor`'daki ağaç boyu/kalınlığından hesaplanır, 65 cm'nin katına yuvarlanır (şu an her ağaçtan sabit 15,6 m × 95 cm).
- **Tarif tuşları (1–7) kaldırılır;** B inşa modu yalnızca taş gibi ahşap olmayanlar için kalır.
- **Eski yapı sistemi** (`StructuralIntegritySubsystem`, `LogConstructionActor`) donmuş kalır.
- **Test:** Farklı boydaki iki ağaç farklı kütük sayısı verir; toplam modül boyu ağaç boyunu aşmaz.

### Faz 10 — Fizik durumları ve kütük hareketi (orta; Faz 12–13'ün ön şartı)

Katalog: 6–10 ve "geçici destekle kaldırma".

- Dağınık bayraklar (`bCarried`, `bAnchored`, `bTemporary`) yerine tek `Loose / Carried / Installed` durumu.
- Düşürme, itme, uzun parçayı sürükleme, eğimde yuvarlanma, çarpışmanın hasar/sonuç etkisi.
- Bir ucu geçici destekle kaldırma.
- **Test:** Eğimde bırakılan kütük belli mesafe yuvarlanır; `Installed` parça yuvarlanmaz; itilen parça hareket eder.

### Faz 11 — Açıklık açma ve oluk (orta; Faz 10'dan bağımsız)

Katalog: 28, 43.

- Duvardan seçilen modül aralığını kesip kapı/pencere açıklığı açma; etkilenecek bölge turuncu sınırla gösterilir. Kesim ve kasa tanıma hazır.
- Boyuna oluk aleti (`ECarveTool` genişletilir, `CutProfile` üzerine).
- Açılır kapı bu fazın dışında, sonraya kalır.
- **Test:** Duvarda açıklık açılınca yapı tanıma "kapı kasası" / "pencere kasası" der; üstteki parçalar destekte kalır.

### Faz 12 — Lojistik (büyük; Faz 10'a bağlı)

Katalog: 13, 14, 65–67 (68, 15 en sona).

- Sırayla: depolama sehpası, el arabası, kütük kızağı.
- Halatlı hat/zipline ve su üstünde taşıma en sonda.
- **Test:** El arabasına yüklenen parçalar ve hacim korunur; boşaltınca `Loose` olur.

### Faz 13 — Diğer kullanımlar (orta-büyük; Faz 10'a bağlı)

Katalog: 71–76.

- Tuzak ve fırlatma: Faz 10 fiziği + mevcut hasar modeli (yeni sistem gerekmez).
- Tezgâh/mobilya: platform ve yarım kütük parçalarıyla.
- Sal/yüzer platform: sahnedeki `BuoyancyManager`'a bağlanır.
- Makine yapımı en sonda; tasarım kararı gerekir.

### Faz 14 — Düşman hasarı ve ses (orta)

Katalog: 61.

- Basit saldırgan aktör mevcut hasar sistemini kullanır (şu an yalnızca admin Y tuşu hasar veriyor).
- Ses her mekaniğe ayrı geçişle eklenir (şu an hiçbirinde yok).

## Bağımlılıklar

```
Faz 7 ─┐
Faz 8 ─┼─ (bağımsız, sırayla)
Faz 9 ─┘
Faz 10 ──► Faz 12, Faz 13
Faz 11  (bağımsız)
Faz 14  (en sonda; Faz 8 yanma hasarını da kullanır)
```

Öncelik: Faz 8 (bug olarak işaretlendi) → 7 → 9 → 10 → 11 → 12 → 13 → 14.

## `KARARLAR.md`'ye eklenecek satırlar

| Karar | Gerekçe |
|---|---|
| Ateş yerleştirme modu değil, odunun özelliği; yanan parça çevresinde ateş simülasyonu taşır | Log/kazık/tahta temelde odundur ve yanabilir; kütüğe tıklayınca campfire çıkması yanlıştı |
| Ateş yeri taş halkasıyla oyuncu tarafından kurulur, otomatik oluşmaz | Ham madde zinciri tutarlı olmalı |
| İki ateşleme kaynağı: yanan parçadan alev alma + çakmak/çıra aleti | Kullanıcı seçti |
| Yanma "oyun mantığı" simülasyonu (yayılma, yakıt, hasar); akışkan simülasyonu yok; aktif yanan parça sayısı sınırlı | Performans, greybox'a yeter |

## Açık sorular

- Taş halkasındaki taş sayısı ve halka çapı (öneri: 6–8 taş, denemeyle ayarlanır).
- Çakmak/çıra: sonsuz kullanım mı, sarf malzemesi mi?
- Yanık parça: kömür gibi yakıt olarak kalsın mı, yoksa yok olsun mu? (Mevcut davranış: yanık parça ateşe yakıt olarak atılabiliyor.)
