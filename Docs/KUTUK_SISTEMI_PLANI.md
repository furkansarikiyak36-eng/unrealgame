# Kütük Sistemi Planı — "Serbest kullanım, sabit kesim noktaları"

## Temel fikir

Bir kütükle ne yapılacağına oyuncu karar verir: odun, şenlik ateşi, enine kesip parçalara ayırmak, evin temeline koymak. Hiçbiri tarif ya da menü değildir; hepsi kütüğü fiziksel olarak kesip yerleştirmekten doğar.

Serbest olan: **ne yapılacağı ve parçanın nereye konacağı.**
Sabit olan: **kütüğün nereden kesileceği.** Kesimler yalnızca kütük üzerindeki belirli noktalardan yapılır, bu noktalar çizgilerle gösterilir.

Sabit noktaların amacı modülerliktir: aynı noktalardan kesilen parçalar birbirine ve yapıya otomatik olarak uyar.

## Görsel dil (Sons of the Forest'tan)

| Renk / şekil | Anlamı | Durum |
|---|---|---|
| Kırmızı kesikli çizgi | Burası kesilecek | ✅ Var |
| Beyaz kısa işaret | Bu aletin kullanılabileceği diğer noktalar | ✅ Var |
| Beyaz çizgili parça | Konacak parçanın önizlemesi | ✅ Var |
| Beyaz ok | Bağlantının yönü (parça nereye uzanacak) | ⬜ Faz 4 |
| Turuncu sınır | Bir kesimden ya da sökmeden etkilenecek bölge | ⬜ Faz 6 |

## Modül: kesim noktalarının ölçüsü

- **Modül (M):** Sabit 65 cm. Ağaçtan çıkan 390 cm'lik kütük tam 6M (1/6). Farklı boydaki kütükler de aynı ızgarayı paylaşır; ızgara kütüğün devrildiği andaki bir ucundan sayılır.
- **Enine kesim noktaları:** Kütüğün iki ucundan ölçülen modül çizgileri. Kesilen parçalar her zaman 1M–6M boyunda olur: 1M kütük, 2M, 3M (yarım), 4M, 6M (tam).
- **Kesim izleri kalıcı:** Bir parça kesildiğinde modül çizgileri parçada kalır, yani parçanın kendi ucundan yeniden hesaplanmaz. 4M'lik bir parça yine tam modül çizgilerinden kesilir.
- **Boyuna kesim noktaları:** Merkezden dikey ya da yatay. Bütün → yarım → çeyrek.
- **Çentik noktaları:** Uçta köşe payı kadar içeride ve modül çizgilerinin üstünde. Böylece duvardaki çentikler aynı aralıkla hizalanır.
- **En kısa parça:** 1M. Baltayla bundan kısası kesilmez. Odun 1M'lik parçanın yarılmasıyla elde edilir.


## Bir kütükten neler çıkar (hepsi kesimle, tarifsiz)

```
Tam kütük (6M)
├─ olduğu gibi ─────────────── duvar kütüğü, kiriş, temel kirişi (sill)
├─ enine kes ───────────────── 1M–5M parçalar
│   ├─ 1M bütün ─────────────── temel ayağı / oturak / kesme kütüğü
│   ├─ 1M → yar → yar ───────── odun (çeyrek)
│   └─ 2M–3M ────────────────── direk, kısa kiriş, payanda
├─ boyuna yar ──────────────── yarım kütük: zemin, basamak, sıra
│   └─ yarımı tekrar yar ────── kalas / tahta
├─ çentik aç ───────────────── köşe geçmesi (otomatik ya da baltayla)
└─ yak ─────────────────────── odunlar ateş halkasına → şenlik ateşi
```

Bir parçanın ne olduğu tariften değil, **şeklinden** anlaşılır: boyu kaç modül, bütün/yarım/çeyrek mi, çentiği var mı. Envanterdeki "Odun", "Kalas", "Yarım kütük" gibi eşyalar bu sınıflandırmadan türetilir.

## Mevcut durum

| Parça | Durum |
|---|---|
| Ağaç kesme, gövde → 2 büyük kütük → 4 kütük | ✅ Çalışıyor, testli |
| Kütüğü oymaya alma, 5 alet, iki vuruşla kesim | ✅ Testli |
| Modül ızgarası (65 cm), sabit snap noktaları, dolu yerin kapanması | ✅ Faz 1, testli |
| Çaprazlama yerleştirme + otomatik eyer çentiği | ✅ Testli |
| Kırmızı kesim çizgileri, çizgili önizlemeler | ✅ Var, görsel test edilmedi |
| Her parçayı taşıma (hacme göre omuz), yatır / dik / çaprazlama koyma | ✅ Faz 2, testli |
| Parçanın şeklinden sınıflandırılması | ❌ Tarif tuşları (1–7) kullanılıyor |
| Ateş | ❌ Yok |
| Direk, kiriş, temel | ❌ Yok |
| Destek ilişkisi, sökme | ❌ Kodda yapısal bütünlük sistemi var, bağlı değil, denenmedi |

## Fazlar

Her faz kendi başına oynanabilir bir sonuç verir. Her fazda grafiksiz otomatik teste yeni adımlar ve rehberli demoya yeni görevler eklenir.

### Faz 1 — Modül kesim ızgarası ✅
- `ACarvableLog` modül boyunu ve modül çizgilerinin başlangıcını taşır. Kesilen parçalar bu bilgiyi devralır.
- Enine kesim noktaları modül çizgilerine, çentik noktaları modül çizgisine oturur.
- Kütük üzerinde modül çizgileri soluk beyaz halkalarla görünür, kesim yapılacak olan kırmızı.
- **Test:** 6M kütük 2M'den kesilir → 2M + 4M. 4M parça yine aynı çizgilerden kesilir → 1M + 3M. Toplam boy korunur.

### Faz 2 — Her parça taşınabilir ✅
- Oyulabilir kütük de omza alınır; omuz kapasitesi parça sayısına göre değil, hacme (ağırlığa) göre olur. Bir tam kütük, iki yarım ya da altı 1M parça gibi.
- Yerleştirme: sol tık koyar. Yerdeki kütüğe bakınca çaprazlama (çentikli), zemine bakınca yatır ya da dik (R ile seçilir). E omza alır, elin doluyken bırakır.
- Yerdeki kütük ile oyulabilir kütük tek bir türde birleşir. "Oymaya hazırla" adımı kalkar.
- **Test:** Kesilmiş bir parça omza alınıp başka yere konur; şekli ve kesim izleri korunur.

### Faz 3 — Odun ve ateş ✅
- Parça sınıfı geometriden hesaplanır: boy (modül), kesit (bütün/yarım/çeyrek), çentik. HUD'da adı yazar.
- Tarif tuşları (1–7) kaldırılır; aynı sonuçlar kesimle elde edilir.
- **Şenlik ateşi:** Yere bir ateş yeri konur, etrafında snap noktaları belirir. Oraya konan odunlar yakılır. Yanma süresi odunların hacmine bağlıdır. Tam kütük de konabilir ama geç tutuşur ve uzun yanar.
- **Test:** 1M parça iki kez yarılır → 4 odun. Odunlar ateş yerine konur → ateş yanar, süre hacimle orantılı.

### Faz 4 — Temel ve iskelet ✅
- **Temel ayağı:** 1M–2M bütün parça dik konur, zemine oturur.
- **Temel kirişi:** Ayakların üstüne kütük yatırılır, otomatik çentikle oturur (bugünkü çaprazlama mantığı).
- **Direk:** 2M–6M parça dik konur.
- **Kiriş:** Direklerin tepesinde beyaz snap noktaları görünür; kiriş uçlarına çentik açılıp oturur.
- **Duvar sırası:** Temel kirişinin üstüne dizilen kütükler, köşelerde otomatik eyer çentiğiyle kilitlenir. Yarım çap kadar kaydırmalı sıralar oluşur.
- Bağlantının yönü beyaz okla gösterilir.
- **Test:** 4 ayak + 4 temel kirişi + 2 sıra duvar kurulur; bütün parçalar sabit kalır, köşelerde çentik açılır.

### Faz 5 — Destek ilişkisi ✅
- Her konan parça neye dayandığını kaydeder: zemin, ayak, kiriş, çentik.
- **Payanda:** Yarım kütük köşeye 45° snap noktasından çapraz konur.
- Bir direk sökülünce yük yeniden hesaplanır. Destek yetmezse parçalar düşer.
- Kodda var olan yapısal bütünlük sistemi bu kayıtlarla beslenir; önce denenip ayıklanması gerekir.
- **Test:** Payandalı bir platformun orta direği sökülür ve platform ayakta kalır. Payandasız olanı çöker.

### Faz 6 — Açıklık açma, sökme, yapı tanıma (tanıma ✅, açma yok)
- Kapı ve pencere, duvardaki modül çizgilerinden kesilir. Etkilenecek bölge turuncu sınırla gösterilir.
- Sökülen parça tekrar kullanılabilir hâlde geri döner; kesim izleri ve çentikleri korunur.

## Netleştirilecek kararlar

1. ~~Modül~~ → sabit 65 cm seçildi (Faz 1).
2. **Tarif tuşları (1–7):** Faz 3'te tamamen kaldırılsın mı?
3. ~~Omuz kapasitesi~~ → hacme göre seçildi: 1 tam kütük (Faz 2).
4. **Ağaç boyu:** Ağaçtan çıkan gövde şu an ağaçtan bağımsız sabit bir boyda (15,6 m). Ağacın gerçek boyuna ve kalınlığına bağlansın mı? Modül sistemiyle birlikte ele alınabilir.

## Durum notu (13 Ekim akşamı)

Fazlar 1–5 uygulandı, otomatik testle doğrulandı; ayrıntı ve sayılar `DURUM_RAPORU.md` içinde. Planda iki sapma:
- **Faz 3:** Parça sınıflandırması ayrı bir sistem olarak yazılmadı. Ateş yakıtı doğrudan hacimden, yapı rolleri geometri ve yönden (Wood paketinin kuralları) türüyor.
- **Faz 5:** Kiriş açıklık ve taşma kuralı eklendi (destekler arası en fazla 4,5 modül, taşma en fazla 2 modül); payanda bu kuralda anlam kazanıyor.
