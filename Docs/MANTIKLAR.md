# Oyunun inşa mantıkları — şu an kodda olan kurallar

Kaynak: kodun kendisi (2026-10-25, `test/chatgpt-world-snap` çalışma kopyası). Her bölümün sonunda **Eksik**: bilinen boşluklar. Kabul ID'leri: [DILIM_SOTF.md](DILIM_SOTF.md).

## 1. Ölçü sistemi
- Modül 75 cm. Ham log boyları 600 / 300 / 150 / 75 (8 / 4 / 2 / 1 modül). Çap yaklaşık 35–36 cm.
- Profiller (her boy için ayrı mesh, `SM_RawLog_<boy>_<profil>`): yuvarlak, yarım, çeyrek, bir yüzü düz, iki yüzü düz, kalas, tahta, oluklu, yalak, çentikli.
- **Eksik:** 3/4 boy (450) yok. Ölçüler dışında boy yok (ör. 200).

## 2. Ağaç ve log işleme
- Ağaç devrilir; boya göre: ≥18 m → 600 + 300, ≥10 m → 600, değilse 300. Devrilme çizgisinde uç uca + dal çubukları (`UTreeFellingSubsystem`).
- Ham log baltayla, **vurulan yere göre** işlenir (`RawLogShapes`). Her işlem 2 vuruş:
  - Ortaya vur (boyun orta %40'ı) → enine kes: L → 2 × L/2. 75 daha kesilmez.
  - Uca vur → boyuna yar: yuvarlak → 2 yarım → çeyrek; kalas → 4 tahta.
  - Yana vur (ortadan uzak) → düzle: yuvarlak → 1 yüz düz → 2 yüz düz → kalas.
  - [R] sonra üste vur → oluk → yalak.
- Ekranda vuruş noktası: renkli kare + işlem adı (kırmızı enine, mavi boyuna, turuncu düzle, yeşil oluk, gri olmaz + sebep).
- Ham olmayan (oyulmuş) loglarda serbest oyma araçları: çentik, enine kes, yar, yuvarlak/düz çentik, sivri uç, pah, düzelt, kırlangıç, oluk; kesim kılavuz çizgisi gösterilir.
- 1 log → 4 yakacak (`CraftFirewood`).
- **Eksik:** ham loga sivri uç (kazık); 3/4 kesim; inşa sırasında otomatik boy kesme; işlem sonrası parça sayısı/hacim kontrolü (test yok).

## 3. Taşıma
- Omuz kapasitesi = **bir 300'lük log** hacmi (veya 2 × 150, 4 × 75). Daha uzunu (600) omuza alınmaz, iple sürüklenir.
- Birden çok parça omuzda üst üste durur; yük hızı düşürür: hız = 340 − 120 × yük.
- [E] al / bırak, sol tık koy, [R] yatır ↔ dik ↔ eğik, tekerlek eğim açısı (15–75°).
- Araç (`AHaulCart`): el arabası 2, kızak 4, raf 8 omuz yükü; yalnız odun. Sabitlenmiş (yapıdaki) parça yüklenmez.
- **Eksik:** araçta taş/çubuk; taş kızağı, zipline (sınıflar var, bağlı değil); su üstünde taşıma.

## 4. Yerleştirme — karar sırası
Taşınan log için, baktığın şeye göre bu sırayla denenir (ilk tutan kazanır):
1. Araç (araba/kızak/raf) → yükle.
2. Hasarlı parçaya bakıyorsan ve elindeki ondan sağlamsa → **değiştir** (eskisi omuza gelir).
3. Sabit bir direğin tepesi → **kiriş** (ucu direkle hizalı, aynı boydaki başka direğe doğru).
4. Kütük yüzüne bakıyorsan (`LogSnapSystem`, 75 cm çizgilerinde snap noktaları):
   - **Ön/arka yüz:** yanına paralel (değerek), dik T, 45°.
   - **Alt yüz:** altına paralel, modül ızgarasında.
   - **Uç yüz:** uca uç uzatma (düz dayama, boylar toplanır), ucuna dik T, ucundan 45°, ucuna direk.
   - **Dikey loga (direğe):** direk üstüne direk, direk tepesine kiriş, direk yanına direk/kiriş.
5. Dik/eğik tutuyorsan ve yatan logun üstüne bakıyorsan → **direk diker** (logun üstüne yuva açılır) ya da **çatı kirişi** (önündeki daha yüksek paralel loga yaslanır, açı ikisine oturacak şekilde; yoksa seçili açı).
6. Kesilmiş (yarım/tahta) parça → **döşeme** (kirişe dik, 30 cm aralık).
7. Yatan logun üstü → [G] moduna göre: **düz** (üst üste duvar sırası) / **45°** / **90°** (çapraz ya da son modüle bakınca **köşe geçmesi**) / **yığın** (iki değen logun arasındaki oyuğa). Seçilen olmazsa komşu seçeneğe düşer.
8. Payanda: direk + kiriş varsa, 150–300 cm parça 45° ile ikisine oturur.
9. Hiçbiri değilse → **yere yatır** (bakışa dik; yerde paralel komşuya yapışır).
- [G] (log taşırken): düz → 45° → 90° → yığın. Taşımıyorken admin malzeme tuşu.
- Önizleme ile koyma aynı kararı kullanır.
- **Eksik:** iki uçlu yerleşim (açının iki dayanma noktasından hesaplanması: direk tepesi ↔ zemin/log); bir ucu kaldırıp altına destek; bağlantı yönü oku; korkuluk; gerçek merdiven (taşıyıcı + basamak); dolu noktaya/içinden geçen yerleşimin reddi kısmen var.

## 5. Geçme ve ölçü kuralları (birbirine girme)
- Üst üste duvar sırası: üstteki, alttakine ince yarıçapın ¼'ü kadar gömülür; ucu modül çizgisinde.
- Direk ↔ log, kiriş ↔ direk: birbirine `0,95 × küçük yarıçap` kadar oturur; yuva kesilir.
- Çapraz (T, 45°): her iki log da bindirmenin yarısı kadar çentiklenir (en az 0,5 R).
- Köşe geçmesi: düz çaprazdan `0,2 × R` daha derin (uçlarda end grain boşluğu az); log uçları köşeden yarım modül taşar.
- Döşeme tahtası: kirişe `0,6 × kiriş yarıçapı` gömülür, aralık `max(tahta genişliği, 30 cm)`.
- Uca uç: kesim yok, düz yüzler değer.
- **Eksik:** uca uç için geçmeli eklem (bindirme istenmedi; kilitli uç fikri açık).

## 6. Destek ve çökme (`StructureLink`, `WoodStructure`)
- Her yerleştirme ve sökmeden sonra yapı yeniden değerlendirilir; desteksiz kalan parçalar serbest kalıp düşer ("N parça düştü").
- Zeminle temas, üstüne oturduğu log (yatak boyunca her modülde destek), direk, payanda destek sayılır.
- **Bir sağlam destek yeter** (yatay kiriş/döşeme/çatı için de): diğer uç serbest kalabilir.
- **Tek ucundan tutan kirişin boşta kalan ucu yere sarkar**: destekten 75 cm'den fazla taşan uç, destek noktasında dönerek zemine (ya da altındaki şeye) oturur ve orada kalır. Boy yere yetmiyorsa (kısa log, yüksek direk) olduğu yerde durur. İki yanı da taşıyorsa (ortadan dayanmış) dengededir, yerinde kalır.
- Eğik yatan logun üst ucunu tutan şey (direk, kiriş) sökülürse log düşer. Hiç tutulmamış eğik log (oyuncu yere eğik koymuş) düşmez; altına destek konunca tutulu sayılır.
- Yerleştirme anında kırma yok; önizleme "desteksiz" uyarısı verir ama koymaya izin verir.
- Duvardan açıklık kesilince (2 modül) aradaki 75'lik parçalar düşer, kalanlar değerlendirilir.
- Yan yana bağlanan yatay log'un ucu direğin içine girer (0,95 × küçük yarıçap), dik (T) ve 45° için.
- **Her mekanik her boyla çalışır** (75, 150, 300, 600): vitrin istasyon 16 yüz bağlantıları, üst üste, direk, direk üstü kiriş, direğe dik/45° log ve yaslanma için dört boyu ayrı ayrı sınar.
- **Eksik:** taş destek sayılmıyor (`StructureLink` taşı tanımıyor). Yük miktarı yok, sadece var/yok.

## 7. Hasar ve onarım
- Log hasar alır (`AddDamage`); bağlantı belli hasarda gevşer ve destek vermez. Yanar → kömürleşir.
- Onarım = hasarlı parçanın yerine daha sağlam, aynı türde parça koymak; eskisi omuza gelir.
- **Eksik:** hasar oyuncuya sadece admin tuşuyla veriliyor (oyunda hasar kaynağı yok); taşa hasar kodu var, bağlı değil.

## 8. Taş
- Haritadaki taşlar oyunda: < 30 cm küçük taş → çantaya giren eşya; ≥ 30 cm taş, duvar taşı, arnavut taşı → fiziksel taş (iki elle, 3–75 kg, boyuta göre).
- [E] al; sol tık önizleme, tekerlek hangi yüzün oturacağı, sol tık onay (`StonePlacement`).
- Kaya kırma zinciri: büyük → 3 orta (50–75 cm) → keskiyle duvar taşı; yapı taşı → arnavut taşı (gri/kahverengi).
- 7 ateş taşı 110 cm içinde → kamp ateşi kurulur.
- **Eksik:** taş destek grafiğinde yok; taş duvar/zemin/sütun için kural ve test yok; taş kiriş, taşta açıklık yok; taş şomine tarifi bağlı değil; taş şekil tanıma yok.

## 9. Çubuk
- Çubuk toplanır (çantaya); çubuk modunda çubuk dikilir (`CraftStake`), alan/çit tarifleri.
- Çubuk kamp ateşine yakıt.
- **Eksik:** dikili çubuğu sivriltme, taşla güçlendirme, yakın çubukların çit hattı oluşturması, meşale.

## 10. Şekil tanıma
- Ahşap yapılar adıyla tanınır (`DescribeFeaturesOf`): yatay/dikey duvar, sivri kazık duvarı, savunma duvarı, geçit, kapı/pencere kasası, platform, çatı iskeleti/kaplaması, rampa, merdiven, köprü, korkuluk, iskele, tezgâh. HUD odaklanılan parçanın yapısını yazar.
- İşlev olarak yalnız taş halkası → kamp ateşi.
- **Eksik:** tanınan adın işlev koşulu (merdivende çıkılabiliyor mu), taş yapı tanıma, elle şömine.

## 11. Zemin ve harita
- Voksel kazılabilir zemin; `NoDiggableGround` etiketli bir aktör varsa o haritada kapalı.
- Haritaya konan bazı statik mesh'ler Play'de oynanabilir nesneye çevrilir: `SM_RawLog*` → log, taş mesh'leri → taş, `SM_GB_Tree/Rock` → kesilebilir ağaç/kırılabilir kaya. Tanıma mesh **adına** göre.
- **Eksik:** ada göre tanıma kırılgan (paket değişirse sessizce bozulur), tablo/etiket ile yapılmalı.

## 12. Kayıt
- **Eksik (en büyük risk):** oyuncunun kaydet/yükle yolu yok; oyunun kullandığı log, taş ve taş yapı kayıtta değil.
