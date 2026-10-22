# Faz 7 — Hayatta Kalma ve Kazı/Tahkimat Entegrasyonu

**Dal:** `feature/phase-07-survival-integration`  
**Taban:** `feature/phase-06-shoring-support-damage-collapse`  
**Hedef:** Unreal Engine 5.8 / C++  
**Doğrulama:** Statik kaynak/runner denetimi hazırlanıyor. UBT/UHT, Automation ve PIE motor içinde çalıştırılmadı.

## Amaç ve eklenenler

Mevcut `USurvivalComponent` tek health/stamina/needs sahibi olmaya devam eder. İkinci bir oyuncu stat sistemi üretilmeden şu parçalar eklendi:

- `USurvivalExcavationAdapter`: tool swing, ağır kazı, tahkimat yerleştirme/taşıma/tamir ve taşıma yükü maliyetlerini mevcut stamina'ya bağlar.
- `UUndergroundExposureComponent`: vücut sıcaklığı, oyuncu ıslaklığı, karanlık maruziyeti, su maruziyeti ve sıcaklık stresini sınırlandırılmış adımlarla hesaplar.
- `FSurvivalExposureSample`: sıcaklık, çevresel nem, ışık seviyesi, yeraltı, su teması ve derinlik giriş sözleşmesi.
- `FSurvivalExposureSnapshot`: versiyonlu SaveGame alanlarına sahip durum snapshot'ı; Faz 8'deki birleşik kayıt transaction'ına temel oluşturur.
- `AForestCharacter`: yeni bileşenleri taşır; mevcut swing stamina maliyeti korunur ve pickaxe ağır kazı maliyeti ayrı efor ölçeğinden geçer. Taşıma sırasında sürekli stamina gideri de aynı çevresel maliyet hesaplamasını kullanır.

## Kurallar ve kapsam sınırları

- Sıcaklık girdisi [-80, 80] C; çevresel nem ve ışık [0,1]; su derinliği [0,100000] cm sınırlarında doğrulanır.
- Vücut sıcaklığı oyun modeli olarak [25,45] C aralığında sınırlıdır. 35 C altı veya 39.2 C üstü stres sayılır; grace süresi geçerse mevcut SurvivalComponent'e hasar uygulanır.
- Su teması ıslanmayı artırır; ortam nemi ıslaklığın artışını, yeraltı ise kuruma hızının düşmesini etkiler. Islaklık ve soğuk/sıcak durumları aksiyon stamina maliyetini yükseltir.
- Yeraltı ve düşük ışık koşulu görünürlük çarpanı ve karanlık maruziyet sayacı üretir. Bu fazda otomatik torch, render exposure veya gerçek görüş mesafesi yönetimi eklenmedi; diğer sistemler `GetVisibilityFactor` API'sini tüketebilir.
- Sürekli su maruziyeti izlenir fakat oksijen/gaz/boğulma ve yüzme mekaniği bu kapsamda değildir.
- Her Tick'te çevresel ilerleme en fazla 0.25 saniyedir. Snapshot restore yalnızca alanları geri yükler; offline time catch-up yapmaz ve restore sırasında hasar çağırmaz. Sonraki hasar yalnızca yeni Tick delta'sından gelir.
- Mevcut log-only `USaveGameSubsystem` değiştirilmedi. Exposure snapshot kayda hazırdır; tam voxel/tahkimat/oyuncu save/load entegrasyonu Faz 8 kapsamındadır.

## Çevre örneğinin kaynağı

Repoda oyuncu konumundan sıcaklık, nem, aydınlık ve su derinliğini birlikte üreten doğrulanmış bir `ISurvivalRegionProvider` bulunmuyor. Sistem sahte otomatik algılama yapmaz: su/biome/underground/light volume veya Blueprint çevre sağlayıcısı, oyuncunun `UndergroundExposure` bileşenine `SetEnvironmentSample` ile örnek sağlamalıdır. Varsayılan örnek açık, kuru, su dışı ortamdır. Çevre volume'larının otomatik birleşimi ve dünyaya özel sorgu adaptörü bu PR'da tamamlanmış sayılmaz.

## Testler

Yeni test adları:
- `MyProject2.World.Survival.ActionCostsAreValidatedAndEnvironmentScaled`
- `MyProject2.World.Survival.ExposureSampleRejectsInvalidValues`
- `MyProject2.World.Survival.ExposureSnapshotRestoresWithoutCatchup`

Faz 7 runner Faz 1–6 test adlarını korur ve 3 yeni adla toplam 26 test bekler. Bu statik eşleşme Unreal testlerinin gerçekten çalıştığını kanıtlamaz.

Windows Git Bash/MSYS + UE 5.8, Editor kapalıyken:
```bash
bash Scripts/run_phase7_tests.sh
# Gerekirse:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase7_tests.sh
```

## Kapanış kapıları

1. UE 5.8 Development Editor UBT/UHT build başarılı olmalı.
2. 26/26 Automation testi gerçek çalışmada başarılı olmalı.
3. PIE'da açık/kuru, karanlık yeraltı, soğuk-ıslak tünel ve su teması örnekleri doğrulanmalı.
4. Çevre sağlayıcısının geçişleri doğru gönderdiği ve eski sample'ların kalıcı kalmadığı doğrulanmalı.
5. Snapshot restore sonrasında offline catch-up olmadığını ve timer'ın çift ilerlemediğini doğrulama.
6. Faz 8'de snapshot'ın birleşik dünya save/load transaction'ına bağlanması.
7. Unreal Insights ile tick/adaptör maliyetinin ölçülmesi.

Bunlar oyun dengeleme formülleridir; gerçek tıbbi veya çevresel güvenlik modeli değildir.
