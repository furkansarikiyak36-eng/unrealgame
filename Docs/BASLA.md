# Buradan başla (web Claude için)

Bu sayfayı oku, gerisini ihtiyacın olunca oku. Hepsi `Docs/` altında, `.md`. Kaynak kodu (`.cpp`) okuyamazsın; `KOD_HARITASI.md` kodu senin için özetler.

## Proje
`MyProject2`: Unreal Engine 5.8, C++, sanatsız (greybox) hayatta kalma oyunu. Odak: **kütük sistemi** (ağaç kesme, kesme ve oyma, taşıma, yerleştirme, yapı, destek, ateş, hasar). Referanslar (her oyundan yalnızca bir şey): kütük/taş/çubuk Sons of the Forest; baskın ve az voxel 7 Days to Die; kalıntılarla hikâye Subnautica; bölge zorluğu ve günlük seçim Frostpunk + The Long Dark; tarım/hayvancılık çiftlik oyunları; köprü/iple dünyayı medenileştirme Death Stranding. Ayrıntı `KARARLAR.md`.

## Ne nerede
| Dosya | Ne için | Ne zaman |
|---|---|---|
| `DURUM_RAPORU.md` | Ne çalışıyor, ne eksik, kontroller | İlk, 1 sayfa |
| `KARARLAR.md` | Verilmiş kararlar ve bekleyenler | Tasarım önermeden önce |
| `PROJE_GUNLUGU.md` | Yapılan her işin kaydı (en sondaki girişler en yeni) | Bir şeyin neden yapıldığını merak edince |
| `KOD_HARITASI.md` | Her sınıfın genel fonksiyonları ve açıklamaları | Kod yazmadan önce, hangi fonksiyon var diye |
| `TEST_DURUMU.md` | Son test sonucu, hangi kontroller geçiyor | Bir şeyin çalıştığından emin olmak için |
| `KUTUK_SISTEMI_PLANI.md` | Fazlar ve tasarım ilkesi | Plan konuşurken |

## Tasarım ilkesi (özet)
Oyuncu bir kütükle ne yapacağına kendi karar verir; tarif yok. Kesimler yalnızca sabit noktalarda (65 cm'lik modül çizgileri). Parçalar fiziksel nesne; bir parçanın işlevini şekli, yönü ve bağlantıları belirler (dik = direk, yatay = kiriş, eğik = payanda, yarım kütük yatay = döşeme). Görsel dil: kırmızı kesikli çizgi = kesilecek yer, beyaz = bağla.

## Kod yazıyorsan (zip ile teslim)
1. Zip içindeki yollar `Source/MyProject2/` ile başlasın. Başka klasöre dosya koyma.
2. Yeni mantığı **kendi dosyalarında** yaz. `CarvableLog`, `ForestCharacter`, `StructureLink`, `MechanicsDemo` gibi mevcut sınıflara dokunman gerekiyorsa, hangi fonksiyonu nasıl değiştirmek istediğini notta açıkça yaz; kaynağı okumadan değiştirme.
3. Zip'in yanına kısa bir not koy: ne yaptın, hangi mevcut dosyalara dokundun, hangi fonksiyonlar yeni, nasıl test edilir.
4. Derleyemeyeceğin için ilk denemede derleme hatası çıkabilir; bu normal. Claude Code (bilgisayarda) zip'i açar, derler, düzeltir, testleri çalıştırır ve sonucu günlüğe yazar.
5. Kod stili: tab girinti, açıklayıcı İngilizce yorum (`/** ... */`), `MYPROJECT2_API`. Yeni genel fonksiyonlara açıklama yaz: `KOD_HARITASI.md` bunlardan üretilir.

## Kurallar
- Kullanıcı Türkçe konuşur; yanıtlar Türkçe.
- Oyunda görülmeyen bir şeyi "çalışıyor" diye yazma: `TEST_DURUMU.md` yalnızca grafiksiz mantığı doğrular, görsel kısım kullanıcı tarafından oyunda denenir.
- Her mekanik değişikliği için `AMechanicsDemo` otomatik testine kontrol eklenir (Claude Code yapar).

## Şu anki durum (kısa)
Kütük sistemi uçtan uca çalışıyor: ağaç → kesim ve oyma (9 alet) → taşıma → direk, kiriş, payanda, döşeme, kabin → destek ve kırılma → ateş, hasar, onarım → yapı tanıma. Açık işler ve kararlar `KARARLAR.md` içinde.
