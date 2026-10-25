# Asset paket analizi — mesh + materyal + doku + kopya maliyeti (2026-10-19)

Her yerleştirilebilir mesh için, sahneye konduğunda peşinden gelen her şey: kendi geometrisi, bağlı parçaları, materyalleri, dokuları (bugünkü 2K sınırları dahil), iskelet/fizik asset'i; üstüne her kopyanın maliyeti. 1089 mesh. Tam liste: `Saved/asset_bundles.csv` (Excel), tablo sayfasında satıra tıklayınca da açılıyor.

## Nasıl okunur

| Kavram | Anlamı |
|---|---|
| **İlk kopya VRAM** | Asset ilk kez ekranda tam yüklenince: geometri + tüm dokuları. Sonraki kopyalar bunu tekrar ödemez. |
| **Kendi dokusu / paylaşılan doku** | Paylaşılan doku başka mesh'lerle ortak; aynı paketten ikinci bir asset kullanınca tekrar ödenmez. |
| **Kopya başı RAM** | Instanced (HISM/PCG/foliage) ~0,2 KB; Megaplant skeletal ağaç ~7 MB (bugün 500 ağaçla ölçüldü). |
| **Kopya başı GPU** | Nanite'sız: yakında en detaylı halin üçgeni, gölge açıksa iki kez çizilir; uzaklaştıkça LOD'lar düşer. Nanite: ekrana göre, kopya sayısından az etkilenir. |
| **Disk** | Asset + bütün bağımlılıkları; Megaplant'ın üretim verisi ("yalnız editör") ayrı yazıldı, oyuna girmez. |

Sahneden bağımsız sabit maliyet (her sahnede): VRAM ~1,4 GB motor havuzları, GPU 2,5 ms boş harita, 6,6 ms gökyüzü+ışık seti (High).

## 1. Örnek kartlar

### Ağaç — KiteDemo Scots Pine

`ScotsPine_01` — paket KiteDemo, Statik, kullanan: PerfTest

| | |
|---|---|
| Geometri | LOD0 60 120 üçgen, 4 LOD, tampon ~4,73 MB |
| Materyal | 5 (ScotsPine_01_Billboard_Mat, ScotsPine_01_Branches_2_Mat, ScotsPine_01_Branches_Mat, ScotsPine_01_Fronds_Mat …) |
| Doku | 9 adet, 29,3 MB (kendi 10,7 / paylaşılan 18,7) |
| **İlk kopya VRAM** | **~34,1 MB** |
| Disk | 76,5 MB |
| Kopya başı | RAM ~0,2 KB, GPU 60.1k üçgen (+gölge ×2) |
| 100 / 1.000 kopya | RAM ~0,02 MB / ~0,2 MB; yakında görünen her kopya 120 240 üçgen |

Dokular: ScotsPine_01_Atlas_Billboards_Normal_Tex 2048x2048→2048x2048 BC5 5,3 MB; ScotsPine_01_Atlas_Billboards_Tex 2048x2048→2048x2048 DXT5 5,3 MB; ScotsPine_01_Atlas_Normal_Tex 4096x4096→2048x2048 BC5 5,3 MB (paylaşılan); ScotsPine_01_Atlas_Tex 4096x4096→2048x2048 DXT5 5,3 MB (paylaşılan); ScotsPine_01_Atlas_Specular_Tex 4096x4096→2048x2048 DXT1 2,7 MB (paylaşılan); T_ScotsPine_Bark_01_N_Tex 2048x4096→1024x2048 BC5 2,7 MB (paylaşılan); T_ScotsPine_Bark_01_Tex 2048x4096→1024x2048 DXT1 1,3 MB (paylaşılan); T_ScotsPine_Bark_03_D_Tex 512x4096→256x2048 DXT5 0,7 MB (paylaşılan) …

### Kütük — UnrealExport 600 cm

`SM_RawLog_600` — paket UnrealExport, Statik, kullanan: Main 

| | |
|---|---|
| Geometri | LOD0 188 üçgen, 1 LOD, tampon ~0,01 MB |
| Materyal | 3 (MI_RawLog_Bark, MI_RawLog_Cut, M_RawLog) |
| Doku | 7 adet, 12,7 MB (kendi 0,0 / paylaşılan 12,7) |
| **İlk kopya VRAM** | **~12,7 MB** |
| Disk | 86,5 MB |
| Kopya başı | RAM ~0,2 KB, GPU 0.2k üçgen (+gölge ×2) |
| 100 / 1.000 kopya | RAM ~0,02 MB / ~0,2 MB; yakında görünen her kopya 376 üçgen |

Dokular: T_EndGrain_Slice_N 2048x2048→2048x2048 BC5 5,3 MB (paylaşılan); T_EndGrain_Slice_C 2048x2048→2048x2048 DXT1 2,7 MB (paylaşılan); T_ugqkajmew_1K_N 1024x1024→1024x1024 BC5 1,3 MB (paylaşılan); wooden_fence_normal 4096x4096→1024x1024 BC5 1,3 MB (paylaşılan); T_ugqkajmew_1K_B 1024x1024→1024x1024 DXT1 0,7 MB (paylaşılan); T_ugqkajmew_1K_ORM 1024x1024→1024x1024 DXT1 0,7 MB (paylaşılan); wooden_fence_albedo 4096x4096→1024x1024 DXT1 0,7 MB (paylaşılan)

### Çalı — Megascans Elderberry VarA

`SM_wfzobb2ia_VarA` — paket Fab/Megascans/Plants/Elderberry_wfzobb2ia, Statik, kullanan: Main 

| | |
|---|---|
| Geometri | LOD0 11 270 üçgen, 3 LOD, tampon ~0,64 MB |
| Materyal | 4 (M_MS_Billboard, M_MS_Foliage, MI_wfzobb2ia, MI_wfzobb2ia_Billboard) |
| Doku | 9 adet, 16,0 MB (kendi 0,0 / paylaşılan 16,0) |
| Diğer | 14 MaterialFunction, 1 MaterialParameterCollection |
| **İlk kopya VRAM** | **~16,6 MB** |
| Disk | 17,4 MB |
| Kopya başı | RAM ~0,2 KB, GPU 11.3k üçgen (+gölge ×2) |
| 100 / 1.000 kopya | RAM ~0,02 MB / ~0,2 MB; yakında görünen her kopya 22 540 üçgen |

Dokular: T_Default_WindNoise 4096x4096→4096x4096 DXT1 10,7 MB (paylaşılan); T_wfzobb2ia_1K_Billboard_B-O 1024x1024→1024x1024 DXT5 1,3 MB (paylaşılan); T_wfzobb2ia_1K_Billboard_N-T 1024x1024→1024x1024 DXT5 1,3 MB (paylaşılan); T_wfzobb2ia_1K_N 1024x1024→1024x1024 BC5 1,3 MB (paylaşılan); T_wfzobb2ia_1K_B 1024x1024→1024x1024 DXT1 0,7 MB (paylaşılan); T_wfzobb2ia_1K_ORT 1024x1024→1024x1024 DXT1 0,7 MB (paylaşılan); T_DefaultColor 32x32→32x32 DXT1 0,0 MB (paylaşılan); T_DefaultMasks 32x32→32x32 DXT1 0,0 MB (paylaşılan) …

### Çim — Megascans Kikuyu VarA

`SM_wiljearqx_VarA` — paket Fab/Megascans/Plants/Kikuyu_Grass_wiljearqx, Statik, kullanan: Main 

| | |
|---|---|
| Geometri | LOD0 2 007 üçgen, 4 LOD, tampon ~0,15 MB |
| Materyal | 4 (M_MS_Billboard, M_MS_Foliage, MI_wiljearqx, MI_wiljearqx_Billboard) |
| Doku | 9 adet, 16,0 MB (kendi 0,0 / paylaşılan 16,0) |
| Diğer | 14 MaterialFunction, 1 MaterialParameterCollection |
| **İlk kopya VRAM** | **~16,1 MB** |
| Disk | 14,7 MB |
| Kopya başı | RAM ~0,2 KB, GPU 2.0k üçgen (+gölge ×2) |
| 100 / 1.000 kopya | RAM ~0,02 MB / ~0,2 MB; yakında görünen her kopya 4 014 üçgen |

Dokular: T_Default_WindNoise 4096x4096→4096x4096 DXT1 10,7 MB (paylaşılan); T_wiljearqx_1K_Billboard_B-O 1024x1024→1024x1024 DXT5 1,3 MB (paylaşılan); T_wiljearqx_1K_Billboard_N-T 1024x1024→1024x1024 DXT5 1,3 MB (paylaşılan); T_wiljearqx_1K_N 1024x1024→1024x1024 BC5 1,3 MB (paylaşılan); T_wiljearqx_1K_B 1024x1024→1024x1024 DXT1 0,7 MB (paylaşılan); T_wiljearqx_1K_ORT 1024x1024→1024x1024 DXT1 0,7 MB (paylaşılan); T_DefaultColor 32x32→32x32 DXT1 0,0 MB (paylaşılan); T_DefaultMasks 32x32→32x32 DXT1 0,0 MB (paylaşılan) …

## 2. Türe göre: ilk kopya VRAM ve kopya başı maliyet

Her türde en ucuzdan pahalıya; "ilk kopya" asset'i sahneye almanın bedeli, "kopya başı" her ek kopyanın. Binlerce kopyalanacak türlerde (çim, çalı, taş) kopya başı GPU belirleyici; birkaç kopya konacaklarda (ağaç, yapı) ilk kopya VRAM'i.

### Ağaç (28 asset)

| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |
|---|---|---|---|---|---|
| SoulCave | 14 | 0,0 – 14,7 – 40,0 | 1 956 üçgen | 0/14 | 17,8 |
| Fab/Megascans/Plants/Dragon_Tree_siumT | 5 | 16,0 – 16,0 – 16,0 | 548 üçgen | 0/5 | 14,5 |
| ParagonProps | 5 | 10,7 – 23,0 – 29,1 | 912 üçgen | 4/5 | 67,0 |
| KiteDemo | 4 | 31,3 – 39,4 – 42,1 | 58 321 üçgen | 0/4 | 117,2 |

### Çalı (37 asset)

| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |
|---|---|---|---|---|---|
| SoulCave | 1 | 2,1 – 2,1 – 2,1 | 1 215 üçgen | 0/1 | 5,0 |
| Fab/Megascans/Plants/Dead_Shrubs_vdknafgha | 8 | 16,0 – 16,0 – 16,1 | 563 üçgen | 0/8 | 15,2 |
| Fab/Megascans/Plants/Elderberry_wfzobb2ia | 7 | 16,1 – 16,2 – 17,4 | 3 211 üçgen | 0/7 | 17,0 |
| Fab/Megascans/Plants/Raspberry_wf0oefeja | 8 | 16,0 – 16,6 – 18,1 | 9 192 üçgen | 0/8 | 17,7 |
| Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | 8 | 16,1 – 17,9 – 19,1 | 25 926 üçgen | 0/8 | 18,9 |
| KiteDemo | 5 | 18,7 – 29,5 – 40,4 | 2 537 üçgen | 0/5 | 31,1 |

### Çim / çiçek / zemin bitkisi (208 asset)

| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |
|---|---|---|---|---|---|
| SoulCave | 11 | 0,0 – 2,7 – 6,0 | 106 üçgen | 0/11 | 2,1 |
| Fab/Megascans/Plants/Bog_Marshcress_theqfivr | 5 | 16,0 – 16,0 – 16,0 | 312 üçgen | 0/5 | 15,3 |
| Fab/Megascans/Plants/Curbside_Weeds_ujzjfexja | 11 | 16,0 – 16,0 – 16,0 | 175 üçgen | 0/11 | 14,9 |
| Fab/Megascans/Plants/Dry_Grass_tbbqejqr | 5 | 16,0 – 16,0 – 16,1 | 488 üçgen | 0/5 | 13,8 |
| Fab/Megascans/Plants/English_Ivy_xfcnebhqx | 9 | 16,0 – 16,0 – 16,3 | 406 üçgen | 0/9 | 14,5 |
| Fab/Megascans/Plants/Ground_Cover_sfcnq | 8 | 16,0 – 16,0 – 16,0 | 164 üçgen | 0/8 | 13,5 |
| Fab/Megascans/Plants/Lady_Fern_wdvlditia | 9 | 16,0 – 16,0 – 16,1 | 446 üçgen | 0/9 | 16,0 |
| Fab/Megascans/Plants/Lily_of_the_Valley_xikkdhjja | 8 | 16,0 – 16,0 – 16,0 | 96 üçgen | 0/8 | 14,4 |
| Fab/Megascans/Plants/Matted_Pratia_uegjcflia | 8 | 16,0 – 16,0 – 16,1 | 614 üçgen | 0/8 | 15,9 |
| Fab/Megascans/Plants/Pink_Cordyline_sfhhN | 7 | 16,0 – 16,0 – 16,1 | 739 üçgen | 0/7 | 14,6 |
| Fab/Megascans/Plants/Thatching_Grass_uddmcgbia | 11 | 16,0 – 16,0 – 16,0 | 422 üçgen | 0/11 | 15,0 |
| Fab/Megascans/Plants/Violet_Wood_Sorrel_uchkajuia | 8 | 16,0 – 16,0 – 16,1 | 492 üçgen | 0/8 | 14,8 |
| Fab/Megascans/Plants/Wood_Sorrel_wcwmchzja | 8 | 16,0 – 16,0 – 16,2 | 804 üçgen | 0/8 | 14,5 |
| Fab/Megascans/Plants/Beech_Fern_vmkpdbeia | 8 | 16,0 – 16,1 – 16,2 | 2 374 üçgen | 0/8 | 15,2 |
| Fab/Megascans/Plants/Common_Hemp_Nettle_xh1kdfbqx | 7 | 16,0 – 16,1 – 16,2 | 1 484 üçgen | 0/7 | 15,0 |
| Fab/Megascans/Plants/Field_Poppy_vmcobd0ja | 8 | 16,0 – 16,1 – 16,1 | 1 158 üçgen | 0/8 | 16,2 |
| Fab/Megascans/Plants/Greater_Burdock_xh5pdgiqx | 4 | 16,0 – 16,1 – 16,1 | 1 400 üçgen | 0/4 | 15,7 |
| Fab/Megascans/Plants/Rembrandt_Water_Lily_uc2kajgia | 7 | 16,0 – 16,1 – 16,2 | 1 336 üçgen | 0/7 | 15,5 |
| Fab/Megascans/Plants/Ribbon_Grass_tbdpec3r | 6 | 16,0 – 16,1 – 16,1 | 1 352 üçgen | 0/6 | 14,8 |
| Fab/Megascans/Plants/Small_Balsam_xefmcgrqx | 8 | 16,0 – 16,1 – 16,1 | 929 üçgen | 0/8 | 14,7 |
| Fab/Megascans/Plants/Wheat_Grass_tdcrdbur | 6 | 16,0 – 16,1 – 16,2 | 1 218 üçgen | 0/6 | 14,3 |
| Fab/Megascans/Plants/Yellow_Archangel_wcwmcfvia | 8 | 16,0 – 16,1 – 16,2 | 1 504 üçgen | 0/8 | 16,5 |
| Fab/Megascans/Plants/Kikuyu_Grass_wiljearqx | 8 | 16,1 – 16,2 – 16,4 | 2 805 üçgen | 0/8 | 14,8 |
| Fab/Megascans/Plants/Wild_Grass_vczndjqja | 8 | 16,0 – 16,3 – 16,4 | 4 788 üçgen | 0/8 | 15,6 |
| Fab/Megascans/Plants/Alexandra_Palm_ukgkcilia | 4 | 16,3 – 16,4 – 16,4 | 7 822 üçgen | 0/4 | 14,4 |
| Fab/Megascans/Plants/Sage_ve3gbfoja | 8 | 16,0 – 16,4 – 16,5 | 6 560 üçgen | 0/8 | 16,8 |
| KiteDemo | 10 | 5,7 – 23,4 – 34,1 | 2 410 üçgen | 0/10 | 19,7 |

### Kaya / taş (182 asset)

| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |
|---|---|---|---|---|---|
| Greybox | 1 | 0,0 – 0,0 – 0,0 | 24 üçgen | 0/1 | 0,0 |
| Fab/Megascans/3D/Beach_Boulder_ukopeijs | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,9 |
| Fab/Megascans/3D/Beach_Boulder_wenobdn | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,4 |
| Fab/Megascans/3D/Beach_Rock_wd3efb0 | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,5 |
| Fab/Megascans/3D/Castle_Wall_Stone_sdAta | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,3 |
| Fab/Megascans/3D/Military_Trenches_Scatter_Rock_S_10_ydxsabu | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,3 |
| Fab/Megascans/3D/Mossy_Stones_Pack_tliiadmva | 2 | 2,8 – 2,8 – 2,8 | Nanite | 2/2 | 6,9 |
| Fab/Megascans/3D/Nordic_Beach_Rock_vbumba2fa | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,6 |
| Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx | 7 | 2,8 – 2,8 – 2,8 | Nanite | 7/7 | 5,2 |
| Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx | 6 | 2,8 – 2,8 – 2,8 | Nanite | 6/6 | 6,1 |
| Fab/Megascans/3D/Rock_shopk | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,8 |
| Fab/Megascans/3D/Sandstone_Boulder_vmjjfdm | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 4,9 |
| Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw | 7 | 2,8 – 2,8 – 2,8 | Nanite | 7/7 | 5,9 |
| Fab/Megascans/3D/Small_Stones_Pack_vdjkbfmiw | 2 | 2,8 – 2,8 – 2,8 | Nanite | 2/2 | 5,8 |
| Fab/Megascans/3D/Beach_Boulder_ukopeess | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 5,5 |
| Fab/Megascans/3D/Mossy_Forest_Boulder_wfstcdnaw | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 6,5 |
| Fab/Megascans/3D/Nordic_Beach_Rocks_ulznddxva | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 6,9 |
| KiteDemo | 21 | 10,4 – 13,9 – 15,7 | 2 712 üçgen | 16/21 | 267,2 |
| SoulCave | 67 | 2,0 – 29,3 – 42,9 | 1 742 üçgen | 0/67 | 45,3 |
| ParagonProps | 58 | 26,7 – 80,1 – 238,9 | 3 250 üçgen | 12/58 | 445,9 |

### Kütük / odun (99 asset)

| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |
|---|---|---|---|---|---|
| SoulCave | 2 | 1,3 – 2,7 – 2,7 | 428 üçgen | 0/2 | 2,9 |
| Building | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,8 |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stick_Wood_S_01_xflicih | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 4,0 |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stick_Wood_S_03_xfikceq | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,1 |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stick_Wood_S_04_xfekfgv | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 4,6 |
| Fab/Megascans/3D/Oak_Tree_Log_wd0oddjbw | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 7,0 |
| Fab/Megascans/3D/Old_Wooden_Beam_vkbhcbg | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 1,9 |
| Fab/Megascans/3D/Old_Wooden_Log_wcjheff | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,8 |
| Fab/Megascans/3D/Old_Wooden_Log_wcokfdy | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 2,8 |
| Fab/Megascans/3D/Old_Wooden_Log_wcokfh1 | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 3,4 |
| Fab/Megascans/3D/Old_Wooden_Log_wcolfdz | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 3,0 |
| Fab/Megascans/3D/Tree_Debris_Pack_vdqjfjhiw | 6 | 2,8 – 2,8 – 2,8 | Nanite | 6/6 | 6,2 |
| Fab/Megascans/3D/Wood_Debris_Pack_vdqidbfiw | 5 | 2,8 – 2,8 – 2,8 | Nanite | 5/5 | 5,3 |
| Fab/Megascans/3D/Wood_Debris_vbzlbcciw | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 2,9 |
| Fab/Megascans/3D/Wooden_Beam_tgnhde0fa | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 2,2 |
| Fab/Megascans/3D/Wooden_Beam_uebpbhcfa | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 3,0 |
| Fab/Megascans/3D/Wooden_Table_ulzrcgoaw | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,0 |
| Fab/Megascans/3D/Old_Tree_Stump_wd0qcaobw | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 7,5 |
| Fab/Megascans/3D/Broken_Stump_rkswd | 1 | 3,0 – 3,0 – 3,0 | Nanite | 1/1 | 8,2 |
| LogWall | 7 | 2,7 – 5,4 – 5,4 | Nanite | 7/7 | 8,4 |
| ParagonProps | 3 | 0,0 – 11,3 – 11,3 | 156 üçgen | 0/3 | 18,0 |
| UnrealExport | 58 | 10,7 – 12,7 – 12,7 | 188 üçgen | 0/58 | 86,5 |
| KiteDemo | 2 | 12,9 – 12,9 – 12,9 | 5 853 üçgen | 0/2 | 272,8 |

### Yapı / eşya / diğer (490 asset)

| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |
|---|---|---|---|---|---|
| Lighting | 1 | 0,0 – 0,0 – 0,0 | 1 078 üçgen | 0/1 | 0,2 |
| Fab/Megascans/3D/Ancient_Temple_Jungle_Roots_tjztef0fa | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 6,9 |
| Fab/Megascans/3D/Bolete_Mushrooms_pdvcB | 5 | 2,8 – 2,8 – 2,8 | Nanite | 5/5 | 6,5 |
| Fab/Megascans/3D/Bolete_Mushrooms_qdzrT | 4 | 2,8 – 2,8 – 2,8 | Nanite | 4/4 | 6,4 |
| Fab/Megascans/3D/Military_Trenches_Equipment_Stake_Wood_ydxrddw | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 4,9 |
| Fab/Megascans/3D/Old_Pickaxe_ucylebxfa | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 2,1 |
| Fab/Megascans/3D/Old_Pitchfork_uhcjeeuga | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 2,6 |
| Fab/Megascans/3D/Old_Stove_ueppbilga | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,2 |
| Fab/Megascans/3D/Puffball_Mushrooms_qdzs3 | 8 | 2,8 – 2,8 – 2,8 | Nanite | 8/8 | 6,5 |
| Fab/Megascans/3D/Rusty_Shovel_uddjeelqx | 1 | 2,8 – 2,8 – 2,8 | Nanite | 1/1 | 5,1 |
| Fab/Megascans/3D/Blacksmith_Anvil_uckmegpfa | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 6,0 |
| Fab/Megascans/3D/Old_Scythe_tmgqcb2fa | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 3,7 |
| Fab/Megascans/3D/Old_Well_Crank_ugcicbhga | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 6,7 |
| Fab/Megascans/3D/Small_Rope_Spool_villceo | 1 | 2,9 – 2,9 – 2,9 | Nanite | 1/1 | 6,2 |
| Fab/Megascans/3D/Modular_Fence_Post_xh0rah1iy | 5 | 3,1 – 3,2 – 3,3 | Nanite | 5/5 | 7,0 |
| SoulCave | 78 | 0,0 – 13,7 – 63,2 | 582 üçgen | 0/78 | 18,4 |
| KiteDemo | 5 | 0,1 – 21,4 – 43,1 | 4 098 üçgen | 1/5 | 0,5 |
| ParagonProps | 373 | 0,0 – 34,5 – 238,5 | 2 212 üçgen | 45/373 | 83,3 |
| Fab/Megascans/3D/Rusty_Sheep_Scissors_udggahcqx | 1 | 122,6 – 122,6 – 122,6 | Nanite | 1/1 | 128,5 |

## 3. En pahalı 20 asset (ilk kopya VRAM)

| Asset | Paket | Tür | İlk kopya VRAM MB | Doku MB (kendi) | Geometri MB | Kopya başı GPU | Kullanan |
|---|---|---|---|---|---|---|---|
| Rock_Well_Large_Clasps | ParagonProps | Kaya / taş | 238,9 | 235,5 (2,7) | 3,4 | Nanite | — |
| TurretTower | ParagonProps | Yapı / eşya / diğer | 238,5 | 236,1 (16,0) | 2,4 | Nanite | — |
| Mid_Lane_Platforms_1 | ParagonProps | Yapı / eşya / diğer | 231,9 | 231,6 (0,0) | 0,4 | 8.5k üçgen | — |
| Mid_Lane_Platforms_1c | ParagonProps | Yapı / eşya / diğer | 231,3 | 230,2 (0,0) | 1,1 | Nanite | — |
| Turret_Inhib_Dark_GDC | ParagonProps | Yapı / eşya / diğer | 229,9 | 229,1 (5,3) | 0,8 | 72.4k üçgen | — |
| Tower_TrimDecoA2 | ParagonProps | Yapı / eşya / diğer | 219,8 | 219,8 (10,7) | 0,0 | 0.6k üçgen | — |
| MinionSpawner_2 | ParagonProps | Yapı / eşya / diğer | 215,3 | 214,7 (0,0) | 0,5 | Nanite | — |
| Turret_Inhib_Light_GDC | ParagonProps | Yapı / eşya / diğer | 195,5 | 194,5 (16,0) | 1,1 | 92.9k üçgen | — |
| Turret_Dark_GDC | ParagonProps | Yapı / eşya / diğer | 189,8 | 189,1 (0,0) | 0,7 | 62.6k üçgen | — |
| Floating_Pillar_Base_B | ParagonProps | Yapı / eşya / diğer | 188,8 | 188,7 (0,0) | 0,1 | 2.2k üçgen | — |
| Floating_Pillar_Base_C | ParagonProps | Yapı / eşya / diğer | 188,8 | 188,7 (0,0) | 0,1 | 1.6k üçgen | — |
| TerminusRingBack | ParagonProps | Yapı / eşya / diğer | 183,2 | 182,7 (0,0) | 0,5 | Nanite | — |
| TerminusRingFront | ParagonProps | Yapı / eşya / diğer | 183,1 | 182,7 (0,0) | 0,4 | 8.7k üçgen | — |
| TerminusPillarBigYins | ParagonProps | Yapı / eşya / diğer | 182,9 | 182,7 (10,7) | 0,2 | 2.9k üçgen | — |
| Jungle_Arch_1 | ParagonProps | Yapı / eşya / diğer | 172,3 | 172,0 (0,0) | 0,3 | 6.7k üçgen | — |
| Base_Platorm_2 | ParagonProps | Yapı / eşya / diğer | 172,2 | 172,0 (16,0) | 0,2 | 5.0k üçgen | — |
| TerminusGateway | ParagonProps | Yapı / eşya / diğer | 172,2 | 172,0 (0,0) | 0,2 | 3.9k üçgen | — |
| Agora_Deco_Portal_B | ParagonProps | Yapı / eşya / diğer | 170,5 | 169,3 (5,3) | 1,1 | Nanite | — |
| TerminusClasps1 | ParagonProps | Yapı / eşya / diğer | 169,6 | 169,4 (0,0) | 0,2 | 3.6k üçgen | — |
| TerminusEyebrows | ParagonProps | Yapı / eşya / diğer | 161,8 | 161,4 (0,0) | 0,4 | 9.1k üçgen | — |

## 4. Kopya başına en pahalı 20 asset (yakında, Nanite'sız)

| Asset | Paket | Tür | LOD0 üçgen | Gölgeyle | 100 kopya yakında | İlk kopya VRAM MB | Kullanan |
|---|---|---|---|---|---|---|---|
| Turret_Inhib_Light_GDC | ParagonProps | Yapı / eşya / diğer | 92 874 | 185 748 | 18,6 M üçgen | 195,5 | — |
| Turret_Light_GDC | ParagonProps | Yapı / eşya / diğer | 77 004 | 154 008 | 15,4 M üçgen | 155,4 | — |
| Turret_Inhib_Dark_GDC | ParagonProps | Yapı / eşya / diğer | 72 424 | 144 848 | 14,5 M üçgen | 229,9 | — |
| Turret_Dark_GDC | ParagonProps | Yapı / eşya / diğer | 62 584 | 125 168 | 12,5 M üçgen | 189,8 | — |
| ScotsPine_01 | KiteDemo | Ağaç | 60 120 | 120 240 | 12,0 M üçgen | 34,1 | PerfTest |
| HillTree_Tall_02 | KiteDemo | Ağaç | 58 321 | 116 642 | 11,7 M üçgen | 39,4 | — |
| HillTree_02 | KiteDemo | Ağaç | 49 495 | 98 990 | 9,9 M üçgen | 42,1 | PerfTest |
| SM_vgztealha_VarC | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | Çalı | 44 003 | 88 006 | 8,8 M üçgen | 19,1 | Main  |
| SK_Mannequin | PerfTest | Karakter | 41 052 | 82 104 | 8,2 M üçgen | 8,9 | PerfTest |
| SK_Mannequin | Characters | Karakter | 41 052 | 82 104 | 8,2 M üçgen | 8,9 | Main  |
| SM_vgztealha_VarA | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | Çalı | 37 829 | 75 658 | 7,6 M üçgen | 18,7 | Main  |
| SM_wf0oefeja_VarD | Fab/Megascans/Plants/Raspberry_wf0oefeja | Çalı | 34 050 | 68 100 | 6,8 M üçgen | 18,1 | Main  |
| SM_vgztealha_VarD | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | Çalı | 30 360 | 60 720 | 6,1 M üçgen | 18,1 | Main  |
| ScotsPineTall_01 | KiteDemo | Ağaç | 27 824 | 55 648 | 5,6 M üçgen | 31,3 | — |
| SM_vgztealha_VarH | Fab/Megascans/Plants/Bigleaf_Hydrangea_vgztealha | Çalı | 25 926 | 51 852 | 5,2 M üçgen | 17,9 | Main  |
| SM_wfzobb2ia_VarB | Fab/Megascans/Plants/Elderberry_wfzobb2ia | Çalı | 24 403 | 48 806 | 4,9 M üçgen | 17,4 | Main  |
| SM_TreeMerge_1 | SoulCave | Ağaç | 19 914 | 39 828 | 4,0 M üçgen | 16,0 | — |
| SM_wf0oefeja_VarE | Fab/Megascans/Plants/Raspberry_wf0oefeja | Çalı | 17 804 | 35 608 | 3,6 M üçgen | 17,1 | Main  |
| SM_Cave_Statue_01 | SoulCave | Yapı / eşya / diğer | 17 066 | 34 132 | 3,4 M üçgen | 36,5 | — |
| SM_Cave_Brick_Optimization03 | SoulCave | Yapı / eşya / diğer | 16 352 | 32 704 | 3,3 M üçgen | 36,6 | — |
