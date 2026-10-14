# MyProject2 — 14 Fazlık Uygulama Omurgası

**Hedef:** Faz 0–14 boyunca bütün mekanik kodlarını mevcut MyProject2 çekirdeğiyle uyumlu, modüler ve birbirine bağlanabilir biçimde kurmak. Kullanıcıya ara demo paketi verilmez; tek Windows demo bütün fazlar tamamlandıktan sonra hazırlanır.  
**Motor:** Unreal Engine 5.8  
**Ana yaklaşım:** Önce sistemleri tasarlayıp kodlamak ve entegrasyon sözleşmelerini kurmak; kapsamlı hata avı, bütün katalog incelemesi, regresyon ve performans analizi Faz 14'te toplu yapılır.

## Çalışma prensibi

1. **Kod üretimi ana ilerleme ölçüsüdür.** 485 mekanik satırının her biri için ayrı editör kanıtı çıkarılması sonraki kodlamanın ön koşulu değildir. Mekanikler sahip fazlarında uygulanır; mevcut modüller bulunup tekrar kullanılır, eksik olanlar ortak mimariye uygun geliştirilir.
2. **Katmanlar ve sözleşmeler önce gelir.** Her modül kendi sorumluluğunu taşır; diğer modüllere kararlı C++ arayüzleri, ortak kimlikler ve açık işlem sonuçları üzerinden bağlanır. Aynı envanter, dünya hücresi, yapı grafı veya üretim stoğu iki farklı sistemde kopya otorite olarak tutulmaz.
3. **Geliştirme sırasında yalnızca hafif kontroller.** Yeni kodun yerel tutarlılığını korumak için gereken statik/syntax kontrolleri ve küçük hedefli kontroller yapılır. 485 mekanik için kapsamlı kanıt toplama ve uçtan uca hata avı her adımda tekrarlanmaz.
4. **UE testleri geliştirmeyi bekletmez.** Unreal Engine 5.8 build ve oyun testleri kullanıcının Claude Code/UE ortamında çalıştırılabilir. Bu çalışma ortamında UE mevcut olmadığı için sırf motor testi beklenerek kod aşaması durdurulmaz; çalıştırılmamış motor testleri başarılı diye de raporlanmaz.
5. **Belirsizlikleri uydurmayız.** Eksik gereksinim ya da risk açık bir backlog maddesi olur ve sahibi olan fazda çözülür. Örneğin RAID kaynağındaki 69 başlık/63 detay farkına altı hayalî mekanik eklenmez.
6. **Toplu derin analiz Faz 14'tedir.** Bütün fazlar kodlandıktan sonra 485 mekanik, katmanlar arası çağrı zincirleri, çifte sahiplik, kaynak/miktar bilançosu, save/load, hata/rollback, performans ve regresyon birlikte gözden geçirilir. Bulunan kusurlar yayın kapısı geçmeden düzeltilir.
7. **Kullanıcıya tek teslimat.** Ara faz paketleri yalnızca dahili geliştirme/doğrulama içindir. Tek kullanıcıya açık Windows demo, son kabul turundan sonra teslim edilir.

## Katman mimarisi

- **L0 — Ortak çekirdek:** Kanonik ID'ler, world/time/seed, durum sahipliği, olay kimliği, transaction/rollback, ortak sonuç tipleri ve sürümlü kayıt sınırları. Ana sözleşmeler Faz 2'de sabitlenir; daha önce yapılan prototipler bunlarla uyumlu olacak şekilde adaptörlenir.
- **L1 — Alan sistemleri:** Kazı/terrain, kütük, taş, çubuk, materyal, su-toprak, tarım/hayvan, bölge, lojistik, üretim ve raid kendi veri modeli ve iş kurallarına sahip olur.
- **L2 — Entegrasyon adaptörleri:** Alan sistemlerini birbirine bağlar. Örnekler: fiziksel öğe↔envanter, materyal↔world cell, ortak yapı grafı, üretim↔stok, raid↔yapı hasarı. Adaptörler otoriter state sahibi değil; doğrulanmış dönüşüm/işlem köprüsüdür.
- **L3 — Oyun akışı/orkestrasyon:** Oyuncu girdisi, olay zamanlayıcıları, baskın akışı ve üretim kuyrukları. Aynı işi yapan ikinci bir orkestratör oluşturulmaz.
- **L4 — Son sertleştirme:** Faz 14'te çapraz faz analizleri, kapsamlı hata/kenar durum taraması, regresyon, kalıcılık ve performans; ardından tek final paket.

Yeni bir sistem eklenirken önce mevcut sorumlusu aranır. Aynı işi yapan eski sistem varsa paralel otorite oluşturmak yerine tek bir kanonik sistem seçilir ve uyum adaptörü yazılır.

## Fazlar

| Faz | Başlık | Uygulama çıktısı | Faz içi ilerleme ölçütü |
|---|---|---|---|
| 0 | Baseline ve katalog eşleştirmesi | Kanonik 485 satır, dokuz aile eşleştirmesi, statik kaynak triage ve entegrasyon riskleri | ID ve eşleştirmeler tutarlı; açık sorunlar doğru fazlara aktarılmış |
| 1 | Kazı veri temeli | Sparse yerel 3B kazı delta modeli, sabit koordinat sözleşmesi, geri alınabilir kazı işlemi ve backend sınırı | Kuyu/tünel/oda için aynı veri modeli mümkün; geçersiz işlem durumu değiştirmez |
| 2 | Core contract ve kalıcı dünya verisi | Kimlik/sahiplik, event/idempotency, sparse delta, sürümlü kayıt sözleşmeleri | Modüller ortak kontratları kullanır; ikinci state otoritesi oluşmaz |
| 3 | Kazılabilir arazi | Runtime hacim sorgusu, mesh/collision bağlama katmanı, chunk seam ve kalıcı delta | Kazı verisi ile üretilen görünür geometri arasında açık adaptör ve deterministik state |
| 4 | Yeraltı yapısı ve tehlikeleri | Destek/çökme, toprak-kaya profili, su girişi ve tuzaklar | Aynı dünya/hacim kontratı üzerine bağlanır |
| 5 | Kütük sistemini sağlamlaştırma | Mevcut `ACarvableLog` akışı, yapı tipleri, hasar ve geri kazanım | Mevcut çalışan çekirdek korunur; yeni sistemlerle tek sorumluluk sınırı belirlenir |
| 6 | Taş, çubuk ve envanter entegrasyonu | Fiziksel öğe→taşıma/stok→işleme→inşa→kurtarma | Kaynak tüketimi/spawn/geri alma tek bir işlem sözleşmesine bağlanır |
| 7 | Yapı grafı ve materyal simülasyonu | Kanonik yapı grafı; yangın/nem/darbe/çürüme adaptörleri | Wood/stone/stick etkileri ortak bağlantı ve sahiplik katmanına bağlanır |
| 8 | World State | Su, toprak, nem, biyokütle, yangın, çürüme ve kimya | Ortak zaman/seed/world state üzerinden çalışır |
| 9 | Tarım ve hayvancılık | Ekim→büyüme→hasat; yem→hayvan→ürün/gübre→toprak | Hasat, stok, yem ve çıktı ayrı stok adaları oluşturmaz |
| 10 | Bölge ve lojistik | Bölgesel kaynak/risk; patika, yol, köprü, rampa, kızak, halat/makara | Rota ve taşıma sistemleri world/structure state'e adaptörle bağlanır |
| 11 | Oyuncu üretim zincirleri | Kütük→testere→tahta/kereste/yan ürün; stok, kapasite, kuyruk ve güç | Üretim girdi/çıktıları ortak kaynak muhasebesinden geçer |
| 12 | Raid Director | Tek spawn sahibi, scheduler, hedef, navigasyon ve ortak yapı hasarı | Baskın orchestration'ı tek akıştan geçer; dünya/yapı etkisi ortak arayüzü kullanır |
| 13 | Bölge hikâyesi ve atmosfer | En az üç mekanikçe ayrışan bölge; harabe/iz/nesne temelli anlatı | Çevresel anlatı bölge verisi ve oyuncu keşif döngüsüne bağlanır |
| 14 | Bütünsel analiz, hata ayıklama ve içerik kilidi | 485 mekanik toplu inceleme, entegrasyon/regresyon, save/load, kaynak bilançosu, performans ve blocker raporu | Katman sınırları ve çapraz sistem akışları analiz edilir; kritik kusurlar kapatılır |
| 15 | Tek final Windows demo | Tek paket, README, kontroller, test kayıtları, performans ve bilinen sorunlar | Temiz açılış, 30–60 dakika oynama, iki save/load turu ve kabul checklist'i |

### Faz 2 — uygulanan çekirdek sözleşme

- `UWorldPersistenceSubsystem`, dünya kimliği, deterministik kaydedilmiş seed, monotonik dünya revizyonu ve kazı alt sisteminin commit/rollback/reset olaylarını ortaklaştırır. `UExcavationVolumeSubsystem` kazı hücrelerinin tek otoriter sahibi olmaya devam eder.
- `ApplySphereEditOnce`, `ApplyCapsuleEditOnce` ve `ApplyBoxEditOnce`, çağıranın sağladığı `FGuid` komut kimliğiyle tekrar denemelerin aynı edit'i ikinci kez uygulamasını engeller. Son 100.000 komut kimliği ve son 100.000 olay anahtarı kayıt zarfında tutulur; bu sınırlı geçmiş penceresinin dışındaki kimlikler kalıcı idempotency garantisi kapsamında değildir.
- `UWorldCoreSaveGame` sürümlü manifest, dünya ID/seed/revizyonu, kazı sparse snapshot'ı ve sınırlı idempotency/event defterlerini taşır. Yüklemede manifest ve defterler önce doğrulanır; sparse snapshot'ın doğrulanması ve tek adımda değiştirilmesi mevcut `ImportSnapshot` yoluna bırakılır. Başarılı yükleme kaydedilmiş revizyonu korur.
- **Sınır:** Bu, genel kayıt zarfının Faz 2 temelidir ve şu anda kazı sparse delta'sını kaydeder. Terrain mesh/collision, envanter ve diğer alan sistemlerinin kendi sahipliğindeki kayıtları bu aşamada tamamlanmış sayılmaz; ilgili fazlarda aynı zarf genişletilecektir.
- `Scripts/tests/test_world_persistence_contract.py` CI'a bağlı statik kontrat kontrolüdür. Unreal Engine 5.8 C++ derleme veya oyun otomasyonu bu ortamda çalıştırılmadı; motor doğrulaması ve bütüncül çapraz sistem audit'i Faz 14'e kadar açık kalır.

## Kazı teknolojisi seçimini nasıl ele alacağız

Faz 1'de bütün performans/benchmark bürokrasisini tamamlamadan ortak veri sözleşmesini ve değiştirilebilir backend sınırını kuracağız. İlk uygulama seyrek yerel 3B hücre/density delta yaklaşımıdır; A/B Dynamic Mesh CSG alternatifi not edilir ancak ilk kod katmanını bloke etmez. Nihai optimizasyon/alternatif seçimi, gerçek kısıtlar görüldükten sonra Faz 14 bütünsel analizinde yapılabilir. Kuyu, yatay tünel, oda ve chunk-sınırı gereksinimleri baştan veri modelinin parçasıdır; yalnızca heightmap deformasyonuna indirgenmez.

## Kanıt ve durum bildirimi

U0–U6 etiketleri kataloğun mevcut bilgi durumunu anlatır; geliştirme sırasını tek başına bloke etmez. Statik kontrol, UE automation ve görsel/uzun oturum kabulü birbirinden ayrı tutulur. Çalıştırılmamış bir test hiçbir zaman geçmiş olarak yazılmaz; fakat bekleyen test, uygun kod katmanının geliştirilmesini durdurmaz.

## Tek kullanıcı teslimatı

Faz 0–14 tamamlandıktan ve Faz 14 bütünsel analizindeki kritik kusurlar giderildikten sonra Faz 15 başlar: tek Windows packaged build, temiz başlatma, 30–60 dakikalık oynanış, iki save/load çevrimi, kontroller/README, sonuç kayıtları, performans ve bilinen sınırlamalar raporu. Ara fazlarda kullanıcıya paket teslim edilmez.
