# Asset raporu — hangisi maliyetli, hangisi değil (2026-10-19)

Kaynak: asset kayıt etiketleri (`Scripts/inventory_assets.py`, asset yüklenmeden), harita bağımlılıkları (`Scripts/map_dependencies.py`); rapor `Scripts/write_asset_report.py` ile üretildi. Ham veri: `Saved/asset_inventory.csv`.

## Önce bilinmesi gerekenler

- **Bir asset ancak bir haritada kullanılırsa oyunda maliyet yaratır.** Kullanılmayan asset yalnız disk ve editör açılış süresi yer.
- **Doku tahmini = tam çözünürlük + tüm mip'ler.** Unreal dokuları gerektiği kadar akıtır; gerçek VRAM bundan az olur ve akış havuzuyla (~1 GB) sınırlıdır. Yine de "bu paket ne kadar ağır" sorusunun iyi bir ölçüsü.
- **Mesh üçgeni:** Nanite mesh'lerde üçgen sayısı çizimi az etkiler. Nanite olmayanlarda en detaylı LOD (LOD0) önemli; 50 binin üstü "ağır" sayıldı.
- **Skeletal (Megaplant) ağaçlar:** GPU'da Nanite sayesinde ucuz, ama her biri ayrı actor ve RAM yer (editörde ~500-700 ağaç sınırı).

## Genel tablo

| | Asset | Disk |
|---|---|---|
| Projedeki tüm asset'ler | 3152 | 13.3 GB |
| Main haritasının kullandıkları | 1641 paket | 7.4 GB |
| PerfTest haritasının kullandıkları | 325 paket | 1.0 GB |

Doku: 991 adet, tam çözünürlükte toplam ~11.1 GB. Statik mesh: 841 (282 Nanite). Skeletal mesh: 111.

## Paketler (diske göre)

| Paket | Karar | Disk MB | Main'de kullanılan / toplam asset | Doku (4K+) | Doku tam çöz. MB | Main'deki doku MB | Statik mesh (Nanite) | Nanite'sız LOD0 üçgen | >50k mesh | Skeletal | Not |
|---|---|---|---|---|---|---|---|---|---|---|---|
| OldWest | Ağır | 3678 | 259 / 275 | 155 (140) | 2006 | 1905 | 69 (34) | 175k | 0 | — | Main'in en büyük yükü: 140 adet 4K doku. 2K'ya sınırlanırsa ~4 kat küçülür. |
| ProceduralNtr_vol2 | Ağır | 3588 | 84 / 281 | 75 (68) | 4201 | 551 | 60 (0) | 1243k | 5 | — | 1,6 GB HDRI ve 682 MB FloatRGBA zemin dokusu kullanılmıyor; çam ağaçları ~260k üçgen, Nanite kapalı. |
| ProceduralNaturePack | Ağır | 1651 | 45 / 220 | 75 (24) | 1148 | 230 | 48 (0) | 188k | 1 | — | 207 MB HDRI kullanılmıyor; large_tree_w_roots_a 132k üçgen, Nanite kapalı. |
| PN_GrassLibrary | Ağır | 1091 | 0 / 626 | 232 (31) | 594 | 0 | 110 (0) | 80k | 0 | 1 (0 M Nanite) | Mesh'ler küçük; 'sorunlu' görünen 193 doku 18×2 gibi minik pivot painter verisi. Diskteki asıl yük 923 MB zemin dokuları. |
| Megaplant_Library/Tree_Baltic_Pine | Orta | 956 | 36 / 85 | 12 (12) | 187 | 112 | 24 (24) | 0k | 0 | 20 (11 M Nanite) |  |
| B3D | Ağır | 804 | 24 / 32 | 17 (14) | 1717 | 224 | 12 (6) | 146k | 2 | — | 8K RAW dokular (her biri 170-340 MB) kullanılmıyor; Main yalnız 8K normal haritasını kullanıyor (85 MB). |
| Megaplant_Library/Tree_Japanese_Cypress | Orta | 282 | 54 / 61 | 8 (8) | 437 | 437 | 11 (11) | 0k | 0 | 18 (114 M Nanite) | Cypress 01_A 83 M Nanite üçgen (kullanılmıyor). Displacement dokuları sıkıştırılmamış 4096×8192. |
| Fab/Megascans/Plants (küçük paketler) ×31 | Hafif | 264 | 433 / 657 | 150 (0) | 160 | 160 | 223 (0) | 577k | 0 | — |  |
| Fab/Megascans/3D (küçük paketler) ×47 | Hafif | 249 | 281 / 289 | 149 (0) | 131 | 125 | 93 (93) | 0k | 0 | — |  |
| Megaplant_Library/Tree_Dead_Pine | Orta | 210 | 72 / 73 | 6 (6) | 96 | 96 | 18 (18) | 0k | 0 | 22 (14 M Nanite) |  |
| Megaplant_Library/Tree_Rowan | Orta | 160 | 38 / 57 | 4 (4) | 53 | 53 | 26 (26) | 0k | 0 | 11 (11 M Nanite) |  |
| Megaplant_Library/Tree_Norway_Maple | Orta | 153 | 39 / 82 | 5 (5) | 235 | 64 | 30 (30) | 0k | 0 | 20 (0 M Nanite) | Displacement dokusu sıkıştırılmamış 4096×8192 (170 MB). |
| Fab/Megascans/Surfaces (küçük paketler) ×21 | Hafif | 151 | 85 / 85 | 64 (0) | 55 | 55 | 0 (0) | 0k | 0 | — |  |
| Megaplant_Library/Tree_European_Aspen | Orta | 140 | 26 / 78 | 5 (5) | 224 | 224 | 31 (31) | 0k | 0 | 18 (26 M Nanite) | Displacement dokusu sıkıştırılmamış 4096×8192 (170 MB). |
| Fab/Megascans/3D/Rusty_Sheep_Scissors_udggahcqx | Orta | 128 | 5 / 5 | 3 (3) | 43 | 43 | 1 (1) | 0k | 0 | — |  |
| Fab/Megascans/Plants/European_Spindle_wk1ncbxja | Ağır | 23 | 17 / 27 | 5 (0) | 5 | 5 | 10 (0) | 376k | 3 | — |  |
| Fab/Megascans/Plants/Black_Locust_wdvhberja | Orta | 15 | 15 / 23 | 5 (0) | 5 | 5 | 8 (0) | 160k | 1 | — |  |
| UnrealExport | Hafif | 14 | 66 / 75 | 2 (0) | 8 | 8 | 58 (0) | 12k | 0 | — | Kütükler: 58 mesh, toplam ~12k üçgen. Çok hafif. |
| PN_OpenWorldFoliage | Hafif | 12 | 18 / 28 | 8 (0) | 8 | 8 | 0 (0) | 0k | 0 | 1 (0 M Nanite) |  |
| Fab/Textures | Hafif | 8 | 6 / 11 | 11 (1) | 11 | 11 | 0 (0) | 0k | 0 | — |  |

5 MB'tan küçük ve ağır mesh'i olmayan paketler tabloya alınmadı (hepsi hafif).

## En ağır Nanite'sız mesh'ler (LOD0 üçgen)

| Üçgen | Mesh | Paket | LOD | Kullanan |
|---|---|---|---|---|
| 261k | pine_tree_full_a | ProceduralNtr_vol2 | 4 | Main |
| 261k | pine_tree_full_a_fol | ProceduralNtr_vol2 | 3 | Main |
| 260k | pine_tree_tall_full_a | ProceduralNtr_vol2 | 3 | Main |
| 256k | pine_tree_tall_full_b | ProceduralNtr_vol2 | 3 | Main |
| 154k | SM_wk1ncbxja_VarC | Fab/Megascans/Plants/European_Spindle_wk1ncbxja | 3 | Main, PerfTest |
| 132k | large_tree_w_roots_a | ProceduralNaturePack | ? | Main |
| 82k | SM_wk1ncbxja_VarD | Fab/Megascans/Plants/European_Spindle_wk1ncbxja | 3 | Main, PerfTest |
| 81k | grass_mountain_low_01 | ProceduralNtr_vol2 | 1 | — |
| 72k | SM_FirewoodStackBlackCherry01 | B3D | 7 | Main |
| 71k | SM_wdvhberja_VarC | Fab/Megascans/Plants/Black_Locust_wdvhberja | 4 | Main |
| 63k | SM_FirewoodStackBlackCherry02 | B3D | 7 | Main |
| 51k | SM_wk1ncbxja_VarE | Fab/Megascans/Plants/European_Spindle_wk1ncbxja | 3 | Main, PerfTest |
| 44k | SM_vgztealha_VarC | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 4 | Main |
| 44k | SM_wk1ncbxja_VarB | Fab/Megascans/Plants/European_Spindle_wk1ncbxja | 3 | Main, PerfTest |
| 41k | SK_Mannequin | PN_GrassLibrary | 1 | PerfTest |
| 41k | SK_Mannequin | PN_OpenWorldFoliage | ? | Main |
| 38k | SM_vgztealha_VarA | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 4 | Main |
| 34k | SM_wf0oefeja_VarD | Fab/Megascans/Plants/Raspberry_wf0oefeja | 4 | Main |
| 33k | snow_mountain_low_01 | ProceduralNtr_vol2 | ? | — |
| 32k | SM_wdvhberja_VarA | Fab/Megascans/Plants/Black_Locust_wdvhberja | 4 | Main |

Çözüm: Nanite açmak ya da kullanmamak. Bitkilerde Nanite Foliage projede açık.

## En ağır Nanite ağaçlar (skeletal)

| Nanite üçgen | Ağaç | Disk MB | Kullanan |
|---|---|---|---|
| 83.4 M | Tree_Japanese_Cypress_01_A | 8.3 | — |
| 15.8 M | Tree_Japanese_Cypress_01_B | 3.3 | — |
| 12.0 M | Tree_European_Aspen_01_A | 3.4 | — |
| 6.5 M | Tree_Japanese_Cypress_01_D | 2.2 | — |
| 6.2 M | Tree_European_Aspen_01_C | 3.3 | PerfTest |
| 6.0 M | Tree_European_Aspen_01_B | 2.3 | PerfTest |
| 4.3 M | Tree_Rowan_Sapling_01_D | 0.8 | Main, PerfTest |
| 3.9 M | Tree_Japanese_Cypress_01_C | 2.3 | Main |
| 3.8 M | Tree_Dead_Pine_A | 2.2 | Main |
| 3.8 M | Tree_Dead_Pine_C | 1.7 | Main |
| 3.6 M | Tree_Japanese_Cypress_01_E | 1.8 | Main, PerfTest |
| 3.0 M | Tree_Rowan_Sapling_01_C | 0.8 | Main |

Nanite'ta üçgen sayısı çizimi sınırlı etkiler ama Nanite akış havuzunu (256 MB) ve diski doldurur.

## En büyük dokular (tam çözünürlük tahmini)

| MB | Doku | Boyut | Format | Paket | Kullanan |
|---|---|---|---|---|---|
| 1658 | HDRI_Epic_Courtyard_Daylight | 7372x3686 | FloatRGBA | ProceduralNtr_vol2 | — |
| 682 | forest_ground_debris_a_merge | 8192x8192 | FloatRGBA | ProceduralNtr_vol2 | — |
| 341 | T_FirewoodBlackCherryPieces01_RAW_8K_M | 8192x8192 | B8G8R8A8 | B3D | — |
| 341 | T_FirewoodBlackCherryPieces01_RAW_8K_O | 8192x8192 | B8G8R8A8 | B3D | — |
| 341 | T_FirewoodBlackCherryPieces01_RAW_8K_R | 8192x8192 | B8G8R8A8 | B3D | — |
| 207 | HDRI_Epic_Courtyard_Daylight | 7372x3686 | ? | ProceduralNaturePack | — |
| 171 | T_PVE_MeshDisplacement_01 | 4096x8192 | B8G8R8A8 | Megaplant_Library/Tree_European_Aspen | Main, PerfTest |
| 171 | T_PVE_Mesh_Displacement_02 | 4096x8192 | B8G8R8A8 | Megaplant_Library/Tree_Japanese_Cypress | Main, PerfTest |
| 171 | T_PVE_MeshDisplacement_01 | 4096x8192 | B8G8R8A8 | Megaplant_Library/Tree_Japanese_Cypress | Main, PerfTest |
| 171 | T_FirewoodBlackCherryPieces01_RAW_8K_H | 8192x8192 | G16 | B3D | — |
| 171 | T_PVE_MeshDisplacement_01 | 4096x8192 | B8G8R8A8 | Megaplant_Library/Tree_Norway_Maple | — |
| 85 | grass_mountain01_albedo | 4097x4097 | B8G8R8A8 | ProceduralNtr_vol2 | — |
| 85 | grass_mountain01_normal | 4097x4097 | B8G8R8A8 | ProceduralNtr_vol2 | — |
| 85 | grass_mountain01_snow_mask | 4097x4097 | B8G8R8A8 | ProceduralNtr_vol2 | — |
| 85 | snow_mountain01_albedo | 4097x4097 | B8G8R8A8 | ProceduralNtr_vol2 | — |
| 85 | snow_mountain01_normal | 4097x4097 | B8G8R8A8 | ProceduralNtr_vol2 | — |
| 85 | snow_mountain_grass01 | 4097x4097 | B8G8R8A8 | ProceduralNtr_vol2 | — |
| 85 | T_FirewoodBlackCherryPieces01_RAW_8K_N | 8192x8192 | BC5 | B3D | Main |
| 85 | T_FirewoodBlackCherryPieces01_4K_M | 4096x4096 | B8G8R8A8 | B3D | — |
| 85 | T_FirewoodBlackCherryPieces01_4K_O | 4096x4096 | B8G8R8A8 | B3D | — |

FloatRGBA ve B8G8R8A8 sıkıştırılmamış (piksel başına 8 ve 4 bayt; DXT1 0,5, BC5/DXT5 1 bayt).

## Sorunlu dokular (sıkıştırılmamış ya da akışsız, 64 pikselden büyük)

| Doku | Boyut | Format | Akışsız | Paket | Kullanan |
|---|---|---|---|---|---|
| HDRI_Epic_Courtyard_Daylight | 7372x3686 | FloatRGBA |  | ProceduralNtr_vol2 | — |
| forest_ground_debris_a_merge | 8192x8192 | FloatRGBA |  | ProceduralNtr_vol2 | — |
| T_FirewoodBlackCherryPieces01_RAW_8K_M | 8192x8192 | B8G8R8A8 |  | B3D | — |
| T_FirewoodBlackCherryPieces01_RAW_8K_O | 8192x8192 | B8G8R8A8 |  | B3D | — |
| T_FirewoodBlackCherryPieces01_RAW_8K_R | 8192x8192 | B8G8R8A8 |  | B3D | — |
| T_PVE_MeshDisplacement_01 | 4096x8192 | B8G8R8A8 |  | Megaplant_Library/Tree_European_Aspen | Main, PerfTest |
| T_PVE_Mesh_Displacement_02 | 4096x8192 | B8G8R8A8 |  | Megaplant_Library/Tree_Japanese_Cypress | Main, PerfTest |
| T_PVE_MeshDisplacement_01 | 4096x8192 | B8G8R8A8 |  | Megaplant_Library/Tree_Japanese_Cypress | Main, PerfTest |
| T_PVE_MeshDisplacement_01 | 4096x8192 | B8G8R8A8 |  | Megaplant_Library/Tree_Norway_Maple | — |
| grass_mountain01_albedo | 4097x4097 | B8G8R8A8 | evet | ProceduralNtr_vol2 | — |
| grass_mountain01_normal | 4097x4097 | B8G8R8A8 | evet | ProceduralNtr_vol2 | — |
| grass_mountain01_snow_mask | 4097x4097 | B8G8R8A8 | evet | ProceduralNtr_vol2 | — |
| snow_mountain01_albedo | 4097x4097 | B8G8R8A8 | evet | ProceduralNtr_vol2 | — |
| snow_mountain01_normal | 4097x4097 | B8G8R8A8 | evet | ProceduralNtr_vol2 | — |
| snow_mountain_grass01 | 4097x4097 | B8G8R8A8 | evet | ProceduralNtr_vol2 | — |
| T_FirewoodBlackCherryPieces01_4K_M | 4096x4096 | B8G8R8A8 |  | B3D | — |
| T_FirewoodBlackCherryPieces01_4K_O | 4096x4096 | B8G8R8A8 |  | B3D | — |
| T_FirewoodBlackCherryPieces01_4K_R | 4096x4096 | B8G8R8A8 |  | B3D | — |
| snowy_road_tracks_b_merge | 4096x4096 | B8G8R8A8 |  | ProceduralNtr_vol2 | — |
| snowy_road_tracks_merge | 4096x4096 | B8G8R8A8 |  | ProceduralNtr_vol2 | — |
| ground_snow_default_merge | 4096x4096 | B8G8R8A8 |  | ProceduralNtr_vol2 | Main |
| snow_mud_puddles_merge | 4096x4096 | B8G8R8A8 |  | ProceduralNtr_vol2 | Main |
| T_PVE_MeshDisplacement_03 | 2048x4096 | B8G8R8A8 |  | Megaplant_Library/Tree_Baltic_Pine | Main, PerfTest |
| T_PVE_MeshDisplacement_02 | 2048x4096 | B8G8R8A8 |  | Megaplant_Library/Tree_Baltic_Pine | — |
| TX_Lookup_01a_LUT | 256x16 | B8G8R8A8 |  | OldWest | — |
| grass_04_04_PIV | 112x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| grass_01_03_PIV | 98x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| grass_02_03_PIV | 98x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| grass_03_06_PIV | 98x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| grass_02_09_PIV | 89x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| grass_03_03_PIV | 89x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| lowGrass_02_02_PIV | 72x2 | FloatRGBA | evet | PN_GrassLibrary | — |
| grass_04_04_VEC | 112x2 | B8G8R8A8 | evet | PN_GrassLibrary | — |
| grass_01_03_VEC | 98x2 | B8G8R8A8 | evet | PN_GrassLibrary | — |
| grass_02_03_VEC | 98x2 | B8G8R8A8 | evet | PN_GrassLibrary | — |
| grass_03_06_VEC | 98x2 | B8G8R8A8 | evet | PN_GrassLibrary | — |
| grass_02_09_VEC | 89x2 | B8G8R8A8 | evet | PN_GrassLibrary | — |
| grass_03_03_VEC | 89x2 | B8G8R8A8 | evet | PN_GrassLibrary | — |

## Hafif ve güvenle kullanılabilir olanlar

- **Megascans bitkileri ve yüzeyleri** (Low / tier 3 indirilmiş): paket başına ~8-12 MB, dokular küçük; bitkilerin çoğu 2-30k üçgen. İstisna: European Spindle VarC/VarD, Black Locust VarC, Bigleaf Hydrangea VarA/C (38-154k üçgen).
- **UnrealExport kütükleri**: 58 mesh, toplam ~12k üçgen, 14 MB.
- **Megascans 3D taş ve kayalar**: Nanite; çizim maliyeti düşük.
- **PN_GrassLibrary çimleri**: 50-2.400 üçgen; maliyetleri ayarlardan (gölge, cull) geliyor, mesh'ten değil.

## Öneriler (kazanç sırasıyla)

1. **Kullanılmayan dev dokuları sil** (disk ve editör): ProceduralNtr_vol2 HDRI (1,6 GB) ve forest_ground_debris_a_merge (682 MB), ProceduralNaturePack HDRI (207 MB), B3D 8K RAW setleri (~1,2 GB). Silmeden önce onay gerekir.
2. **OldWest dokularını 2K'ya sınırla** (Max Texture Size 2048 ya da LOD Bias 1): Main'in doku yükü ~4 kat küçülür.
3. **Nanite'sız ağır mesh'lere Nanite aç**: vol2 çam ağaçları (~260k), large_tree_w_roots_a (132k), Spindle VarC/D, Black Locust VarC, B3D odun yığınları (63-72k).
4. **Megaplant displacement dokuları** (T_PVE_MeshDisplacement, 4096×8192, sıkıştırılmamış, 170 MB): ağaç üretim aracının verisi; oyunda VRAM'de görülmedi (rhi dökümü), ama paketlenmiş oyunu şişirir. Paketlemeden önce kontrol edilmeli.
5. **Kullanılmayan ağır ağaç varyantı**: Japanese Cypress 01_A (83 M Nanite üçgen) hiçbir haritada yok.
