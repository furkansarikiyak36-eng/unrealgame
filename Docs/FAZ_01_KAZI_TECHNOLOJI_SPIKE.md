# Faz 1 — Runtime kazı teknolojisi spike şartnamesi

**Durum:** Deney planı hazır; gerçek UE 5.8 ölçümü/teknoloji seçimi bekliyor.  
**Kapsam:** Yalnızca geliştirici iç test haritası. Kullanıcıya ara demo teslimi değildir.  
**Bağımlılık:** Faz 0 kaynak triage tamamlandı; açık motor doğrulama/entegrasyon engelleri sürüyor.  
**Kural:** Bu belge iki adayın ölçüm metodunu tanımlar; ölçüm olmadan teknoloji seçilmiş sayılmaz.

## Mevcut repo zemini

- `MyProject2.uproject` UE 5.8 kullanıyor ve `GeometryScripting` eklentisi etkin.
- `MyProject2.Build.cs` `GeometryScriptingCore` ve `GeometryFramework` modüllerini bağlıyor.
- `WorldCore/ChunkState.h` terrain height/deformation ve water/soil/chemistry/biomass hücrelerini tutuyor; kaynak denetiminde runtime 3B kazı hacmi + collision + delta-persistence zinciri bulunmadı.
- `WorldPersistence` world/chunk state'i serileştiriyor, ancak bu metotlarda canlı kazı hacmi geometrisi serileştirme pipeline'ı gösterilmedi.
- Yalnız heightmap deformasyonu reddediliyor: dikey çukur üretse bile gerçek yatay tünel, alttan oyma ve kapalı oda gereksinimini karşılayamaz.

## Karşılaştırılacak adaylar

### A — Sparse 3D yoğunluk/SDF alanı + yüzey çıkarımı

Yerel kazı hacmi, seyrek 3B hücre/density veya signed-distance delta'larıyla temsil edilir. Bir kazı işlemi ilgili hücreleri günceller; yüzey çıkarımıyla chunk mesh'i ve collision mesh'i yenilenir. Değişiklikler temel arazi yerine sparse delta olarak kaydedilir.

**Beklenen avantajlar**
- Dikey çukur, yatay tünel, alttan oyma ve odanın aynı veri modelinde temsil edilebilmesi.
- Tek bir kazı işleminin komşu chunk delta'larına ve save/load'a deterministik biçimde uygulanabilmesi.
- Yeraltı suyu/boşluk ve daha sonraki destek/çökme sistemlerinin aynı hacim maskesini sorgulaması için temiz sınır.

**Riskler**
- Mesh üretimi ve çarpışma yenilemesinin maliyeti; chunk sınırında normal/vertex/collision dikişleri.
- İnce cell boyutu ve seyrek veri yapısına bağlı bellek maliyeti.
- Delik/tünel kesişmelerinde topolojik hatalar; oyuncu/AI navigasyonunun güncellenmesi.

### B — Chunk mesh'i üzerinde yerel CSG/mesh-boolean kesileri

Arazi chunk'ı dynamic mesh olarak tutulur; çukur/tünel/oda, hacim kesicileri veya boolean işlemleriyle doğrudan mesh topolojisine uygulanır. Kalıcılık, değişmiş mesh'in tamamı yerine kesici işlemlerin sıralı/sürüm kontrollü delta günlüğüyle veya chunk mesh snapshot'ıyla sağlanır.

**Beklenen avantajlar**
- Mesh üzerinde doğrudan ve lokal çalışma; mevcut arazi mesh'iyle entegre edilebilecek prototip.
- Kaba küp hücresi görünümünden kaçınabilme ve daha düzgün kesit üretme potansiyeli.

**Riskler**
- Birikimli boolean işlemlerinde topoloji/triangulation bozulması ve işlem maliyeti.
- Chunk sınırını aşan tünelde aynı kesinin iki mesh'te tutarlı uygulanması.
- Çarpışma/normal/material güncellemelerinin mesh işlemiyle aynı transaction'da tutarlılığı.
- Mesh snapshot'ı ile tekrar üretilebilir sparse delta arasında save size/upgrade trade-off.

## Ortak test dünyası ve sabit test seti

İki aday aynı UE 5.8 test dünyasında, aynı ölçekte ve aynı makinada karşılaştırılmalı. İç doğrulama haritasında:
1. **Dikey kuyu:** 2 × 2 m kesit, 5 m derinlik.
2. **Yatay tünel:** 2 × 2 m kesit, 12 m uzunluk; en az bir chunk sınırını geçer.
3. **Yeraltı odası:** 6 × 5 × 3 m boşluk; kuyu ve tünelle bağlı.
4. **Tekrar kesişme:** Var olan tünelden ikinci tünel açıp ortak boşluk oluşturma.
5. **Sınır/geri alma:** Chunk kenarına 0.5 m mesafede kesim ve aynı bölgeye ardışık kazı.
6. **Çarpışma:** Oyuncu gövdesi, fiziksel kütük/taş, düşen parça ve basit AI nav/path query.
7. **Kalıcılık:** Kaydet, world'ü kapat, tekrar aç; aynı kazı delta'sı bir kez uygulanmalı, mesh/collision ve hücre state uyuşmalı.
8. **Hata durumu:** Geçersiz veya sıfır boyutlu kesici reddedilmeli; world delta'sı ya tamamen commit edilmeli ya hiç değişmemeli.

Su girişi, yapı desteği/çökme ve tuzakların işlevsel kabulü Faz 3–4 kapılarıdır; bu spike'ta yalnızca hacim verisine bağlanabilme maliyeti not edilir.

## Ölçüm kaydı

Her aday/çözünürlük koşusunda şu alanlar doldurulacak:
- UE sürümü, branch/commit SHA, build config, CPU/GPU/RAM ve map seed.
- Seçili cell boyutu, aktif volume chunk sayısı, mesh vertex/triangle sayısı.
- Kazı başına game-thread süresi, mesh üretim süresi, collision update süresi, worst-frame spike.
- İşlem başına yeni/yenilenen mesh-chunk sayısı ve bellek farkı.
- Kaydedilen delta/snapshot boyutu; reload süresi ve geometri/collision karşılaştırma hash'i.
- Tünelin sınırdan geçiş durumu, fizik test sonucu, hata/rollback sonucu.
- Ekran görüntüsü/recording, Unreal logu, test raporu.

`Docs/FAZ_01_KAZI_SPIKE_SABLONU.csv` ölçüm satırlarını tutar. Tüm sonuçlar doldurulmadan A veya B seçilmiş sayılmaz.

## Başarı/kabul koşulları

- Her iki aday aynı dikey kuyu+yatay tünel+oda+chunk-seam+save/load senaryosunu temsil edebilmeli.
- Görünen yüzey, collision ve persisted delta tek bir değişiklikten aynı sonucu göstermeli.
- Tünel sınır dikişi, ghost collision, oyuncunun geçemediği boşluk veya reload sırasında çift uygulama olmamalı.
- Performans yalnızca ortalama frame time ile değil, kazı başına worst spike ve collision rebuild ölçümüyle raporlanmalı.
- Ölçüm sonucu, seçilmeyen yöntemin neden elendiğini ve kalan riskleri açıkça belgelemeli.

**Seçim:** Beklemede — UE 5.8 host üzerinde gerçek ölçüm alınmadı.

## Unreal Engine 5.8 kaynakları

- [Geometry Scripting Users Guide — UE 5.8](https://dev.epicgames.com/documentation/unreal-engine/geometry-scripting-users-guide-in-unreal-engine?application_version=5.8): `UDynamicMeshComponent` runtime'da geometri düzenlemeyi destekleyen Dynamic Mesh yolunun parçasıdır; aynı belgede Nanite/Lumen desteği olmadığı belirtilir.
- [UDynamicMeshComponent API — UE 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GeometryFramework/UDynamicMeshComponent): çalışma anında dinamik mesh bileşeni ve partial mesh updates bilgisi.
- [Geometry Scripting Reference — UE 5.8](https://dev.epicgames.com/documentation/unreal-engine/geometry-scripting-reference-in-unreal-engine): Epic bu eklenti ailesinin beta olduğu ve shipping kullanırken temkin gerektiği uyarısını yapar.

Bu kaynaklar API olanaklarını tanımlar; proje için doğru yöntemi seçmez. Seçimi yukarıdaki test sonuçları belirleyecek.
