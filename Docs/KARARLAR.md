# Kararlar

## Verilen kararlar

| Karar | Gerekçe |
|---|---|
| Greybox: sanatsız, kod odaklı. Mesh'ler script ile üretilir, elle düzenlenmez | Mekanikleri sanat bağımlılığı olmadan denemek |
| Tasarım ilkesi: **serbest kullanım, sabit kesim noktaları.** Bir kütükle ne yapılacağı oyuncuya kalır (odun, ateş, temel, duvar), tarif yok; kesimler yalnızca belirli noktalarda | Modülerlik: aynı noktalardan kesilen parçalar birbirine uyar |
| Görsel dil (Sons of the Forest): kırmızı kesikli çizgi = kesilecek yer, beyaz = bağla, hayalet yerine çizgi önizleme | Kullanıcı referans görselleri verdi |
| Modül = **65 cm** sabit (390 cm'lik kütük 6M). Izgara kütüğün devrildiği andaki bir ucundan sayılır, kesilen parçalara geçer | Farklı boydaki kütükler aynı ızgarayı paylaşır |
| Omuz kapasitesi **hacme göre** (1 tam kütük = 2 yarım = 6 tane 1M) | Kullanıcı seçti |
| Oyma derinliği çapın 1/6, 1/3, 1/2'si; oval ve düz çentik genişliği kütük çapı | Üstüne aynı boy kütük oturur |
| Çentik ve kesim yönü yalnızca üst/alt yüz (45° ara açı yok); kütük çevrilerek çalışılır | Baltayla gerçekçi sınırlar |
| Yapı destek hesabı Wood paketinin `WoodStructureLogic` mantığıyla; bizim kodumuz (`ACarvableLog`) ana sistem | Çalışan ve testli olan bozulmasın |
| Kiriş açıklık kuralı: destekler arası ≤ 4,5 modül, taşma ≤ 2 modül; aşılırsa kırılır | Payandaya anlam verir (SotF yapısı: payandalarla ara direk sökülebilir) |
| Hasar eşikleri: ≥ 0,6 eklemler gevşer, ≥ 0,85 hiçbir şey taşımaz | Wood paketindeki sabitler |
| Her mekanik değişikliği için `AMechanicsDemo` otomatik testine kontrol eklenir | Grafiksiz doğrulama |
| Dosyalar web Claude ile **zip + not** ile paylaşılır; tünel ve yazma yetkisi verilmez | Güvenlik, kota |
| Editör, uzun derleme ve test işlerinde kapalı tutulur, iş bitince `Demo_WoodChain` açık yeniden başlatılır | Derleme ve test aynı DLL'i kullanır |
| Ağaçtan çıkan kütük sayısı ağaç boyundan (boyun %85'i, en çok 4); tarif tuşları (1,2,4–7) yok, odun yalnızca kesilerek hazırlanır | Faz 9 |
| Taşlar tek tek konur, 7 taş halkası ateş yeri olur | Faz 8b |
| Hareket durumu Loose / Carried / Installed; sert çarpma kütüğe hasar verir | Faz 10 |
| Düşman duvarı yaparak hasar verir (yapıdaki hasar kuralları aynen); kazık ve çarpan kütük düşmana zarar verir | Faz 14 |
| **İnşa modu yok, her şey fiziksel montaj.** Hazır parça (prefab) koyan hiçbir mod yok: B inşa modu, kütük duvar planı, [3] taş blok tarifi, [T] mala ve admin [H] oda kaldırıldı. Çubuk da tek tek çakılır (kazık); çit, dar aralıklı kazıklardan kendiliğinden oluşur; meşale, yakılan kazıktır | Kullanıcı: "inşa modu neden var, physical assembly temelde bu oyun" (14 Ekim) |
| **Referans oyunlar (her birinden yalnızca bu alınır):** **Subnautica** — hikâye sinematiksiz, dünyadaki kalıntılarla anlatılır. **Frostpunk + The Long Dark** — her bölge zorlayıcı ama aşılabilir malzemeler sunar (ekipman/zorluk The Long Dark'a yakın); ertesi güne çıkmak için her gün seçim yapılır. **Çiftlik oyunları** — tarım, hayvancılık ve hayvanların verdikleri (deri vb.). **7 Days to Die** — günlük üsse karşı baskınlar, yeraltına inebilme, toprağı sıkıştırıp beton örme; voxel çok küçültülmüş hâlde, baskınla birlikte (tam voxel kazı değil). **Sons of the Forest** — kütük, taş ve çubukla yapılabilen bütün mekanikler, fiziksel montaj. **Death Stranding** — grafik/hikâye alınmaz; dünya gerçekten zorlu, keşfedilen yere köprü ve ip eklenince kolaylaşır: dünyayı medenileştirmek. Diğer oyunlar yalnızca benzerlik karşılaştırmasıdır | Kullanıcı açıkladı (14 Ekim) |
| Ateş yerleştirme modu değil, odunun özelliği: her parça yanabilir (tutuşuyor → yanıyor → yanık), yakıt = hacim (tam kütük ~593 sn), hasar yakıt bitince 1'e varır; yanan parça yanındakini ısıtır; aynı anda en çok 12 parça yanar | Faz 8 (13 Ekim planı) |
| Kamp ateşi 7 taşla kurulur ([C]), odun atmak ateşi yakmaz; çakmak ([F] basılı) ya da yanan odun yakar | Ham madde zinciri; iki ateşleme kaynağı kararı |

## Bekleyen kararlar

1. **İki kütük sistemi / eski yapı sistemi:** `ACarvableLog` ana sistem; Wood paketinin bileşenleri ve eski `StructuralIntegritySubsystem`/`LogConstructionActor` kullanılmıyor. Öneri: dondur, ileride sil.
2. **Yapılmayanlar (öneri sırasıyla):** (a) ses, (b) halat hattı ve su üstü taşıma, sal, (c) uzun parçayı sürükleme ve bir ucu kaldırma, (d) açıklık önizleme çizgisi ve açılır kapı, (e) düşman görseli ve yol bulma, otomatik gece baskını, (f) arabanın odundan yapılması, (g) kütük tuzağı mekanizması.
3. **Paketin kullanılmayan sistemleri** (Inventory, Stone, Stick, Farming, World, Material): derleniyor ama hiçbir şeye bağlı değil. Hangisi oyuna alınacak?
4. **Web Claude erişimi:** zip akışı kullanılıyor; zip'i otomatik entegre eden `Scripts/apply_zip.py` yazılsın mı?
