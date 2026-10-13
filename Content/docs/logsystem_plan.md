# KÜTÜK SİSTEMİ - PLAN VE DEVİR NOTU (MyProject2, Unreal)

Bu dosya, başka bir Claude oturumunun işi kaldığı yerden sürdürebilmesi için yazıldı.
Kullanıcı Türkçe konuşur; cevaplar Türkçe olmalı.

## 0. ÇALIŞMA ŞEKLİ
- Hedef: Sons of the Forest'tan esinlenen, tek bir fiziksel kütük sistemi. Katalogdaki 76 mekaniğin HEPSİ uygulanacak (kullanıcı kararı).
- Unreal Engine 5.8. Proje: C:\Users\merts\Downloads\MyProject2 . C++ modülü: `MyProject2`. API makrosu: `MYPROJECT2_API`.
- Projede zaten var (kaynağı okunamadı, DOKUNMA): `ACarvableLog` (DynamicMeshComponent kullanır; dışa açık: sourceMesh, bStartAsleep, meshComponent), `ALogConstructionActor`, `ALogWallBlueprint`.
- mc connector (Unreal MCP) `Source/` klasörüne YAZAMAZ; write_file sadece Content/ ve Saved/ altına (tam Windows yoluyla) yazar. Bu yüzden C++ dosyaları Claude'un kendi çalışma alanında üretilip ZIP olarak verilir. Kullanıcı zip'i `Source/MyProject2/` altına taşıyıp kendisi derler.
- Her faz zip'i `Source/MyProject2/Wood/` klasörünün TAMAMINI (kümülatif) içerir; üzerine yazılır. Zip adları: LogSystem_Faz1.zip, LogSystem_Faz2.zip.
- Hiçbir faz derlenip test EDİLMEDİ. Derleme hatası gelirse kullanıcı hata metnini yapıştıracak; düzelt.
- Mevcut greybox mesh'ler: `/Game/Greybox/Meshes/` -> SM_GB_Log, SM_GB_LogHalf, SM_GB_LogHollow, SM_GB_LogShort, SM_GB_RawLog_250 / _375 / _500.

## 1. TASARIM KARARLARI
- Tek veri modeli: her parça `FWoodPieceData` taşır; 76 ayrı kod yolu yazılmaz.
- 3 durum: Loose (serbest, itilebilir) / Carried (taşınıyor) / Installed (yapıya bağlı).
- Modül M = tam kütüğün 1/6'sı (390 cm kütükte 65 cm). Parçalar 1M-6M. En kısa parça 1M. Çentik noktaları modül çizgileri üzerinde. Kesim çizgileri gösterilir, sabit noktalar parçaları modüler ve otomatik oturur yapar.
- Taşıma kapasitesi HACME göre (parça sayısına göre değil). Varsayılan 1000 L; tam kütük (390x40 cm) ~490 L.
- Fiziksel temas != taşıma. Gerçek bağlantı noktası ve destek kuralı belirleyici olacak (Faz 3).
- Parçanın işlevi (direk/kiriş/payanda/zemin) envanter adından değil geometri, yön, bağlantı ve konumdan türetilir (EWoodRole).
- Kesit kuralı: yuvarlak kütük önce YATAY (Lateral) eksende yarılır -> Half; Half sonra DİKEY (Vertical) eksende -> Quarter. Dikey eksen = Length x Lateral.
- Plan açık kararları (henüz kesinleşmedi): modül 1/6 mı sabit 60 cm mi; tarif tuşları 1-7'nin kaldırılması; ağaç boyu/kalınlığını gövdeye bağlama (şu an sabit 15.6 m).

## 2. KATALOG VE DURUM (76 MEKANİK)
Durum: [F1]=Faz1 kodu yazıldı, [F2]=Faz2 kodu yazıldı (kural+veri, görsel yaklaşık), [ ]=yapılmadı.

### A. Ağaç ve fiziksel durum
1 Ağacı kesme (projede mevcut, doğrulanmış) | 2 Gövdeyi parçalara ayırma (mevcut, doğrulanmış) | 3 Boya göre üretim [ ]
4 Kütüğü alma [F1] | 5 Bırakma [F1] | 6 Düşürme [F1] | 7 Yuvarlama [F1] | 8 Sürükleme [F1] | 9 İtme [F1] | 10 Çarpışma [F1 OnImpact olayı]

### B. Taşıma
11 Omuzda taşıma+kapasite [F1] | 12 Yükseğe kaldırıp yerleştirme [ ] | 13 Yük taşıma aracı [ ] | 14 Halat hattı/zipline [ ] | 15 Su üstü taşıma [ ] | (ek) geçici destekle kütük kaldırma [ ]

### C. Kesme ve şekillendirme
16 Enine kesim [F2] | 17 Modül boyunda kesim [F2] | 18 Yarım kütük çıkarma [F2] | 19 Uzunlamasına yarma [F2] | 20 Yarım yuvarlak parçalar [F2] | 21 Tahta çıkarma [F2] | 22 Çeyrek parça [F2] | 23 Uç düzeltme [F2]
24 Köşe çentiği [F2 veri] | 25 Eyer çentiği [F2 veri] | 26 Oval çentik [F2 veri] | 27 Düz çentik/uç geçmesi [F2 veri] | 28 Boyuna oluk [F2 veri] | 29 Kırlangıç kuyruğu [F2 veri]
30 Ucu sivriltme [F2 veri] | 31 Pah kırma [F2 veri] | 32 Ateşlik odun [F2]
(Not: 24-31 için GEOMETRİ henüz üretilmiyor, sadece kural ve veri. Bkz. Faz 2b.)

### D. Yapı kurma (Faz 3'te yapılacak) - hepsi [ ]
33 Temel çevresi | 34 Dikey direk | 35 Yatay kiriş | 36 Çapraz payanda | 37 Yükseltilmiş platform | 38 Döşeme/tahta zemin | 39 Yatay kütük duvarı | 40 Dikey kütük duvarı | 41 Sivri kazık duvarı | 42 Kapı/geçit | 43 Pencere açıklığı | 44 Kapı-pencere kasası | 45 Çatı iskeleti | 46 Çatı yüzeyi | 47 Merdiven/rampa | 48 Köprü/geçit | 49 İskele | 50 Korkuluk/bariyer | 51 Kazık destekli savunma | 52 Geçici inşaat iskelesi
İpucu: her biri ayrı model/tarif OLMAK ZORUNDA DEĞİL. Yatay=kiriş, dik=direk, çapraz=payanda, tahta=döşeme.

### E. Destek, söküm, hasar (Faz 4) - hepsi [ ]
53 Snap/hizalama | 54 Destek yolu kontrolü | 55 Ara direği kaldırma | 56 Yapıdan sökme [F1 Uninstall temeli] | 57 Kısmi söküm | 58 Destek kaybı | 59 Yapısal çöküş | 60 Onarım | 61 Savunma hasarı | 62 Yangın etkisi | 63 Gevşeme/bağlantı kaybı | 64 Malzeme geri kazanma

### F. Yapı dışı kullanımlar (Faz 5) - hepsi [ ]
65 Depolama | 66 El arabasına yükleme | 67 Kütük kızağı | 68 Halatlı hat | 69 Yakacak üretimi | 70 Ateş yakma | 71 Tezgah/mobilya | 72 Makine yapımı | 73 Savunma tuzağı | 74 Fırlatma [F1 ThrowPiece temeli] | 75 Geçici köprüleme | 76 Su üstü kullanım (sal/platform)

## 3. YAZILAN DOSYALAR (Source/MyProject2/Wood/)
- `WoodTypes.h/.cpp`: EWoodState, EWoodCrossSection (Round/Half/Quarter/Plank), EWoodEndShape (Flat/Pointed/Chamfered), EWoodJointType (None/Corner/Saddle/Oval/FlatLap/Groove/Dovetail), EWoodRole, FWoodNotch{ModuleLine,JointType}, FWoodPieceData (SourceSpecies, SourceTreeHeightCm=1560, DiameterCm=40, FullLogLengthCm=390, LengthModules 1..6, CrossSection, EndA/EndB, bHollow, bLengthGroove, Notches, DamageFraction; GetModuleLengthCm/GetLengthCm/GetVolumeLiters/GetLateralCm/GetVerticalCm), FWoodSlice{S0,S1 (0..1 boyunca), L0,L1,V0,V1 (-0.5..0.5)}.
- `WoodPieceComponent`: aktöre eklenir, kök primitive'i yönetir. PickUp(AttachTo,Socket), Drop, ThrowPiece(Velocity), Roll, DragToward, Push, Install(Structure,AttachTo), Uninstall; OnStateChanged, OnImpact; GetPrimitive. BeginPlay'de fizik durumuna dokunmaz (mevcut CarvableLog bozulmasın).
- `WoodCarrierComponent`: oyuncuya eklenir. MaxCarryLiters, CarrySocket, CarryOffset, StackStep; SetCarryAttachComponent, CanCarry, FindPieceInView (sphere sweep, ECC_Visibility), TryPickUp, DropPiece, DropLast, DropAll, ThrowLast; OnCarriedChanged.
- `WoodCutLibrary`: saf kurallar (SnapToModuleLine, CrossCut, SplitLengthwise, SplitQuarter, MakePlank, TrimEnd, AddNotch, CutGroove, SharpenEnd, ChamferEnd, MakeFirewood) -> FWoodCutResult{bSuccess,Reason,Pieces}.
- `WoodCutterComponent`: aynı aktöre eklenir; LocalLengthAxis (varsayılan X), LocalLateralAxis (varsayılan Y). GetCutLineWorldPositions, FindNearestCutLine, CutAtModule, CutAtWorldPoint, SplitLengthwise, SplitQuarter, MakePlank, TrimEnd, AddNotch, CutGroove, SharpenEnd, ChamferEnd, MakeFirewood; OnCutDone, OnCutFailed; BlueprintNativeEvent ReshapeVisual(Piece,NewData,Slice). Yeni parçaları SpawnActor(Owner->GetClass()) ile üretir, ölçeği kesim oranına göre değiştirir (YAKLAŞIK görsel).

## 4. BİLİNEN SINIRLAR / RİSKLER
- Derlenmedi. UHT/derleme hataları çıkabilir.
- Kesim görseli ölçek tabanlı yaklaşık; yarım silindir, sivri uç, çentik, oluk, kırlangıç GERÇEK GEOMETRİ ÜRETMİYOR.
- Mevcut `ACarvableLog` kodu okunamadı; mevcut 5 araçlı carve mode, snap noktaları ve çapraz yerleştirme/eyer çentiği ile entegrasyon yapılmadı. Kullanıcıdan CarvableLog.h/.cpp istenebilir.
- Fizik simülasyonu için DynamicMeshComponent'in basit çarpışması gerekir.
- Ağaç boyu/kalınlığı hâlâ sabit 15.6 m (Mekanik 3 yapılmadı).

## 5. SIRADAKİ İŞLER (sırayla)
1. (Opsiyonel ama önerilen) Faz 2b: ReshapeVisual'ı GeometryScript/DynamicMesh ile doldur VEYA CarvableLog'un mevcut kesim kodunu WoodCutterComponent'e bağla. Önce CarvableLog.h/.cpp iste.
2. Faz 3 - Yapı iskeleti (33-52): bağlantı grafiği (UWoodJointPoint / UWoodStructureComponent), snap noktaları, rol türetme (açıya göre direk/kiriş/payanda), yükseltilmiş platform, duvarlar, kapı/pencere açıklığı, çatı, merdiven, köprü, iskele, korkuluk.
3. Faz 4 - Destek grafiği ve hasar (53-64): destek yolu, çöküş, söküm, onarım, savunma hasarı, yangın.
4. Faz 5 - Lojistik ve diğer roller (65-76) + Mekanik 3 ve 12-15.

## 6. TESLİM BİÇİMİ
Her fazın sonunda: `/mnt/user-data/outputs/LogSystem_FazN.zip` (Source/MyProject2/Wood tamamı + README_FazN.txt) ve kısa Türkçe özet: eklenenler, kapsanan mekanik numaraları, bilinen sınırlar, sıradaki adım sorusu.
