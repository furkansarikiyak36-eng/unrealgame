# Asset Kataloğu

Content Drawer'daki her klasör ve her mesh için kısa açıklama: **Ne** (ne olduğu, boyu, oyundaki rolü), **Mesh** (üçgen sayısı, Nanite / LOD) ve **Doku** (sayı, tür, en büyük boyut, video belleği). Megascans'ta aynı bitki / taşın varyantları tek satırda. Doku türleri: *renk* (base color), *normal*, *maske* (pürüzlülük, AO, yükseklik, ORM vb.), *billboard* (uzak görünüm resmi).

Kaynak: 20 Ekim 2026 envanteri (`Saved/asset_inventory.json`, `asset_bundles.json`, `mesh_roles.json`). Yenilemek için editörde envanter betikleri, sonra `python Scripts/write_asset_catalog.py`. Açıklamalar asset adlarından bir sözlükle çevrildi; sözlükte olmayan özel adlar (Agora, Dawn, Dusk...) olduğu gibi kaldı.

| Klasör | Ne | Mesh | Asset |
|---|---|---|---|
| [Maps](Maps.md) | Haritalar | 0 | 3 |
| [UnrealExport](UnrealExport.md) | Ham kütükler (kendi) | 58 | 77 |
| [LogWall](LogWall.md) | Kütük duvar (kendi) | 7 | 7 |
| [Building](Building.md) | Yapı (kendi) | 1 | 3 |
| [PerfTest](PerfTest.md) | Test içeriği (kendi) | 1 | 32 |
| [Audio](Audio.md) | Sesler (kendi) | 0 | 6 |
| [Characters](Characters.md) | Karakter | 1 | 26 |
| [Gameplay](Gameplay.md) | Oynanış materyalleri | 0 | 5 |
| [Greybox](Greybox.md) | Greybox | 1 | 2 |
| [Fab](Fab.md) | Megascans (Fab) | 317 | 1246 |
| [KiteDemo](KiteDemo.md) | KiteDemo (Epic) | 47 | 303 |
| [ParagonProps](ParagonProps.md) | Paragon (Epic) | 482 | 2422 |
| [SoulCave](SoulCave.md) | Soul: Cave (Epic) | 173 | 601 |
| [SampleMap](SampleMap.md) | Paragon örnek harita | 0 | 15 |
| [Lighting](Lighting.md) | Örnek ışık | 1 | 6 |

Not: Fab, KiteDemo, ParagonProps, SoulCave, SampleMap ve Lighting depoda yok (bkz. `Docs/RAPOR_2026-10-20.md`); katalog yine de onları anlatır, çünkü haritalar onları kullanıyor.
