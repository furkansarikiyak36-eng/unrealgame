# PerfTest 50×50 m — maliyet raporu (2026-10-19)

Makine: i7-11800H, 16 GB RAM, RTX 3060 Laptop 6 GB. Editör içinde ölçüldü (paketli oyunda editör yükü olmaz, sonuçlar biraz daha iyi çıkar).
Ayarlar: Lumen (yazılım, global distance field), VSM gölge, Nanite açık, tam çözünürlük + TSR (oturumluk), Volumetric Cloud.
Hedef: 60 FPS = kare başına 16,7 ms.

## 1. Kısa sonuç

- Sahneyi en çok **boş sahnenin kendisi** zorluyor: hiçbir bitki yokken GPU 14,5 ms (bütçenin %87'si).
- Bitkiler içinde en pahalısı **çim**. Sayıdan çok, her çimin gölgeye ve Lumen'e girmesi pahalı.
- **Ağaç ve çalı ucuz** (12 ağaç +1,5 ms, 40 çalı +0,6 ms), ama Spindle çalısının üçgen sayısı tehlikeli derecede yüksek.
- **Mekanikler (oyun kodu) zorlamıyor**: oyun sırasında game thread ~8 ms, bunun çoğu editörün kendisi.
- Sınır RAM'de değil GPU'da. RAM editörde sıkışık (16 GB'ın ~2 GB'ı boş) ama kareyi yavaşlatan o değil.

Şu anki hali (çim 2/m², sabit; 12 ağaç; 40 çalı): alanın içinde **60 FPS**, GPU 15,6 ms. Pay neredeyse yok.

## 2. Katman katman ölçüm (alanın içinde, kamera yerde)

Ölçüm çim 3/m² iken aynı turda alındı (katmanlar gizlenip açılarak).

| Katman | Adet | GPU (ms) | Ek maliyet |
|---|---|---|---|
| Boş sahne (gök, ışık, Lumen, zemin) | – | 14,5 | – |
| + Kikuyu çimi (3/m², rüzgârsız) | 7.484 | 19,3 | **+4,8** |
| + Megaplant ağaçları | 12 | 20,8 | +1,5 |
| + Spindle / Elderberry çalıları | 40 | 21,3 | +0,6 |

Havadan bakınca tüm katmanlar toplam yalnız +0,6 ms (60 FPS). Maliyet "kameranın içinde olduğu" yerde.
VRAM: üç katman birlikte ~30 MB. Ölçümler arası ±3-5 ms oynama var (editör belleği yer yer diske taşıyor).

### Çim yoğunluğu (Kikuyu, alanın içinde, kare süresi)

| Yoğunluk | Çim | Kare (ms) | Not |
|---|---|---|---|
| 12/m² rüzgârlı | 29.929 | ~39 | |
| 9/m² | 22.500 | 35 | |
| 4,5/m² | 11.253 | 26,5 | |
| 3/m² rüzgârlı | 7.484 | 26,6 | sabit maliyet görünüyor |
| 3/m² rüzgârsız | 7.484 | 21,8 | rüzgâr −4,8 ms |
| 4/m² rüzgârsız (+ağaç, çalı) | 10.000 | 19,7 | |
| **2/m² rüzgârsız (+ağaç, çalı)** | **4.981** | **16,7** | **şu anki** |

## 3. Boş sahnenin GPU dökümü (tek kare, ProfileGPU)

| Geçiş | ms | Ne |
|---|---|---|
| Lumen (GI + yansıma, ayrı kuyrukta) | ~9 | En büyük kalem |
| Volumetric Cloud | 2,3 | 3D bulutlar; düz gökyüzüyle ~0 |
| Gölge, Nanite olmayan mesh'ler (VSM) | 1,8 | Çim ve çalılar |
| Lumen kart yakalama | 1,3 | Çim Lumen'e giriyor |
| TSR (kenar yumuşatma) | 1,2 | |
| Velocity (rüzgârlı çim) | 1,1 | Rüzgâr kapatılınca gider |
| Base pass (asıl boyama) | 0,7 | Ucuz |

### 3b. Boş sahne yeniden ölçüldü (editör yeniden başlatıldıktan sonra, bitkiler gizli)

İlk 14,5 ms ölçümü şişmiş editörde alınmıştı (belleğin çoğu diske taşmıştı). Temiz editörde:

| Ayar (kümülatif) | GPU, yerde (ms) | VRAM (GB) |
|---|---|---|
| Varsayılan: viewport %50, AA yok, hepsi Epic | 7,8 | 2,82 |
| + Volumetric Cloud kapalı | 6,2 | 2,81 |
| + Yansıma ve gölge kalitesi High (GI High) | 5,5 | 2,41 |
| + GI kalitesi Medium | 5,0 | 2,30 |
| + Lumen tamamen kapalı (karşılaştırma) | 5,0 | 2,12 |
| Tam çözünürlük + TSR, hepsi Epic | 9,7 | 2,90 |
| %67 + TSR, bulut kapalı, GI/yansıma/gölge High | **5,9** | 2,48 |

Editör viewport'u küçük (~1055×647), o yüzden çözünürlüğün etkisi burada küçük; 1080p tam ekran oyunda daha büyük olur.
Lumen'i "High"a indirmek ile tamamen kapatmak arasında bu sahnede ~0,5 ms fark var: Lumen'i kapatmaya gerek yok, kalitesini indirmek yeterli.

### 3c. Kalite seviyeleri, tam sahne (çim 2/m² + 12 ağaç + 40 çalı, viewport %50, AA yok)

| Kalite (tüm sg.* grupları) | GPU, yerde (ms) | Render thread | VRAM (GB) | Havadan GPU |
|---|---|---|---|---|
| Epic (şu anki) | 16,3 | 16,2 | 2,82 | 5,6 |
| High | 10,9 | 7,1 | 2,41 | 5,3 |
| Medium | 9,4 | 6,7 | 2,43 | 5,3 |
| Low | 5,4 | 3,8 | 2,39 | 4,7 |

Epic → High en büyük kazanç (−5,3 ms). High → Medium yalnız −1,6 ms.
UE 5.8'de Medium Lumen GI'yi kapatmıyor (daha düşük kart çözünürlüğü, daha seyrek prob); Lumen yansımalarını kapatıp SSR'a geçiyor,
VSM sayfa havuzunu 512'ye ve gölge mesafesini %70'e indiriyor, `foliage.DensityScale=0.4` (PCG/HISM çimi etkilemez).
Low: Lumen GI ve yansımaları kapalı, distance field AO/gölge kapalı, gölge kalitesi 0 (512 px, tek kaskad, mesafe %60), volumetric fog kapalı, malzeme kalitesi Low, SSS kapalı, doku mip bias, iskelet LOD bias 2, görüş mesafesi %40. Görüntü düz ve soluk: çim altı gölgeler neredeyse yok, zemin ve çim açık renkli.
Görüntü farkı küçük: Medium'da çim altı gölgeler hafifliyor, orta mesafede yansıma kaynaklı gri-mavi lekeler artıyor, bulutlar biraz sadeleşiyor.

### 3d. High üstüne ek kısıntılar (tam sahne, kümülatif, yerde GPU)

| Adım | GPU (ms) | Kazanç |
|---|---|---|
| High | 12,2 | – |
| + Volumetric Cloud kapalı | 8,5 | **−3,7** |
| + çim gölgesi kapalı | 8,2 | −0,3 (görüntüde çim havada durur) |
| + çim cull 25 m | 8,1 | ~0 (alan 50 m, çimin çoğu zaten yakında) |
| + Lumen yansıma yerine SSR | 8,1 | ~0 |

2/m² çimde, çim gölgesi, cull ve yansıma artık kayda değer kazanç vermiyor. Tek büyük kalem bulutlar.
Cull, alan büyüyünce (yüzlerce metre çim) önemli hale gelir.

### 3e. VRAM dökümü (PerfTest, High, `rhi.DumpResourceMemory all`)

Kart: 6.144 MB, kullanılan 2.722 MB (Unreal + Windows), boş 3.273 MB. Kartta yalnız Unreal ve Epic Launcher var (tarayıcılar dahili GPU'da).
Unreal: 2.540 MB; bunun 1.753 MB'ı kalıcı kaynak (2.146 adet), kalanı kare başına geçici render hedefleri.

| Kalem | MB | Ne |
|---|---|---|
| Gölge (VSM sayfa havuzu + HZB) | 344 | Sabit havuz; High = 2048 sayfa |
| Lumen (sahne kartları, radiance cache) | 322 | Sabit; kart atlası boyutuna bağlı |
| Nanite akış havuzu | 280 | Sabit; ini'de 256 MB'a ayarlı |
| Virtual Texture havuzları | 257 | Megascans dokuları buradan akıyor |
| Distance Fields (Lumen için) | 194 | Global DF sayfa atlası |
| Diğer render hedefleri, motor/editör dokuları | 180 | Slate, parçacık, gökyüzü |
| Asset dokuları (VT dışı) | 85 | Megascans 44, gökyüzü 16, bulut 16, ağaçlar ~23 |
| Mesh tamponları (vertex/index) | 56 | 1.270 tampon |
| GPUScene / instance verisi | 20 | |

Sonuç: VRAM'in ~%77'si motorun sabit havuzları; sahnenin içeriği (doku + mesh) 0,4 GB'ın altında.
Unreal'ın kullanabileceği ~5,2-5,6 GB'a göre ~2,5-3 GB içerik payı var. Gerekirse havuzlar küçültülebilir:
VSM 1024 sayfa (−128 MB), Lumen kart atlası 2048 (~−150 MB), VT havuz ölçeği, Distance Field çözünürlüğü.

## 4. Tüm bitkiler — üçgen sayıları

### Çim: Megascans Kikuyu Grass (PCG, Nanite yok, 4 LOD, gölge açık, cull yok)

| Varyant | Adet | LOD0 | LOD1 | LOD2 | LOD3 (billboard) |
|---|---|---|---|---|---|
| VarC | 580 | 5.291 | 3.026 | 629 | 7 |
| VarG | 611 | 4.712 | 2.441 | 471 | 7 |
| VarF | 626 | 4.552 | 2.491 | 463 | 9 |
| VarE | 637 | 2.805 | 1.674 | 347 | 5 |
| VarD | 606 | 2.676 | 1.524 | 330 | 6 |
| VarH | 664 | 2.630 | 1.479 | 411 | 8 |
| VarA | 621 | 2.007 | 1.252 | 206 | 6 |
| VarB | 636 | 1.425 | 905 | 135 | 6 |

Toplam 4.981 çim. Hepsi en detaylı halde olsa ~16 M üçgen; LOD sayesinde gerçekte çok daha az.
LOD3 billboard'ları ~15-20 m'den sonra soluk beyaz blok gibi görünüyor.

### Çalılar (HISM, Nanite yok, 3 LOD, gölge açık, cull yok)

| Mesh | Boy | Adet | LOD0 üçgen | LOD1 | LOD2 |
|---|---|---|---|---|---|
| European Spindle VarC | 287 cm | 5 | **154.303** | 22.943 | 21 |
| European Spindle VarD | 275 cm | 6 | **81.808** | 14.620 | 18 |
| European Spindle VarE | 258 cm | 3 | 51.181 | 8.275 | 22 |
| European Spindle VarB | 223 cm | 4 | 43.737 | 6.577 | 21 |
| European Spindle VarA | 175 cm | 2 | 16.710 | 2.775 | 16 |
| European Spindle VarJ | 158 cm | 4 | 14.760 | 2.462 | 19 |
| Elderberry VarA | 119 cm | 1 | 11.270 | 3.150 | 11 |
| European Spindle VarI | 111 cm | 3 | 9.488 | 1.116 | 18 |
| Elderberry VarF | 85 cm | 4 | 4.364 | 1.177 | 8 |
| Elderberry VarD | 89 cm | 3 | 3.211 | 883 | 9 |
| Elderberry VarC | 91 cm | 3 | 3.084 | 851 | 9 |
| European Spindle VarH | 87 cm | 2 | 2.084 | 422 | 13 |

40 çalının LOD0 toplamı ~1,8 M üçgen; gölgeyle iki kez çiziliyor. Şu an 40 tane olduğu için +0,6 ms;
**yüzlerce Spindle konursa** ilk zorlanacak yer burası. Spindle VarC/VarD için Nanite açmak ya da LOD1'i zorlamak gerekir.

### Ağaçlar: Megaplant (skeletal + Nanite assembly, gölge açık)

| Ağaç | Boy | Nanite üçgen | Kemik | Kesilebilir |
|---|---|---|---|---|
| European Aspen 01 C | 7,7 m | 6,20 M | 698 | evet |
| European Aspen 01 B | 10,9 m | 6,02 M | 528 | evet |
| Rowan Sapling 01 D | 5,3 m | 4,34 M | 334 | evet |
| Japanese Cypress 01 E | 3,9 m | 3,60 M | 44 | hayır |
| Rowan Sapling 01 A | 3,0 m | 2,31 M | 184 | hayır |
| Baltic Pine Sapling 01 B | 5,4 m | 1,99 M | 825 | evet |
| Dead Pine D (×2) | 13,4 m | 1,72 M | 73 | evet |
| Norway Maple Sapling 01 A | 3,0 m | 0,10 M | 564 | hayır |
| Norway Maple Sapling 01 B (×3) | 1,9 m | 0,07 M | 333 | hayır |

Kaynak toplam ~28 M Nanite üçgen; Nanite ekranda gerekeni çizdiği için 12 ağaç yalnız +1,5 ms.
Asıl ağaç maliyeti **RAM**: her Megaplant ağacı ayrı actor ve kendi kemik verisi var.
Daha önce 500 ağaç editör RAM'ini +3,5 GB artırdı; bu PC'de editörde ~500-700 ağaç sınır.
(Üçgen sayıları ağaçlar indirildiği günkü taramadan; parça azaltması sonrası biraz farklı olabilir.)

## 5. Mekanikler zorluyor mu?

- Oyun sırasında (PIE) game thread 8,1 ms, editörde boştayken 7,3-8,4 ms: oyun kodunun payı ~1 ms.
- 13 tick'leyen dünya alt sistemi var (yanma, gün döngüsü, yapı sağlamlığı, olaylar, ağaç kesme...). Ölçülen toplam etkileri küçük.
- Hayvanlar (`AnimalActor`) tick etmiyor. Ağaç kesme yalnız vuruşta çalışıyor.
- Sonuç: **mekanikler değil, görüntü (GPU) zorluyor**.

## 6. Öneriler (etkisi büyükten küçüğe)

1. **Volumetric Cloud yerine düz gökyüzü**: ~2,3 ms, çok az görsel kayıp.
2. **Çimin gölgesini kapat, Lumen dışına al, cull 20-25 m**: ~3 ms; beyaz billboard'lar da kaybolur.
3. **Zemini yeşil doku yap** (Megascans Wild_Grass yüzeyi hazır): 2/m² çim halı gibi görünür, maliyet ~0.
4. **Lumen kalitesini bir kademe indir** (scalability "High"): Lumen ~9 ms'nin bir kısmı.
5. **Spindle VarC/VarD'ye Nanite aç** ya da bu iki varyantı kullanma: çalı sayısı artınca önemli.
6. **Ağaçları az ve görevli tut** (kesilecek odun kaynağı): RAM sınırı ağaç sayısında.
7. **Editörü uzun oturumlardan sonra yeniden başlat**: silinen test katmanları belleği şişiriyor, ölçümler oynuyor.
