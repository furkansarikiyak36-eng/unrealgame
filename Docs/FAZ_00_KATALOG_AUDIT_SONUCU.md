# Faz 0 — Mekanik kataloğu ilk audit sonucu

**Durum:** Kanonik kaynak triage eşitlendi — baseline kapısı tamam; uygulama blokajları kendi fazlarına taşındı  
**Beklenen satır:** 485  
**Kaynak:** Repo içindeki kanonik snapshot `Docs/MEKANIK_KATALOG_TAKIP_CSV.csv`  
**Denetleyici:** `Scripts/audit_mechanic_catalog.py`

## Sonuç

| Ölçüt | Sonuç |
|---|---:|
| Katalog satırı | 485 |
| Benzersiz ID | 485 |
| Yinelenen ID | 0 |
| Şema/kimlik/zorunlu kaynak triage alanı hatası | 0 |
| U0 — denetim gerekli | 64 |
| U1 — veri/model veya kısmi sistem izi | 88 |
| U2 — kaynakta yerel davranış izi | 333 |
| U3–U6 — entegrasyon/test/oyun/kalıcılık kanıtı | 0 |
| U0 seviyesinde açık kalan satır | 64 |

Bu rapor “485 mekanik başarısız” anlamına gelmez. Aile eşleştirmelerinde bulunan kaynak-triage durumları 14 Ekim 2026'da kanonik CSV'ye senkronlandı: 333 U2, 88 U1 ve 64 U0. U1/U2 yalnızca kaynak/model veya yerel davranış izini bildirir; oyuncudan dünya sonucuna entegrasyon, olumlu/olumsuz testler, görsel kabul veya kalıcılık kanıtı anlamına gelmez. U0 kayıtları tahminle yükseltilmedi.

`audit_mechanic_catalog.py` birim testleri, katalog satırlarının kanıt seviyesine göre gereken alanları denetler. Ayrı `validate_catalog_mapping.py` testi, ana katalogdaki kimliklerin dokuz aile dosyasına doğru sistemde bire bir eşlendiğini doğrular. Bu Python araçları mekanik davranışını kanıtlamaz; UE 5.8 derlemesi veya oyun içi testi yerine geçmez.

## Faz 14'te yapılacak toplu denetim

485 mekanik için ayrıntılı çağrı zinciri, pozitif/negatif/kenar testleri, görsel kabul, save/load ve performans incelemesi geliştirme ön koşulu değil; Faz 14'ün toplu audit girdisidir. Kodlama sırasında kritik belirsizlik çıkarsa uydurmak yerine ilgili satırın blocker'ı kaydedilir ve diğer bağımsız katmanlar ilerletilir.

Katalog denetleyicisinin keşif ve sıkı modları:

```bash
python Scripts/audit_mechanic_catalog.py <katalog.csv> --expected-count 485 --json-out Saved/TestRuns/catalog_audit.json
python Scripts/audit_mechanic_catalog.py <katalog.csv> --expected-count 485 --strict
```

İlk komut keşif raporu üretir. İkinci komut kanıt alanları, kimlik veya satır sayısında hata olduğunda sıfır olmayan çıkış vermelidir. 485 satırın tümü dokuz aile dosyasına eşlendi ve U0/U1/U2 kaynak triage durumu kanonik kayda aktarıldı. U0 maddeleri kendi teslim fazlarında uygulanacak backlog kayıtlarıdır; Faz 1 kodlamasını bloke etmez. Sparse 3B delta yaklaşımı ilk veri backend'i olarak seçildi; performans/alternatif incelemesi daha sonra, gerekirse Faz 14'te yapılır.
 

## Sonraki ilerleme — LOG alanının satır satır eşleştirilmesi

Repo dalına `Docs/FAZ_00_LOG_KATALOG_ESLESTIRME.csv` eklendi. Bu dosya LOG-001–LOG-076 kayıtlarının her biri için katalog ID'sini, kaynak yolu/adayı, satır özelindeki blocker'ı ve eksik test/editör kanıtını ayrı satırda tutar.

- LOG satırları: **76/76 kaynak denetim tablosuna eşlendi**.
- Kanıt seviyesi: **59 satır U2** (kaynakta yerel davranış izi var) ve **17 satır U0** (bu incelemede uygulanmış davranış zinciri bulunamadı ya da özellik ayrı geliştirme gerektiriyor). U2, otomatik test veya editör kabulü değildir; 59 U2 satırının da row-specific test/editör kanıtı bekliyor.
- Bazı ihtiyaçlar için kaynak yolu bulundu; bazıları “incelenen akışta bulunmadı” olarak açıkça işaretlendi (ör. zipline hattı, sal/su üstü kullanım, makine yapımı, genel savunma tuzağı). Bunlar tamamlandı olarak sayılmaz.
- CI dokuz eşleştirme dosyasının her birinde beklenen satır sayısını, 11 sütunlu şemayı, kimlikleri, durum biçimini ve blocker/not alanlarını denetler. Keşif modu U0/U1/U2 durumlarını başarı kanıtına dönüştürmez. En son doğrulanmış Python CI: [run #88](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38510701119), başarılı. Bu Unreal Engine build/editor testi değildir.

Dokuz katalog eşleştirme dosyası **485/485 satırı** kapsıyor: LOG 76, INVENTORY 32, RAID 63, WORLD 35, MATERIAL 38, FARMING 57, CORE 12, STONE 96, STICK 76. Kaynak triage dağılımı **333 U2**, **88 U1**, **64 U0**, **0 U3–U6**. Bu, katalog ve statik kaynak triage işinin tamamlandığını; davranışların bütünsel entegrasyon denetiminin Faz 14'e bırakıldığını gösterir.


## Yeni kod işi — STICK crafting atomikliği

P0-027 için kaynak düzeltmesi ve testler branch'e eklendi. `UCraftingComponent` artık `UStickInventoryComponent::ApplyCraftTransaction` kullanır. Üç motor otomasyon testi duplicate girdiler, kapasite başarısızlığı ve başarılı girdi/çıktı bilançosunu kapsar. `Scripts/run_tests.sh` bunları ayrı `StickInventoryTests.log` içinde çalıştırıp `Scripts/verify_test_logs.py` üzerinden en az 3 benzersiz test sonucu istemektedir.

**Bu düzeltme UE 5.8 üzerinde henüz çalıştırılmadı.** Kod düzeyi ilerleme tamamlandı; engine build ve motor automation geçmeden P0-027 kapanmış sayılamaz.


## Faz 1 kazı spike dosyaları (hazırlık, ölçüm bekliyor)

- [Teknoloji spike şartnamesi](./FAZ_01_KAZI_TECHNOLOJI_SPIKE.md)
- [Doldurulacak benchmark matrisi](./FAZ_01_KAZI_SPIKE_SABLONU.csv)
- [Matrisi ve kanıt kapısını doğrulayan araç](../Scripts/validate_excavation_spike.py)

Matriste iki aday ve iki çözünürlük için dört koşu bulunuyor. Mevcut satırların tüm kararları “Bekliyor”; bu doğru durumdur. Validator yalnızca şemanın ve gerekli ölçüm kanıtının varlığını denetler, gerçek fizik/mesh/performance ölçümü yapmaz.


## Taş yerleştirme — güncel kaynak/entegrasyon durumu

Taş yerleştirme çalışması, bağımsız component seviyesinden `AForestCharacter` girişlerine genişletildi. Fiziksel pickup, carry state, face/socket önizlemesi, collision-clearance ve graph-bearing hedefte reciprocal bağlantı commit/rollback davranışı kaynak kodunda var. Dolu hedef socket'i ikinci bir taşın kullanması reddediliyor.

Bunlar kaynak kodu ve beş Unreal Automation Test kaydıdır; graph save/load, karma ahşap-taş yapılar ve oyun içi çarpışma/görsel kabul kapsamlı Faz 14 incelemesinde yeniden ele alınacak açık işlerdir. Bunlar diğer bağımsız fazların kodlamasını bloke etmez.


## Güncel taş yerleştirme graph kapısı

Son kaynak turunda yapısal hedefe anonim (NAME_None) bağlantı bırakma davranışı kaldırıldı. Yerleştirme kaynak yüzünün/socket'inin yerel kimliğini kaydeder ve hedef AStoneActor üzerindeki yakın, doğru yöne bakan, boş socket bulunmadan yapısal commit yapmaz. UStructuralGraph::ConnectTo reciprocal link'i iki uçta kaydeder; hedef socket doluysa commit reddedilir. Diğer legacy structural actor ailelerine adapter bu değişiklikte eklenmedi ve fail-closed kalıyor.

StonePlacementTests.cpp beş otomasyon senaryosu içeriyor. UE 5.8 çalıştırması Claude Code/UE ortamında yapılacak; bu eksik Faz 0 baseline işini yeniden açmaz, ilgili özellik kabulinin sonraki doğrulama girdisidir.


## Stone Trap P0-025 — ilk kod uygulaması

- Trigger volume için Pawn overlap ve BeginOverlap callback eklendi.
- Bilinen alıcılar: RaidEnemy, player, StoneActor ve CarvableLog; diğer aktörler standard Unreal damage event yolunu kullanır.
- Rearm cooldown'u atlayamaz.
- İki motor otomasyon testi eklendi; Windows test runner ayrı StoneTrap suite'ini çalıştırıp en az iki benzersiz başarılı sonuç ister.
- Unreal Engine 5.8 testleri henüz çalıştırılmadı. Damage tuning/physics/overlap kabulü açık.


## 14 Ekim 2026 — kanonik kaynak ve eşleştirme kapısı

- Orijinal 485 satırlık gereksinim CSV'si, kütüphane girdisinden alınarak `Docs/MEKANIK_KATALOG_TAKIP_CSV.csv` yoluna sürüm kontrollü kaynak snapshot'ı olarak eklendi. Orijinal satırlardaki U0 değerleri korunmuştur; bu kayıtlar doğrulanmış mekanik anlamına gelmez.
- `Scripts/validate_catalog_mapping.py`, ana katalog ID'leri ile dokuz aile eşleştirmesini karşılaştırır. Beklenen aile sayıları LOG 76, STONE 96, STICK 76, INVENTORY 32, RAID 63, FARMING 57, WORLD 35, MATERIAL 38, CORE 12'dir. Kontrol, eksik/fazla ID, çift ID, yanlış sistem ve satır sayısı sapmalarında başarısız olur.
- `Scripts/tests/test_validate_catalog_mapping.py` için 8 test, Python CI çalıştırıcısına bağlandı.
- GitHub Actions [run #38515014058](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38515014058) bu snapshot, dokuz eşleştirme ve araç testleri eklendikten sonra başarılı oldu. Bu yalnızca platformdan bağımsız Python/CSV/shell doğrulamasıdır.
- Ayrı kaynak brief başlığında RAID için 69 alt mekanik yazarken yedi alt bölüm toplamı ve ayrıntılı liste 63 adlandırılmış madde veriyor. P0-001 açık kaldı; karar verilene kadar altı yeni madde uydurulmuyor.
- **Kalan işler faz sahiplerine taşındı:** 64 U0 kayıt uygulama/araştırma kuyruğunda; U1/U2 satırların test ve editör kanıtları ilgili özellik fazlarında üretilecek. UE 5.8 testlerinin Claude Code üzerinden çalıştırılması kod geliştirmesini burada bekletmez; sonuçlar doğrulanmadan özellikler U4–U6 olarak işaretlenmez.
