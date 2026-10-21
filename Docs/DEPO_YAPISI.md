# Depo yapısı — ne nerede

| Klasör | İçerik | Kim dokunur |
|---|---|---|
| `Source/MyProject2/` | Bütün C++ kodu. Kökte oyuncu ve kütük sistemi (`ForestCharacter`, `CarvableLog`, `RawLogShapes`, `StructureLink`, `LogShowcase`…); alt klasörler mekanik grupları: `Wood/` yapı mantığı, `WorldCore/` dünya, kazı (voksel), `RegionSeason/` mevsim, `Stone*`, `Stick*`, `Phase*` (ilk plan fazları), `Raid*`, `Inventory*`, `Material*`, `Tests/` | ChatGPT (kod), Claude (derleme düzeltmesi) |
| `Content/Maps/` | Haritalar: `PerfTest` (ana test, editör bununla açılır), `LogShowcase` (kütük vitrini, pişmiş), `KitShowcase`, `Main`, `SnapPlacementTest` | Claude |
| `Content/UnrealExport/` | Oyundaki bütün kütük mesh'leri `SM_RawLog_<boy>[_şekil]` ve kütük materyalleri. **Silinmez.** | — |
| `Content/LogShowcase/Baked/` | Vitrinin pişmiş parçaları (`Scripts/bake_log_showcase.sh` üretir) | Script |
| `Content/Excavation/`, `RegionSeason/`, `Audio/`, `LogWall/`, `Gameplay/`, `Building/` | Kazı materyalleri, iklim verisi, sesler, eski duvar mesh'leri, yardımcı materyaller | Claude |
| `Content/Fab`, `KiteDemo`, `ParagonProps`, `SoulCave`, `SampleMap`, `Lighting` | Üçüncü taraf paketler: **repoda yok**, sadece yerel diskte (lisans, boyut) | — |
| `Scripts/` | Editör ve test scriptleri (Python ve bash). `editor_tools/` derleme, editörde Python çalıştırma, ölçüm; `pie_*.py` Play testleri; `bake_*` vitrin pişirme; `tests/` statik sözleşme testleri; `update_docs.py` belgeleri üretir | Claude |
| `Docs/` | Belgeler: `PROJE_GUNLUGU.md` (her işin kaydı), `DEVIR_TESLIM.md`, `KARARLAR.md`, `KOD_HARITASI.md` ve `TEST_DURUMU.md` (otomatik), `GELISTIRME_AKISI.md`, `AssetKatalogu/`, `ClaudeHafiza/` | Herkes okur; Claude günceller |
| `do/` | Kullanıcının eski teslimatları (zip'ler, UnrealExport kaynak FBX'leri, end grain dokuları) — arşiv | — |
| `Config/` | Unreal ayarları (`DefaultGame.ini`: kazı zemini ayarları) | Claude |
