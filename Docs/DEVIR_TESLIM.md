# Devir teslim — 2026-10-21

Bu belge, projeyi devralan yapay zekâ aracı (ya da geliştirici) içindir. Claude Code'un bu bilgisayarda yaptığı işin durumunu, nasıl çalışıldığını ve tuzakları anlatır. Ayrıntılar için önce [BASLA.md](BASLA.md), sonra [GELISTIRME_AKISI.md](GELISTIRME_AKISI.md), [KARARLAR.md](KARARLAR.md) ve [PROJE_GUNLUGU.md](PROJE_GUNLUGU.md) (en yeni girişler en sonda).

Kullanıcı Türkçe konuşur; yanıtlar Türkçe.

## 1. Proje

- **Motor:** Unreal Engine 5.8, C++ modülü `Source/MyProject2`, Windows 11.
- **Proje yolu:** `C:\Users\merts\Downloads\MyProject2`.
- **Depo:** github.com/furkansarikiyak36-eng/unrealgame (herkese açık). Çalışma dalı: `integrate/web-plan`. Ana dal: `main`.
- **Yedek etiketi:** `backup-2026-10-21`. Bu belgenin yazıldığı andaki tam durumu gösterir.
- **Test haritası:** `/Game/Maps/PerfTest` (200×200 m çayır, ağaçlar, kayalar, senaryo).
- **Tür:** hayatta kalma ve fiziksel montaj oyunu. Hazır yapı ya da menüden kurma yok; kütük, taş ve çubuk elle taşınır, kesilir, birleştirilir. Referans oyunlar ve hangisinden ne alındığı [KARARLAR.md](KARARLAR.md) ve aşağıdaki §6'da.

## 2. GitHub'da olmayanlar (yalnız bu diskte)

| Ne | Neden | Yer |
|---|---|---|
| Üçüncü taraf paketler (~20 GB): KiteDemo, ParagonProps, SoulCave, Fab/Megascans, SampleMap, Lighting, Plugins/Marketplace | Lisans ve 100 MB dosya sınırı | `Content/...` (bkz. `.gitignore`) |
| Ücretli uç kesit dokuları (`T_EndGrain_Slice_*`, `wooden_fence_*`) | Lisans | `Content/UnrealExport/Textures/` |
| Derleme çıktısı, önbellek, günlükler | Yeniden üretilir | `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/` |

Tam yerel yedek (GitHub'da olmayanlar dahil): `C:\Users\merts\Yedek\MyProject2_2026-10-21`. Paketlerin yeniden indirme listesi: [RAPOR_2026-10-20.md](RAPOR_2026-10-20.md).

## 3. Nasıl çalışılır

### Derleme
- **Başlık (.h) değişti:** editör kapalıyken tam derleme. `bash Scripts/editor_tools/rebuild_restart.sh` editörü kaydedip kapatır, `Build.bat` çalıştırır, PerfTest ile yeniden açar. Hata olursa `Scripts/editor_tools/build_last.log` dosyasına bakılır.
- **Yalnız .cpp değişti:** Live Coding. Editörün Cmd kutusuna `LiveCoding.Compile` yazılır. Live Coding ile eklenen sınıfların Python bağlaması editör yeniden açılana kadar olmaz.
- **Kurucu (constructor) ya da varsayılan alt nesne değişikliği:** tam derleme.

### Açık editörü sürmek (Claude Code bunu MCP ile yaptı)
- Editörün MCP sunucusu `http://127.0.0.1:8000/mcp`, istemci `python Scripts/ue_mcp.py tools/call '<json>'`.
- **Editörde Python çalıştırma:** `CMDREF=tb2 bash Scripts/editor_tools/pyrun.sh <script.py> "<argümanlar>" <Saved altındaki çıktı json> [bekleme]`.
  - Komut kutusunun referansı (`tb1`/`tb2`) oturuma göre değişir; `python Scripts/editor_tools/find_cmdref.py` bulur.
  - Pencere arkadaysa yazma başarısız olur ("Could not focus widget"); önce editör penceresi öne alınır.
- **PIE başlat/durdur:** EditorAppToolset `StartPIE` / `StopPIE`.
- **FPS ölçümü:** `bash Scripts/editor_tools/measure_pie.sh <etiket>`. Önce `python Scripts/editor_tools/throttle.py` çalıştırılır; arka plandaki editör 3 FPS'ye kısılır ve ölçüm 333 ms çıkar.
- **PIE ekran görüntüsü:** Cmd kutusuna `HighResShot 1`; dosya `Saved/Screenshots/WindowsEditor/`.

### Test
- **PIE test scriptleri** (`Scripts/pie_*.py`):
  - `pie_assembly_test.py`: kütük taşıma ve şekil.
  - `pie_wall_test.py`: duvar sırası.
  - `pie_stone_size_test.py`: taş zinciri.
  - `pie_shovel_test.py`, `pie_voxel_probe.py`, `pie_voxel_scene.py`: kazma.
  - `pie_season_dig_test.py`, `pie_hydro_live.py`: mevsim ve su.
- **Karakter test kancaları:** `TestPressInteract`, `TestPressPrimary`, `TestShapeHit`, `TestShovel`, `TestBuildWall`, `TestCarryLoad`, `TestLastMessage`.
- **Dünya alt sistemlerine Python'dan erişim:** `unreal.ObjectIterator(cls)` ile, `o.get_outer() == world` olanı seçerek. `SubsystemBlueprintLibrary` Python'da yok.

### Kayıt
Her işten sonra [PROJE_GUNLUGU.md](PROJE_GUNLUGU.md) sonuna giriş eklenir ve `python Scripts/update_docs.py` çalıştırılır; bu `KOD_HARITASI.md` ve `TEST_DURUMU.md` belgelerini üretir. Sonra commit ve `git push origin integrate/web-plan`. Zorla gönderme (`--force`) yok.

## 4. Tuzaklar (yaşandı, tekrarlanmasın)

1. **Editöre klavye kısayolu gönderme.** SlateInspector `PressKey` (Ctrl+A, BackSpace) odak görüntü alanındaysa sahneyi seçip siler; bir sahne böyle kaybedildi. Metin için `FillForm` ve ardından `Type` (boş metin, submit) kullanılır.
2. **Haritayı kaydetmeden önce açık haritayı kontrol et.** Kullanıcı başka harita açabiliyor; bir script KitShowcase'i PerfTest'in üstüne kaydetti. Scriptlerdeki `get_editor_world()` korumasını kullan. Kurtarma: `Saved/Autosaves/Game/Maps/<Harita>_AutoN.umap`.
3. **`unreal.Rotator` sırası `(roll, pitch, yaw)`.** Her zaman anahtar kelimeyle yaz (`unreal.Rotator(roll=0, pitch=0, yaw=90)`).
4. **UE 5.8 FBX içe aktarma.** Interchange UCX çarpışmasını atıyor; eski içe aktarıcı ölçek 100 ve temiz klasör ister (`Scripts/import_unrealexport.py`).
5. **Dağıtılmış bitki örnekleri (çim) Lumen'e girmemeli.** `affect_dynamic_indirect_lighting` ve `affect_distance_field_lighting` kapalı olmalı; açıkken GPU 142 ms'ye çıkmıştı.
6. **GeometryCore üçgen normali `(C−A)×(B−A)`.** DynamicMesh üretirken sarım buna göre; ters sarım tek yönlü ve ters çarpışma verir.
7. **Live Coding ile yeni sınıf/enum** Python'da görünmez; editör yeniden açılmalı.

## 5. Şu anki durum (2026-10-21)

### Çalışıyor (PIE'de test edildi)
- **Ağaç kesme:** balta sesi, çatırtı, devrilme. Ağaç boyuna göre 600/300 cm ham kütük bırakır.
- **Ham kütük işleme** (`RawLogShapes`), vuruş yerine göre:
  - Ortası: enine keser (600 → 300 → 150 → 75).
  - Ucu: boyuna yarar (yarım, çeyrek).
  - Yanı: düzler (Hewn1 → Hewn2 → kalas → 4 tahta).
  - [R]: oluk / yalak.
- **Taşıma ve yerleştirme:** omza 300 cm'ye kadar, 600 cm iple sürüklenir.
  - Yatırma: 75 cm ızgara, yakındaki kütüğe paralel/dik snap.
  - Direk, kiriş, payanda, döşeme, eğik parça.
  - Köşe çaprazı (düz çentik).
  - **Duvar sırası:** duvara dönük bakınca paralel üst üste.
- **Yapı grafiği:** destek ve kırılma (`StructureLink`, `Wood/`). Üst üste paralel kütük `Stacked` bağlantısıyla taşınır.
- **Taş zinciri:**
  - Kazma ile: büyük kaya (≥100 cm) → 3 orta (50–75 cm) → 3 yapı taşı (30–40 cm, en küçük parça).
  - Keski ile: orta taş → duvar taşı, yapı taşı → en az 30 cm arnavut taşı.
- **Kazılabilir voksel toprak** (7 Days to Die tarzı, yalnız toprak):
  - [B] kürek, sol tık kazar, sağ tık çuvaldan döker (çuval 1 m³).
  - Yaklaşık 2,5 m (±0,8) toprağın altı ana kaya; arada iri kaya parçaları. Kaya kazılmaz.
  - Sistem: 50 cm hücreler, 16 m chunk'lar. Bir chunk'ın yeniden kurulması yaklaşık 5 ms, kare başına 4 ms bütçe.
  - Kazılan yerdeki çim siliniyor.
  - Zemin dünya açılışında oyuncu başlangıcı ve yerleştirilen her şeyin altında kuruluyor; kurulan zemin silinmiyor.
- **Mevsim ve iklim** (ChatGPT'nin kodu, birleştirildi): 360 günlük yıl, 90 günlük mevsimler. PerfTest'te "ılıman orman" bölge hacmi var.
- **Katmanlı su–toprak simülasyonu:** oyuncu çevresinde 256 m'de chunk'lar (`ForestCharacter::WorldChunks`). Kare başına yaklaşık 0,24 ms.
- **Performans (PIE, PerfTest):** 60 FPS sınırında, GPU yaklaşık 11 ms.

### Açık işler (öncelik sırasıyla öneri)
1. **Köşe:** iki duvarın kesiştiği köşede çapraz kütüğün iki duvara oturduğu henüz test edilmedi. Tek duvar ucunda "desteksiz" görünüyor.
2. **Kazma:**
   - Ağaç, kaya ya da yapının altı kazılınca havada kalıyor.
   - Kazı kayda bağlı değil; `ExcavationVolumeSubsystem::ExportSnapshot/ImportSnapshot` var, bağlanmadı.
3. **Voksel toprak ile su/tarım ızgarası** (ChatGPT'nin 4 m'lik `DiggableTerrainSubsystem` + `WorldStateSubsystem`) birbirinden habersiz. Çukura su dolması ve sürülmüş toprak buradan bağlanmalı.
4. **Hava durumu mevsim bilmiyor:** `SimulationSubsystem` kışın da 2–31 °C rastgele; kar hiç yağmıyor. Bir yılda toprak nemi 0,30'dan 0,17'ye düşüyor, yağış ve sızma ayarı gerekiyor.
5. **Büyük harita:** voksel chunk'lar hiç boşaltılmıyor (`bUnloadDistantChunks=false`). Büyük haritada uzaktaki fizik nesneleri dondurulup zemin boşaltılmalı. Düz zeminde chunk başına 8 bin üçgen var; düz alanlar sadeleştirilebilir.
6. **Görünmeyen eşyalar:** greybox alet ve eşya mesh'leri silindi; balta, kazma, mala, kürek, çubuk, taş eşyası görünmüyor.

## 6. Tasarım kuralları (kullanıcının kararları)

- **Modelleme yok, birleştirme var.** Hazır parça, tarif ve inşa menüsü yok. Ham maddeler (lif, sarmaşık, kabuk, reçine, kil) yalnız bağlar ya da sızdırmazlık sağlar.
- **Ölçü sistemi:** 75 cm modül. Kütük boyları 600/300/150/75.
- **Referanslar** (her oyundan yalnız bir şey):

  | Oyun | Alınan |
  |---|---|
  | Sons of the Forest | Kütük, taş, çubukla fiziksel montaj |
  | 7 Days to Die | Baskınlar, az voksel, yer altı (yalnız toprak kazılır) |
  | Subnautica | Kalıntılarla hikâye |
  | Frostpunk + The Long Dark | Bölge zorluğu, günlük seçim |
  | Çiftlik oyunları | Tarım ve hayvancılık |
  | Death Stranding | Köprü ve iple dünyayı kolaylaştırma |

- **Kullanıcının isteği:** her şey GitHub'da güncel tutulsun ve her iş günlüğe yazılsın.

## 7. Claude'un hafıza notları

Claude Code'un bu proje için tuttuğu notların kopyası: [ClaudeHafiza/](ClaudeHafiza/). Yukarıdaki tuzaklar ve kurallar bu notlardan derlendi.
