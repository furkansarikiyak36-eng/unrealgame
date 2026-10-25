# Final Demo Teslim Kararı

**Karar durumu:** Kabul edildi  
**Tarih:** 14 Ekim 2026  
**Repo:** `furkansarikiyak36-eng/unrealgame`

## Karar

Kullanıcıya verilecek tek oynanabilir demo, **Faz 0–14 tamamlanıp kabul kapılarından geçtikten sonra Faz 15'te** hazırlanır ve teslim edilir.

Kullanıcıya ara fazlarda D1, D2, D3 veya D4 adıyla ayrı packaged demo teslim edilmeyecek. Bu eski ara demo hedefleri geliştirme planında iç doğrulama senaryolarına dönüştürülmüştür:

- **V1 — Kazı iç doğrulaması:** çukur, yatay tünel, oda, chunk sınırı, çarpışma ve save/load.
- **V2 — Fiziksel kaynak zinciri iç doğrulaması:** toplama, taşıma, envanter/depo, işleme/crafting, yapıda kullanım ve geri kazanım.
- **V3 — Üretim/lojistik iç doğrulaması:** rota, köprü/kızak/halat seçeneklerinden uygun olanı ve kereste üretim zinciri.
- **V4 — Entegre senaryo iç doğrulaması:** kazı, yapı, üretim, tarım/hayvancılık, çevresel hikâye ve Raid Director aynı test dünyasında.

Bu iç doğrulamalar otomasyon testi, editör test haritası veya geliştirici içi build biçiminde olabilir. Bunlar ara faz teslimatı değil, fazın tamamlanma kanıtıdır.

## Faz 15 final teslimatı

Final Windows paketi ancak aşağıdakilerin hepsi tamamlandığında release adayı olur:

1. Faz 0–14 kabul kapıları geçmiştir; katalog maddeleri için durum ve kanıt kaydı vardır.
2. Build ve otomasyon testleri taze loglarla doğrulanmıştır; test scripti çıkış kodlarını kontrol eder.
3. Görsel/editör testi, uzun oturum ve performans testi tamamlanmıştır.
4. Kazı, yapı, fiziksel kaynaklar, üretim/lojistik, tarım/hayvancılık, bölgesel keşif ve baskın tek uçtan uca senaryoda birlikte çalışır.
5. Kazı, dünya durumu, yapılar, envanter/stoklar ve üretim durumu kaydetme/çıkış/yeniden yüklemeden sonra korunur.
6. Windows packaged build, başlatma talimatları, kontroller, bilinen sınırlamalar ve kullanıcı kabul checklist'iyle sunulur.
7. Build, kaynak commit SHA ve test kayıtları aynı sürümle eşleştirilir.

## Değişmeyen ilke

Her fazda iç test ve gerektiğinde geliştirici build'i yapılır. Ancak kullanıcıya final demo dışında paket teslimi verilmez. Final demoyu tamamlandı saymak için gerçek Windows paketinin temiz açılışta başlatılması ve senaryonun çalıştırılması gerekir; kodun derlenmesi veya headless testlerin geçmesi tek başına yeterli değildir.
