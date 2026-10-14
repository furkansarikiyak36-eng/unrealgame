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
| Ateş yerleştirme modu değil, odunun özelliği: her parça yanabilir (tutuşuyor → yanıyor → yanık), yakıt = hacim (tam kütük ~593 sn), hasar yakıt bitince 1'e varır; yanan parça yanındakini ısıtır; aynı anda en çok 12 parça yanar | Faz 8 (13 Ekim planı) |
| Kamp ateşi 7 taşla kurulur ([C]), odun atmak ateşi yakmaz; çakmak ([F] basılı) ya da yanan odun yakar | Ham madde zinciri; iki ateşleme kaynağı kararı |

## Bekleyen kararlar

1. **Tarif tuşları (1–7) ve envanter:** "1 kütük → 4 odun" gibi tarifler hâlâ var, fiziksel kesim fikriyle çelişiyor. Öneri: ahşap parçalar fiziksel kalsın, tarif tuşları kalksın, B inşa modu yalnızca taş gibi ahşap olmayanlar için kalsın.
2. **Gövde boyu:** Devrilen ağaçtan hep 15,6 m × 95 cm gövde çıkıyor; 11 m'lik ince çamdan bile. Öneri: ağacın boyuna ve kalınlığına bağlansın, 65 cm'nin katına yuvarlansın.
3. **İki kütük sistemi:** Wood paketinin bileşen sistemi (`WoodPieceComponent` vb.) ile `ACarvableLog` yan yana. Öneri: `ACarvableLog` ana sistem kalsın, Wood'dan yalnızca mantık kullanılsın.
4. **Eski yapı sistemi** (`StructuralIntegritySubsystem`, `LogConstructionActor`, 🔵 denenmemiş): yeni destek hesabıyla aynı işi yapıyor. Öneri: dondur.
5. **Sıradaki iş (Faz 8 bitti, kalanlar):** (a) oyuncunun kurabilmesi için eğik parça yerleştirme, platform üstüne direk, geçici işaretleme; (b) kapı/pencere açma; (c) lojistik (el arabası, kızak); (d) ağaç boyu bağlama.
6. **Web Claude erişimi:** zip akışı kullanılıyor; zip'i otomatik entegre eden `Scripts/apply_zip.py` yazılsın mı?
