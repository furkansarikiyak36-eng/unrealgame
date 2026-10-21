# Faz 0 — Taş ve çubuk sistemleri statik kaynak denetimi

**Durum:** Kaynak-katalog eşleştirmesi tamamlandı; otomatik/oyun içi kabul açık.  
**Kaynak ailesi:** STONE 96 satır, STICK 76 satır.  
**CI:** Dokuz katalog eşleştirme dosyası `Scripts/run_python_tests.sh` tarafından satır sayısı ve şema kontrollerinden geçiriliyor.

## Taş sistemi

- Eşleştirme: [FAZ_00_STONE_KATALOG_ESLESTIRME.csv](./FAZ_00_STONE_KATALOG_ESLESTIRME.csv)
- Kaynak taraması: **71 U2**, **9 U1**, **16 U0**; U3–U6 yok.
- İlgili modüller: `StoneActor`, `StoneMiningComponent`, `StoneThrowComponent`, `StonePlacementComponent`, `StoneConnectionGraph`, `StoneSupportComponent`, `StoneWorkingComponent`, `StoneFireplace`, `StoneStoragePile`, `StoneSled`, `StoneTrapActor`.
- **P0 blocker — yüz seçimi:** `StonePlacementComponent::CycleFace` açık TODO; geçerli yüz/socket seçimi tamamlanmış sayılamaz.
- **P0 blocker — oyuk/eklem işleme:** `StoneWorkingComponent::HollowGroove` ve `EngraveJoint` kaynakta yalnızca `Stone != nullptr` döndürür; geometri/bağlantı state'i değiştirmez.
- **P0 blocker — taş tuzağı hasarı:** `StoneTrapActor::Trigger` fizik/soğuma durumunu işler; hedefe hasar bağlama yorum/TODO seviyesindedir.
- **P0 blocker — taş kırma:** `CrushBoulder` kaynak kayayı, en az bir parça üretildiğinde yok ediyor. İstenen parça sayısının tamamı üretilemezse miktar kaybı riski var.
- Duvar/kemer/kolon/temel/kızak/depo/şömine için sınıflar ve yerel metotlar var; bu, tüm bunların `ACarvableLog` ile aynı otoriter yapı grafına bağlandığını veya dünya kayıt sistemiyle beraber kaydedildiğini kanıtlamaz.

## Çubuk sistemi

- Eşleştirme: [FAZ_00_STICK_KATALOG_ESLESTIRME.csv](./FAZ_00_STICK_KATALOG_ESLESTIRME.csv)
- Kaynak taraması: **50 U2**, **20 U1**, **6 U0**; U3–U6 yok.
- İlgili modüller: `StickActor`, `StickInventoryComponent`, `StickBreakingComponent`, `CraftingComponent`, `TorchComponent`, kazık/çit bileşenleri, `TrapMechanismComponent`, `TripWireAlarm`, mobilya/depo/çevre aktörleri ve `StickDemolitionComponent`.
- **P0 blocker — üretim atomikliği:** `CraftingComponent::Craft` birden çok girdiyi sırasıyla kaldırıyor; ara kaldırma veya çıktı ekleme başarısızlığında önceki tüketimi geri alma yolu görünmüyor.
- **P0 blocker — parçalama:** `StickBreakingComponent::CompleteBreak` birden fazla çıktı üretmeye çalışıp kaynak çubuğu yok ediyor; kısmi spawn hatasında eksik malzeme riski var.
- **Entegrasyon boşluğu — yağmur toplayıcı:** `WaterCollector::Tick` içinde hava/yağış bağlantısı açıkça sonraya bırakılmış.
- **U0 gereksinimler:** Kafatası ışığı/dizimi, sinek tuzağı, adı belirtilen “Hokey Pokey” tuzağı, zırh rafı ve genel kaldıraç için bu kaynak taramasında tam davranış zinciri bulunmadı.
- `StickDemolitionComponent::RecoverFromStructure` tahmini geri kazanım adedi döndürür; bu adedin fiilen envanter/depo stoğuna eklenmesi bu metoda bağlanmış değil.
- `StickActor`, `StoneActor`, `ACarvableLog` kendi state/connection sistemlerine sahip. Birlikte çalışan bir çatı için tek kanonik `IStructuralNode/UStructuralGraph` kullanımını, kayıt/yüklemeyi ve hasar/çöküş bildirimlerini doğrulamak gerekiyor.

## Ortak kabul koşulları

1. Her catalog ID'nin gerçek bir kaynak çağrı zinciri, test ID'si ve editor/oyun içi senaryosu olmalı.
2. Üretim, kırma, söküm ve depolama işleminde malzeme sayısı korunmalı; kısmi spawn veya dolu envanter kaynak kaybına yol açmamalı.
3. Stone/Stick graph'ları ile ana ahşap `ACarvableLog` yapısı arasında bağlantı, hasar ve çöküş tek otoriter yapı state'inde sonuçlanmalı.
4. UE 5.8 üzerinde derleme ve oyun testi yapılmadan bu satırlar U4/U5/U6 olarak yükseltilmemeli.

## Karar

STONE ve STICK kataloglarının tüm satırları kaynak triage tablosuna işlenmiştir. Bu yalnızca Faz 0'ın kaynak haritası ayağını tamamlar; taze UE testleri, satır-özel test kimlikleri, görsel/editor kanıtları ve yüksek riskli işlem hatalarının düzeltilmesi beklediğinden Faz 0 genel kabulü **açık** kalır.


## İşlenen kaynak kaybı ve no-op başarı engelleri

Son kaynak düzenlemeleri:
- `UStoneWorkingComponent::CrushBoulder`: bütün istenen taş parçaları oluşturulmadan kaynak kaya tüketilmiyor; herhangi bir spawn veya kaynak yok etme başarısızlığında üretilen parçalar geri temizleniyor. Yeni parçalar kaynak taşın `UStoneDefinition` verisiyle başlatılıyor.
- `UStickBreakingComponent::CompleteBreak`: bütün kırık çubuklar oluşturulmadan kaynak çubuk yok edilmiyor; kısmi spawn hatasında üretilmiş çıktılar temizleniyor ve kırma denemesi durduruluyor.
- `HollowGroove`, `EngraveJoint` ve `SmoothSurface` şimdilik `false` dönüyor. Bunlar geometriyi değiştirmeden başarı bildiriyordu; artık sahte başarı üretmiyorlar. Bu metotların gerçek geometrik işlemi hâlâ uygulanmayı bekliyor.

**Doğrulama sınırı:** Bu güncellemeler kaynak seviyesinde işlendi ancak UE 5.8 derleme/oyun içi test koşusu henüz alınmadı. P0-024 ve P0-026, motor testi olmadan kapatılmaz.


## Taş yerleştirme yüzü/socket seçimi — kod ve otomasyon testleri eklendi

`UStonePlacementComponent` artık:
- `BeginPlacement(AStoneActor*)` ile yalnızca tanımı olan Loose/Carried taşın yerleştirmesini başlatıyor; özgün transform, state, attach parent ve socket bilgisini tutuyor.
- `UpdatePlacementTarget` ile nokta/normal girişini doğruluyor; yüzey normali boyunca yerel Z eksenini hizalıyor.
- `CycleFace` ile geçerli `UStoneDefinition::Faces` seçeneklerini, ardından geçerli `Sockets` seçeneklerini dolaşıyor; yüz normalini temas yönüne çeviriyor ve socket yerel konumunu temas noktasına oturtuyor.
- `ConfirmPlacement` için gerçek geçerli hedef şartı koyuyor; `CancelPlacement` taşın önceki fiziksel konum/durum/attach durumuna dönmesini deniyor.

`Source/MyProject2/Tests/StonePlacementTests.cpp` üç Unreal Automation Test içeriyor: begin/cancel, yüz ve socket hizalaması, geçerli hedef olmadan onayın reddi.

**Kabul durumu:** Kod/test kaynakları branch'e eklendi; UE 5.8 derlemesi ve test çalıştırması bu ortamda yapılmadığından P0-023 kapanmış sayılmaz. Otomasyon sonucu, preview collision ve gerçek Blueprint input bağlantısı motor hostunda doğrulanmalı.


## Güncel entegrasyon — oyuncu taş taşıma, çarpışma ve yapısal socket bağlantısı

Son kaynak turunda taş sistemi oyuncunun ana input akışına bağlandı; artık yalnızca bağımsız bir component/demo çağrısı değildir.

- `AForestCharacter` constructor'ı `UStoneCarryComponent` ve `UStonePlacementComponent` oluşturur.
- `Interact/E`: görüşteki fiziksel `AStoneActor` tanımı ve taşıma ağırlığı uygunsa alınır; eldeki taş E ile bırakılır.
- Sol tık: eldeki taşta ilk tık yerleştirme önizlemesini başlatır; önizleme açıkken ikinci sol tık onaylar.
- `Q`/mouse wheel yüz/socket seçimini, `R` yön döndürmeyi, sağ tık iptali kullanır. Taş taşınırken hareket hızı/tool visibility de mevcut karakter durumuna bağlandı.
- Yerleştirme trace'i oyuncunun göz konumunu/yönünü kullanır, taşın kendisini trace dışında tutar ve seçilen yüz/mesh sınırını temas düzlemine yaklaştırır.
- `IsPlacementClear` component overlap sorgusuyla bloklayan fiziksel nesne/pawn çakışmasını kontrol eder; hedef destek aktörü, istenen temas yüzeyine izin vermek için overlap sorgusundan çıkarılır. Bu nedenle bütün hedef geometrisi/karma collider durumları UE 5.8 editöründe ayrıca doğrulanmalı.
- Hedef bir `IStructuralNode` ve `UStructuralGraph` taşıyorsa yerleştirme, yapısal bağlantı kabul edilmeden `Installed` state'ine geçmez. Açıkça seçilen kaynak socket'i yalnızca hedef taşta yakın ve boş bir socket ile eşleşirse commit edilir; `ConnectTo` iki uçta da reciprocal bağlantı kaydı üretir. State geçişi başarısızsa yeni bağlantı geri alınır.
- Socket doluluğu, paylaşılan `UStoneDefinition` içindeki transient `OccupiedBy/Occupant` alanlarından değil, aktörün kendi `UStructuralGraph::Connections` listesinden okunur. Aynı DataAsset'i paylaşan taşlar böylece birbirlerinin runtime occupancy state'ini devralmaz.

### Otomasyon kapsamı (kaynakta eklendi, henüz UE 5.8'de çalıştırılmadı)

`Source/MyProject2/Tests/StonePlacementTests.cpp` beş Unreal Automation Test içerir:

1. `MyProject2.StonePlacement.Lifecycle.BeginCancel`
2. `MyProject2.StonePlacement.Selection.FaceAndSocket`
3. `MyProject2.StonePlacement.Lifecycle.ConfirmGate`
4. `MyProject2.StonePlacement.Validation.CollisionClearance`
5. `MyProject2.StonePlacement.Structure.SocketConnection` — reciprocal graph bağlantısını ve dolu target socket'in ikinci taş tarafından yeniden kullanılmasının reddedilmesini kontrol eder.

`Scripts/run_tests.sh` artık `MyProject2.StonePlacement` suite'ini ayrı logda çalıştırır. `Scripts/verify_test_logs.py` minimum **5** benzersiz başarılı stone placement sonucu olmadan doğrulamayı geçirmez.

**Açık kalan sınır:** Bu beş C++ testi UE 5.8 üzerinde henüz derlenmedi/çalıştırılmadı. Genel ahşap graph adaptörü, yapı çöküşünün bütün sistemlere yayılması, graph save/load ve editör görsel doğrulaması hâlâ kabul kapılarıdır. Bu kod düzeyi ilerleme “oyun içinde onaylandı” anlamına gelmez.


## En son durum — yerleştirme ve yapısal socket kapısı

Yapısal hedefe yerleştirme artık yalnızca Installed state'i vermiyor. Hedef actor, IStructuralNode ve UStructuralGraph taşıyan bir AStoneActor ise kaynak yüzü/socket'i hedef taştaki yakın, doğru yöne bakan ve boş socket ile eşleşmeden commit edilmez. Kaynak face/socket adı ve hedef socket adı iki yönlü graph link'ine kaydedilir. State geçişi başarısızsa yeni graph bağlantısı geri alınır.

Socket world-position/normal hesabı AStoneActor mesh component transform'undan yapılır; bu, actor transform'una ek mesh-relative scale/transform'u da içerir. Yerleştirme bounds-ofseti de mesh component relative transform'unu dikkate alır. Kullanılmış socket/yüz adı ortak UStoneDefinition asset alanlarından değil actor graph'ından türetilir.

Otomasyon dosyası artık beş Unreal Automation Test içeriyor; beşinci test reciprocal bağlantıyı ve dolu hedef socket'in tekrar kullanılmasının reddedilmesini doğrular. Bu testler UE 5.8 hostunda henüz çalıştırılmadı. Legacy ahşap/diğer yapısal sınıflar için adapter bu değişiklikte uygulanmadı; graph-bearing ancak AStoneActor olmayan hedeflere placement fail-closed davranır.


## Taş tuzağı — tetik ve hasar hattı eklendi

Taş tuzağı artık sadece fiziği etkinleştirmiyor. Trigger volume QueryOnly / Pawn-overlap olarak ayarlanıyor ve BeginOverlap callback doğrudan Trigger(Victim) çağırıyor. Trigger, bilinen gameplay hedefleri için mevcut alıcı metotlarını kullanıyor: RaidEnemy, ForestCharacter, AStoneActor ve ACarvableLog; diğer aktörlere Unreal'ın standart ApplyDamage/TakeDamage yolunu iletiyor. Kütükte raw damage pointleri 0..1 normalized damage fraction'a editable çarpanla dönüştürülüyor.

Cooldown işlemi de düzeltildi: bir tuzak cooldown bitmeden Rearm çağrısıyla yeniden kurulmuş sayılmıyor. İki Unreal Automation Test yazıldı: damage dispatch + cooldown ve null-victim safety. Bunlar UE 5.8'de henüz çalıştırılmadı; fiziksel rolling/falling impact, gerçek overlap ve damage tuning editörde doğrulanmalı.


### Ek not — Stone Trap overlap kanalları

Trap trigger volume artık Pawn **ve** PhysicsBody ile overlap yapacak şekilde ayarlı; böylece karakter/düşman yanında fiziksel taş ve kütük gövdeleri de tetik alanına girebilir. StoneTrapTests.cpp, bu iki collision cevabını açık assertion ile kontrol ediyor. Bu yalnızca test kodudur; gerçek Unreal overlap/physics davranışı UE 5.8'de çalıştırılana kadar onaylanmış sayılmaz.


### Stone Trap alıcı testleri genişletildi

StoneTrapTests.cpp artık dört test içerir:
- Damage.DispatchAndCooldown — ARaidEnemy receiver, cooldown ve rearm.
- Damage.NullVictimSafety — null victim güvenliği ve cooldown.
- Damage.PhysicalStone — AStoneActor health azalması.
- Damage.CarvableLog — yapı kütüğünde normalize hasar artışı.

Windows/UE 5.8 test runner ve log verifier bu suite için en az dört benzersiz başarılı test sonucu istiyor. Kod ve test kaynakları branch'te; UE 5.8 C++ build/automation hâlâ çalıştırılmadı.
