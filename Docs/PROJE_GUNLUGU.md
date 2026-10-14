# Proje günlüğü

Yapılan her iş buraya yazılır; **sona eklenir, eski girişler değişmez** (hata düzeltmesi dışında). Sırayla eskiden yeniye.
Her giriş: ne yapıldı, hangi dosyalar, test sonucu, bulunan hatalar, bekleyen kalan iş.
Güncel durum için `DURUM_RAPORU.md`, kodun ne yaptığı için `KOD_HARITASI.md`, verilen ve bekleyen kararlar için `KARARLAR.md`.
Yapan: **CC** = Claude Code (bilgisayarda, derler ve test eder), **Web** = claude.ai oturumu (tasarım ve zip ile kod).

---

### 2026-10-12 — Proje kuruldu (CC)
- `MyProject2` boş bir C++ projesiydi; asıl kod eski `Downloads/MyProject`'teydi (6.000 satır: ForestCharacter, CarvableLog, yapı sistemleri…).
- Eski kodun tamamı `Source/MyProject2` altına taşındı (`MYPROJECT_API` → `MYPROJECT2_API`), eski kodun tüm mesh ve materyal yolları `ForestTypes.cpp` içinde toplandığı için yalnızca orayı greybox'a çevirmek yetti.
- `Scripts/build_greybox.py`: 20 greybox mesh (kütük, yarım, kısa, oyuk, kalas, kiriş, odun, taş, taş blok, kapı/pencere çerçevesi, 3 oyma kütüğü, 3 alet, 2 ağaç, kaya), 7 renk materyali, `Greybox_Mechanics` haritası. Editör kapalıyken `UnrealEditor-Cmd -run=pythonscript` ile çalışır.
- `.uproject`'e CableComponent, GeometryScripting, PythonScriptPlugin, EditorScriptingUtilities eklendi; `Build.cs`'e CableComponent, GeometryScriptingCore, GeometryFramework.
- Test: derlendi; harita hatasız yükleniyor. Oyunda görsel test yapılmadı.

### 2026-10-12/13 — MCP sunucusu (CC)
- Tünel `localhost:8000` bağlanamıyordu: yeni projede ModelContextProtocol plugin'i ve `bAutoStartServer` yoktu. `.uproject` ve `Config/DefaultEditorPerProjectUserSettings.ini` düzeltildi.
- Yazma yetkisi için toolset plugin'leri (AllToolsets, EditorToolset…) kullanıcı tarafından eklendi. Güvenlik notu: `trycloudflare` tüneli herkese açık ve şifresiz.

### 2026-10-13 — Oyma sistemi (CC)
- Oval çentik ve düz çentik eklendi (`CutProfile`, `ECarveTool::NotchRound/NotchFlat`); genişlik kütük çapına, derinlik çapın 1/6, 1/3, 1/2'sine bağlandı.
- Kesikli kırmızı kesim çizgisi (`GetCutGuide`, `ForestOutline::DrawDashed`); hayalet önizleme yerine çizgi önizleme (Sons of the Forest tarzı).
- Sabit kesim noktaları: köşe çentikleri, ortada bir nokta, düz çentiğin uç geçmesi; dolu yer kapanır.
- İki vuruş hatası: kütük yuvarlandığı için "aynı yer" karşılaştırması tutmuyordu → toleranslı karşılaştırma + kütük oturunca sabitleme + dönme sönümü.
- Test: `AMechanicsDemo` otomatik modu (`-MechanicsDemoQuit`). İlk sürümde 10 kontrol.

### 2026-10-13 — Demo ve rehberli mod (CC)
- `AMechanicsDemo`: haritaya konan aktör. Otomatik oynatma (testler) ve rehberli mod (oyuncu yapar, görev listesi HUD'da işaretlenir).
- `Demo_WoodChain` haritası (`build_greybox.py` içinde `GREYBOX_BUILD=demo`).
- Grafiksiz test altyapısı; her mekanik değişikliğinde test ve sonuç satırı (`GEÇTİ/KALDI/SONUÇ`) log'a yazılır.

### 2026-10-13 — Taşıma, hacim, çaprazlama (CC)
- Tüm ağaçtan çıkan kütükler doğrudan `ACarvableLog`; "oymaya hazırla" adımı kalktı.
- Omuz kapasitesi hacme göre (1 tam kütük = 2 yarım = 6 tane 1M); yük yürüyüşü yavaşlatır.
- Çaprazlama yerleştirme (`ComputeCrossPlacement`, `PlaceAcross`): taşınan kütük yerdeki kütüğün üstüne dik oturur, altına eyer çentiği otomatik açılır; geçtiği bütün alt kütüklere çentik.
- Bulunan hata: büyük kütük bölünürken iki parça birbirine değerek doğuyor, fizik birini 93 m öteye fırlatıyordu → parçalar yanlara açılarak doğuyor, `APickupItem` hız sınırı.

### 2026-10-13 — Modül ızgarası, Faz 1–2 (CC)
- 65 cm modül; kesilen parçalar ızgarayı korur; enine kesim yalnızca modül çizgilerinde, en kısa parça 1M.
- Hacme göre taşıma tamamlandı; direk dikme (`GetStandTransform`).
- `Docs/KUTUK_SISTEMI_PLANI.md` yazıldı.

### 2026-10-13 — Wood paketi ve yapı mantığı (Web + CC)
- Web: `do/` altında 4 zip (Wood paketi: veri modeli, kesim kuralları, yapı mantığı, alt sistem, testler). 
- CC: son zip (`LogSystem_Faz4_Kismi`) `Source/MyProject2/Wood/` altına alındı, ilk kez Unreal'da derlendi; tek hata `FEdge` ad çakışması. Paketin 4 testi geçti.
- `StructureLink`: oyundaki kütükleri Wood yapı grafiğine bağlar (bağlantı bulma, destek hesabı, düşürme). Kiriş (direğin tepesine), söküm, çöküş.
- Wood bileşenleri (`WoodPieceComponent` vb.) derleniyor ama hiçbir aktöre bağlı değil: oyundaki kütük `ACarvableLog`.

### 2026-10-13 — Grup 1–5: geometri, yapı, ateş, hasar, tanıma (CC)
Kullanıcı istedi: "1-5 kısmını yap". Test 74/74, Wood birim testi 11/11.
1. **Geometri kesimi:** sivri uç, pah, uç düzeltme, kırlangıç (`ShapeEnd`, `CutDovetail`); uç şekli kesimde korunur; yarma tekrarı (yarım → çeyrek) ve düz yüz yönü (`FlatAngle`).
2. **Yapı:** payanda (`ComputeBracePlacement`), döşeme tahtası (`ComputeFloorPlacement`), kiriş açıklık kuralı (destekler arası ≤ 4,5 modül, taşma ≤ 2 modül; aşılırsa kiriş ortadan kırılır `SnapAt`), 3 katlı kabin.
3. **Ateş:** `ACampfire` (yakıt hacimle orantılı, tam kütük 600 sn, üst sınır 900 sn); R üç kez → ateş modu; ateşe odun atma.
4. **Hasar, yangın, onarım:** `AddDamage` (≥0,6 eklemler gevşer, ≥0,85 taşımaz), ateşin 160 cm yakını yanar, yanık parça sağlam parçayla değiştirilir (`ReplaceDamaged`, çentikler aynen). Admin Y.
5. **Yapı tanıma:** `WoodStructureAnalysis.cpp` yazıldı (web paketinde yalnızca bildirim vardı); 15 yapı türü; HUD'da parçaya bakınca adı. 7 tür dünyada, tamamı birim testlerinde doğrulandı. Birim testleri `WoodStructureAnalysisTests.cpp`.
- Bulunan hatalar: ateşe odun reddi (`AddFuel` taşınan parçayı reddediyordu); 130 cm parçaya sivri uç açılamıyordu; gevşemiş direk açıklık hesabında hâlâ destek sayılıyordu; fiziğe bırakılan kiriş ortadaki çarpışmayla durmaya devam ediyordu; kabin testinde rastgele takılma (alt kütük yerde dönünce taşma sınırı aşılıyordu; test temel kütüklerini sabitleyerek giderildi, kural doğru).
- Rehberli demo 20 görev.
- **Kalan:** pencere kasası, çatı, merdiven, rampa, korkuluk tanınıyor ama oyuncu kuramıyor (eğik parça yerleştirme, platform üstüne direk, "geçici" işaretleme yolları yok); kapı/pencere açma yok; lojistik yok.

### 2026-10-13 — Kayıt sistemi (CC)
- `Scripts/update_docs.py` (`KOD_HARITASI.md` ve `TEST_DURUMU.md` üretir), `Scripts/run_tests.sh` (derle + iki test + kayıt + doküman).
- `BASLA.md`, `KARARLAR.md`, bu günlük. Bundan sonra her iş sonunda günlüğe giriş eklenir ve `update_docs.py` çalıştırılır.

### 2026-10-13 — Kayıt sistemi testi (CC)
- `Scripts/run_tests.sh` baştan sona çalıştırıldı: derleme başarılı, mekanik test 74/74, Wood birim testi 11/11. Kayıtlar `Saved/TestRuns/` altında, `KOD_HARITASI.md` ve `TEST_DURUMU.md` yenilendi.
- Editör çalışırken script reddeder (beklenen); test için editör kapatıldı, bitince `Demo_WoodChain` ile yeniden açıldı.

### 2026-10-14 — Yeni paket analizi: Material/Inventory/Stone/Stick/Raid/Farming/World (CC, entegre EDİLMEDİ)
- `do/` altına gelen: `..._Combined_UE58_Review.zip` (609 .h/.cpp, ~11 bin satır, 60+ klasör: Inventory, Stone, Stick, Raid, Farming (PhaseT), World (W1–W6), Material (M1–M6), Phase1–7 ALogActor), `..._Material_Addon_...zip` (yalnız Material), `README_MATERIAL_TR.zip`. "(1)" kopyaları birebir aynı (md5).
- Bulgular: UE'de hiç derlenmemiş (paket kendisi söylüyor), otomasyon testi yok. Yansıtılmış tür adı çakışması yok (176 tür); ama (1) paket `MyProject2.Build.cs` bizimkini ezer ve CableComponent/GeometryScriptingCore/GeometryFramework/EnhancedInput'u siler → derleme kırılır; (2) `Core/MaterialTypes.h` ve `MaterialCore/MaterialTypes.h` aynı adlı iki farklı başlık (EMaterialType / EMaterialClass), ikisi de include yolunda → belirsiz include; (3) `ALogActor` ikinci bir kütük sistemi (karar #3 ile çelişir); `InventoryCore`+`CraftingRecipe` tarifsiz kütük ilkesiyle çelişir; (4) paketin `MyProject2.cpp` dosyası `MyProject2.h` include'unu atar.
- Paketin kendi Python scriptleri çalıştırılmadı (indirilen kod).
- Repo `origin/main`'e web'den `TOTAL_PLAN.md` yüklenmiş (Faz 7–14 planı); henüz çekilmedi.

### 2026-10-14 — Paket C++ koduna alındı: derleniyor, testler bozulmadı (CC)
Kullanıcı istedi: "hepsini entegre et". `do/..._Combined_UE58_Review.zip` içindeki 608 .h/.cpp (Inventory, Stone, Stick, Raid, Farming PhaseT, World W1–W6, Material M1–M6, Phase1–7) `Source/MyProject2/` altına kopyalandı. **Yalnızca derleniyor; hiçbiri oyuna/kütüklerimize bağlı değil, yeni sistemlerin testi yok.** Mevcut testler: mekanik 74/74, Wood 11/11.
- `MyProject2.Build.cs` birleştirildi (bizim bağımlılıklar korundu; PhysicsCore, AIModule, GameplayTasks, NavigationSystem, RenderCore, RHI eklendi; `Wood` hariç her alt klasör include yolu; `bUseUnity=false` çünkü paket dosyaları anonim namespace'te aynı adlı yardımcılar tanımlıyor).
- Düzeltilen derleme sorunları: `Core/MaterialTypes.h` → `Core/StructuralMaterialTypes.h` (aynı adlı iki başlık); `EMaterialProperty` → `EMaterialPropertyKind` (motorla çakışma); `BuoyancyComponent` → `LogBuoyancyComponent` (Water plugin ile çakışma); `FCraftingRecipe` (Phase6) → `FBenchRecipe`; `ALogActor::IsGrounded` → `IsLogGrounded` (arayüzle çakışma); `Role`/`Instigator` parametre adları (AActor ile gölgeleme); UHT'nin desteklemediği `TMap<int32,TArray<uint8>>` ve `TArray<TWeakObjectPtr<AActor>>` yansıtmadan çıkarıldı; adsız UFUNCTION parametreleri adlandırıldı; Raid düşman ölümü dinamik delegeye bağlandı (`HandleEnemyDestroyed`); `GetComponentsByClass` → `FindComponentByClass`; `AddImpulse`, `FRandRange`, `GetSafeNormal`, transform döndürme düzeltmeleri; `;UCLASS(` aynı satırdaki 7 başlık ayrıldı.
- Kalan: KOD_HARITASI/TEST_DURUMU üreticisi (`update_docs.py`) yeni klasörleri henüz taramıyor. Sıradaki: yanma (Material) → kütük hasarı, kazık/tuzak/kızak kütük parçalarından, Raid düşman ve yapı saldırısı.

### 2026-10-14 — Faz 8: yanma sistemi (CC)
Test: mekanik 81/81 (+7 yeni), Wood 11/11. Görsel (alev, ışık, HUD) oyunda denenmedi.
- **Yanma parçanın özelliği:** `ACarvableLog` yanma durumu `Intact → Igniting → Burning → Charred`. Tutuşma ısısı yarıçapa bağlı (`GetIgniteSeconds` = yarıçap×0,4, 2–20 sn; tam kütük 14 sn). Yakıt = hacim (`ACampfire::FuelSecondsOf`), hasar yakıt bitince tam 1'e varır (yanık). Yanarken modül başına koni alev + ışık; alev yakıtla küçülür.
- **`UBurnSubsystem` (yeni, `BurnSubsystem.h/.cpp`):** dünya başına yanma sürücüsü. Yanan parça 50 cm içindeki odunu ısıtır (10 cm'de tam hız, 50'de sıfır), ısı kesilince parça soğur, yanan parça kendi içine düştüğü ateşi yakar. Aynı anda en çok 12 parça yanar (`MaxBurning`). `Advance(sn)` testte zaman atlatır.
- **Yapıya etkisi yeni kod gerektirmedi:** hasar 0,6'yı geçince eklemler gevşiyor, kiriş kırılıyor (mevcut `SetDamage` yeniden değerlendirir).
- **Kamp ateşi:** artık odun atmak yakmaz; `IsBurning = yanıyor && yakıt var`. Yakan: çakmak (`ApplyHeat`, 2 sn) ya da yanan parça (elde taşınıp ateşe atılırsa ya da yanında dururken). `ACampfire::Ignite`.
- **Oyuncu:** **[F]** basılı tut = çakmak (350 cm içinde bakılan parçayı/ateşi ısıtır, "Tutuşuyor %"). **[C]** = 7 taştan ateş yeri (envanterde 7 taş). **R artık yalnızca yatır/dik**, "ateş modu" ve `ECarryPlace::Fire` kaldırıldı (kütüğe tıklayınca ateş çıkması bug'ıydı). HUD: bakılan parçada "YANIYOR n sn" / "yanık".
- Testler: çakmak ısı birikimi (yarıda yalnızca ısınır), yayılma (değen yanar, 4 m ötedeki ısınmaz), yakıtın yarısında %50 hasar ve sonunda yanık, 12 sınırı, yanan direk kirişi indirir, yanan parçayla ateş alma, 7 taşlı ateş yeri (taşsız kurulmaz). Eski ateş testleri çakmak akışına çevrildi.
- Bulunan hatalar: ısınan parça ısı alırken de soğuyordu (yayılma yarı hızdaydı); yanan direk testi direğin yakıtına göre süre almalıydı; testte ateş yeri zemin dışına kuruluyordu (boş gösterici çökmesi).
- **Kalan (Faz 8):** taş toplama/yerleştirme taş-taş yapılmadı (7 taş envanterden harcanıyor, tek tek halka yerleştirme yok); çakmak sınırsız; yanık parça hâlâ yakıt olarak atılabiliyor; yanma alev/ışığı gerçek oyunda gözle görülmedi. KOD_HARITASI.md yeni paket klasörlerini hâlâ taramıyor.

### 2026-10-14 — Faz 7, 8b, 9, 10, 11, 12, 13, 14 (CC)
Kullanıcı istedi: "plandaki fazları hepsini entegre et, en son derle". Tek seferde yazıldı, sonra derlendi: ilk derleme hatasız; mekanik test **101/101** (+20 yeni), Wood **12/12** (+1: tezgâh). Görsel kısım (çizgiler, alev, HUD, yeni aktörler) oyunda gözle denenmedi.
- **Faz 7:** R üçlü döngü: yatır → dik → **eğik** (tekerlek açıyı 15–75° değiştirir; `ComputeSlopePlacement`, `ECarryPlace::Slope`). Yatan, yapıdaki bir kütüğün üstüne dik ya da eğik parça konabilir. **[V]** parçayı geçici (iskele) işaretler. Testte 30° parça "Rampa" tanındı, geçici iki parça "İskele" tanındı, kiriş üstüne direk dikildi.
- **Faz 8b:** Taşlar tek tek konur (**[C]**, `AFireStone`); 7. taş halkayı tamamlayınca ateş yeri kurulur, taşlar toplanır.
- **Faz 9:** Ağaçtan çıkan kütük sayısı ağaç boyundan: `AHarvestableActor::LogsFromHeight` (boyun %85'i, en çok 4); gövde ve yarım gövde kendi sayısını taşır. Tarif tuşları 1,2,4,5,6,7 kaldırıldı (3 = taş bloğu kaldı). Test, ağaç boyuna göre beklenen sayıyı kullanıyor; kısa ağaçta eksik kütüğü test tamamlıyor.
- **Faz 10 (kısmi):** `ELogMotion` Loose / Carried / Installed (`GetMotion`); **[X]** it (`Push`); sert çarpma hasarı (`AddImpactDamage`, eşik 1,2 milyon, `NotifyHit`). **Yapılmadı:** uzun parçayı sürükleme, bir ucu geçici destekle kaldırma, eğimde yuvarlanmanın testi.
- **Faz 11:** **[O]** duvardaki kütükten 2 modüllük açıklık keser (`CutOpening`, iki yanda en az 1 modül kalır, yapıdaki parçalar yapıda kalır). Yeni alet `ECarveTool::Groove` ("Oluk", boyuna). **Yapılmadı:** açıklık önizleme çizgisi, açılır kapı; "kapı kasası" tanıması açıklıktan sonra denenmedi.
- **Faz 12:** `AHaulCart` (el arabası 2 yük, kızak 4, sehpa 8 omuz yükü; hacim korunur, yüklenen parça Carried, boşaltılan Loose; [X] ile itilir, sehpa itilmez). Admin **[U]** üçünü önüne koyar. **Yapılmadı:** halat hattı, su üstü taşıma, arabanın odundan yapılması (şimdilik yalnız admin ile çıkıyor).
- **Faz 13 (kısmi):** Kısa bacaklı (<100 cm) platform **Tezgâh** olarak tanınıyor (`EWoodFeatureKind::Table`, birim testi). Sivri kazık düşmana zarar verir, fırlatılan/itilen kütük çarpınca düşmana zarar verir (`HitEnemy`). **Yapılmadı:** sal, yaylı/tuzak mekanizması (kütük tuzağı), mobilyanın kullanımı.
- **Faz 14 (kısmi):** `ARaidEnemy` (hedef: oyuncu, yoksa en yakın yapı; duvarı yıkar, kazığa gelince can kaybeder, baltayla 25 vurulur) ve `RaidKit::StartDefaultRaid` (paketin Raid yöneticisi koddan kurulan plan ve varlıklarla çalışır). Admin **[Z]** baskın başlatır. Paketteki `IsOnNavMesh` haritada gezinme ağı yoksa açık zemini geçerli sayacak şekilde düzeltildi. Testlerde: baskın başladı ve düşman doğdu, düşman orta direği yaktı ve kiriş kırıldı, kazık 25 can/sn aldı. **Yapılmadı:** ses (hiçbir mekanikte yok), düşman görseli (kırmızı direk), yol bulma (düz çizgi), oyuncuya hasarın testi, otomatik gece baskın takvimi.
- Bulunan hatalar: UE'de `NormalImpulse` büyük çıktığı için ilk eşik (400 bin) köşe testini bozdu (alt kütüklere hasar); paketin spawn bulucusu gezinme ağı yok diye hiç yer bulamıyordu; düşman testinde direkler yanlış eksende diziliydi.


### 2026-10-14 — Faz 0 kaynak denetimi (devam ediyor)

Bu çalışma, önceki kod geliştirme fazlarından ayrı bir kabul/kanıt turudur; yeni kullanıcı demosu değildir.
- 14 fazın kabul kapıları ve takip CSV'si repo dalına eklendi. Kullanıcıya tek demo Faz 15 sonunda verilecek; iç doğrulama senaryoları teslim paketi sayılmaz.
- Mekanik kataloğu denetleyicisi ve 9 birim testi eklendi. Denetleyici kimlik tekrarlarını, şema sorunlarını, U0/unclassified satırları, seviyeye göre kaynak/test/editör kanıtını ve boş blocker/not alanlarını görünür kılar.
- GitHub Actions `Python tooling checks` başarılı sonucu verdi (run #20); bu yalnızca Python araç testidir, Unreal Engine derlemesi değildir.
- LOG-001–LOG-076 için kaynak kanıt denetimi `Docs/FAZ_00_LOG_KANIT_DENETIMI.md` içine yazıldı. Ana `ACarvableLog` ağacı/kütük/kesim/taşıma/yapı/fire loop çekirdeği korunacak; uzun halat/zipline, su üstü taşıma, bazı yapı türleri, makine/tuzak ve genel salvage kapsamları ayrı kanıt veya geliştirme istiyor.
- Kaynak denetimi, fiziksel envanter köprüsünün `ALogActor/AStoneActor/AStickActor` sınıflarıyla çalıştığını ama oynanış kütüğünün `ACarvableLog` olduğunu; inventory drop metodunun doğrudan fiziksel aktör üretmediğini; raid yapısal hasar resolver'ının `ACarvableLog` yolunu göstermediğini; WorldPersistence'in chunk/world-state verisini kaydedip tek başına canlı yapı/araç/oyuncu aktör state'ini kapsamadığını teyit eden riskleri belgeledi.
- 485 katalog kaydının tümünü kaynak zinciri, test kimliği ve editör kanıtıyla eşleştirme bitmedi. Faz 0 açık. UE 5.8 derleme, editör ve paket testi gerçek hostta yapılmadan başarılı olarak işaretlenmeyecek.


### 2026-10-14 — LOG kataloğu satır statüsü güncellemesi

`Docs/FAZ_00_LOG_KATALOG_ESLESTIRME.csv` içindeki 76 LOG satırı kaynak taramasıyla sınıflandırıldı:
- **59 U2:** kaynakta yerel davranış izi var; satır özelinde otomatik test/editör kanıtı bekliyor.
- **17 U0:** bu statik incelemede uygulanmış davranış zinciri bulunamadı veya özellik başka bir geliştirme gerektiriyor.
- **0 U4/U5/U6:** bu tur satır özelinde otomatik test, editör görsel doğrulaması veya uzun oturum/kayıt/performance kabulü iddia etmiyor.

Python tooling CI, bu CSV'yi beklenen 76 satırla keşif modunda denetleyecek şekilde güncellendi. Bu ilerleme LOG ailesinin kaynak haritalamasıdır; kalan 409 katalog maddesini ya da UE 5.8 oyun içi doğrulamasını kapatmıyor.


### 2026-10-14 — 485 katalog satırının kaynak triage haritalaması

Satır bazlı eşleştirme dokuz aileyle 485/485'e ulaştı:
- LOG 76
- INVENTORY 32
- RAID 63
- WORLD 35
- MATERIAL 38
- FARMING 57
- CORE 12
- STONE 96
- STICK 76

İlk statik kaynak seviyeleri: U2=333, U1=88, U0=64; U3–U6=0. U2/U1 kaynakta bir algoritma/model izinin bulunmasıdır; bu, katalog satırının uçtan uca geçtiği anlamına gelmez. Her satırda isimli otomatik test sonucu ve editör/oyun içi kanıtı bekliyor; U0 maddelerinin uygulama ya da doğru kaynak sahibi bulunması gerekiyor.

`Scripts/run_python_tests.sh` 9 CSV'yi beklenen satır sayılarıyla audit ediyor. Güncel Python CI run #88 başarılı: https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38510701119. Bu sonuç UE 5.8 derleme veya oyun testi değildir. Faz 0 açık; tek final demo ilkesi değişmedi.


### 2026-10-14 — Çubuk crafting işlem atomikliği

- `UStickInventoryComponent` için toplu girdi preflight, duplicate item aggregation, tüketim sonrası kapasite hesabı ve snapshot rollback içeren native transaction API eklendi.
- `UCraftingComponent` artık craft output/input için bu API'yi kullanıyor; `bRequiresTool` kontrolü eklendi; şekil state'i sayım envanterinde olmadığı için `RequiredShape` içeren tarifler fail-closed şekilde reddediliyor.
- Üç Unreal Automation Test eklendi ve UE host çalıştırıcısı/log doğrulayıcısı ayrı `MyProject2.Inventory.StickCraftTransaction` suite'ini zorunlu kılıyor.
- Bu kodun Unreal Engine 5.8 derlemesi/otomasyon sonucu bu ortamda alınmadı. Güncel GitHub Actions yalnızca Python tooling ve katalog kontrollerini çalıştırır; C++ doğrulaması, kullanıcı tarafından UE hostunda `bash Scripts/run_tests.sh` çalıştırılana kadar açık kalır.


### 2026-10-14 — Kırma işlemlerinde kısmi çıktı geri alma

- `UStoneWorkingComponent::CrushBoulder`, istenen tüm taş parçalarını oluşturamadığında spawn edilmiş parçaları yok eder ve kaynak kayayı korur. Parçalar kaynak `UStoneDefinition` ile initialize edilir.
- `UStickBreakingComponent::CompleteBreak`, kısmi spawn başarısızlığında çıktıları geri temizler, kaynak çubuğu korur ve sürekli frame başına tekrar denemeyi durdurur.
- Taş `HollowGroove/EngraveJoint/SmoothSurface` no-op metotları artık geometri değişimi yapmadan başarı döndürmez; gerçek işlemler hâlâ backlog.
- P0-024 ve P0-026 kod düzeyi üzerinde ilerletildi; UE 5.8 build/automation sonucu henüz yok.


### 2026-10-14 — Faz 1 kazı teknolojisi spike hazırlığı

- İki runtime kazı adayı tanımlandı: sparse 3B density/SDF delta ve Dynamic Mesh lokal CSG/boolean.
- Ortak test kümesi ve 25 cm/50 cm ölçüm koşuları `Docs/FAZ_01_KAZI_TECHNOLOJI_SPIKE.md` ve `Docs/FAZ_01_KAZI_SPIKE_SABLONU.csv` içinde belgelendi.
- `Scripts/validate_excavation_spike.py` + 7 validator testi eklendi; Python CI, boş/yarım ölçümle “seçildi” kararına geçilmesini engelleyecek.
- CI runner'ı 14 adımlı Python/shell/katalog/spike matrix check düzenine getirildi.
- Durum: Faz 1 sadece şartname/validator hazır; ölçüm/teknoloji seçimi bekliyor ve Faz 0 kabul kapısına bağlı. Unreal 5.8 hostta benchmark yapılmadı.


### 2026-10-14 — Taş yerleştirme lifecycle ve face/socket seçimi

- `UStonePlacementComponent` için açık `BeginPlacement`, `CancelPlacement` ve doğrulamalı `UpdatePlacementTarget` eklendi.
- `CycleFace` artık geçerli tanımlı yüzleri ve socket'leri sıralıyor; seçilen yüzün yerel normalini yüzeyin ters normaline, seçilen socket'in yerel konumunu temas noktasına hizalıyor. Yüzey normali Z ekseniyle hizalanıyor (normal vektörünün X yönüne çevrilmesi hatası önlendi).
- Geçerli trace/placement hedefi olmadan `ConfirmPlacement` commit etmiyor. İptal, özgün actor transform/state/attach bilgilerini geri yüklemeyi dener.
- `Source/MyProject2/Tests/StonePlacementTests.cpp` içine üç UE Automation Test eklendi. `Scripts/run_tests.sh` Stone Placement suite'ini ayrı logla çalıştırıp parser üzerinden minimum üç testi zorunlu kontrol ediyor.
- Derleme ve UE 5.8 otomasyon sonucu henüz yok; bu nedenle bu kod düzeyi ilerlemedir, kabul edilmiş feature değildir.


### 2026-10-14 — Fiziksel taş yerleştirme: input + graph bağlama

- `AForestCharacter` içine `UStoneCarryComponent` ve `UStonePlacementComponent` bağlandı. E ile fiziksel taş alma/bırakma; sol tık ile önizleme/onay; Q/tekerlek ile yüz/socket seçimi; R ile döndürme; sağ tık ile iptal giriş akışı eklendi.
- `CarrySpeed` ve `IsHandsBusy` fiziksel taş taşımasını hesaba katıyor; pickup/drop/confirm/cancel sırasında tool görünürlüğü ve hareket hızı güncelleniyor.
- Yerleştirme mesh sınırıyla destek düzlemine oturtuluyor ve `ComponentOverlapMulti` ile bloklayan fiziksel çakışma aranıyor.
- Yapısal hedef actor graph sahibiyse, selected socket yalnızca hedef taşta yakın ve boş socket ile eşleştiğinde `UStructuralGraph::ConnectTo` üzerinden iki uçlu bağlantı commit ediliyor. Commit başarısız olursa `Installed` state'i verilmez; state geçişi başarısızsa yeni graph link'i geri alınır.
- Socket occupancy artık ortak `UStoneDefinition` asset içindeki transient alanlardan okunmuyor; per-actor graph connections temel alınır.
- Beş UE Automation Test yazıldı ve `Scripts/run_tests.sh` / `verify_test_logs.py` bu suite için minimum 5 farklı başarılı sonuç ister.
- GitHub Actions Python tooling/shell/katalog kontrolleri başarılı; bu C++ kodu **UE 5.8 üzerinde henüz derlenip test edilmedi**. P0-023/P0-030/P0-032/P0-033 “In Progress” durumunda kalır.


### 2026-10-14 — Taş yerleştirme: graph socket occupancy ve mesh-local hizalama

- Yapısal hedefe yerleştirme artık sadece Installed state'i vermiyor. Hedef AStoneActor / UStructuralGraph ise kaynak yüzü veya socket'i hedef taşın yakındaki doğru yöne bakan boş socket'iyle eşleştirilmeden commit edilmez.
- UStructuralGraph::ConnectTo iki uçta da bağlantı kaydı oluşturuyor. State geçişi beklenmedik biçimde başarısız olursa bu işlem sırasında yaratılan link geri alınır.
- Kaynak/hedef socket dünya pozisyon ve normal hesapları UStaticMeshComponent::GetComponentTransform() üzerinden yürütülüyor; böylece yontulmuş/parçalı taşların mesh-relative scale/transform bilgisi göz ardı edilmiyor. Mesh bounds'ın temas düzlemine oturması da component-relative transform'u hesaba katıyor.
- Kullanılmış local socket/face adları actor graph'ından türetiliyor; hedefte kullanılan socket ikinci bağlantı için sunulmuyor.
- StonePlacementTests.cpp beş C++ Automation Test içeriyor: begin/cancel, face/socket hizalama, carry/confirm rollback, collision clearance ve reciprocal graph/occupied-socket testleri. verify_test_logs.py en az beş başarılı benzersiz sonuç ister.
- Son Python tooling CI kaynak dosyalarını derlemez. UE 5.8 Windows build, beş C++ otomasyon testi ve görsel editor testi henüz çalıştırılmadı; P0-023/P0-030/P0-032/P0-033 In Progress olarak kalır.


### 2026-10-14 — Stone Trap damage dispatch

- StoneTrapActor TriggerVolume artık QueryOnly/Pawn-overlap yapılandırmasına sahip ve BeginOverlap ile Victim dispatch çağırıyor.
- Trigger hedef hasarını mevcut alıcı metotlarına yönlendiriyor: ARaidEnemy::TakeHit, AForestCharacter::TakeEnemyHit, AStoneActor::ApplyDamage, ACarvableLog::AddDamage. Diğer aktörlerde UGameplayStatics::ApplyDamage fallback kullanılıyor.
- Kütük hasarı için configurable raw-damage-to-normalized-fraction çarpanı eklendi.
- Rearm, cooldown süresi tamamlanmadan tuzağı tekrar aktif hale getirmiyor.
- İki UE Automation Test eklendi; Scripts/run_tests.sh ve log verifier güncelleniyor. UE 5.8 build/test henüz çalıştırılmadı.
