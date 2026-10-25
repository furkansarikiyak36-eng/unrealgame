# Content Drawer — asset maliyet raporu (2026-10-19)

Bütün `/Game` içeriği: 4748 asset, 19.6 GB disk. Bunun 1089 mesh'i (statik + skeletal) tek tek hesaplandı; tam liste `Saved/asset_cost_per_mesh.csv` (Excel'de açılır, her satır bir mesh).

Yeni eklenen kitler 🆕 ile işaretli: SoulCave, KiteDemo, PN_MeadowFlowers, PN_Fern, PN_coniferBushes_I, PN_WildBerries, PN_tropicalGroundPlants.

## Nasıl okunur (ölçüm ve tahmin yöntemi)

| Sütun | Ne demek | Nereden |
|---|---|---|
| LOD0 üçgen | Yakından çizilen en detaylı hal | Asset etiketi (kesin) |
| Nanite | Evetse üçgen sayısı çizimi az etkiler, ekrana göre çizilir | Asset etiketi (kesin) |
| Mesh MB | Nanite'sız: vertex+index tamponu (LOD0 × 1,35). Nanite: diskteki veri (256 MB akış havuzundan akar) | Tahmin |
| Doku MB | Mesh'in materyallerinin dokuları, tam çözünürlük + mip'ler. Oyunda akıtılır; ilk kez görünürken bu kadar VRAM ister, aynı dokuyu paylaşan mesh'ler bir kez öder | Tahmin |
| Kopya başı GPU | Yakındaki her kopya: Nanite'sızda LOD0 üçgeni (gölgeyle 2 kez çizilir); uzakta LOD'lar düşer | Kesin + model |
| Kopya başı RAM | Instanced (HISM/PCG/foliage) ~0,2 KB oyunda, ~1 KB editörde; skeletal ağaç (Megaplant) ~7 MB | Bugünkü ölçümler |
| Kalite | Doku çözünürlüğü + LOD/Nanite | Asset etiketi |

Önemli: bir asset yalnız bir haritada kullanılırsa oyunda maliyet yaratır. "Kullanan" sütunu Main ve PerfTest'e göre.

Sabit motor maliyeti (her sahnede, asset'ten bağımsız): VRAM ~1,4 GB (gölge, Lumen, Nanite, distance field havuzları), GPU 2,5 ms boş haritada, 6,6 ms gökyüzü+ışık setiyle (High kalite).

## 1. Paketler

| Paket | Karar | Disk MB | Mesh (Nanite / skeletal) | Ne var | Doku (4K+) | Doku MB | Nanite'sız LOD0 toplam | >50k mesh | Main'de kullanılan |
|---|---|---|---|---|---|---|---|---|---|
| ParagonProps | Ağır | 10848 | 482 (66 / 4) | 373 Yapı / eşya / diğer, 58 Kaya / taş, 43 Karakter | 1015 (434) | 9677 | 1548k | 4 | 0 / 2422 |
| KiteDemo 🆕 | Ağır | 6481 | 47 (17 / 0) | 21 Kaya / taş, 10 Çim / çiçek / zemin bitkisi, 5 Çalı | 168 (106) | 5205 | 279k | 2 | 0 / 303 |
| SoulCave 🆕 | Orta | 1179 | 173 (0 / 0) | 78 Yapı / eşya / diğer, 67 Kaya / taş, 14 Ağaç | 174 (1) | 611 | 442k | 0 | 0 / 601 |
| SampleMap | Orta | 367 | 0 (0 / 0) | — | 0 (0) | 0 | 0k | 0 | 0 / 15 |
| Fab/Megascans/Surfaces (küçük paketler) ×55 | Hafif | 346 | 0 (0 / 0) | — | 167 (0) | 143 | 0k | 0 | 85 / 222 |
| Fab/Megascans/Plants (küçük paketler) ×31 | Hafif | 264 | 223 (0 / 0) | 187 Çim / çiçek / zemin bitkisi, 31 Çalı, 5 Ağaç | 150 (0) | 160 | 577k | 0 | 433 / 657 |
| Fab/Megascans/3D (küçük paketler) ×47 | Hafif | 249 | 93 (93 / 0) | 35 Kaya / taş, 32 Yapı / eşya / diğer, 26 Kütük / odun | 149 (0) | 131 | 0k | 0 | 281 / 289 |
| Fab/Megascans/3D/Rusty_Sheep_Scissors_udggahcqx | Hafif | 128 | 1 (1 / 0) | 1 Yapı / eşya / diğer | 3 (3) | 43 | 0k | 0 | 5 / 5 |
| Fab/Megascans/Surfaces/Castle_Wall_sdvtcgja | Hafif | 113 | 0 (0 / 0) | — | 4 (4) | 53 | 0k | 0 | 0 / 5 |
| UnrealExport | Hafif | 81 | 58 (0 / 0) | 58 Kütük / odun | 4 (2) | 40 | 12k | 0 | 68 / 77 |
| PerfTest | Hafif | 16 | 1 (0 / 1) | 1 Karakter | 8 (0) | 8 | 41k | 0 | 0 / 32 |
| Characters | Hafif | 14 | 1 (0 / 1) | 1 Karakter | 8 (0) | 8 | 41k | 0 | 18 / 26 |
| Fab/Textures | Hafif | 8 | 0 (0 / 0) | — | 11 (1) | 11 | 0k | 0 | 6 / 11 |

5 MB'tan küçük ve ağır mesh'i olmayan paketler (çoğu tek tek Megascans asset'i) tabloda yok; hepsi hafif.

## 2. Türe göre seçenekler

Her türde: kopya başı en ucuz ve dokusu en hafif olanlar önce. "Kopya başı GPU" yakından çizilen üçgen; binlerce kopyalanacak şeylerde (çim, çalı) bu ve dokular belirleyici, birkaç tane konacak şeylerde (ağaç, yapı) daha az.

### Ağaç (28 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| SoulCave 🆕 | 14 | 0/14 | 0.1k – 2.0k – 19.9k | 15 | 2048 | 2K doku, 2 LOD |
| ParagonProps | 5 | 4/5 | 0.9k – 3.6k – 4.5k | 53 | 8192 | 4K+ doku, Nanite |
| Fab/Megascans/Plants/Dragon_Tree_siumT | 5 | 0/5 | 0.4k – 0.5k – 0.8k | 16 | 4096 | 4K+ doku, 3 LOD |
| KiteDemo 🆕 | 4 | 0/4 | 27.8k – 58.3k – 60.1k | 109 | 4096 | 4K+ doku, 4 LOD |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| ScotsPine_01 | KiteDemo 🆕 | 60k | 4 | 85 | PerfTest |
| HillTree_Tall_02 | KiteDemo 🆕 | 58k | 4 | 109 | — |
| HillTree_02 | KiteDemo 🆕 | 49k | 4 | 145 | PerfTest |
| ScotsPineTall_01 | KiteDemo 🆕 | 28k | 4 | 85 | — |
| SM_TreeMerge_1 | SoulCave 🆕 | 20k | 2 | 15 | — |
| SM_TreeMerge_3 | SoulCave 🆕 | 16k | 2 | 15 | — |
| SM_TreeMerge_4 | SoulCave 🆕 | 14k | 2 | 15 | — |
| SM_TreeMerge_2 | SoulCave 🆕 | 14k | 2 | 15 | — |

### Çalı (37 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| Fab/Megascans/Plants/Raspberry_wf0oefeja | 8 | 0/8 | 0.7k – 9.2k – 34.0k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Dead_Shrubs_vdknafgha | 8 | 0/8 | 0.2k – 0.6k – 2.1k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 8 | 0/8 | 1.1k – 25.9k – 44.0k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Elderberry_wfzobb2ia | 7 | 0/7 | 1.3k – 3.2k – 24.4k | 16 | 4096 | 4K+ doku, 3 LOD |
| KiteDemo 🆕 | 5 | 0/5 | 0.4k – 2.5k – 6.1k | 85 | 4096 | 4K+ doku, 3 LOD |
| SoulCave 🆕 | 1 | 0/1 | 1.2k – 1.2k – 1.2k | 2 | 1024 | 1K doku, 2 LOD |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| SM_vgztealha_VarC | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 44k | 4 | 16 | Main |
| SM_vgztealha_VarA | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 38k | 4 | 16 | Main |
| SM_wf0oefeja_VarD | Fab/Megascans/Plants/Raspberry_wf0oefeja | 34k | 4 | 16 | Main |
| SM_vgztealha_VarD | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 30k | 4 | 16 | Main |
| SM_vgztealha_VarH | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 26k | 4 | 16 | Main |
| SM_wfzobb2ia_VarB | Fab/Megascans/Plants/Elderberry_wfzobb2ia | 24k | 3 | 16 | Main |
| SM_wf0oefeja_VarE | Fab/Megascans/Plants/Raspberry_wf0oefeja | 18k | 4 | 16 | Main |
| SM_vgztealha_VarF | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 14k | 4 | 16 | Main |

### Çim / çiçek / zemin bitkisi (208 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| SoulCave 🆕 | 11 | 0/11 | 0.0k – 0.1k – 13.9k | 3 | 1024 | ≤512 doku, 2 LOD |
| Fab/Megascans/Plants/Thatching_Grass_uddmcgbia | 11 | 0/11 | 0.0k – 0.4k – 0.6k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Curbside_Weeds_ujzjfexja | 11 | 0/11 | 0.0k – 0.2k – 0.3k | 16 | 4096 | 4K+ doku, 3 LOD |
| KiteDemo 🆕 | 10 | 0/10 | 0.3k – 2.4k – 4.8k | 24 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Lady_Fern_wdvlditia | 9 | 0/9 | 0.1k – 0.4k – 1.1k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/English_Ivy_xfcnebhqx | 9 | 0/9 | 0.1k – 0.4k – 2.7k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Yellow_Archangel_wcwmcfvia | 8 | 0/8 | 0.4k – 1.5k – 3.1k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Wood_Sorrel_wcwmchzja | 8 | 0/8 | 0.2k – 0.8k – 3.7k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Wild_Grass_vczndjqja | 8 | 0/8 | 0.2k – 4.8k – 5.0k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Violet_Wood_Sorrel_uchkajuia | 8 | 0/8 | 0.1k – 0.5k – 1.1k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Small_Balsam_xefmcgrqx | 8 | 0/8 | 0.1k – 0.9k – 1.9k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Sage_ve3gbfoja | 8 | 0/8 | 0.4k – 6.6k – 9.2k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Matted_Pratia_uegjcflia | 8 | 0/8 | 0.2k – 0.6k – 1.4k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Lily_of_the_Valley_xikkdhjja | 8 | 0/8 | 0.0k – 0.1k – 0.5k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Kikuyu_Grass_wiljearqx | 8 | 0/8 | 1.4k – 2.8k – 5.3k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Ground_Cover_sfcnq | 8 | 0/8 | 0.1k – 0.2k – 0.3k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Field_Poppy_vmcobd0ja | 8 | 0/8 | 0.2k – 1.2k – 2.4k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Beech_Fern_vmkpdbeia | 8 | 0/8 | 0.0k – 2.4k – 3.3k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Rembrandt_Water_Lily_uc2kajgia | 7 | 0/7 | 0.2k – 1.3k – 2.5k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Pink_Cordyline_sfhhN | 7 | 0/7 | 0.5k – 0.7k – 1.2k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Common_Hemp_Nettle_xh1kdfbqx | 7 | 0/7 | 0.2k – 1.5k – 3.3k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Wheat_Grass_tdcrdbur | 6 | 0/6 | 0.2k – 1.2k – 2.4k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Ribbon_Grass_tbdpec3r | 6 | 0/6 | 0.7k – 1.4k – 2.1k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Dry_Grass_tbbqejqr | 5 | 0/5 | 0.1k – 0.5k – 1.1k | 16 | 4096 | 4K+ doku, 3 LOD |
| Fab/Megascans/Plants/Bog_Marshcress_theqfivr | 5 | 0/5 | 0.2k – 0.3k – 0.6k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Greater_Burdock_xh5pdgiqx | 4 | 0/4 | 0.1k – 1.4k – 2.0k | 16 | 4096 | 4K+ doku, 4 LOD |
| Fab/Megascans/Plants/Alexandra_Palm_ukgkcilia | 4 | 0/4 | 6.7k – 7.8k – 8.4k | 16 | 4096 | 4K+ doku, 4 LOD |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| SM_Cave_Leaf_Pile_Combined01 | SoulCave 🆕 | 14k | 1 | 5 | — |
| SM_Cave_Leaf_Pile_Combined02 | SoulCave 🆕 | 10k | 1 | 5 | — |
| SM_ve3gbfoja_VarA | Fab/Megascans/Plants/Sage_ve3gbfoja | 9k | 4 | 16 | Main |
| SM_ukgkcilia_VarC | Fab/Megascans/Plants/Alexandra_Palm_ukgkcilia | 8k | 4 | 16 | Main |
| SM_ve3gbfoja_VarD | Fab/Megascans/Plants/Sage_ve3gbfoja | 8k | 4 | 16 | Main |
| SM_ve3gbfoja_VarF | Fab/Megascans/Plants/Sage_ve3gbfoja | 8k | 4 | 16 | Main |
| SM_ukgkcilia_VarA | Fab/Megascans/Plants/Alexandra_Palm_ukgkcilia | 8k | 4 | 16 | Main |
| SM_ukgkcilia_VarB | Fab/Megascans/Plants/Alexandra_Palm_ukgkcilia | 7k | 4 | 16 | Main |

### Kaya / taş (182 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| SoulCave 🆕 | 67 | 0/67 | 0.1k – 1.7k – 11.9k | 29 | 2048 | 2K doku, 1 LOD |
| ParagonProps | 58 | 12/58 | 0.3k – 4.0k – 9.9k | 247 | 8192 | 4K+ doku, Nanite |
| KiteDemo 🆕 | 21 | 16/21 | 2.0k – 5.2k – 9.2k | 195 | 8192 | 4K+ doku, 4 LOD |
| Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw | 7 | 7/7 | 0.1k – 0.1k – 0.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx | 7 | 7/7 | 0.1k – 0.2k – 0.7k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx | 6 | 6/6 | 0.1k – 0.2k – 0.3k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Small_Stones_Pack_vdjkbfmiw | 2 | 2/2 | 0.1k – 0.1k – 0.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Mossy_Stones_Pack_tliiadmva | 2 | 2/2 | 0.2k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Greybox | 1 | 0/1 | 0.0k – 0.0k – 0.0k | 0 | — | doku yok, 1 LOD |
| Fab/Megascans/3D/Sandstone_Boulder_vmjjfdm | 1 | 1/1 | 0.5k – 0.5k – 0.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Rock_shopk | 1 | 1/1 | 0.7k – 0.7k – 0.7k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Nordic_Beach_Rocks_ulznddxva | 1 | 1/1 | 2.6k – 2.6k – 2.6k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Nordic_Beach_Rock_vbumba2fa | 1 | 1/1 | 0.3k – 0.3k – 0.3k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Mossy_Forest_Boulder_wfstcdnaw | 1 | 1/1 | 1.5k – 1.5k – 1.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Military_Trenches_Scatter_Rock_S_10_ydxsabu | 1 | 1/1 | 0.8k – 0.8k – 0.8k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Castle_Wall_Stone_sdAta | 1 | 1/1 | 0.7k – 0.7k – 0.7k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Beach_Rock_wd3efb0 | 1 | 1/1 | 0.2k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Beach_Boulder_wenobdn | 1 | 1/1 | 1.0k – 1.0k – 1.0k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Beach_Boulder_ukopeijs | 1 | 1/1 | 1.5k – 1.5k – 1.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Beach_Boulder_ukopeess | 1 | 1/1 | 2.0k – 2.0k – 2.0k | 3 | 1024 | 1K doku, Nanite |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| SM_LandinPlatformRocks | SoulCave 🆕 | 12k | 1 | 14 | — |
| SM_RiverRocksBIG8 | SoulCave 🆕 | 10k | 1 | 29 | — |
| SM_RiverRocksBIG9 | SoulCave 🆕 | 10k | 1 | 29 | — |
| Rock_Formation_Strip_C_Ring_C | ParagonProps | 10k | 3 | 319 | — |
| Rock_Formation_Strip_C_Arch_A | ParagonProps | 10k | 3 | 319 | — |
| Rock_Formation_Strip_C_Ring_F | ParagonProps | 10k | 3 | 319 | — |
| SM_MergedMesh_Cave_Rocks_Stalactite_01_7 | SoulCave 🆕 | 10k | 1 | 23 | — |
| Rock_Well_Halo_B | ParagonProps | 9k | 3 | 319 | — |

### Kütük / odun (99 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| UnrealExport | 58 | 0/58 | 0.0k – 0.2k – 0.6k | 43 | 4096 | 4K+ doku, 1 LOD |
| LogWall | 7 | 7/7 | 0.4k – 0.4k – 0.7k | 5 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Tree_Debris_Pack_vdqjfjhiw | 6 | 6/6 | 0.1k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Wood_Debris_Pack_vdqidbfiw | 5 | 5/5 | 0.1k – 0.1k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| ParagonProps | 3 | 0/3 | 0.0k – 0.2k – 0.2k | 11 | 2048 | 2K doku, 1 LOD |
| KiteDemo 🆕 | 2 | 0/2 | 4.5k – 5.9k – 5.9k | 195 | 8192 | 4K+ doku, 4 LOD |
| SoulCave 🆕 | 2 | 0/2 | 0.1k – 0.4k – 0.4k | 3 | 1024 | 1K doku, 1 LOD |
| Building | 1 | 1/1 | 0.5k – 0.5k – 0.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Wooden_Table_ulzrcgoaw | 1 | 1/1 | 1.0k – 1.0k – 1.0k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Wooden_Beam_uebpbhcfa | 1 | 1/1 | 0.2k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Wooden_Beam_tgnhde0fa | 1 | 1/1 | 0.1k – 0.1k – 0.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Wood_Debris_vbzlbcciw | 1 | 1/1 | 0.1k – 0.1k – 0.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Wooden_Log_wcolfdz | 1 | 1/1 | 1.2k – 1.2k – 1.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Wooden_Log_wcokfh1 | 1 | 1/1 | 1.0k – 1.0k – 1.0k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Wooden_Log_wcokfdy | 1 | 1/1 | 0.8k – 0.8k – 0.8k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Wooden_Log_wcjheff | 1 | 1/1 | 0.5k – 0.5k – 0.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Wooden_Beam_vkbhcbg | 1 | 1/1 | 0.2k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Tree_Stump_wd0qcaobw | 1 | 1/1 | 3.0k – 3.0k – 3.0k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Oak_Tree_Log_wd0oddjbw | 1 | 1/1 | 1.1k – 1.1k – 1.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stick_Wood_S_04_xfekfgv | 1 | 1/1 | 1.3k – 1.3k – 1.3k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stick_Wood_S_03_xfikceq | 1 | 1/1 | 0.8k – 0.8k – 0.8k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stick_Wood_S_01_xflicih | 1 | 1/1 | 1.3k – 1.3k – 1.3k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Broken_Stump_rkswd | 1 | 1/1 | 4.1k – 4.1k – 4.1k | 3 | 1024 | 1K doku, Nanite |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| Tree_Stump_01 | KiteDemo 🆕 | 6k | 4 | 195 | — |
| SM_Vegetation_Debris_002 | KiteDemo 🆕 | 5k | 4 | 195 | — |
| SM_RawLog_75_Notch_Saddle | UnrealExport | 1k | 1 | 43 | Main |
| SM_RawLog_75_FlatNotch_TopBottom | UnrealExport | 0k | 1 | 41 | Main |
| SM_RawLog_75_SaddleA_Cradle | UnrealExport | 0k | 1 | 41 | Main |
| SM_glowstick02 | SoulCave 🆕 | 0k | 1 | 3 | — |
| SM_RawLog_75_SaddleB_Upper | UnrealExport | 0k | 1 | 41 | Main |
| SM_RawLog_75_Notch_HalfLap | UnrealExport | 0k | 1 | 43 | Main |

### Yapı / eşya / diğer (490 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| ParagonProps | 373 | 45/373 | 0.0k – 2.2k – 92.9k | 69 | 8192 | 4K+ doku, 4 LOD |
| SoulCave 🆕 | 78 | 0/78 | 0.0k – 0.6k – 17.1k | 14 | 2048 | doku yok, 1 LOD |
| Fab/Megascans/3D/Puffball_Mushrooms_qdzs3 | 8 | 8/8 | 0.0k – 0.1k – 0.1k | 3 | 1024 | 1K doku, Nanite |
| KiteDemo 🆕 | 5 | 1/5 | 0.6k – 4.0k – 5.2k | 21 | 8192 | 4K+ doku, Nanite |
| Fab/Megascans/3D/Modular_Fence_Post_xh0rah1iy | 5 | 5/5 | 1.1k – 1.4k – 3.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Bolete_Mushrooms_pdvcB | 5 | 5/5 | 0.1k – 0.1k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Bolete_Mushrooms_qdzrT | 4 | 4/4 | 0.2k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Lighting | 1 | 0/1 | 1.1k – 1.1k – 1.1k | 0 | — | doku yok, 1 LOD |
| Fab/Megascans/3D/Small_Rope_Spool_villceo | 1 | 1/1 | 2.1k – 2.1k – 2.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Rusty_Shovel_uddjeelqx | 1 | 1/1 | 1.0k – 1.0k – 1.0k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Rusty_Sheep_Scissors_udggahcqx | 1 | 1/1 | 1.5k – 1.5k – 1.5k | 43 | 4096 | 4K+ doku, Nanite |
| Fab/Megascans/3D/Old_Well_Crank_ugcicbhga | 1 | 1/1 | 2.1k – 2.1k – 2.1k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Stove_ueppbilga | 1 | 1/1 | 1.5k – 1.5k – 1.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Scythe_tmgqcb2fa | 1 | 1/1 | 1.3k – 1.3k – 1.3k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Pitchfork_uhcjeeuga | 1 | 1/1 | 0.6k – 0.6k – 0.6k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Old_Pickaxe_ucylebxfa | 1 | 1/1 | 0.2k – 0.2k – 0.2k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stake_Wood_ydxrddw | 1 | 1/1 | 0.5k – 0.5k – 0.5k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Blacksmith_Anvil_uckmegpfa | 1 | 1/1 | 2.3k – 2.3k – 2.3k | 3 | 1024 | 1K doku, Nanite |
| Fab/Megascans/3D/Ancient_Temple_Jungle_Roots_tjztef0fa | 1 | 1/1 | 1.5k – 1.5k – 1.5k | 3 | 1024 | 1K doku, Nanite |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| Turret_Inhib_Light_GDC | ParagonProps | 93k | None | 570 | — |
| Turret_Light_GDC | ParagonProps | 77k | None | 442 | — |
| Turret_Inhib_Dark_GDC | ParagonProps | 72k | None | 645 | — |
| Turret_Dark_GDC | ParagonProps | 63k | None | 517 | — |
| SM_Cave_Statue_01 | SoulCave 🆕 | 17k | 1 | 36 | — |
| SM_Cave_Brick_Optimization03 | SoulCave 🆕 | 16k | 1 | 36 | — |
| SM_S_Soul_Statue | SoulCave 🆕 | 10k | 1 | 16 | PerfTest |
| Base_Wall_Piece_K_B | ParagonProps | 10k | 3 | 281 | — |

### Karakter (45 mesh)

| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |
|---|---|---|---|---|---|---|
| ParagonProps | 43 | 5/43 | 0.1k – 3.0k – 9.7k | 62 | 4096 | 4K+ doku, 3 LOD |
| PerfTest | 1 | 0/1 | 41.1k – 41.1k – 41.1k | 8 | 2048 | 2K doku, 1 LOD |
| Characters | 1 | 0/1 | 41.1k – 41.1k – 41.1k | 8 | 2048 | 2K doku, 1 LOD |

En ağır 8 (yakından, Nanite'sız):

| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |
|---|---|---|---|---|---|
| SK_Mannequin | PerfTest | 41k | 1 | 8 | PerfTest |
| SK_Mannequin | Characters | 41k | 1 | 8 | Main |
| Dusk_Spire_Bottom | ParagonProps | 10k | 4 | 46 | — |
| Dusk_WallA_Strait02 | ParagonProps | 6k | 3 | 94 | — |
| Dusk_Spire_Gate_Top | ParagonProps | 5k | 2 | 62 | — |
| Dusk_WallA_Strait01_Bent45 | ParagonProps | 5k | 3 | 62 | — |
| Dusk_WallA_Strait01_BentInhibitor | ParagonProps | 5k | 3 | 62 | — |
| Dusk_InhibitorRing_Dusk_InhibitorRing | ParagonProps | 5k | 3 | 81 | PerfTest |

## 3. Kopya başına maliyet — hızlı karşılaştırma

| Ne | Örnek | Kopya başı RAM | Yakından GPU | 1.000 kopya | Not |
|---|---|---|---|---|---|
| Instanced bitki (Nanite'sız) | Kikuyu çim 1,4-5,3k üçgen | ~0,2 KB (editörde ~1 KB) | 1,4-5,3k üçgen (+gölge) | RAM ~0,2 MB, GPU yakındakilere bağlı | Bugünkü ölçüm: 7.500 çim +4,8 ms (gölge+Lumen dahil) |
| Instanced çalı (Nanite'sız) | Spindle VarC 154k | ~0,2 KB | 154k üçgen (+gölge) | Yakında onlarcası bile ağır | Nanite açılmalı |
| Instanced Nanite taş/kaya | Megascans 3D | ~0,2 KB | Ekrana göre, ucuz | Çok ucuz | En verimli tür |
| Skeletal ağaç (Megaplant) | Aspen 6 M Nanite | ~7 MB | Nanite, ucuz | RAM ~7 GB, CPU +5 ms | 500 ağaç ölçüldü: RAM +3,5 GB, oyun thread +2,5 ms |
| Statik ağaç (PNP / KiteDemo) | Nanite açılırsa | ~0,2 KB | Nanite ile ucuz | Ucuz | Orman için doğru seçim |
| Ayrı actor olarak yerleştirilmiş mesh | Bina, eşya | ~1-2 KB + bileşen | Mesh'e bağlı | Onlarca-yüzlerce için uygun | Binlerce için instanced kullan |

## 4. En pahalı dokular ve sorunlu formatlar

| Doku | Boyut | Format | Tahmini MB | Paket | Kullanan |
|---|---|---|---|---|---|
| DS360_059_Ref | 4000x2000 | FloatRGBA | 488 | ParagonProps | — |
| KimsBackYard | 10000x5000 | ? | 381 | KiteDemo 🆕 | — |
| T_GDC_LargeVolcanicRock_001_D2_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | PerfTest |
| T_GDC_LargeVolcanicRock_001_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | PerfTest |
| T_LargePlainsBoulder002_N | 8192x8192 | BC5 | 85 | ParagonProps | PerfTest |
| T_Cliff01_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_Cliff01_D_CC_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_Boulder05_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_GroundRevealRock001a_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_GroundRevealRock001a_D_CC_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_GroundRevealRock002_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_GroundRevealRock002_D_CC_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_LargePlainsBoulder002_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_LargePlainsBoulder002_D_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_MountainRock_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_MountainRock_Closed_D2 | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_MountainRock_Closed_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_Scree_001_N | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_Scree_001_D_CC | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_Scree002_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_Scree002_D_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| D_Tree_Stump_01_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| D_Tree_Stump_01_D_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |
| T_Vegetation_Debris_002_N | 8192x8192 | BC5 | 85 | KiteDemo 🆕 | — |
| T_Vegetation_Debris_002_D_R | 8192x8192 | DXT5 | 85 | KiteDemo 🆕 | — |

FloatRGBA (8 B/piksel) ve B8G8R8A8 (4 B/piksel) sıkıştırılmamış; DXT1 0,5, DXT5/BC5/BC7 1 B/piksel.

## 5. Sonuç ve öneriler

**Geometri olarak en hafif:** Megascans 3D (taş, kaya, kütük; hepsi Nanite), UnrealExport kütükleri (58 mesh ~12k üçgen), PN_coniferBushes_I genç kozalaklılar (28-3,5k üçgen, 4 LOD), PN_WildBerries çalıları (0,1-7k), PN_MeadowFlowers, Megascans çim/çiçekleri.

**Doku olarak en ağır (VRAM ve disk):** KiteDemo (5,1 GB, 92 adet 4K; ağaç başına 100-190 MB doku), ProceduralNtr_vol2 (4,2 GB), OldWest (2,0 GB; Main'de kullanılıyor), B3D (1,7 GB, çoğu kullanılmayan 8K), ProceduralNaturePack (1,1 GB), SoulCave (0,66 GB), PN_Fern (0,52 GB). Pure Polygons kitlerinin dokuları da 4K (mesh başına 40-85 MB).

**Kopya başı GPU olarak en riskli (Nanite'sız, yakında çok üçgen):** ProceduralNtr_vol2 çamları (~260k, Main'de kullanılıyor), large_tree_w_roots_a (132k, Main), Spindle VarC/VarD (154k/82k), Black Locust VarC (71k), KiteDemo ağaçları (28-60k).

**Ağaç seçimi için:**
- **PN_coniferBushes_I** (yeni): en hafif geometri, 4 LOD, statik, instanced olabilir. Boyları ve görünüşü ağaç olarak yetiyorsa orman için en ucuz seçenek. Dokuları 2K'ya indirmek VRAM'i dörtte birine düşürür.
- **KiteDemo ağaçları** (ScotsPine, HillTree): Epic'in açık dünya demosu; 28-60k üçgen, Nanite kapalı, dokular çok büyük. Nanite açılıp dokular 2K'ya inerse kaliteli bir orta yol.
- **ProceduralNtr_vol2 çamları:** 260k üçgen, Nanite kapalı, 250-300 MB doku. Main'de kullanılıyor; Nanite açılmadan ormanda kullanılmamalı. Uzak kart versiyonları (pine_tree_card_distant, 8-60 üçgen) uzak orman için çok ucuz.
- **SoulCave ağaçları:** mobil demo, düşük poligon ve 1-2K doku; stil olarak gerçekçi Megascans'la uyuşmayabilir.
- **Megaplant:** GPU'da ucuz (Nanite), ama skeletal: ağaç başına ~7 MB RAM ve CPU. Az sayıda kesilebilir ağaç için.

**Kullanılmayan ve silinirse yer açanlar** (yalnız disk/editör; onay gerekir): ProceduralNtr_vol2 HDRI 1,6 GB + zemin dokusu 682 MB, ProceduralNaturePack HDRI 207 MB, B3D 8K RAW setleri ~1,2 GB, Japanese Cypress 01_A. KiteDemo 6,5 GB tutuyor ve henüz hiçbir haritada yok.

**Toplu ve geri alınabilir iyileştirmeler:** 4K dokuları 2K'ya sınırlamak (Max Texture Size; asset silinmez), ağır Nanite'sız mesh'lere Nanite açmak.
