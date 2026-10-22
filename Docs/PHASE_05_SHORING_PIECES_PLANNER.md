# Faz 5 — Tahkimat Parçaları ve Hendek Planlayıcısı

**Dal:** \`feature/phase-05-shoring-pieces-planner\`  
**Taban:** \`feature/phase-04-scan-to-dig-placement\`  
**Hedef motor:** Unreal Engine 5.8 / C++  
**Durum:** Uygulama ve statik test kapsamı eklendi. UBT/UHT, gerçek Automation çalıştırması ve PIE doğrulaması bu ortamda yapılmadı.

## Amaç

Kazılmış smooth-voxel hendeğe yerleştirilen tahkimatı voxel mesh'ine birleştirmeden ayrı fiziksel aktörler olarak yönetmek. Voxel density tek otorite olmaya devam eder. Planlayıcı yalnızca aday geometri üretir; oyuncu/developer açıkça commit etmeden aktif parça oluşturulmaz.

## Eklenen sistemler

| Dosya | Sorumluluk |
|---|---|
| \`Shoring/ShoringTypes.h\` | GUID'ler, parça durumları, destek ankrajları, plan ve parça sözleşmeleri |
| \`Shoring/ShoringLayoutPlanner.h/.cpp\` | Hendek ekseni, genişliği, derinliği ve istasyon aralığından sınırlandırılmış geometri üretimi |
| \`Shoring/PlaceableShoringPiece.h/.cpp\` | Ayrı aktör, greybox görünüm, çarpışma, bağlantı GUID'leri ve destek state'i |
| \`Shoring/ExcavationShoringSubsystem.h/.cpp\` | Önizleme, commit, taşıma, silme, destek yeniden doğrulama ve voxel değişimiyle invalidation |
| \`Tests/ShoringLayoutPlannerTests.cpp\` | Plan geometrisi, bütçe, stale revision ve aktör yaşam döngüsü testleri |
| \`Scripts/run_phase5_tests.sh\` | Faz 1–5 dünya otomasyon testleri ve UE 5.8 build runner |

## Planlayıcı davranışı

- Başlangıç ve bitiş noktaları hendek ekseninin üst/rim konumlarıdır; uçların Z değerleri farklı olabilir.
- Varsayılan değerler: 200 cm genişlik, 400 cm derinlik, 150 cm istasyon aralığı.
- İstasyon sayısı aralık sınırını aşmayacak biçimde hesaplanır; ilk ve son istasyon uçlara yerleşir.
- Her istasyonda iki dikme; istasyonlar arasında iki duvara kaplama tahtaları; etkinse her istasyonda enine payanda üretilebilir.
- Platform ve merdiven isteğe bağlıdır. Varsayılan gri kutu mesh'i yalnızca ilk oynanabilir bloklama içindir; Blueprint alt sınıfıyla ahşap mesh'ler atanabilir.
- \`MaxStations\`, \`MaxPieces\`, boyut aralıkları ve bağlantı tutarlılığı allocation/spawn öncesinde kontrol edilir.
- Her parçada GUID, plan GUID'si, destek ankrajı, density revision ve çift yönlü bağlantı kimlikleri bulunur.

## Commit ve durum geçişleri

1. Plan hazırlanır ve preview aktörleri collision kapalı/Planned state'te gösterilir.
2. Commit öncesi density revision kontrol edilir; dikme ankrajları shared scanner üzerinden güncel voxel yüzeyine yeniden sorgulanır.
3. Dünya çarpışması ve mevcut tahkimatla çakışmalar kontrol edilir. Herhangi bir doğrulama başarısızsa hiçbir aktif aktör oluşturulmaz.
4. Başarılı commit, her tahkimat elemanını ayrı \`APlaceableShoringPiece\` olarak kaydeder.
5. Bir eleman taşındığında plan \`NeedsRevalidation\` olur. Parça silinince komşu bağlantılar güncellenir; eksik socket anchor planın yeniden geçerli sayılmasını engeller.
6. Voxel değişimi, destek ankrajının bulunduğu kirli chunk'lardaki planları yeniden doğrulama durumuna geçirir. Eski ghost preview'lar temizlenir.

State anlamları:
- \`Planned\`: yalnızca önizleme; çarpışma yok.
- \`Active\`: commit edilmiş ve mevcut durumda geçerli kabul edilen parça.
- \`NeedsRevalidation\`: geometri/bağlantı değişti; doğrulama tamamlanana kadar çarpışma kapalı.
- \`Invalid\`: ankraj veya gerekli bağlantı kayboldu.
- \`Removed\`: dünyadan kaldırılmış eleman.

## Faz 5 kapsam sınırları

- Planner düz/segmentli hendek ekseni varsayar. Yüzey ankrajları gerçek voxel wall hit ile commit sırasında doğrulanır; büyük sapma planı reddeder.
- Preview şu an engine cube tabanlı özel-depth greybox'tır; nihai translucent ghost materyali/UMG araçları bu fazda yoktur.
- Tahkimat dayanımı, yük hesabı, hasar, gevşeme ve çökme Faz 6 kapsamındadır. Bu kod gerçek zemin mekaniği veya gerçek kazı güvenliği hesabı değildir.
- Voxel delta ve fiziksel tahkimatın save/load entegrasyonu Faz 8 kapsamındadır.
- Her testin Unreal içinde geçtiği veya gerçek bir oyun haritasında çalıştığı iddia edilmiyor.

## Test

Windows Git Bash/MSYS ve UE 5.8 kurulu proje dizininde, Unreal Editor kapalıyken:

\`\`\`bash
bash Scripts/run_phase5_tests.sh
# Gerekirse:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase5_tests.sh
\`\`\`

Runner, 16 mevcut Faz 1–4 testine ek olarak dört Faz 5 test adı arar (toplam 20). Build veya test başarısız olursa PR taslak kalmalıdır.

## Faz 5 kapanış ölçütleri

1. UE 5.8 Development Editor build ve 20/20 Automation testinin başarılı olması.
2. PIE'da gerçek kazılmış hendekte duvar ankrajı, merdiven/platform konumu ve çakışmaların görsel kontrolü.
3. Bir dikme/kaplama/payanda taşındığında ya da silindiğinde bağlantı GUID'leri ve \`NeedsRevalidation/Invalid\` state geçişlerinin kontrolü.
4. Voxel duvarı kazıldığında yalnızca ilgili tahkimat planlarının invalidation aldığının ve yeniden doğrulama sonrası state'in doğru olduğunun doğrulanması.
5. Unreal Insights ile 100+ tahkimat parçasında commit/revalidation maliyetlerinin ölçülmesi.