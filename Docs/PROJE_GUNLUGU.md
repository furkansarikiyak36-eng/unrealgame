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


### 2026-10-14 — Stone Trap fiziksel hedef overlap kapsamı

- TriggerVolume overlap tepkisi Pawn yanında PhysicsBody kanalına da açıldı; fiziksel taş/kütük gövdeleri de trigger event üretebilir.
- StoneTrap otomasyon testinde iki collision kanalının overlap ayarı assertion ile kontrol ediliyor.
- Son kaynak diff'i için Python tooling CI tetiklendi; UE 5.8 derlemesi ve C++ testleri bu ortamda çalıştırılmadı.


### 2026-10-14 — Stone Trap alıcı kapsamı

- StoneTrap C++ testleri 4'e yükseltildi: düşman/cooldown, null victim, taş health, CarvableLog normalized damage.
- Log verifier ve sentetik Python testleri artık en az dört ayrı StoneTrap Automation Test path'i gerektiriyor.
- UE 5.8 derleme ve gerçek Automation Test koşusu henüz yapılmadı; Python CI sadece log verifier/katalog/araç testlerini çalıştırır.

### 2026-10-14 — Faz 0 bulgu matrisi bütünlük kontrolü

- Docs/FAZ_00_BULGU_MATRISI.csv satır bazında tarandı; P0-012 ayrı bulgu satırı olarak eksikti, kimliği P0-013 durum alanına \nP0-012 metniyle karışmıştı.
- P0-012 ilk matris commit'inin geçmişinden geri kuruldu; teslimat kararı güncellendi: V1–V4 yalnızca iç doğrulama, kullanıcının tek Windows demo paketi Faz 0–14 ve release gate'leri tamamlandıktan sonra.
- P0-013 durumu yeniden Open yapıldı ve P0-010 severity alanındaki baştaki boşluk temizlendi.
- Scripts/audit_findings_matrix.py ile şema, alan sayısı, zorunlu hücreler, severity, tek satırlı status, benzersiz/ardışık Finding_ID denetimi eklendi. Dokuz birim test yeni Python CI listesine bağlandı.
- UE 5.8 C++ derlemesi ve motor otomasyon testleri bu değişiklikle çalıştırılmış sayılmaz; bu matris denetimi yalnızca belge bütünlüğünü doğrular.

### 2026-10-14 — RAID katalog sayımı kanıtı

- Ana RAID başlığı 69 alt mekanik bildiriyor; aynı kaynakta yedi alt bölümün sayıları 6+7+9+12+10+10+9=63 ediyor.
- Ayrıntılı isimli liste ve repo RAID eşleştirmesi 63 kayıt içeriyor. Altı ek gereksinimin isimleri/ID'leri kaynakta bulunmadığından uydurularak eklenmedi.
- P0-001, onaylı kaynak kararıyla başlığın 63'e çekilmesi veya eksik altı gereksinimin asıl kaynaktan kurtarılması tamamlanana kadar açık bırakıldı.

### 2026-10-14 — Kaynak katalog repo içine alındı ve kimlik eşleştirmesi sıkılaştırıldı

- Kütüphanedeki orijinal 485 satırlık MEKANIK_KATALOG_TAKIP_CSV.csv, Docs/MEKANIK_KATALOG_TAKIP_CSV.csv konumuna değişmeden alınmıştır; bu bir kanonik gereksinim snapshot'ıdır. Tüm satırlar hâlâ U0 durumundadır; eklemek onların doğrulandığı anlamına gelmez.
- Scripts/validate_catalog_mapping.py, ana katalogdaki 485 Mechanic_ID'nin dokuz türetilmiş CSV'ye doğru sistemde, eksiksiz ve bir kez eşlendiğini denetler.
- Scripts/tests/test_validate_catalog_mapping.py kapsamında 8 hata/başarı senaryosu eklendi.
- Scripts/run_python_tests.sh ana katalog snapshot'ını ve tüm eşleştirmeleri CI içinde doğrulayacak şekilde güncellendi; ana CSV için ayrıca keşif raporu çalıştırır, U0 kanıt boşluklarını geçerli kabul etmezmiş gibi göstermeden açıkça listeler.


### 2026-10-14 — InventoryComponent kapasite preflight ve rollback

- \`UInventoryComponent::CanFitItem\` önce bütün \`Slots\` dizisini sayarken \`FindEmptySlot\` yalnızca \`min(MaxSlots, Slots.Num())\` içinden çalışıyordu. \`MaxSlots\` oyun sırasında düşürülürse ön kontrol gerçekte kullanılabilir olandan fazla kapasite sayabiliyor ve \`AddItem\` kısmi mutasyonla hata dönebiliyordu.
- Stack araması, preflight, serbest/kullanılan slot hesapları ve save restore kapasite hesabı aynı etkin slot aralığına bağlandı.
- \`AddItem\` ve \`AddItemData\` beklenmeyen yerleştirme başarısızlığında öncesindeki slot snapshot'ını geri yüklüyor.
- \`Source/MyProject2/Tests/InventoryCapacityTests.cpp\` içinde kapasite küçülürken kısmi eklemeyi reddetme/rollback ve geçerli ekleme testleri eklendi.
- Bu C++ testleri UE 5.8 hostunda henüz çalıştırılmadı; build/Automation kabulü bekliyor.


### 2026-10-14 — Inventory save restore failure rollback

- RestoreFromSavedItems önceki envanteri kapasite/ağırlık kontrolünden önce ClearAll ile siliyordu; reddedilen veya kısmen başarısız bir restore mevcut stoğu kaybettirebilirdi.
- Restore artık mevcut slotların snapshot'ını alır; restore sırasında OnInventoryChanged ve OnItemAdded bildirimlerini bastırır, başarısızlıkta eski slotları geri koyup normalize eder ve tek nihai değişiklik bildirimi yapar.
- Negatif count/weight veya geçersiz stack metadata içeren kayıtlar fail-closed reddedilir.
- InventoryCapacityTests.cpp içine oversized save verisinin reddedilmesini ve eski stoğun korunmasını sınayan üçüncü UE Automation testi eklendi.
- Unreal Engine 5.8 build/Automation bu ortamda çalıştırılmadı; motor doğrulaması beklemede.


- Ek doğrulama: FInventoryItem::IsEmpty() negatif Count değerleri için de true döndürdüğünden, restore döngüsünde Count < 0 kontrolü IsEmpty() sonrasındaydı ve bozuk negatif kayıt sessizce atlanabiliyordu. Kontrol sırası düzeltildi, NaN/sonsuz/negatif ağırlık da reddediliyor.
- Inventory.Core.Capacity altında negatif Count girdisini reddedip mevcut stoğu koruyan dördüncü UE Automation testi eklendi; log parser minimumu 4 benzersiz teste çıkarıldı.


### 2026-10-14 — Inventory item definition validation fail-closed

- UInventoryItemData::ValidateData artık NaN/sonsuz ağırlığı da reddediyor; negatif kontrolü tek başına NaN için yeterli değildi.
- BeginPlay item definitions'ı kaydetmeden önce doğruluyor; global item registry de invalid tanımları kaydetmiyor.
- CanAddItem, AddItem ve AddItemData geçersiz data/payload'ı reddediyor; tanımsız raw payload için MaxStack ve finite/non-negative Weight kontrolü de eklendi.
- InventoryCapacityTests.cpp içine NaN weight için definition validation, CanAddItem/AddItem ve tanımsız AddItemData reject testleri eklendi. Inventory.Core.Capacity minimumu 5 benzersiz UE automation sonucuna çıkarıldı.
- Bu testler UE 5.8'de henüz çalıştırılmadı; Python CI sadece log parser ve scriptleri doğrular.


### 2026-10-14 — UE test runner: başarısız koşuda raporları koruma

- Önceki akış, UE süreç kodu veya test-log doğrulaması başarısız olduğunda bile Scripts/update_docs.py çağırabiliyordu.
- KOD_HARITASI.md ve TEST_DURUMU.md üretimi artık yalnızca bütün UE süreç çıkış kodları ve fail-closed log doğrulaması başarılıysa çalışır; aksi durumda mevcut raporlar değiştirilmez.
- Scripts/tests/test_runner_fail_closed.py ile bu kapıyı ve tüm suite süreç kodlarının overall status'a katılmasını denetleyen dört statik test eklendi; Python CI çalıştırıcısına bağlandı.

### 2026-10-14 — Implementation-first katman değişimi ve kazı veri çekirdeği (Web)

Kullanıcının önceliği netleştirildi: 485 mekaniği satır satır denetleyip kodlamayı bekletmek yerine, mevcut MyProject2 koduyla uyumlu katmanlar ve adaptörler kurularak bütün mekanikler geliştirilecek; kapsamlı bütüncül hata/regresyon/performance analizi Faz 14'te yapılacak. UE 5.8 testleri Claude Code/UE host akışına ait; bu oturumda çalıştırılmadı ve çalıştırılmış gibi raporlanmayacak.

- Docs/PLAN_14_FAZ_UYGULAMA_OMURGASI.md implementation-first L0–L4 katmanlarına göre güncellendi. Faz 14 artık 485 mekanik ve tüm sistemler için toplu analiz/hata kapısı.
- Docs/FAZ_TAKIP.csv içinde Faz 0 kaynak-triage baseline'ı tamamlandı olarak işaretlendi; U0/U1/U2 mekanik durumları kodlamayı bloke etmiyor. Faz 1 kazı veri temeli çalışması olarak başlatıldı.
- 485 mekanik ana katalogda 333 U2, 88 U1 ve 64 U0 olarak aile eşleştirmeleriyle eşitlendi; 485 unique ID var. Bu dağılım oyun içi tamamlanma iddiası değildir.
- Inventory-to-world bridge düzeltildi: null world/actor class/invalid item kontrolü eklenip fiziksel spawn sonrası DropItem çağrılıyor; drop reddedilirse spawned actor temizleniyor. Kaynak kontratını kontrol eden Python testi CI betiğine bağlandı; Python tooling CI run #417 başarılı.
- Faz 1 için WorldCore/ExcavationTypes.h, ExcavationVolumeSubsystem.h/.cpp eklendi. Sparse cell override, küresel carve/fill edit, sınır/cap doğrulaması ve operation-id tabanlı rollback koruması bulunuyor. Veri backend'i henüz mesh/collision, nav veya save/load sistemine bağlanmış değildir; bunlar ilgili sonraki katmanlar.
- Doğrulama: dosyalar branch'e kaydedildi; bu oturumda Unreal derleme veya motor otomasyonu çalıştırılmadı. Yeni C++ kodunun UE 5.8'de gerçek compile sonucu Claude Code tarafında alınmalı.

**Sıradaki:** Faz 1 veri API'sini mevcut world/chunk/persistence kontratlarıyla uyumlu hale getirmek ve consumer'ların yalnızca UExcavationVolumeSubsystem API'sini kullanmasını sağlamak; ardından Faz 2'nin ortak kimlik/sahiplik/sürüm sözleşmelerini modül sınırlarına uygulamak.

### 2026-10-14 — Kazı veri katmanına chunk indeksi ve seam API'si (Web)

- ExcavationVolumeSubsystem'e 3B cell→chunk koordinat dönüşümü (negatif koordinatlar için floor division), chunk başına override hücresi listesi ve editin etkilediği komşu chunk'ları döndüren API eklendi.
- Aktif sparse override varken cell boyutu veya chunk eksen boyutu değiştirilirse dünya-koordinat sorguları/editler reddediliyor; böylece önceden yazılmış cell kimlikleri sessizce farklı geometriye kaymıyor.
- Aynı hücreyi tekrar düzenleyen sonraki işlem, yoğunluk aynı olsa bile hücrenin işlem sahipliğini alır. Bu sayede önceki rollback yeni işlemin sonucunu yanlışlıkla silemez; rollback sırası doğrulanır.
- GitHub Actions Python tooling CI run #417 başarılıdır. Bu kontrol Python/shell/CSV araçlarını çalıştırır; yeni C++ kaynak için Unreal derlemesi değildir. UE testleri Claude Code host sürecinde çalıştırılacaktır.

### 2026-10-14 — Faz 1: kazı veri çekirdeği uygulandı (Web)

- Sphere, capsule ve axis-aligned box carve/fill API'leri ortak ApplyBoundedEdit transaction yolunu kullanıyor. Aday hücre kümesi doğrulanmadan canlı sparse state değiştirilmez; işlem boyutu MaxCellsPerEdit ile sınırlıdır.
- Density sorgusu sparse override'ı base terrain provider sonucundan önce çözümler. UExcavationBaseDensitySource Blueprint interface'i mevcut/prosedürel arazi katmanının temel density örneklemesini kendi sahipliğinde tutmasına izin verir.
- Sürümlü FExcavationVolumeSnapshot veri sözleşmesi ve all-or-nothing ExportSnapshot/ImportSnapshot eklendi. Import duplicate hücreyi, geçersiz density'yi ve limit aşımını reddeder; başarılı import chunk indeksini yeniden kurup reset olayı yayınlar.
- Chunk başına override indeksi, negatif koordinatlar için floor division, seam komşu chunk listesi, edit/rollback değişim event'i ve snapshot-reset event'i sağlandı.
- Faz 1 kapsamı veri çekirdeği olarak kodlandı. Gerçek terrain sınıfı provider implementasyonu, mesh/collision/nav üretimi ve genel world persistence ayrı sonraki fazlarda bağlanacak.
- GitHub Actions Python tooling run #459 başarılı; bu Unreal C++ derlemesi değildir. UE 5.8 testleri bu aşamada çalıştırılmadı; Claude Code/UE host akışına bırakıldı ve Faz 1 kodlaması bu nedenle bloke edilmedi.

### 2026-10-14 — Faz 2: dünya kimliği, idempotency ve kayıt zarfı

- `UWorldPersistenceSubsystem` eklendi: kanonik dünya ID'si, kaydedilen world seed, değişiklik revizyonu, kazı commit/rollback/reset olaylarını dinleme ve çekirdek kayıt yüklenirken yapay revizyon artışını bastırma.
- Sphere/capsule/box editleri için caller-provided `FGuid` ile idempotent command ingress eklendi. Aynı komut kimliği bellekte veya save/load sonrasında defterdeyse ikinci edit uygulanmıyor; geçersiz/reddedilmiş çağrılar deftere yazılmıyor.
- `UWorldCoreSaveGame` manifest, sparse excavation snapshot ve son 100.000 komut/olay anahtarını saklıyor. Yükleme manifest/defterleri doğruluyor ve ancak snapshot importer kabul ederse canlı kazı durumunu değiştiriyor. Commit ve rollback aynı operasyon ID'sini kullansa da farklı olay anahtarına sahip.
- Faz 2 statik Python kontrat testi CI betiğine bağlandı. Bu kod kazı delta'sı ve ortak save envelope temelini sağlar; terrain geometri, envanter ve diğer domain kayıtlarının tamamlandığını iddia etmez.
- Unreal Engine 5.8 build/Automation bu oturumda çalıştırılmadı; çalışma ağacına kaydedildiğinde yalnızca hafif statik CI kontrolü yapılır. Motor doğrulaması ve kapsamlı sistem denetimi Faz 14'te.

### 2026-10-14 — Faz 3: runtime voxel mesh/collision adapter

- `AExcavationTerrainChunkActor` eklendi: `UDynamicMeshComponent` ile 3B density örneklerini marching-tetrahedra izoyüzeyine çevirir, mesh üçgenlerini sınırlı tutar ve complex-as-simple collision'ı günceller.
- `UExcavationTerrainChunkSubsystem` eklendi: 3B görsel chunk aktörlerini kaydeder, kazı/rollback event'lerinde yalnızca etkilenmiş chunk'ları yeniler, snapshot reset/load olayında geometriyi tekrar kurar. Bu kayıt yalnızca renderer cache'idir; density ve simülasyon state sahipliği mevcut subsistemlerde kalır.
- Otomatik streaming oyuncu çevresinde 3D chunk'ları her tick sınırlı bütçeyle üretir; coordinate actor BeginPlay'den önce atanır. Varsayılan düz yüzey sağlayıcısı, yüklü Landscape proxy'lerini okuyabilen alternatif fallback sağlayıcısı ve aktif proje provider'ına öncelik eklendi.
- `MyProject2.Build.cs` içine Landscape modül bağımlılığı eklendi; `Scripts/tests/test_excavation_terrain_adapter_contract.py` statik kontrat testi Python CI'a bağlandı.
- **Açık kalan:** Landscape'in orijinal render/collision yüzeyinden voxel mesh'e yetki devri, malzeme stratification ve navigation güncellemesi henüz tamamlanmış kabul edilmez. Bu kodda geometri/collision üretimi var; UE 5.8 compile/Automation bu oturumda çalıştırılmadı. Faz 3 tracker bu nedenle In Progress durumunda bırakıldı.

### 2026-10-14 — Faz 3: malzeme ID'leri ve navigation adaptörü

- Ortak UExcavationBaseDensitySource sözleşmesine temel malzeme ID sorgusu eklendi. UExcavationVolumeSubsystem, sparse kazı deltalarını değiştirmeden base provider üzerinden ID örnekleyebiliyor.
- AExcavationTerrainChunkActor, FDynamicMeshMaterialAttribute üçgen ID'lerini ayarlıyor; StratifiedMaterialLayers ile provider ID'lerini mesh material slotlarına yönlendiriyor. Otomatik renderer spawn için projeye özel Blueprint chunk sınıfı seçme noktası eklendi.
- Landscape provider, GetPhysicalMaterialAtLocation değerini kullanıcı tarafından ayarlanabilen fiziksel materyal→katman ID eşleştirmesinden geçiriyor. Eşleştirme listesi terrain renderer subsystem üzerinden sağlanıyor; default ID 0 SurfaceMaterial kullanır.
- Mesh değişikliği ve temizliği sonrasında NavigationSystem nav octree güncelleme çağrısı eklendi; Python kontrat testleri material-ID, Landscape map ve nav update davranışlarını kapsayacak şekilde genişletildi.
- Açık kalan ana blocker: ayrı voxel mesh üretmek Landscape'in orijinal render surface/collision'unu otomatik kaldırmaz. Gerçek runtime delik/yetki devri veya Landscape collision'ını güvenli şekilde yönetme entegrasyonu ve UE 5.8 build/oyun içi kabul bitene kadar Faz 3 In Progress kalır.

### 2026-10-14 — Faz 4: yeraltı desteği, çökme, sızıntı ve tuzaklar

- AUndergroundSupportActor, mevcut ABuildingNodeActor/UStructuralIntegritySubsystem bağlantısına katılan dört destek profiliyle eklendi: timber post, timber brace, stone pillar ve reinforced beam. Varsayılan temel mesh görünürdür; BP sınıfı ile özelleştirilebilir. Islaklık temelli dayanıklılık kaybı ve destek durumu event'i var.
- UUndergroundHazardSubsystem, sparse excavation density event'lerine bağlı çatı açıklığı hesabı, çalışan desteğin yakınlık kontrolü ve sınırlı otomatik roof-cell carve ile cave-in davranışı sağlar. Large edit reconciliation ve stability işi tick bütçeleriyle kuyruklanır.
- Mevcut UWorldStateSubsystem 2B hücresinden yalnızca mevcut yüzey suyu hacmi veya eşik üstü soil moisture varsa sınırlı su çıkarılır; 3B cave cell water pocket oluşur ve açık alt hücreye sınırlı yerçekimi akışı yapar. Kaynak 2B hücredeki debit ile 3B pocket ayrı ayrı kaydedilir; kuru zeminde kaynaksız su oluşturulmaz.
- AUndergroundTrapActor, mevcut AStoneTrapActor hasar davranışını ve overlap akışını kullanır; oyuk boşluğu doğrulaması ve su cebi hacmi ile tetikleme girişi ekler.
- UWorldCoreSaveGame schema 2: underground support kimliği/transform/tip/dayanıklılık profili, 3D water pockets ve sızıntıdan etkilenen yüzey hücrelerinin su/nem değerleri eklenmiştir. Schema 1 core kayıtları için uyumluluk yolu korunur.
- `Scripts/tests/test_underground_hazard_contract.py` CI'a bağlandı; statik Python kontrat testleri başarılı (#587/#588). Unreal Engine 5.8 C++ build/gameplay testi çalıştırılmadı. Çökme/sızıntı denge ayarı ve bütünsel entegrasyon analizi Faz 14'te.



### 2026-10-14 — Faz 5: kütük onarımı ve yapı bağlantısı

- `ACarvableLog::CanReplace` artık onarım adayının gerçek kesit boyutlarını, split sayısını, hareket/taşıma durumunu, ateş durumunu ve eklem kayıtlarının boş olmasını doğrular; yalnızca eşit uzunluğa bakarak farklı profilli parça yerleştirilmez.
- `ReplaceDamaged` joint replay'i backup mesh üzerinden transaction yaptı. Bilinmeyen/başarısız kesim türü veya sınır dışı joint konumu onarımı iptal eder ve replacement mesh/metadata/transform geri alınır.
- Başarılı onarım mevcut `StructureLink::Settle` ile tekrar değerlendirilir; ardından `OnLogRepaired` yayınlanır. Mevcut MechanicsDemo gözlemcileri değişmeden kaldı.
- Yeni statik kontrat testi runner'a eklendi. UE 5.8 build ve görsel oyun içi doğrulama bu ortamda çalıştırılmadı; 485 mekanik için genel bütünsel denetim Faz 14'e bırakıldı.


### 2026-10-14 — Faz 6: envanter transfer işlem sınırı (devam ediyor)

- Mevcut `UInventoryComponent` API'sine `TransferItem` eklendi; ikinci bir envanter sistemi oluşturulmadı.
- Aynı kaynak/hedef, boş kimlik, geçersiz miktar ve yetersiz stok reddediliyor. Kaynaktaki canonical `FInventoryItem` payload korunuyor; hedef kapasitesi kaynak düşülmeden önce kontrol ediliyor.
- Kaynak düşümü sonrası hedef eklemesi beklenmedik şekilde başarısız olursa kaynak stoğu geri eklenmeye çalışılıyor; rollback de başarısızsa hata loglanıyor. Mevcut `DropItem` ve `bCanBeDropped` politikası değişmeden kaldı.
- `Scripts/tests/test_inventory_transfer_contract.py` eklendi ve Python runner'a bağlandı. Testler kaynak sözleşmesini kontrol eder; UE C++ build/Automation sonucu değildir.
- **Faz 6 devam ediyor:** fiziksel taş/çubuk aktörleri, storage, processing, construction/salvage ve save/load uçtan uca akışları henüz bu commit'te bağlanmadı. Bu nedenle faz tamamlandı olarak işaretlenmedi; genel çapraz-sistem audit Faz 14'te.


### 2026-10-14 — Faz 6: fiziksel taş/çubuk ve storage save/load entegrasyonu

- `AForestCharacter` stack kaynaklarını artık `UInventoryComponent` üzerinden sayar, ekler, siler ve tarif üretir; eski `TMap<EForestItem,int32>` stok kopyası kaldırıldı. Kütük/gövde taşıma yine `ACarvableLog`/mevcut fiziksel gövde akışında kalır.
- `ForestItems::GetInventoryID` ve `GetItemWeight` eklendi; `Stick` item ID'si, görünür `SM_GB_Stick` greybox mesh'i ve runtime item tanımı eklendi. Yalnızca serbest ham `AStickActor` stoğa dönüştürülür; işlenmiş/yapıya bağlanmış parça silinmez.
- `DropInventoryItem` envanterden önce miktar/politika önkontrolü yapar. Genel kaynakları pickup yığını olarak, taş ve çubuğu gerçek fiziksel aktör olarak çıkarır; kısmi spawn başarısızlığında geçici aktörleri temizler, başarılı spawn grubundan sonra kaynağı bir defa düşer. Envantere tam sığmayan pickup dünyada kalır.
- `TransferItem` kaynak/hedefin slot snapshot'larını ve event suppression durumunu saklar; commit başarısızsa iki tarafın eski slot düzeni aynen geri gelir. `ExchangeItems` processing/tarif girdisi ile çıktısını aynı işlemde günceller; başarısızlıkta stok kaybı/çoğalması önlenir.
- `AInventoryStorageActor` aynı envanter ve transfer API'sini kullanır. World save schema 3 oyuncu stack stoğuna ek olarak her depoyu benzersiz anahtarla sıralı snapshot'a alır; load sırasında tüm depoların anahtarı/miktarı/kapasitesi önceden doğrulanır. Schema 1/2 kayıtları geriye dönük desteklenir.
- Statik kontratlara fiziksel kaynak, storage ve schema-3 doğrulamaları eklendi. CI #675'te görülen tek hata test beklentisinin eski `CanDropItem(ItemID, Count)` metnini aramasıydı; gerçek API `CanDropItem(ID, Count)` olduğundan test bu sözleşmeye düzeltildi. Son dal head'i için temiz CI koşusu henüz doğrulanmış değil. Unreal Engine 5.8 C++ build/Automation bu ortamda çalıştırılmadı.
- **Açık:** gerçek UE E2E'de depo transferi, processing, physical drop/pickup, build/salvage ve save/load turları. Bu fazlar arası bilanço ve tam çapraz-sistem audit Faz 14'e ayrıldı; bu log ara demo/paket teslimi anlamına gelmez.


### 2026-10-14 — Faz 6: statik CI doğrulaması

- Güncel dal head'i için Python tooling CI **#696 başarılı** (run `38530520494`). `InventoryTransferContract` ve `Phase6InventoryIntegration` içindeki iki assertion uyuşmazlığı düzeltilerek tekrar çalıştırıldı.
- Bu sonuç yalnızca Python tooling/statik kaynak sözleşmelerinin geçtiğini gösterir; Unreal Engine 5.8 C++ derlemesi, Automation, Blueprint/oyun içi akışlar veya görsel kabul çalıştırılmadı.
- **Faz 6 durumu:** kod + statik CI hazır; depo transferi, işleme, physical drop/pickup, build/salvage ve en az iki save/load turunun UE E2E kabulü bekliyor. Bu nedenle faz takipte In Progress kalır; Faz 14 toplu 485 mekanik denetimi daha sonra.


### 2026-10-14 — Faz 6: uygulama katmanı kapatıldı

- Faz 6 tracker'ı artık `Implemented` durumunda: tek envanter sahibi, atomik transfer/tarif dönüşümü, fiziksel taş/çubuk drop/salvage köprüsü, storage aktörü ve schema-3 oyuncu/depo snapshot'ları kodlandı.
- Python tooling CI #696 başarılıdır (run [38530520494](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38530520494)); source sözleşmelerinin geçtiğini doğrular, UE C++ build veya oyun testi değildir.
- Kalan motor kabulü Faz 14'e taşındı; sonraki kod fazına geçişi bloke etmez. Demo/paket yalnızca son bütünsel denetimden sonra.


### 2026-10-14 — Faz 7: tek malzeme durumu ve bütçeli hava entegrasyonu

- `ACarvableLog` içine `MoistureContent`, `MaterialAgeDays` ve `SimulateMaterialEnvironment` eklendi. Hasar/çürüme bütünlüğün tek `Damage` değerine yazar; mevcut gevşeyen eklem/taşıma eşiği geçilince `StructureLink::Settle` çalışır.
- `UWoodMaterialSubsystem`, `UWorldStateSubsystem` günlük saatini, sıcaklığı ve yağışını kullanır. Yağmur gökyüzü görüşüyle sınırlanır; log listesi ada/path ismine göre sıralanır ve tick başına işleme `MaxLogsProcessedPerTick` ile sınırlandırılır.
- `UBurnSubsystem` mevcut yanma sahibi olarak tutuldu. Nemli ahşabın tutuşma süresi ve yakıt süresi uzar; malzeme adaptörü yangın durumu/yanan listesi oluşturmaz.
- Opsiyonel `UMaterialDefinition` profilinden WaterAbsorption/WaterDamageRate/DecayRate okunur; profil yoksa varsayılan ahşap katsayıları kullanılır. Hasar etkileri mevcut ACarvableLog/StructureLink zincirine döner.
- `Scripts/tests/test_wood_material_integration_contract.py` oluşturuldu ve Python CI runner'a bağlandı. CI henüz bu yeni kaynak üzerinde yeniden çalıştırılmadı. UE 5.8 build/Automation çalıştırılmadı.
- **Açık:** ortak world-save zarfının dinamik ahşap aktörlerini yeniden oluşturmaması nedeniyle nem/yaş alanları şu anda yalnızca aktör durumudur; kalıcılık, dünya hücresi nemiyle korelasyon, yangın/çürüme denge ölçümü ve uzun süreli performans Faz 14'te çapraz sistem denetimine bırakıldı. Faz 7 tracker bu nedenle In Progress.


### 2026-10-14 — Faz 7: statik CI doğrulaması ve uygulama katmanı kapanışı

- Python tooling CI **#737 başarılı** (run `38531310496`); Phase 7 wood material/structure sözleşme testinin 6 kontrolü geçti. Runner toplam katalog/araç testlerini de başarıyla tamamladı.
- Bu CI Unreal Engine 5.8 C++ build/Automation, aydınlatma/görsel test, dünya aktörü save/load veya uzun süreli çürüme/yanma dengesi testi değildir.
- Tracker Faz 7'yi `Implemented` durumuna aldı. Nem/yaşlanma alanlarının ortak dünya-kayıt zarfında dinamik aktörleri yeniden oluşturmaması, dünya hücresi nemiyle tam eşleme ve performans/denge kabulü Faz 14 çapraz-sistem audit'inde kalacak; sonraki faz kodlamasını bloke etmez.


### 2026-10-14 — Faz 8: ortak World State ve kayıt katmanı

- UWorldStateSubsystem su/toprak/kimya/biyokütle/yangın/çürüme akışını tek işlem sırasına bağladı. Toprak nemi biyokütle nemine taşınır; toksin, biyokütle sağlığını ayarlanabilir ve sınırlandırılmış katsayıyla etkiler. Hücre yangını, aynı nem alanını ateşleme/yayılım kararında kullanır.
- Ortak USimulationState günlük deterministik rüzgâr yönü taşır; world-state yönü bu kaynaktan okuyup hücre yangınına verir. Kütük yangını UBurnSubsystem sahibi olmaya devam eder.
- LastObservedWorldDay ile AccumulatedDeltaDays birbirinden ayrıldı. Her simülasyon adımı en fazla bir gün, pending catch-up en fazla 365 gün; tarihsel yağış tekrar üretimi en fazla 365 günle sınırlı. Daha eski pasif geçmiş performans için yaklaşık ele alınır, eksiksiz tarihsel replay garantisi verilmez.
- FChunkStateSnapshot / FWorldStateSnapshot ve schema-4 UWorldCoreSaveGame bağlantısı eklendi. Saat/hava/seed ve chunk/cell state doğrulanır; chunk-store için yeni map kurulmadan mevcut map değiştirilmez. Schema 1–3 kayıtlar geriye dönük okunabilir; bu kayıtlarda world-state snapshot yoksa mevcut hücre durumu otomatik silinmez.
- Statik sözleşme testi Scripts/tests/test_phase8_world_state_integration_contract.py eklendi ve Python runner'a bağlandı. CI/UE doğrulama sonucu ayrıca kaydedilecek; burada Unreal Engine 5.8 derlemesi veya oyun içi save/load koşturulmuş kabul edilmiyor. Daha derin cross-domain rollback ve performans ölçümü Faz 14'te.


### 2026-10-14 — Faz 8: statik CI doğrulaması

- GitHub Actions Python tooling CI **#794 başarılı** (run [38534211068](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38534211068)). Faz 8'in 7 statik dünya-durumu sözleşme testi ve mevcut tüm Python, save-contract, envanter, yeraltı ve 485 katalog kontrolleri geçti.
- CI sırasında şema 3'ü zorunlu bekleyen eski Faz 4 ve Faz 6 test kabulleri schema 4 ile uyumlu hale getirildi; yükleme kodu schema 1–4'ü kabul eder, schema 1–3 için yeni world-state snapshot alanı yoksa eski dünya hücreleri üzerine boş snapshot zorla uygulanmaz.
- Bu CI Python/shell/statik kaynak kontrolleridir; Unreal Engine 5.8 C++ derlemesi, gerçek SaveGame round-trip, uzun süreli simülasyon veya oyun içi E2E çalıştırılmadı. Bunlar, diğer fazlar arası state rollback ve performans kabulüyle birlikte Faz 14'e bırakılır.


### 2026-10-14 — Faz 9: tarım ve hayvancılık entegrasyonu

- ACropFieldActor mevcut UInventoryComponent ile tohumları ekim öncesi doğrular/düşer; büyüme mümkün olduğunda UWorldStateSubsystem hücresindeki nem, su ve NPK değerlerini kullanır. Hasat, kapasite ve feed tanımı doğrulandıktan sonra mevcut hayvan UFeedStorageComponent stoğuna aktarılır.
- AAnimalActor tüketilen yemden sınırlı manure biriktirir. Süt/yumurta/yün toplama ürün miktarını önce inventory'ye başarıyla ekler ve sonra üretim komponentinden düşer; kapasite yoksa hayvanın ürün stoku korunur. Manure, geçerli ortak dünya hücresine N/P/K olarak uygulanır.
- Eski UFeedCycleComponent kendi TMap<FName,float> feed stokunu tutan ikinci bir sahip olmaktan çıkarıldı; var olan UFeedStorageComponent üzerine çalışan bir adaptördür. Feed/seed/product ID'leri proje inventory tanımlarına göre sağlanmalıdır.
- Scripts/tests/test_phase9_farming_integration_contract.py sekiz statik sözleşme kontrolüyle runner'a eklendi. Son GitHub Actions Python tooling CI #837 başarılı (run 38535234996); yeni Faz 9 ve tüm mevcut Python/katalog kontrolleri geçti.
- Bu CI Unreal Engine 5.8 derlemesi, oyuncu envanteri ve hayvan/bitki actor SaveGame round-trip'i veya gerçek tarım ekonomisi E2E testi değildir. Dinamik actor kalıcılığı, kaynak bilançosu ve uzun oturum ayarı Faz 14'te toplu doğrulanacaktır.


### 2026-10-14 — Faz 10: bölge ve lojistik katmanı (uygulama)

- \`URegionLogisticsSubsystem\` eklendi. 8×8 chunk varsayılan bölge kimliği negatif koordinatlarda floor-division ile doğru hesaplanır.
- Bölge biyokütlesi, soil N/P/K, su hacmi ve çevresel risk özeti mevcut \`UWorldStateSubsystem/UChunkStateStore\` içinden sorgulanır; ikinci bir kaynak/tehlike stoğu yaratılmaz.
- Trail/road/bridge/ramp/sled-route/ropeway ortak rotalama grafına bağlandı. Mesafe, altyapı tipi, kapasite, operasyon durumu, kondisyon ve hem link hem bölge riski tahmini seyahat süresini etkiler. Arama 2.048 node / 8.192 link ile sınırlıdır.
- Köprü \`BuildBridge\` ve zipline \`BeginPlay\` akışları, sadece \`LogisticsLinkKey\` ayarlandığında rota grafına opsiyonel kayıt yapar. Ortak junction için aynı node key'leri kullanılır; anahtar verilmezse eski fiziksel davranış sürer.
- \`UWorldCoreSaveGame\` schema 5 route metadata snapshot'ını saklar. Schema 1–4 geriye uyumlu kalır; eski kayıtlar yeni boş grafla mevcut durumu silmez. Lojistik değişiklikleri ortak world revision/event akışına bildirilir.
- \`Scripts/tests/test_phase10_region_logistics_contract.py\` ve Python runner bağlantısı eklendi. GitHub Actions Python tooling CI #879 başarılıdır (run [38536422646](https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38536422646)); Faz 10 sözleşme kontrolleri ve mevcut Python/katalog testleri geçti. Unreal Engine 5.8 C++ compile/oyun içi rota, köprü/zipline ve save/load E2E bu ortamda çalıştırılmadı. Tam motor ve çapraz-sistem kabulü Faz 14'te.


### 2026-10-14 — Faz 11: üretim kuyruğu ve üretim stok zinciri (uygulama)

- \`UProductionRecipeAsset\`, \`UProductionChainComponent\` ve üretim job/snapshot sözleşmeleri eklendi. Mevcut \`ACraftingBench\` bileşeni kullanır; \`FBenchRecipe\` ile tanımlı girdiler artık stok tüketimi ve çıktı eklemesi yapan gerçek queue job oluşturabilir.
- \`UInventoryComponent::ExchangeItemBatches\`, çoklu girdileri topluca doğrular ve bütün slot düzenini snapshot/rollback ile atomik tutar. Kuyruk iş başlarken girdiyi tek defa ayırır; iptal girdileri aynı depoya geri verir. Çıktı kapasitesi doluysa job \`BlockedOutput\` olur ve ürün miktarı kaybolmaz.
- Sınırlı FIFO queue, batch miktarı, recipe doğrulaması, progress kaydı, pause/resume/cancel, power/worker gates eklendi. Yerleşik örnek \`HalfLog x1 -> Plank x2\`, 5 saniye, 100 W, 1 işçi; diğer tarifler data asset veya Blueprint plan API'siyle konfigüre edilebilir.
- World SaveGame schema 6, kararlı station key ile kuyruk snapshot'larını kaydeder. Envanter stok sahipliği yine player/AInventoryStorageActor/UInventoryComponent'te kalır; aktif queue kaydı için input ve output inventory sahiplerinin core save'da kaydediliyor olması doğrulanır. Schema 1–5 okunmaya devam eder.
- Repo içinde mevcut shared generator/power-grid sahibi bulunmadığından üretim gücü \`AvailablePowerWatts\` dış girdisidir; güç koşulu var ama generator/grid ekonomisine bağlanmış sayılmaz. Fiziksel kütükten stackable HalfLog üretimini otomatikleştirme, Unreal Engine 5.8 compile ve oyun içi queue/save-load E2E de henüz kabul edilmedi; Faz 14'e bırakıldı.
- \`Scripts/tests/test_phase11_production_chain_contract.py\` Python runner'a eklendi; CI sonucu son dal başında doğrulanacaktır.


### 2026-10-14 — Faz 11: statik CI kanıtı

- GitHub Actions Python tooling CI #938 başarılıdır: https://github.com/furkansarikiyak36-eng/unrealgame/actions/runs/38537881165
- Faz 11 üretim zinciri sözleşme testinin 12 kontrolü ve mevcut Python/katalog kontrolleri geçti. Bu iş akışı Unreal Engine C++ derlemesi veya oyun içi üretim/save-load testi değildir; motor kabulü Faz 14'te yapılacak.
