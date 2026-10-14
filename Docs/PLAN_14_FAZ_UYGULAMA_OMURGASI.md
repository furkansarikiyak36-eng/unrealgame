# MyProject2 — 14 Fazlık Uygulama Omurgası

**Hedef:** Faz 0–14'ü kanıtla kapatmak. Kullanıcıya ara demo paketi verilmez; tek Windows demo, Faz 15'te bütün kapılar geçildiğinde hazırlanır.  
**Motor:** Unreal Engine 5.8  
**Çalışma kuralı:** Her faz ayrı kapsam, branch/commit, otomatik test, editör doğrulama checklist'i ve kabul kanıtı üretir. UE/Windows gerektiren kanıtlar uygun hostta çalıştırılmadan "geçti" işaretlenmez.

## Fazlar ve tamamlanma kanıtı

| Faz | Başlık | Zorunlu çıktı | Kabul kapısı |
|---|---|---|---|
| 0 | Baseline, katalog ve sahiplik denetimi | ID bazlı gereksinim kataloğu; kaynak/çağrı zinciri; U0–U6 durum; eski test ve gerçek test ayrımı | Kapsamdaki her satır bir sahibi, gerçek davranış durumu, test/kanıt ve açık riske sahip |
| 1 | Kazı teknolojisi spike'ı | En az iki hacim/mesh yaklaşımının aynı çukur+tünel+oda+chunk-sınırı+save/load senaryosunda karşılaştırması | Ölçüm tablosu, UE 5.8 derleme/çarpışma/kayıt kanıtı ve gerekçeli teknoloji kararı |
| 2 | Core contract ve kalıcı dünya verisi | Kanonik kimlik, sahiplik, olay, işlem/idempotency, sparse delta ve sürümlü kayıt sözleşmeleri | Çift otorite yok; eski kayıt/bozuk veri/tekrarlanan olay testleri geçer |
| 3 | Kazılabilir arazi | V1 iç test haritası; lokal hacim değişimi; chunk seam; collision; kalıcı delta | Dikey çukur, yatay tünel, oda, seam, tekrar kazı/doldurma, save/load testleri |
| 4 | Yeraltı yapısı ve tehlikeleri | Destek/çökme; toprak-kaya profili; su girişi; tuzak ve tünel etkileşimi | Aynı geometri ve dünya durumunda deterministik sonuç; kayıt sonrası durum korunur |
| 5 | Kütük sistemini sağlamlaştırma | Mevcut `ACarvableLog` akışının regresyon kilidi; eksik yapı/geri kazanım/hasar senaryoları | Mevcut testler korunur; fiziksel oyun içi doğrulama checklist'i tamamlanır |
| 6 | Taş, çubuk ve envanter entegrasyonu | Fiziksel öğe→taşıma/stok→işleme→inşa→kurtarma tam zinciri | Başarı, dolu envanter, spawn hatası, iptal, drop ve save/load'da kayıp/çoğalma yok |
| 7 | Yapı grafı ve materyal simülasyonu | Tek kanonik yapı/bağlantı grafı; materyal durumu; yangın/nem/darbe/çürüme adaptörleri | Etki yalnızca bir kez uygulanır; fiziksel aktör ve dünya hücresi arasında çift sahiplik yok |
| 8 | World State | Su, toprak, nem, biyokütle, yangın, çürüme ve kimya; olay/lazy simülasyon | Chunk komşuluğu ve zaman atlama deterministik; kalıcı kayıt ve sınır testleri geçer |
| 9 | Tarım ve hayvancılık | Ekim→büyüme→hasat; yem→hayvan durumu→ürün/üreme/gübre→toprak | Üretim girdileri/çıktıları ortak stokta; aç/kapat/zaman atlatma sonuçları tutarlı |
| 10 | Bölge ve lojistik | Bölgesel kaynak/risk; patika, yol, köprü, rampa, kızak, halat/makara | Yük ve rota maliyeti ölçülebilir değişir; altyapı hasarı ve save/load çalışır |
| 11 | Oyuncu üretim zincirleri | Kütük→testere→tahta/kereste/yan ürün; stok, kapasite, kuyruk ve güç | Darboğaz, durdurma/devam, giriş tüketimi/çıkış üretimi ve kalıcılık uçtan uca geçer |
| 12 | Raid Director | Tek spawn sahibi; scheduler, zorluk, plan, navigasyon, hedef ve ortak yapı hasarı | Baskın başlat→dalga→sonuç/iptal uçtan uca; çift spawn yok; yıkılan engel sonrası rota yenilenir |
| 13 | Bölge hikâyesi ve atmosfer | En az üç mekanikçe ayrışan bölge; harabe/iz/nesne temelli çevresel anlatı | Keşif oyuncuya işe yarar bilgi verir; bölge farkları rota/kaynak/tehdit kararlarını etkiler |
| 14 | Performans, regresyon ve içerik kilidi | Uzun oturum, kayıt, kazı yükü, AI/üretim yükü, profil ve bilinen sınırlamalar raporu | Kritik blocker yok; release adayı özellikler U6; performans hedefleri ölçümle raporlanmış |

## Kanıt seviyeleri

- **U0:** Denetim gerekli.
- **U1:** Veri/model var.
- **U2:** Yerel davranış var.
- **U3:** Oyuncu girdisinden dünya sonucuna entegrasyon var.
- **U4:** Olumlu/olumsuz/sınır otomatik testleri var.
- **U5:** UE editöründe/oyunda görsel ve oynanış doğrulaması var.
- **U6:** Save/load, uzun oturum ve hedef donanım performans doğrulaması var.

Sınıf/dizin varlığı tamamlanma kanıtı değildir. Headless test U5 yerine geçmez; plan, mock veya Python referans testi UE geometri/collision kanıtı yerine geçmez. Her kabul kaydında commit SHA, test komutu, sonuç logu, host/UE sürümü ve bilinmeyenler yazılır.

## Tek kullanıcı teslimatı

Faz 0–14 ve V1–V4 iç doğrulama senaryoları tamamlanınca Faz 15 başlar: tek Windows packaged build, temiz başlatma, 30–60 dakika senaryo, iki save/load çevrimi, kontroller/README, test logları, performans ve bilinen sınırlamalar raporu. Ara fazlarda kullanıcıya paket teslim edilmez.
