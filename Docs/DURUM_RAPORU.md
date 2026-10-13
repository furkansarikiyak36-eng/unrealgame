# MyProject2 — Durum Raporu (13 Ekim 2026, akşam)

## Özet

`MyProject2`, eski projedeki (`Downloads/MyProject`) hayatta kalma ve kütük inşa mekaniklerinin sanat varlıklarından arındırılmış **greybox** sürümü. Unreal Engine 5.8, C++.

Kütük sistemi artık baştan sona çalışıyor ve otomatik testle doğrulanıyor: ağaç → gövde → kütükler → kesim, oyma ve şekillendirme → taşıma → direk, kiriş, payanda, döşeme, duvar → destek hesabı ve çöküş → ateş, hasar, onarım → yapı tanıma. Otomatik testin son 4 çalıştırması 74/74, Wood paketinin 11 birim testi 11/11 geçti.

**Görsel taraf oyunda gözle doğrulanmadı.** Çizgiler, HUD, ateşin ışığı ve alevi, yanık renk geçişi, parçaların elde duruşu: grafiksiz test bunları görmüyor. Oyunda denemek gerekiyor.

## Proje yapısı

| | |
|---|---|
| Kod | `Source/MyProject2` (~10.000 satır) ve `Source/MyProject2/Wood` (web Claude'un paketi) |
| Haritalar | `Greybox_Mechanics` (başlangıç haritası), `Demo_WoodChain` (rehberli kütük demosu) |
| Greybox varlıkları | `Content/Greybox`; `Scripts/build_greybox.py` ile üretiliyor (elle düzenlenmiyor) |
| Dokümanlar | `Docs/KUTUK_SISTEMI_PLANI.md`, bu rapor |

## Çalışan mekanikler (hepsi otomatik testli)

**Ağaç ve kütük:** Ağaç kesme (çam 6, geniş ağaç 8 vuruş), gövde → 2 büyük kütük → 4 tam kütük (390 × 70 cm, 6 modül).

**Kesim ve şekillendirme (yeni):**
- **Modül ızgarası:** 65 cm'lik sabit çizgiler; kesilen parçalar aynı ızgarayı taşıyor.
- **Aletler:** Çentik, oval çentik, düz çentik (uç geçmesi dahil), enine kesim, boyuna yarma, **sivri uç, pah, uç düzeltme, kırlangıç**.
- **Sivri uç ve pah:** Giden odun hacmi hesapla %2–4 içinde tutuyor. Uç düzeltme sivri ucu bir modül geri keserek ızgaraya oturtuyor.
- **Kırlangıç:** Dipte geniş, yüzeyde dar; aynı derinlikte dar düz çentikten fazla, geniş olandan az odun alıyor.
- **Yarma tekrarı:** Yarım, sonra çeyrek (her biri %25). Parçanın düz yüzünün yönü kayıtlı.
- **Uçlar kesimde korunuyor:** Sivri uç bir parçada, pah diğerinde kalıyor.
- **Sabit kesim noktaları, iki vuruş kuralı, kırmızı kesikli kesim çizgileri.**

**Taşıma ve yerleştirme:** Hacme göre omuz (1 tam kütük = 2 yarım = 6 tane 1M). Sol tık koyar: yerdeki kütüğe çaprazlama (eyer çentiği otomatik), zemine yatır / dik, direğe kiriş, kirişli direğe **payanda**, kirişe **döşeme tahtası**, ateşe odun, yanık parçaya onarım. R yatır ↔ dik ↔ ateş.

**Yapı ve destek:**
- **Destek hesabı:** Zeminden yayılıyor; kiriş en az iki destekli parçaya oturmalı.
- **Açıklık kuralı:** Kirişin destekleri arası en fazla 4,5 modül (292 cm), son destekten taşma en fazla 2 modül. Aşılırsa kiriş en uzun aralığın ortasından kırılıyor.
- **Payanda:** Direğin yanından kirişin altına 45° oturuyor. Payandalarla ara direk sökülünce kiriş duruyor; payandasız sökülünce kırılıyor (Sons of the Forest'taki yapı).
- **Platform:** 4 direk + 2 kiriş + yarım kütük döşeme tahtaları; düz yüzler yukarı bakıyor.
- **Temel çerçevesi ve kabin:** 2 + 2 + 2 kütük, katlar arası çentikli; üç katlı kabin duruyor.
- **Söküm ve çöküş:** Alt kütük sökülünce üstündeki 4 parça düştü, öbür alt kütük yerinde kaldı.

**Ateş (yeni):** Yakıt odunun hacmiyle orantılı (tam kütük 593 sn, yarım 297, çeyrek 148; üst sınır 900 sn). Çeyrek kütükle ateş yakılıyor, odun atılıyor, yandıkça azalıp sönüyor, tam kütük yeniden yakıyor, dolu ateş odunu reddedip omuzda tutuyor.

**Hasar, yangın, onarım (yeni):**
- **Hasar:** %60'ı geçince parçanın eklemleri gevşiyor (taşımıyor, taşınmıyor); %85'i geçince hiçbir şey taşıyamıyor.
- **Yangın:** Ateşin 160 cm yakınındaki odun yanıyor, yalnızca yakındakiler (20 sn'de %40).
- **Saldırı:** Orta direğe %65 hasar: direk ayakta, kiriş kırıldı. Son direğe %90: direk yerinden çıktı.
- **Onarım:** Yanık parça aynı boyda sağlam parçayla değiştiriliyor (konum ve çentikler aynen), yanık parça omza geliyor ve ateşe yakıt olarak atılabiliyor.

**Yapı tanıma (yeni):** Yapının neye benzediğini geometri ve bağlantıdan okuyor, parçaya bakınca HUD'da adı yazıyor. 15 tür: yatay kütük duvarı, dikey duvar, sivri kazık duvarı, savunma duvarı, geçit, kapı kasası, pencere kasası, platform, çatı iskeleti, çatı kaplaması, rampa, merdiven, köprü, korkuluk, iskele. Dünyada doğrulananlar: platform, 3 katlı kabin, sivri kazık duvarı, geçit, kapı kasası, köprü, iskele. Hepsi birim testlerinde de doğrulandı.

**Rehberli demo:** 20 görevlik liste; oyuncunun yaptıklarını algılıyor. Admin Y: bakılan parçaya %30 hasar.

## Web Claude'un Wood paketi

`Source/MyProject2/Wood/` (13 dosya + testler). Yapısal mantığı (`WoodStructureLogic`) ve yapı tanıma (`WoodStructureAnalysis.cpp`, bu tur yazıldı) kullanılıyor. 11 birim testi geçti.

Paketin bileşenleri (`WoodPieceComponent`, `WoodCarrierComponent`, `WoodCutterComponent`, `WoodStructureSubsystem`) derleniyor ama hiçbir aktöre eklenmedi; oyundaki kütükler `ACarvableLog`. İkisini birleştirmek ayrı bir karar.

## Bu turda bulunan ve düzeltilen hatalar

- Ateşe atılan odun reddediliyordu; yakıt hiç eklenmiyordu.
- 130 cm'lik parçalara sivri uç açılamıyordu (gereksiz sıkı uzunluk şartı).
- Hasarlı (gevşemiş) bir direk kirişin açıklık hesabında hâlâ destek sayılıyordu.
- Fiziğe bırakılan kiriş, ortada çarpışma olduğu için düşmüyordu: aşırı açıklıkta artık ortadan kırılıyor.
- Kabinin alt kütükleri yerde dönerse üstündeki kütük taşma sınırını aşabiliyor (kural doğru çalışıyor); test bunu rastgele yakalıyordu, temel kütükleri sabitlenerek giderildi.

## Bilinen eksikler

| Konu | Durum |
|---|---|
| Gövde boyu | Devrilen ağaçtan her zaman 15,6 m × 95 cm gövde çıkıyor; ağacın boyuna bağlı değil |
| Tarif tuşları (1–7) | Hâlâ var; fiziksel kesim fikriyle çelişiyor, kaldırılmaları kararı bekliyor |
| İnşa modu (B) | Envanterdeki eşyaları yerleştiriyor; fiziksel parça sistemine bağlı değil |
| Kapı ve pencere açma | Tanıma var; duvardan parça kesip açıklık açma yok |
| Pencere kasası, çatı, merdiven, rampa, korkuluk | Tanıma birim testli; bu yapıları oyuncunun kurabileceği yerleştirme yolları yok (eğik parça, platform üstüne direk) |
| İskele | Tanıma var; parçayı "geçici" işaretlemenin oyun içi tuşu yok |
| Lojistik, mobilya, tuzak, su | Yok |
| Ses | Hiçbir mekaniğe ses bağlı değil |
| Ateşin görünümü | Ateş yeri taş halkası, 3 koni alev, ışık: kaba greybox |

## Kontroller (güncel)

| Tuş | İş |
|---|---|
| Sol tık | Vur / kes. Elde parça varken: koy |
| E | Parçayı omza al; elde parça varken boş yere bakınca bırak |
| R | Oymada alet değiştir (9 alet). Elde parça varken yatır → dik → ateş |
| Tekerlek | Oymada derinlik veya açı |
| P → L | Admin modu → demoyu baştan başlat (konsol: `MechanicsDemo`) |
| P → Y | Admin: bakılan parçaya %30 hasar |

## Test nasıl çalıştırılır

Editör kapalıyken:

```
UnrealEditor-Cmd.exe MyProject2.uproject /Game/Maps/Demo_WoodChain -game -nullrhi -MechanicsDemoQuit
UnrealEditor-Cmd.exe MyProject2.uproject -ExecCmds="Automation RunTests MyProject2.Wood; Quit" -nullrhi
```

Birincisi 74 kontrolü çalıştırır; sonuç `Saved/Logs/MyProject2.log` içinde `[MechanicsDemo]` satırları, son satır `SONUÇ: N geçti, M kaldı`. Bir yapı parçası düştüğünde o anki yapı grafiği `[Structure]` satırlarıyla log'a yazılır. İkincisi Wood paketinin 11 birim testidir.

## Plan durumu

| Faz | Durum |
|---|---|
| 1 — Modül kesim ızgarası | ✅ |
| 2 — Her parça taşınabilir | ✅ |
| 3 — Odun ve ateş | ✅ (tarif tuşlarını kaldırma kararı bekliyor) |
| 4 — Direk, kiriş, payanda, platform, kabin | ✅ |
| 5 — Destek hesabı, çöküş, hasar, yangın, onarım | ✅ |
| 6 — Yapı tanıma | ✅; kapı/pencere açma ve lojistik yok |
