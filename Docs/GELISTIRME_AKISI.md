# Geliştirme Akışı

Bu proje iki elden geliştiriliyor.
- **Kod yazan taraf** (kullanıcı ya da kullandığı yapay zekâ aracı) mekaniği yazıp GitHub'a ekler.
- **Editör tarafı** (Claude Code, Unreal'ın açık olduğu bilgisayarda) kodu çeker. Ardından derler, editörde bağlar (asset, harita, ayar), oyun içinde test eder ve düzeltmeleri geri gönderir.

Bu belge iki tarafın da uyacağı kuralları anlatır.

## Akış

| Adım | Kim | Ne yapar |
|---|---|---|
| 1 | Kod yazan taraf | `integrate/web-plan` dalına commit atar. Commit mesajında mekaniğin adı, beklediği asset yolları ve nasıl test edileceği yazar (aşağıdaki şablon). |
| 2 | Kullanıcı | Claude'a "X mekaniğini ekledim, entegre et" der. |
| 3 | Claude | `git pull` ile çeker ve derler; derleme hatası varsa düzeltir. |
| 4 | Claude | Editörde bağlar: gereken asset'ler, PerfTest'e yerleşim, ayarlar. |
| 5 | Claude | PIE'de test eder: kesme, taşıma, yerleştirme, FPS. |
| 6 | Claude | Düzeltmeleri commit'leyip gönderir ve `Docs/PROJE_GUNLUGU.md`'ye yazar. |

**Kurallar:**
- İşe başlamadan önce her zaman `git pull`.
- Zorla gönderme (`push --force`) yok.
- Commit'ler küçük ve tek konulu.
- Aynı dosyada aynı anda iki taraf çalışmasın; büyük değişiklikten önce haber verin.

## Mekanik isteği şablonu

Kod yazan taraf commit mesajına ya da `Docs/` altına şunu koysun:

```
Mekanik: <kısa ad>
Ne yapar: <oyuncu ne yapar, ne olur — 2-3 cümle>
Dosyalar: <eklenen / değişen .h/.cpp>
Kullandığı asset'ler: </Game/... yolları; yoksa "editörde seçilsin">
Tuşlar: <hangi tuş, hangi durumda>
Nasıl test edilir: <adımlar ve beklenen sonuç>
Bilinen eksik: <varsa>
```

## Projenin değişmez kuralları (kod yazarken)

- **Motor ve modül:** Unreal Engine 5.8. C++ modülü `Source/MyProject2`.
  - Başlık (.h) değişikliği editör kapalıyken derlenir; yalnız .cpp değişikliği Live Coding ile.
- **İnşa modu yok, her şey fiziksel montaj** ([KARARLAR.md](KARARLAR.md)).
  - Hazır parça (prefab) koyan mod, menüden kurma yok.
  - Parçalar elle taşınır ve bakılan yere konur. Ham maddeler yalnız bağlar ya da sızdırmazlık sağlar.
- **Mevcut sistemleri kullanın, paralel sistem kurmayın:**

  | Konu | Kullanılacak sistem |
  |---|---|
  | Oyuncu | `AForestCharacter`: omuzda taşıma, yerleştirme (yatır / dik / eğik / çapraz), alet |
  | Ağaç, kaya, büyük kütük | `AHarvestableActor` |
  | Kesilebilir ağaç / kırılabilir kaya etiketleri | `UTreeFellingSubsystem` ("Fellable" / "Breakable") |
  | Montaj kütüğü | `ACarvableLog`: kesim, çentik, yerleştirme, yanma |
  | Taş | `AStoneActor` + `UStoneDefinition`, `StoneCarry` |
  | Çanta | `EForestItem` + `UInventoryComponent` |
  | Yapı dengesi | `Core/StructuralGraph` |

- **Ölçüler:**
  - Modül 75 cm. Ham kütükler 600 / 300 / 150 / 75 cm (`/Game/UnrealExport/SM_RawLog_<boy>`).
  - Omuz kapasitesi bir 300 cm kütük; 600 iple sürüklenir.
- **Metin:** Oyuncuya görünen metinler Türkçe; kod ve yorumlar İngilizce.
- **Performans:**
  - Çok sayıda serpilen nesne (çim, küçük taş, bitki) Lumen'e ve mesafe alanı aydınlatmasına sokulmaz (`bAffectDynamicIndirectLighting=false`, `bAffectDistanceFieldLighting=false`).
  - Hedef PerfTest'te 60 FPS (RTX 3060 Laptop, High).
- **Asset'ler:** Depoya 100 MB üstü dosya ve üçüncü taraf kit eklenmez (bkz. `.gitignore`).
  - Kod asset'i yol ile yükler (`LoadObject`). Asset yoksa çökmemeli, sessizce atlamalı.
- **Test:** Oyun içi testler için ForestCharacter'daki `Test*` UFUNCTION'ları kullanılır; Python'dan PIE'de çağrılır. Yeni mekanik test edilebilsin diye benzer bir UFUNCTION eklemek iyi olur.

## Yeni bir bilgisayarda kurulum

1. Unreal Engine 5.8 ve Visual Studio 2022 (C++ oyun geliştirme iş yükü) kurulu olmalı.
2. Depoyu klonlayıp `integrate/web-plan` dalına geçin.
3. **Kitler:** Fab'dan indirip aynı adla `Content/` altına koyun. Liste: [RAPOR_2026-10-20.md](RAPOR_2026-10-20.md), "Bu depoda olmayanlar".
   - KiteDemo, ParagonProps, SoulCave ve Fab/Megascans paketleri.
   - Bunlar olmadan PerfTest'teki çim, ağaç, kaya ve bitkiler görünmez.
4. **Kitlerin doku ve Nanite ayarları:** Editörde `py "Scripts/cap_kit_textures.py"` ve `py "Scripts/enable_nanite_kits.py"` çalıştırılarak aynı 2K sınırı ve Nanite ayarları geri gelir.
5. **Derleme:** `MyProject2.uproject`'e sağ tık → Generate Visual Studio project files, sonra derleyin. Ya da `Build.bat MyProject2Editor Win64 Development -Project=<yol>/MyProject2.uproject`.
6. Açılış haritası PerfTest; Play ile ForestCharacter başlar. Rapor betiklerinin girdileri `Saved/` altındaydı; gerekirse editörde envanter betikleri yeniden çalıştırılır.

## Editör araçları (`Scripts/editor_tools/`)

Claude'un editörü yönetmek için kullandığı yardımcılar. Git Bash'te depo kökünden çalışırlar.
- **MCP:** Editörün MCP eklentisini (127.0.0.1:8000, `Scripts/ue_mcp.py`) kullanırlar.
- **Motor yolu:** Unreal başka bir yere kuruluysa `UE_ROOT` ortam değişkeni ile belirtin.

| Araç | İş |
|---|---|
| `pyrun.sh <betik> "<argümanlar>" <çıktı.json> [bekleme]` | Editörde `Scripts/<betik>`'i çalıştırır, `Saved/<çıktı.json>`'u bekleyip yazdırır. Komut kutusu ref'i `CMDREF` ile (oturuma göre `tb1` / `tb2`). |
| `find_cmdref.py` | Editörün komut kutusunun ref'ini bulur. |
| `rebuild_restart.sh` | Editörü kaydedip kapatır, `Build.bat` ile derler, PerfTest ile yeniden açar, pencereyi öne alır. |
| `measure_view.sh <etiket> x y z pitch yaw` | Sabit kameradan CSV profil ölçümü (kare, GPU, VRAM). |
| `measure_pie.sh <etiket>` | PIE'de ölçüm + ProfileGPU geçiş süreleri. |
| `cap2.py <png> x y z pitch yaw` | Görünüm yakalama. |
| `throttle.py` | Editörün arka plan kısmasını kapatır. Ölçümden önce gerekir; küçültülmüş editör 3 FPS'e iner. |
| `unmin.ps1` | Küçültülmüş editörü geri açar. |
| `res.ps1` | Editörün RAM, CPU ve VRAM kullanımı. |

**Dikkat:**
- Editöre tuş basma komutu gönderilmez (bir sahnenin silinmesine yol açtı); yalnız metin doldurma ve Enter.
- Haritayı sabit yola kaydeden betikler önce o haritanın açık olduğunu kontrol eder.
