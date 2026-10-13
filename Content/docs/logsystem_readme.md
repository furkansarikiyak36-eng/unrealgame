# KÜTÜK SİSTEMİ - README ve DURUM (MyProject2)

Son güncelleme: 2026-10-13 (Faz 3 tamamlandı). Ayrıntılı devir notu: `logsystem_plan.md` (aynı klasör).
Hedef: Sons of the Forest'tan esinlenen tek fiziksel kütük sistemi, 76 mekaniğin hepsi.

## NEREDE KALDIK

1. Faz 1 (taşıma/durum), Faz 2 (kesim/şekil) ve Faz 3 (yapı alt sistemi) KOD olarak tamam. Faz 3 zip'i: `LogSystem_Faz3.zip` (Wood/ klasörünün tamamı).
2. Faz 3'ün mantık katmanı (WoodStructureLogic) UE'siz mock ortamda gerçek test dosyasıyla çalıştırıldı: 38 kontrol geçti.
3. UObject katmanı (subsystem: zemin izi, Install/Uninstall, olaylar) ve Faz 1-2 hâlâ Unreal'da DERLENMEDİ. 2026-10-13 kontrolünde MyProject2 modülünde Wood sınıfları yoktu, otomasyon testi sayısı 0 idi.
4. Projede eski bir yapı sistemi var (`StructuralIntegritySubsystem`, `StructureManager`, `LogConstructionActor`). Yeni destek sistemiyle çakışabilir, karar bekliyor.

## SIRADAKİ ADIMLAR (sırayla)

1. [ ] LogSystem_Faz3.zip içindeki Wood/ klasörünü `Source/MyProject2/Wood/` üzerine kopyala, projeyi derle, hata varsa düzelt
2. [ ] Otomasyon testlerini koştur: `MyProject2.Wood.Structure.*` (4 test; mn ile ben de koşturabilirim)
3. [ ] Eski StructuralIntegritySubsystem / LogConstructionActor ile ilişkiyi kararlaştır (taşı, köprüle, dondur)
4. [ ] Faz 2b: ReshapeVisual'ı GeometryScript/DynamicMesh ile doldur (mekanik 23-31 geometrisi) veya CarvableLog kesim koduna bağla
5. [ ] Faz 4: yapı iskeleti devamı (39-52), hasar ve onarım (59-63)
6. [ ] Faz 5: lojistik ve diğer roller (65-76), mekanik 3 ve 13-15

## DURUM LEJANDI

- DOĞRULANDI: projede çalışıyor, test edilmiş
- YAZILDI: kod var, Unreal'da derlenmedi/çalıştırılmadı
- YAZILDI+MANTIK TESTLİ: kod var, mantık katmanı mock ortamda testten geçti, UE'de derlenmedi
- YOK: hiç yapılmadı

## 76 MEKANİK

### A. Ağaç ve fiziksel durum
| # | Mekanik | Durum |
|---|---|---|
| 1 | Ağacı kesme | DOĞRULANDI (eski sistem) |
| 2 | Gövdeyi parçalara ayırma | DOĞRULANDI (eski sistem) |
| 3 | Boya göre üretim | YOK (boy sabit 15.6 m) |
| 4 | Kütüğü alma | YAZILDI (Faz 1) |
| 5 | Bırakma | YAZILDI (Faz 1) |
| 6 | Düşürme | YAZILDI (Faz 1) |
| 7 | Yuvarlama | YAZILDI (Faz 1) |
| 8 | Sürükleme | YAZILDI (Faz 1) |
| 9 | İtme | YAZILDI (Faz 1) |
| 10 | Çarpışma | YAZILDI (Faz 1, OnImpact olayı) |

### B. Taşıma
| # | Mekanik | Durum |
|---|---|---|
| 11 | Omuzda taşıma + hacim kapasitesi | YAZILDI (Faz 1) |
| 12 | Yükseğe kaldırıp yerleştirme | YAZILDI+MANTIK TESTLİ (Faz 3, PlaceCarried + snap) |
| 13 | Yük taşıma aracı | YOK |
| 14 | Halat hattı / zipline | YOK |
| 15 | Su üstü taşıma | YOK |
| ek | Geçici destekle kütük kaldırma | YOK |

### C. Kesme ve şekillendirme
| # | Mekanik | Durum |
|---|---|---|
| 16-22 | Enine kesim, modül boyu, yarım, yarma, tahta, çeyrek | YAZILDI (Faz 2, görsel yaklaşık) |
| 23 | Uç düzeltme | YAZILDI (veri), geometri YOK |
| 24-29 | Köşe, eyer, oval, düz çentik, oluk, kırlangıç | YAZILDI (sadece kural+veri), geometri YOK |
| 30 | Ucu sivriltme | YAZILDI (veri), geometri YOK |
| 31 | Pah kırma | YAZILDI (veri), geometri YOK |
| 32 | Ateşlik odun | YAZILDI (Faz 2) |

### D. Yapı kurma
| # | Mekanik | Durum |
|---|---|---|
| 33 | Temel çevresi | YAZILDI+MANTIK TESTLİ (yatay kütük + zemin izi) |
| 34 | Dikey direk | YAZILDI+MANTIK TESTLİ (rol yönden türer) |
| 35 | Yatay kiriş | YAZILDI+MANTIK TESTLİ |
| 36 | Çapraz payanda | YAZILDI+MANTIK TESTLİ (çapraz rol; payanda testi doğrudan yok) |
| 37 | Yükseltilmiş platform | YAZILDI+MANTIK TESTLİ (direk+kiriş+döşeme) |
| 38 | Döşeme ve tahta zemin | YAZILDI+MANTIK TESTLİ |
| 39-52 | Duvarlar, kapı/pencere, çatı, merdiven, köprü, iskele, korkuluk, savunma | YOK |

### E. Destek, söküm, hasar
| # | Mekanik | Durum |
|---|---|---|
| 53 | Snap ve hizalama | YAZILDI+MANTIK TESTLİ (uç-uç, uç-gövde modül çizgisi) |
| 54 | Destek yolu kontrolü | YAZILDI+MANTIK TESTLİ (döngüsel destek geçersiz) |
| 55 | Ara direği kaldırma | YAZILDI+MANTIK TESTLİ |
| 56 | Yapıdan sökme | YAZILDI (Faz 3 RemovePiece + Faz 1 Uninstall), UObject kısmı UE'de denenmedi |
| 57 | Kısmi söküm | YAZILDI (Faz 3), UObject kısmı UE'de denenmedi |
| 58 | Destek kaybı | YAZILDI+MANTIK TESTLİ (PreviewRemoval, OnSupportLost) |
| 59 | Yapısal çöküş | YAZILDI, ilk hâli (bCollapseUnsupported), UE'de denenmedi |
| 60 | Onarım | YOK |
| 61 | Savunma hasarı | YOK |
| 62 | Yangın etkisi | YOK |
| 63 | Gevşeme / bağlantı kaybı | YOK |
| 64 | Malzeme geri kazanma | Sadece Uninstall temeli |

### F. Yapı dışı kullanımlar
| # | Mekanik | Durum |
|---|---|---|
| 65-73 | Depolama, el arabası, kızak, halat hattı, yakacak, ateş, mobilya, makine, tuzak | YOK |
| 74 | Fırlatma | Temel var (Faz 1 ThrowPiece) |
| 75 | Geçici köprüleme | YOK |
| 76 | Su üstü kullanım | YOK |

## ÖZET SAYILAR

- Doğrulandı (UE'de): 2
- Yazıldı, UE'de derlenmedi (Faz 1+2): 26
- Faz 3 (mantık testli, UE'de derlenmedi): 14
- Yok: 34

## FAZ 3 KURALLARI (WoodStructureLogic)

- Yön: eksen dikeye/yataya 15 dereceden yakınsa dikey/yatay, değilse çapraz.
- Rol: yuvarlak dikey=direk (sivri uç=kazık), yatay=kiriş, çapraz=payanda. Tahta ve yarım kütük: yatay=döşeme, dikey=duvar, çapraz=çatı.
- Bağlantı: uç-uç (8 cm), uç-gövde, çentikli çaprazlama (çentik temas noktasının modül çizgisinde olmalı), tahta yaslanması. Paralel yan yana parçalar bağlanmaz.
- Yön: diğer ucu daha yüksek olan parça üstte oturur (taşınan). Aynı yükseklikte uç uca = çift yönlü.
- Destek: zemindeki parça destekli. Kiriş/döşeme/çatı en az 2 destekli taşıyıcı ister, diğerleri 1.
- Snap: uç-uç öncelikli, uç-gövde hedefin modül çizgisine oturur.

## BİLİNEN RİSKLER

- Zemin izi sadece ECC_WorldStatic kanalına bakar; dinamik nesneler zemin sayılmaz.
- Pivot uzunluğun ortasında varsayılır; Faz 2 kesimlerinde oyun içinde doğrulanmadı.
- Çöküşte her parça için destek grafiği yeniden hesaplanır (O(n^2), büyük yapıda yavaş).
- Parça yarıçapı kaba (max(yan,dikey)/2); tahta ve yarım kütükte bağlantı toleransı geniş kalır.
- bRequireSupportOnPlace varsayılan kapalı: snap olan parça havada kurulabilir, sadece desteksiz işaretlenir.
- Yarım kütük de yüzey elemanı sayılır (yatay=döşeme); yarım kütükten kiriş istenirse kural değişmeli.
- Faz 3'te düzeltildi: FindSnap artık PruneInvalid çağırmaz; CollectNodes yok olmuş parçaları atlar.

## KURALLAR

- Kullanıcı Türkçe konuşur, cevaplar Türkçe.
- mn connector `Source/` klasörüne yazamaz; C++ dosyaları zip olarak verilir, kullanıcı kopyalayıp derler.
- Her faz zip'i `Source/MyProject2/Wood/` klasörünün tamamını içerir (kümülatif).
- Eski sınıflara (CarvableLog, LogConstructionActor, LogWallBlueprint) kaynak okunmadan dokunulmaz.
