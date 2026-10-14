# Test durumu

Otomatik üretildi (14 October 2026 04:54) — `python Scripts/update_docs.py`. Kayıtlar `Saved/TestRuns/`; `Scripts/run_tests.sh` oluşturur.

## Mekanik test (grafiksiz, `-MechanicsDemoQuit`)

Son çalıştırma: **81 geçti, 0 kaldı** (14 October 2026 04:53, `mekanik_2026-10-14_0451.log`)

Geçen kontrollerin özeti (81):

- Ağaç 6 balta vuruşunda devrildi (beklenen: çam 6, geniş 8)
- Ağaç yıkıldı, yerde gövde kaldı (~15.6 m)
- Gövde ikiye bölündü: 2 büyük kütük (beklenen 2)
- Büyük kütükler bölündü: 4 taşınabilir kütük düştü (beklenen 4)
- Kütük çaprazlama oturdu: açı 90°, eksenler arası 35.7 cm (beklenen 35.7), üstteki kütüğün altına eyer çentiği  -  üçgen 744
- Tek alt kütük: uçtan geçen kütük tek destekte durmadı, 18° devrildi; alttaki yerinde
- Omuz hacme göre: tam kütük alındı (yük %96), ikinci tam kütük sığmadı
- Direk dikildi: dikeylik 1.000, alt uç zeminden 0.5 cm, sabit evet, omuz boş
- Köşe: iki alt kütüğe çaprazlama kondu, 2 çentik açıldı, kütük duruyor (iki destek)
- Köşe 2 sn sonra yerinde
- Direkler için kütük 2M parçalara kesildi: 130, 130, 130 cm
- Tek direk + kiriş: direk duruyor, kiriş desteksiz, fiziğe bırakıldı
- Kiriş yere düştü (132 cm aşağı)
- İki direk + kiriş: kiriş duruyor (iki destek)
- Kiriş 2 sn sonra yerinde
- Direk söküldü (omuzda): kiriş desteksiz kaldı, düşüyor
- Kiriş yere indi
- Sivri uç: kazık oluştu, boy 390 cm aynı, giden odun 153575 cm3 (beklenen 156148)
- Pah: uç pahlandı, giden odun 6223 cm3 (beklenen 6501)
- Şekilli uca yeni şekil verilemiyor (0 nokta), düzeltme iki uçta açık (2 nokta)
- Uç düzeltme: sivri uç geri kesildi (65 cm kısaldı), uç yine modül çizgisinde (1.00M), düz
- Enine kesimde uçlar korundu: kazık ucu bir parçada, pah diğerinde, kesilen uçlar düz
- Kırlangıç: giden odun 25280 cm3, aynı derinlikte dar düz çentik 20102, geniş 28718; arası, yani dipte geniş yüzeyde dar
- Yarma tekrar: yarım (50%) sonra çeyrek (her biri 25.0%), yarma sayısı 2; aynı yarmadan çıkan iki parçanın düz yüzleri ters (90° ve 270°)
- Üç direk (160 cm arayla) + kiriş: kiriş duruyor
- Kiriş 2 sn sonra yerinde
- Orta direk sökülünce (kalan direkler 320 cm arayla, sınır 292 cm) kiriş ortadan kırıldı
- Kırılan kiriş yere indi (143 cm aşağı)
- Direk geri kondu, kiriş yine üç direkte duruyor
- Payanda P1 direğine kondu: 45° eğimli, alt ucu 86 cm yüksekte, duruyor
- Payanda P3 direğine kondu: 45° eğimli, alt ucu 86 cm yüksekte, duruyor
- Payandalar varken ara direk söküldü: kiriş düşmedi
- 3 sn sonra: kiriş ve payandalar yerinde (ara direksiz)
- Dört direk + iki kiriş: iki kiriş de paralel (+X boyunca) yerinde, 195 cm arayla
- İki döşeme tahtası iki kirişin üstüne kondu, ikisi de duruyor
- Döşeme 2.5 sn sonra yerinde, düz yüzler yukarı bakıyor (en kötü 1.000)
- Temel çerçevesi: 2 kütük yerde + 2 kütük çaprazlama, 4 çentik; durum: A:sabit z=36 B:sabit z=36 C:sabit z=72 D:sabit z=72
- Kabin: üç kat, 6 kütük, hepsi duruyor; üst kat 71 cm yukarıda
- Kabin 2.5 sn sonra yerinde
- Tanıma: dört direk + iki kiriş + döşeme tahtası = Platform
- Tanıma: üç katlı kabin = Yatay kütük duvarı (6 kütük)
- Tanıma: platformun parçaları duvar sayılmıyor, kabininkiler platform değil
- Tanıma: yan yana dört sivri uçlu direk = Sivri kazık duvarı
- Tanıma: iki duvar arasında 100 cm boşluk = Geçit (lentosuz)
- Tanıma: boşluğun üstüne kütük konunca = Kapı kasası (iki yan direk + lento)
- Tanıma: iki direk 260 cm arayla + üstünde uzun kütük, ortası boş = Köprü
- Tanıma: geçici işaretli parçalar = İskele
- Yakıt odunun hacmiyle orantılı: tam kütük 593 sn, yarım 297 sn, çeyrek 148 sn (10, 5, 2.5 dk)
- Ateş yeri 7 taşla kuruldu (taşsız kurulmadı); çeyrek kütük yakıt oldu (148 sn) ama yanmadı; çakmak 1 sn yetmedi, 2,5 sn yaktı
- Ateşe üç çeyrek daha atıldı: yakıt 148 -> 593 sn
- Ateş yandıkça yakıt azaldı (100 sn: 593 -> 493) ve söndü
- Tam kütük söneni yeniden yaktı (593 sn); ikincisiyle dolu (900 sn = üst sınır); üçüncüsü atılmadı, omuzda kaldı
- Hasar testi: üç direk + kiriş duruyor
- Saldırı: orta direk %65 hasarlı. %50'de kiriş tutuyordu, %60'ı geçince eklemi gevşedi: direk ayakta, kiriş kırıldı
- Saldırı: son direk %90 hasarlı, %85'i geçince hiçbir şey taşıyamadı: yerinden çıktı
- Yangın testi: üç direk + kiriş duruyor
- Yangın: 20 sn yanınca yalnızca ateşin yanındaki direk yandı (%40 hasar), diğerleri sağlam, kiriş yerinde
- Onarım: yanan direk sağlam parçayla değiştirildi (yeni parça yerinde, hasar 0), yanan parça omza geldi (%41 hasarlı), kiriş duruyor
- Değiştirilen yanık parça ateşe yakıt olarak atıldı (yakıt 568 -> 765 sn)
- Ateş yeni direği de yaktı: eklemleri gevşeyince kiriş yine kırıldı
- Çakmak: kalın kütük hemen tutuşmadı (yarı sürede yalnızca ısındı), 14 sn ısıdan sonra yandı
- Yangın yayılıyor: yanan kütüğe değen kütük ısınıp tutuştu, 4 m ötedeki ısınmadı bile
- Yanma: yakıtın yarısında %50 hasar, bitince yanık (%100); yandığı kütük de yandı, uzaktaki sağlam
- Aynı anda en çok 12 parça yanıyor (15 kütükten 12 tutuştu)
- Yanan yapı testi: üç direk + kiriş duruyor
- Yanan direk: yakıtın %30'unda kiriş yerinde; %60'ı geçince (%70 hasar) eklemleri gevşedi, kiriş kırıldı
- Alev alma: yakıt atılınca ateş yanmadı; yanan kütük ateşe atılınca tutuştu (çakmak gerekmedi)
- Çöküş: kabinin alt kütüğü sökülünce üstündeki 4 parça desteksiz kalıp düştü; öbür alt kütük yerinde
- Ağaçtan çıkan kütük doğrudan oyulabilir (390 cm, çap 70 cm)
- Kesim çizgisi (çentik, beklenen ~çap): 1 çizgi, 94 nokta, kütük boyunca 71 cm
- Çentik uçtaki köşe noktasında (-130) açıldı  -  üçgen 96 -> 746
- Kesim çizgisi (oval, beklenen < genişlik): 1 çizgi, 50 nokta, kütük boyunca 52 cm
- Oval çentik ortada (0), 70 cm x 11.7 cm  -  üçgen 746 -> 1390
- Kesim çizgisi (düz, beklenen = genişlik): 1 çizgi, 34 nokta, kütük boyunca 70 cm
- Düz çentik uçta, uç geçmesi (160), 70 cm x 11.7 cm, boy 390 cm  -  üçgen 1390 -> 1758
- Dolu yerlere yeni çentik açılamıyor (kalan çentik noktası: 0)
- Kesim çizgisi (enine, beklenen 0): 1 çizgi, 96 nokta, kütük boyunca 0 cm
- Enine kesim modül çizgisinde (açık çizgiler: -65 65): 390 -> 130 + 260 cm (2.0M + 4.0M), parça tekrar: 260 -> 130 + 130 cm, çizgi uçtan 4.0M
- Kesim çizgisi (yarma, beklenen tüm boy): 4 çizgi, 8 nokta, kütük boyunca 132 cm
- Oyuncu vuruşu 1: kesim izi açıldı, henüz kesmedi
- Oyuncu vuruşu 2 (1.5 sn sonra, kütük fizikle oturuyorken): boyuna yarıldı  -  parça 87 -> 88

Rehberli demo görevleri otomatik oynatmada tamamlanan: 17

Son çalıştırmalar:

- mekanik_2026-10-13_final.log: 74 geçti, 0 kaldı
- mekanik_2026-10-13_1304.log: 74 geçti, 0 kaldı
- mekanik_2026-10-14_0428.log: 74 geçti, 0 kaldı
- mekanik_2026-10-14_0443.log: sonuç yok
- mekanik_2026-10-14_0448.log: sonuç yok
- mekanik_2026-10-14_0451.log: 81 geçti, 0 kaldı

## Wood birim testleri (`MyProject2.Wood`)

Son çalıştırma: **11 geçti, 0 kaldı** (14 October 2026 04:51)

- ✅ MyProject2.Wood.Analysis.Cabin
- ✅ MyProject2.Wood.Analysis.Floors
- ✅ MyProject2.Wood.Analysis.Openings
- ✅ MyProject2.Wood.Analysis.Roof
- ✅ MyProject2.Wood.Analysis.Scaffold
- ✅ MyProject2.Wood.Analysis.Slopes
- ✅ MyProject2.Wood.Analysis.Walls
- ✅ MyProject2.Wood.Structure.Connections
- ✅ MyProject2.Wood.Structure.OrientationAndRole
- ✅ MyProject2.Wood.Structure.Snap
- ✅ MyProject2.Wood.Structure.SupportPath
