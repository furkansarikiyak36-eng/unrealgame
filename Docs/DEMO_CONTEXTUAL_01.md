# Demo_Contextual_01 — Oynanabilir bağlam sahnesi

**Hedef:** Gerçek oynanışta mekanikleri doğal bağlamlarında denemek. Bu harita, otomatik testlerin sabit koordinatlarla çalıştığı `Demo_WoodChain` haritasının yerine geçmez; onun yanında kullanılır.

**Motor durumu:** Sahne üreticisi ve C++ değişiklikleri bu dalda yazıldı; bu oturumda Unreal Editor, UHT veya oyun içi çalışma başlatılmadı. Bu nedenle harita henüz motor içinde açılmış ve görsel olarak kabul edilmiş sayılmaz.

## Harita düzeni

Harita yaklaşık **70 × 70 metre** büyüklüğünde, küçük ama yürüyerek istasyonlar arasında geçiş yapılabilecek bir gri-kutu sahnesidir. Dekor amaçlı rastgele prop yığını yerine, her istasyon bir mekanik grubunu gösterir. İşlevli olması gereken ağaçlar ve kayalar statik dekor değil `AHarvestableActor` olarak yerleştirilir.

| İstasyon | Oyuncu ne yapar? | Sahne kuralı |
|---|---|---|
| Başlangıç / görev panosu | Rehberli kütük zincirini başlatır; kontrolleri öğrenir | Mekanik demosu oyuncu için başlar; otomatik test aracı değildir |
| Orman | Ağaca balta vurur, ağacı devirir, gövdeyi aşamalı keser ve düşen parçaları taşır | Yalnızca dört etkileşimli ağaç; diğer ağaç kümeleriyle gereksiz kalabalık oluşturulmaz |
| İşleme sehpası | Yuvarlak kütük, enine kesim, boyuna yarma, yarım kütük ve eklem/oluk işlemlerini dener | Ham kütükler sehpa yüksekliğinde ve birbirinden ayrı durur; oluk hazır kesilmiş olarak konmaz |
| İnşa alanı | Direk, kiriş, payanda, döşeme, bağlantı, destek kaybı ve çöküşü dener | Hazır kabin yoktur. Oyuncunun kurduğu parçalar tanım ve destek hesabına girer |
| Taş ocağı | Kayayı kırar, küçük taş toplar ve taşıma akışını dener | Kayalar hasar alabilen gerçek harvestable aktörlerdir |
| Ateş yeri zemini | Yedi küçük taşı tek tek yerleştirir, yakıt ekler ve ateşi yakar | Ateş yeri önceden kurulmaz; oyuncu kurulum adımlarını kendisi yapar |
| Kazık dikme zemini | Kütük ucunu sivriltir ve parçayı doğru yönde zemine dikerek savunma elemanına dönüştürür | Sivri uç tek başına tuzak sayılmaz; yukarı bakmalı, parça dik olmalı ve alt ucu doğrudan zemine dayanmalıdır |
| Taşıma alanı | Kütükleri el arabasına/kızağa yükler, kapasiteyi ve boşaltmayı sınar | Arabalar malzeme stokunun yakınında; oyuncunun bunları araması gereken rastgele bir yere konmaz |

İstasyon işaretleri yalnızca giriş/bağlantı noktalarında bulunur; her bir nesneye uzun talimat yazılmaz. Oyuncunun görebileceği fiziksel ilişki, yazılı etiketten daha önemli olmalıdır.

## Şekle ve konuma bağlı gerçek kurallar

### Oluk

- Yuvarlak ve boyuna yarılmamış kütükte oluk seçeneği / önizlemesi bulunmamalıdır.
- İlk boyuna yarma sonrası oluşan **bir yarım kütüğün düz yüzünde** tek ve boydan boya oluk açılabilir.
- İkinci kez yarılarak çeyrek parçaya dönüştürülen parçada bu oluk seçeneği sunulmaz.
- Oluk geometrisi gerçek malzeme çıkarmalı; yalnızca ortada bir çentik çizip “oluk açıldı” yazmamalıdır.
- Aynı yarım kütüğe ikinci oluk açılması reddedilir; sahte başarı mesajı gösterilmez.

Bunlar `CarvableLog.cpp`, `ForestCharacter.cpp` ve rehberli mekanik demosuna bağlandı. **Geometri ve motor davranışı henüz UE içinde çalıştırılarak doğrulanmadı.**

### Kazık / sivri savunma parçası

- Sivri uç oluşturmak yalnızca malzeme hazırlama adımıdır.
- Savunma kazığı olarak tanınması için parça dikey olmalı, ucu yukarı bakmalı ve alt ucu doğrudan zemine dayanmalıdır.
- Bir kütüğün üstüne konmuş sivri parça veya ucu aşağı bakan parça, işlevsel kazık olarak sınıflandırılmaz.
- Hazır bir kazık duvarı sahneye konmaz; oyuncu parçaları kurar.

Kural `StructureLink::MakeNode` içindeki yapı rolü hesabına ve üç bağlam kontrolüne eklendi. Gerçek oyun içi çarpışma/zemin denemesi yine motor doğrulamasına bağlıdır.

## Haritayı üretme

Ön koşul, C++ modülünün ve Geometry Script/Editor Python eklentilerinin proje ortamında çalışıyor olmasıdır. Editör kapalıyken `Scripts/build_greybox.py` çalıştırılır. Normal çalıştırma meshleri ve gri-kutu haritalarını üretir; yalnızca bağlam sahnesini yenilemek için aynı komut ortamında `GREYBOX_BUILD=contextual` seçilebilir.

Üretim betiği:
- `/Game/Maps/Demo_Contextual_01` haritasını oluşturur.
- `/Game/Maps/Demo_WoodChain` haritasını otomatik mekanik testleri için korur.
- İşlevli ağaç/kayaları gerçek aktör olarak üretir.
- Hazır kabin, hazır ateş çukuru, önceden kesilmiş oluk veya sahte diken tuzağı eklemez.

Haritayı üretmek, C++ derlemesinin veya görsel doğrulamanın başarıyla tamamlandığını göstermez. Harita açıldıktan sonra PlayerStart, istasyon yönleri, etkileşim menzili, prop çarpışmaları, işlevli ağaç/kayalar ve Python sınıf/enum bağları tek tek kontrol edilmelidir.

## Test düzeni ve sınır

- `Demo_WoodChain`: sabit koordinatlı otomatik MechanicsDemo/gerileme test alanı; üzerine fazladan etkileşimli kütük veya rastgele prop yığılmaz.
- `Demo_Contextual_01`: oyuncunun gerçek bağlamlarda deneyebileceği küçük istasyon haritası.
- `MechanicsDemo`: rehberli kütük işlemi; otomatik mod ve testler ayrı çalıştırılır.

Bu dalın `Docs/TEST_DURUMU.md` belgesi son kayıtlı koşumda **101 mekanik kontrolü** raporlar. Bu değişiklikle `RunShaping` içine 7 ek bağlam raporu kondu (yuvarlak kütükte oluk reddi, oluk önizlemesi, geçerli oluk, çeyrek parçada ret ve üç kazık durumu). Derleme/test çalıştırılmadığı için yeni toplamı geçmiş bir test sonucu gibi sunmuyoruz.

Ekli 14 Ekim durum raporundaki **119/119** sonuç, yerel `integrate/web-plan` dalına aittir. GitHub'da bu oturumda erişilebilen dallar `main` ve `plan/phase-00-baseline`; yerel 119-kontrollü sürüm buradaki dala senkronize değil. Bu nedenle bu harita, şimdilik bu dalda mevcut olan mekanik grupları için bir aday sahnedir; tüm 119 yerel kontrolün tek tek bu haritaya eşlendiğini veya yerel sahnenin birebir yeniden üretildiğini iddia etmez.

## Tam 119 kontrolü olan sürüme geçiş için zorunlu eşleme

Ekli yerel durum raporunda yer alan daha geniş oyun akışındaki 119 kontrolün tamamını bu eski dalın sahnesine yerleştirilmiş gibi göstermiyoruz. O sürümde olup bu dalda kaynak sınıfı/harita bağlantısı henüz bulunmayan şu bağlamlar, aynı gerçek mekanik sınıfları senkronize edildikten sonra bu haritaya eklenmelidir:

- **Soğuk bölge ve günlük hayatta kalma:** korunaklı/korunaksız küçük çevre deneyi; ateş, çatı, ıslanma ve giysi farkını kıyaslama.
- **Nehir geçişi:** iki kıyı ve aralarında gerçek nehir aktörü; geçiş öncesi/sonrası ıslanma ve hız farkı.
- **Yar ve ip hattı:** erişilemeyen üst kot, doğru ankraj noktaları ve ip ile çıkış; ardından köprü ile alternatif rota.
- **Kalıntı/hikâye:** tek yanmış kulübe alanında beş notun her biri ayrı ve okunabilir; rehber tabelaları notların yerine geçmez.
- **Tarla ve hayvanlar:** boş ekim parselleri, yabani tohum kaynağı ve gerçek etkileşimli hayvanlar; hayvan besleme bu dalda bağlı değilse çalışan özellik gibi sergilenmez.
- **Baskın ve savunma:** ayrı test şeridi; oyuncu saldırı başlatabilmeli, düşman yolu ve hedef seçimi görülebilmeli, duvar/çit/kazık davranışı güvenli şekilde sınanabilmeli.
- **Taş yerleştirme:** geçerli yüzey/socket, dolu socket ve engel üstüne yerleştirme için birbirinden ayrılmış test noktaları.

Bu sınıfların bir kısmı 14 Ekim yerel raporunda (`integrate/web-plan`) belirtilmiş ancak GitHub'da erişilebilir `plan/phase-00-baseline` dalında bulunmuyor. Onların yerine sahte statik dekor koymak veya bu kontroller geçti demek doğru olmaz. Yerel 119-kontrollü dal senkronize edildiğinde bu istasyonlar aynı map builder içinde gerçek sınıfları çağırarak kurulmalı; ardından her katalog/test ID'si **istasyon + önkoşul + işlem + beklenen sonuç + test kanıtı** ile eşleştirilmelidir.

## Kabul listesi

- [ ] Unreal Editor içinde `Demo_Contextual_01` açılır; harita boş/bozuk asset hatası vermez.
- [ ] Her ağaç/kaya gerçekten hasar alır ve doğru kaynak üretir.
- [ ] Yuvarlak kütükte oluk seçeneği ve önizleme yoktur.
- [ ] Yarım kütüğe boydan boya oluk gerçekten kesilir; kesilen yüz, kütüğün kalan geometrisi ve çarpışması doğru görünür.
- [ ] Çeyrek parçada oluk reddedilir ve açıklayıcı mesaj görünür.
- [ ] Sadece doğru yönde ve zemine dayalı sivri parça savunma kazığı sayılır.
- [ ] Hazır kabin/ateş/tuzağa bağlı olmayan yapı, yangın ve destek akışları baştan sona denenir.
- [ ] Taş çakışma kontrolü, özellikle bir engelin üzerine yerleştirme vakasında geçer.
- [ ] `Demo_WoodChain` üzerinde otomatik testler yeni sahne aktörlerinden etkilenmeden çalışır.
- [ ] Görsel tur, UE build/UHT ve ilgili test logları güncellenir.
