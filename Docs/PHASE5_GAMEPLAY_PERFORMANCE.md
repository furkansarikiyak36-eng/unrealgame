# Faz 5 — Oynanış Sistemleri, Entegrasyon ve Performans

## Uygulanan kapsam

Bu Faz 5 dalı, Faz 4'ün taşınabilir C++ tabanını genişleterek aşağıdaki karar/ön hazırlık sistemlerini ekler:

1. **Raid hedef seçimi:** tehdit, erişilebilirlik, stratejik değer, savunmaya maruz kalma ve mesafe; eşit puanda kararlı hedef ID'si.
2. **Spawn preflight:** tipli sonuçlar, navigasyon/alan izni/çakışma kontrolü, dünya revizyonu ve deneme bütçesi. Gerçek Unreal Actor spawn etmez.
3. **Chunk scheduler:** round-robin adalet, kuyruk/iş/çağrı bütçeleri, aynı chunk ve iş türünde daha yeni revizyonla coalescing, stale işi atma ve gelecek revizyonu erteleme.
4. **M1–M6 material rule resolver:** Global → Definition → Composite → Instance bağlamları, öncelik, gözlem aralıkları ve revizyon değişiminde cache invalidation.
5. **Plant growth:** sıcaklık/ışık/su/N-P-K kaynaklarına bağlı bounded-step büyüme ve tüketim.
6. **Animal care/reproduction:** yem/su memnuniyeti, sağlık/büyüme/üretim, hazır olma süresi ve girdi sırasından bağımsız, tür bazlı deterministik eşleştirme.
7. **Crafting transaction preflight:** tüm girdileri doğrulama, özel slot dizisinde çıktı üretme, kapasite/ağırlık kontrolü ve revizyon + snapshot fingerprint ile commit öncesi yeniden doğrulama.

## Maliyet ve tutarlılık ilkeleri

- Girdi adayları, kuyruk boyutu, iş başına maliyet, tick başı maliyet, taranan iş sayısı ve büyüme alt adımları açık limitlere tabidir.
- Aynı kaynak durumundan çıkan kararlar kararlı ID ve sıralama kurallarıyla deterministiktir.
- Stale world/inventory revision sessizce kabul edilmez.
- Crafting planı başarısızsa canlı envanter değiştirilmez; gerçek uygulama ancak world output hazırlığı başarılı ve revision hâlâ güncelse tek commit yapmalıdır.
- Material resolver yalnızca etki sonucunu üretir; durability, sıcaklık, su veya yangın state'ini kendisi değiştirmez.
- Chunk scheduler iş kuyruğunu yönetir; mesh/collision/navigation yayını için mevcut revision gate yine zorunludur.

## Regresyon testleri

Yeni beş test grubu:
- Raid target feature scoring, revision filtreleme, kararlı tie-break ve tipli spawn sonuçları.
- Chunk scheduler round-robin adaleti, stale iş atma, revizyonla coalescing ve bütçe reddi.
- Material scope precedence, observed-value fallback ve cache invalidation.
- Plant/animal kaynak tüketimi, yetersiz beslenme ve girdi sırasından bağımsız reproduction pair'leri.
- Crafting preflight, slot/ağırlık limiti, yetersiz malzeme, stale revision ve fingerprint uyuşmazlığı.

Toplam portable suite: **16 test grubu**. Son kabul, bu dalın GitHub Actions Configure/Build/Test sonucuna bağlıdır.

## Açık kalan Unreal entegrasyonu

- Raid feature değerlerini canlı düşman/yol/savunma sistemlerinden ölçmek.
- Gerçek navigasyon, oriented overlap ve Actor spawn işlemini preflight kararına bağlamak.
- Chunk işi için worker/async execution, cancelation, derived-product publish ve Unreal Insights profil ölçümü.
- M1–M6 sonuçlarını mevcut material component/interaction matrix/chunk-save yapısına uygulamak.
- Bitki/hayvan actor'larını ortam, envanter ve persistence ile bağlamak.
- Crafting ve world-drop çıktıları için gerçek Actor staging + envanterin atomik commit/rollback entegrasyonu.
- UE 5.8 UBT/UHT, Automation ve Editor/PIE testleri. Bunlar henüz çalıştırılmış kabul edilmez.

## Faz 5 kabul kapısı

- [ ] Taşınabilir CMake configure/build başarılı.
- [ ] On altı test grubu başarılı.
- [ ] Raid/spawn kararlı ve bütçeli.
- [ ] Chunk scheduler adil, stale işlere karşı korumalı ve bounded.
- [ ] Material rule cache değişiklikle birlikte invalidated.
- [ ] Plant/animal sonuçları deterministik ve kaynak tüketimi sınırlı.
- [ ] Craft planı yalnızca revision ve snapshot eşleştiğinde commit için kabul ediliyor.
- [ ] Unreal motor entegrasyonu ayrı bir kabul aşaması olarak kaydediliyor.
