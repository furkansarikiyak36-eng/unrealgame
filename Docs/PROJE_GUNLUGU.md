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
