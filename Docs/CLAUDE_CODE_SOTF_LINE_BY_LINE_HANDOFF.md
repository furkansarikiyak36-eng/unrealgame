# Sons of the Forest Mekanikleri — Claude Code Satır Analizi ve Devam Raporu

**Dal:** feature/sotf-mechanics-expansion  
**Motor hedefi:** Unreal Engine 5.8 / C++  
**Bu raporun amacı:** Claude Code sonraki oturumda önce bu dosyayı okuyup yapılan değişikliklerin sorumluluğunu, çağrı sırasını ve henüz bağlanmamış parçaları anlayabilsin. Bu rapor, kaynak dosyalarını tekrar baştan tarama ihtiyacını azaltan bir kod-konum ve davranış haritasıdır.  
**Test durumu:** Bu değişiklikler sırasında CMake, CTest, Unreal Build Tool (UBT), Automation veya PIE çalıştırılmadı. Test kaynakları eklendi/değiştirildi, başarı durumu doğrulanmış değildir.  
**Dal güvenliği:** Tüm commit'ler yalnızca feature/sotf-mechanics-expansion dalına yazıldı. main dalına birleştirme/değişiklik yapılmadı.

## 1. Bu dilimde değişen dosyalar

| Dosya | Değişiklik | Kod sınırı |
|---|---|---|
| Modules/GameplayIntegration/include/world_mechanics/stone_fireplace_lifecycle.hpp | Güncellendi | Taş halka–şömine state machine ve versioned save record/restore sözleşmesi |
| Modules/GameplayIntegration/src/stone_fireplace_lifecycle.cpp | Güncellendi | Halka, yakıt, ateş, ısı, kap ve güvenli save/restore geçişleri |
| Modules/GameplayIntegration/include/world_mechanics/trap_lifecycle.hpp | Güncellendi | Tuzak state machine, event history ve cooldown-relative save record |
| Modules/GameplayIntegration/src/trap_lifecycle.cpp | Güncellendi | Armed/cooldown/consumed, event dedupe, clock rebasing ve doğrulanan restore |
| Modules/PlacementSnap/include/world_mechanics/placement_snap.hpp | Güncellendi | Doluluk, hedef uyumu, destek geçerliliği ve explicit socket filtresi |
| Modules/StructuralIntegrity/include/world_mechanics/structural_integrity.hpp | Güncellendi | Destek mutasyonları ve versioned structural snapshot API |
| Modules/StructuralIntegrity/src/structural_integrity.cpp | Güncellendi | Kenar/düğüm mutasyonları; private candidate graph ile staged snapshot restore ve topology validation |
| Source/MyProject2/StickPhaseS6_Traps/SpringTrap.cpp | Güncellendi | Eksik root component ve cooldown'ı bypass edebilen fırlatma sırası düzeltmesi |
| Modules/CMakeLists.txt | Güncellendi | Yeni iki taşınabilir C++ kaynak dosyası kütüphaneye eklendi |
| Modules/tests/module_tests.cpp | Güncellendi | Yeni edge case regresyon kaynakları eklendi; çalıştırılmadı |

### Commit dizisi

- a6db887ae8d49b1be4e63a972890ee4132055418 — şömine yaşam döngüsü header'ı.
- dd44505e3d35c119d1f804d65ef148a7fc21cc2c — şömine yaşam döngüsü uygulaması.
- 0a55ae88dfc4eefec28b146b50e3c87e5521fffe — tuzak yaşam döngüsü header'ı.
- c4b38ab57e953563f79e382e96bf69f6b20e1d6e — tuzak yaşam döngüsü uygulaması.
- ded4bff9cdd9647403723111900b878f686600cb — CMake kaynak kaydı.
- 8eb12390fda0565391b95f9bb207a6a68c40c120 — şömine/tuzak test kaynakları.
- 26754d33b79e543f3ec8f194103172e84b466d09 — snap filtresi.
- 2e6ce8a58c03bdd59d0eaa277d3ed0f1761496f8 — snap test kaynakları.
- 0e77346c4ee1c5e1a4c7e18c337c8f8159ad2e3d — support graph header API'si.
- c7ba8081976052df4ab31825fd291a05420935be — support graph mutasyonları.
- 42734ff7b34a10858ed5cdcf50379e8fc29ce401 — support regresyon test kaynakları.
- 5284f699fda3a4f4158781f5c671a62b2cb30686 — Unreal SpringTrap düzeltmesi.
- d9f229c7e48e318c8c9dfdb8c021166ec849de9b — şömine yapılandırma sınırı.
- cd8c7bfeaa5d745558ecb85f6c3bea2be78804ad — tuzak Disarm yapılandırma doğrulaması.
- 9cf88095aca4c44db310aac0f60546b36e6c7346 — şömine save record sözleşmesi.
- b4e616f8fb47aeb80495930b4539fa824e1969ff — şömine güvenli save/restore ve halka yeniden doğrulama.
- b954f6a8bb79b64949495923cdad89d844f85ecd — tuzak cooldown-relative save record.
- c15a757f4730bf35da808efbb36e717c9a629618 — tuzak save/restore, zaman tabanını yeniden kurma.
- fa463137c3475d17b2725c027cdb38bff05ec4ab — şömine/tuzak save-restore regresyon kaynakları.
- b84b0903152946e95821798b7ea4ee49f15116fd — structural graph snapshot API.
- e9b04928f2ea075a44f0e4427b7cb94465a18d33 — candidate graph üstünde staged restore/doğrulama.
- 9627be9c429db68a3a5035f80bfe952e8c47db5b — structural snapshot regresyon kaynakları.
- Bu rapor commit'i ayrıca aynı dalda oluşturulur.

## 2. Satır analizi: şömine yaşam döngüsü

### stone_fireplace_lifecycle.hpp — toplam 92 satır

- **1–6:** Header koruması ve ortak sözleşmeler. core.hpp Result/ErrorCode/Revision/Seconds tiplerini, taş halka header'ı ise geometri tanıma veri tiplerini sağlar.
- **8–14:** wm namespace'i ve StoneFireplaceState. NotBuilt halka geçersiz/yok; Ready geçerli halka var ama ateş yanmıyor; Burning yalnız yakıt varken aktif yanma.
- **16–23:** StoneFireplaceConfig. Halkanın tanıma eşikleri dışarıdan ayarlanabilir; yakıt kapasitesi, yakıt tüketimi, nominal ısı, maksimum tek zaman adımı ve tek taramada kabul edilen taş örneği sayısı da sınırlandırılır.
- **25–34:** StoneFireplaceSnapshot. Save/UI/adaptör tarafına yayınlanabilir durum: runtime revision'ı, halka kaynak revision'ı, state, halka tanıma sonucu, pişirme kabı varlığı, yakıt/ısı ve geometrik değerlendirme. Bu bir save dosyası codec'i değildir.
- **36–43:** StoneFireplaceTick. Advance çağrısının tüketim, kalan yakıt, ısı ve fuel-out sonucunu verir.
- **45–51:** StoneFireplaceSaveRecord schema v1: saved revision, fuel, pişirme kabı ve yükleme öncesi ateş yanıyor muydu bilgisi. Taş geometrisi kaydedilen geçerli sonucuna güvenilmez.
- **53–55:** Sınıfın motor sınırı ve otorite açıklaması.
- **59–71:** Halka reassess, yakıt/ateş/kap/time API'leri ve save/restore entry point'leri.
- **73–74:** Salt okunur snapshot/revision.
- **76–88:** Internal state. 87. satırdaki pending-resume flag, load sonrası halka yeniden görüldüğünde önceki yangını yalnızca yeni halka doğrulaması başarılıysa geri getirir.

### stone_fireplace_lifecycle.cpp — toplam 257 satır

- **9–10 — constructor:** Yalnız ayarları saklar; Actor/spawn/trace yapmaz.
- **12–36 — ConfigValid:** Halka parametrelerini, center-clearance < min ring radius koşulunu, yakıt/ısı/time step/sample limitlerini doğrular.
- **38–74 — ReassessRing:**
  1. Ayar, merkez koordinatları ve nonzero kaynak revision'ı doğrulanır.
  2. Örnek sayısı maxStoneSamples sınırını aşarsa state'e dokunmadan BudgetExceeded döner.
  3. sourceRevision <= ringSourceRevision_ reddedilir; eski/aynı gözlem state'i geri saramaz.
  4. LocalStoneRingRecognizer::Assess geometrik sonucu üretir.
  5. Yalnız çağrı seviyesi doğrulamaları geçince değerlendirme ve source revision atomik olarak saklanır.
  6. Halka kaybolursa ateş NotBuilt olur ve pending resume silinir. Restore sonrası halka yeniden tanınırsa, save kaydında ateş yanıyordu ve yakıt varsa yeniden Burning olur; aksi halde Ready.
  7. Runtime revision bir artırılır. Geometrik başarısızlık API hatası değil, recognized=false sonucudur.
- **75–95 — AddFuel:** Pozitif/sonlu yakıt, geçerli config ve doğru state revision aranır. Tanınmış halka yoksa yakıt eklenemez. Kabul edilen miktar kalan kapasiteyle sınırlıdır. Hiç yer kalmadıysa hata döner; kabul edilmeyen miktar yakıt state'ine yazılmaz.
- **96–116 — Ignite:** Eski revision reddedilir; halka tanınmamışsa veya yakıt yoksa ateş yakılmaz. Zaten yanıyorsa Success(false) idempotent no-op'tur. Başarılı yakma state'i Burning yapar ve revision'ı artırır.
- **117–128 — Extinguish:** Eski revision reddedilir. Yanma yoksa no-op; yanma varsa geçerli halka için Ready, geçersiz halka için NotBuilt durumuna geçer. Kalan yakıt silinmez.
- **129–143 — PlaceCookingPot:** Sadece geçerli halkada ve ateş yanarken kap eklenebilir. Zaten kap varsa ikinci bir kap yaratma kararı üretmez. Kap state'i ateş sönünce otomatik silinmez; dünya adaptörü fiziksel objeyi ayrı yönetmelidir.
- **144–183 — Advance:** Negatif/NaN zaman adımı, config hatası, maksimum yakalama adımından büyük delta ve stale revision reddedilir. Yanma yoksa veya delta sıfırsa state değiştirmeyen snapshot sonuçlanır. Yanarken yakıt deltaSeconds * fuelUnitsPerSecond kadar azalır. Yakıt bittiğinde sıfıra sabitlenir ve state yanmıyor durumuna geçer. Yakıt/ısı hesaplaması deterministiktir; rastgele ışık titremesi bu saf mekanik modelde bulunmaz.
- **184–201 — CreateSaveRecord:** Yalnız internal state tutarlıyken schema v1 kaydı verir. wasBurning intent'ini saklar.
- **202–234 — RestoreFromSaveRecord:** Schema/revision/fuel sınırlarını önce kontrol eder. Invalid kayıt durum değiştirmez. Geçerli kayıt yakıt/kap state'ini geri yükler fakat halka geometrisine güvenmez; state'i NotBuilt yapar ve source revision'ı resetler. Kayıt öncesi yanıyorsa pending resume intent saklanır. Revision max(current,saved)+1 olur.
- **235–247 — Snapshot:** Otoritatif state'in salt okunur kopyası.
- **248–254 — CurrentHeatOutput:** Yanma ve doğrulanmış halka yoksa 0; aksi halde yakıt oranına göre nominal ısı.
- **256:** Namespace kapanışı.

### Şömine adaptöründe henüz yapılmayanlar

Portable state machine, mevcut AStoneFireplace Actor'una henüz bağlanmadı. StoneFireplace.cpp şu an kendi CurrentFuel, bIsLit, light/particle ve Tick tüketimini yönetiyor. İki state machine'i birden otorite olarak çalıştırmayın. Unreal uyarlamasında ya Actor, portable modelin komut/sonuçlarını kullanmalı ya da bir adaptör bu modelin kararlarını tek yerden uygulatmalı. Taş örnekleri bounded yerel query'den ve stabil ID ile sağlanmalı; bütün dünyadaki tüm Actor'ları her Tick taramak kabul edilmez. Inventory yakıtı yalnız AddFuel kabul ettikten sonra tüketilmeli; Actor spawn/etki işlemleri başarısız olduğunda rollback sözleşmesi tanımlanmalı.

## 3. Satır analizi: tuzak yaşam döngüsü

### trap_lifecycle.hpp — toplam 108 satır

- **1–5:** Ortak Result/revision/time sözleşmesi ve namespace.
- **7–19:** Trigger kaynakları ve lifecycle durumları.
- **21–29:** Konfigürasyon: kararlı trap ID, kaynak, cooldown/minimum rearm, damage, one-shot ve cooldown üst sınırı.
- **31–41:** Runtime snapshot; event/instigator history, hasTriggered flag, zamanlar ve trigger count.
- **43–54:** TrapLifecycleSaveRecord schema v1. Mutlak dünya zamanı yerine son tetiklenmeden beri geçen süre ve kalan cooldown saklanır.
- **56–74:** Trigger request/decision contract. Karar alanları side effect uygulamaz; adaptörün uygulayacağı damage/event/state'i tarif eder.
- **78–91:** Arm/Disarm/TryTrigger/Advance/save/restore/snapshot public API.
- **93–104:** Config ve state. hasTriggered_ sentinel zamandan bağımsız olarak trigger geçmişini korur.

### trap_lifecycle.cpp — toplam 161 satır

- **9–10 — constructor:** Konfigürasyonu alır; dünya işlemi yapmaz.
- **12–28 — ConfigValid:** ID, enum, cooldown, minimum rearm gecikmesi, damage ve üst sınırları doğrular.
- **29–49 — Arm:** Config ve revision kontrolü; Consumed tuzak tekrar kurulamaz, zaten Armed ise no-op, CoolingDown iken erken kurma reddedilir. Yalnız Disarmed → Armed gerçek state değişimi revision artırır.
- **50–67 — Disarm:** Config/revision doğrular. Consumed değiştirilemez; zaten kapalıysa no-op. Diğer durumlar Disarmed olur. Bu çağrı cooldown timer'ını iptal etmek isteyen Unreal adaptörü için authoritative karar noktasıdır.
- **68–129 — TryTrigger:**
  1. Config, event ID ve zaman doğrulanır.
  2. Beklenen revision mevcut değilse StaleRevision.
  3. Event ID, son kabul edilmiş event ID'den büyük değilse tekrar/sıra dışı olay Conflict ile reddedilir.
  4. Trigger kaynağı uyuşmuyorsa Unsupported; tuzak Armed değilse Conflict.
  5. Zamanın geri gitmediği kontrol edilir.
  6. Gerçek cooldown, yapılandırılan cooldown ile minimum rearm süresinin maksimumudur; böylece 0 cooldown olsa dahi aynı anda yinelenen overlap callback'leri hemen tekrar tetikleyemez.
  7. Event ID, tetikleme zamanı ve instigator tek kabul işlemi olarak kaydedilir; trigger sayısı ve state revision artırılır.
  8. Tek kullanımlı tuzak Consumed, diğer tuzak CoolingDown olur.
  9. Dönen karar, adaptörün damage uygulayıp uygulamaması gerektiğini bildirir. instigatorId == 0 ise çevresel/kurbanı olmayan event damage uygulamaz.
- **130–148 — Advance:** Zaman/config/revision kontrolü. Cooldown bitmediyse no-op; bitmişse CoolingDown → Armed ve revision artışı.
- **149–181 — CreateSaveRecord:** Dünya saati geçerliyse ve son tetiklenmeden geriye gitmiyorsa state'i kayda çevirir; CoolingDown için cooldown bitimine kalan süreyi yazar.
- **182–245 — RestoreFromSaveRecord:** Schema, trap ID, lifecycle/config tutarlılığı, trigger history ve zaman alanları doğrulanır. Kayıt yanlışsa live state değişmez. Mutlak dünya saati taşınmaz; elapsed ve remaining süreler yeni clock origin'e çevrilir. Revision stale request'leri geçersiz kılacak şekilde artırılır.
- **246–259 — Snapshot:** Diagnostics/runtime snapshot; snapshot disk formatı değildir.

### Tuzak adaptörü kuralı

TryTrigger kabul edilmeden damage/launch/particle gibi etki uygulamayın. Adaptörde hem UTrapMechanismComponent hem de wm::TrapLifecycle kullanılırsa iki ayrı state otoritesi oluşturmayın; birini tek otorite seçin. Event id'sini tekrar oynatma idempotency anahtarı olarak etki katmanına da geçirin. Save/load'da mutlak world time kullanılan cooldownEndsAt değeri, dünya saati resetleniyorsa doğrudan taşınmamalı; kalan süreye çevrilip yeni zaman tabanında yeniden kurulmalıdır.

## 4. Satır analizi: global snap ve placement

### placement_snap.hpp — 114 satır

- **7–20 — SnapCandidate:** İlk 16 satır mevcut aday kimliği/transform/normal/mesafe/socket/region/collision/revision verileridir. 17–19 ilavesi: targetCompatible yanlışsa parça türü uymaz; supportValid yanlışsa yük yolu yoktur; socketOccupied true ise açıkça tanımlanmış socket doludur. Varsayılanlar eski çağrıların source compatibility'sini korur; adaptör bunları live world'den açıkça doldurmalıdır.
- **21–31 — SnapRequest:** İstenen yön/normal, mesafe ve revision/bütçe; 30. satır requireExplicitSocket seçeneği. Serbest zemin yerleşimi için false; fiziksel montaj socket'i zorunluysa true.
- **34–54 — SnapPlankPositionToGrid:** Kalas-only metrik grid yardımcı fonksiyonu. Girdi/spacing doğrulanır, grid hücreleri hesaplanır, sonlu sonuç kontrolü yapılır. Kütük, taş ve kazı otomatik olarak 20 cm grid kullanmaz.
- **55–88 — Resolve:** İstek ve aday bütçesi doğrulanır. 67–78 aday filtresidir: kimlik/transform/normal/mesafe, region, collision, target compatibility, support validity, socket occupancy, gerekli explicit socket ve kaynak revision'ları. 80–86 deterministik sıra: önce explicit socket, sonra küçük mesafe, son eşitlikte küçük kararlı ID. Geçerli aday yoksa NotFound.
- **89–111 — ValidateCommit:** Preview sırasında kabul edilmiş adayın commit öncesi aynı surface/classification revision'a sahip olduğu kontrol edilir. Bölge, hedef uyumu, destek, socket boşluğu ve collision yeniden doğrulanır. Son 3 bool parametrenin varsayılanları geriye dönük çağrı uyumluluğu içindir; gerçek adaptör bunları canlı world sorgusuyla vermelidir.

### Snap için adaptör sözleşmesi

Resolve yalnız seçim yapar, dünyaya nesne spawn etmez. ValidateCommit başarılı olsa bile hemen ardından aynı oyun thread'i/transaction içinde actor spawn, kaynak tüketimi ve bağlantı oluşturma yapılmalı; transaction başarısızsa kaynak ve bağlantı rollback edilmelidir. Adayın socketOccupied, supportValid ve targetCompatible alanlarını sabit true bırakmak filtreyi yalnızca görünüşte çalıştırır.

## 5. Satır analizi: StructuralGraph

### structural_integrity.hpp — 57 satır

- **6–15:** StructureNode: kararlı ID, zemine oturma, etkinlik, başlangıç yükü, kapasite, sağlık ve collapse latch.
- **16–22:** SupportEdge: destekleyen düğüm → desteklenen bağımlı düğüm yönünde kenar; kapasite ve aktiflik.
- **23–35:** SupportState, tek düğüm değerlendirmesi ve graph revision'lı toplu sonuç.
- **38–50:** API. Yeni 41–45 metodları: RemoveEdge, SetEdgeActive, SetNodeDemand, SetNodeGrounded, SetNodeCapacity; bunlar destek kaybı ve yük/zemin değişikliklerini revision'lı hale getirir. ApplyDamage, Repair ve LatchCollapse mevcut davranışlardır.
- **52–54:** Düğüm ve kenar kayıtları, graph revision.

### structural_integrity.cpp — 208 satır

- **7–13 — AddNode:** Kimlik, yük, kapasite ve sağlık aralığını doğrular; ID çakışmasını reddeder; yalnız sonra kaydeder ve revision artırır.
- **14–22 — AddEdge:** ID, self-edge ve kapasite doğrulanır; iki endpoint'in varlığı zorunludur. Kenar yönü supporterId → dependentId olmalıdır.
- **23–29 — RemoveNode:** Düğümü ve ona bağlı tüm kenarları kaldırır; tek revision artırımı graph'ın tamamındaki değişikliği temsil eder.
- **30–35 — RemoveEdge:** Bir destek bağlantısını kaldırır. Olmayan kenar NotFound; başarılı silme revision artırır. Sonraki Assess destek kaybını hesaplar.
- **36–44 — SetEdgeActive:** Kenarı silmeden askıya alma/yeniden etkinleştirme. Aynı değer no-op; değişiklik revision artırır.
- **45–55 — SetNodeDemand:** Sonlu ve negatif olmayan talep; var olmayan düğüm reddedilir; eşit değer no-op; gerçek değişiklik revision artırır.
- **56–64 — SetNodeGrounded:** Düğümün doğal zemin/bedrock ankrajını değiştirir. Zemin kaldırma/yeniden sabitleme olayına karşı revision oluşturur.
- **65–75 — SetNodeCapacity:** Sonlu ve negatif olmayan yeni kapasiteyi uygular. Sağlık oranı Assess sırasında kapasiteye ek olarak uygulanmaya devam eder.
- **76–82 — ApplyDamage:** Hasarı sağlık değerinden düşürür, sıfıra geldiğinde düğümü inactive yapar ve revision artırır.
- **83–90 — Repair:** Collapse latch'li node sıradan tamirle geri getirilemez. Diğer node sağlık üst sınırına kadar onarılır.
- **91–95 — LatchCollapse:** Çift collapse çağrısını idempotent hale getirir; node'u inactive yapar.
- **96–son — Assess:** Aktif kenarlardan inbound/outbound graph kurar; düğüm indegree üzerinden topolojik sıralama yapar; cycle varsa Conflict; yükü bağımlıdan destekleyene geri geçirir; node capacity ve her kenarın kapasitesine bakarak Supported/Overloaded/Unsupported/Inactive üretir. Bu oyun içi deterministik yaklaşık bir yük modelidir; gerçek malzeme sonlu elemanlar analizi (FEA) değildir.

### Önemli model sınırlaması

Destek yükü bir düğümün aktif supporting edge'lerine eşit bölünür. Kiriş eğilmesi, burkulma, moment, yatay yük, malzeme anizotropisi veya gerçek fizik çözümü bu modelde çözülmez. Unreal/Chaos fizik çöküşü ancak collapseCandidate sonuçlarını kullanan ayrı bir adapter tarafından tetiklenmeli; Assess tek başına Actor'ları hareket ettirmez.

## 6. Satır analizi: Unreal SpringTrap düzeltmesi

### Source/MyProject2/StickPhaseS6_Traps/SpringTrap.cpp — 76 satır

- **1–7:** Trap actor, primitive/static mesh, Character ve physics constraint bağımlılıkları.
- **9–26 — constructor:** Tick kapatılır. Önceden BaseMesh oluşturulmadan RootComponent üzerinde child attachment kuruluyordu; artık BaseMesh oluşturulup gerçek root yapılır. SpringArm ve SpringConstraint bu root'a bağlanır; mekanizma component'i yaratılır.
- **28–36 — SetSpringTension:** NaN/sonsuz tension sessizce reddedilir; geçerli değer 0–1 aralığına sıkıştırılır.
- **38–56 — hedef ön kontrolü:** Victim, mechanism, tension ve launch force doğrulanır. Karakter için LaunchCharacter, fizik simüle eden primitive için AddImpulse yolunun kullanılabileceği saptanır. Desteklenmeyen hedef varsa trap state'i tüketilmez ve damage uygulanmaz.
- **58–63 — trigger gate:** Önce Mechanism->Trigger(Victim) kabul etmelidir. Böylece aynı fonksiyon tekrar çağrılıp durduğu yerde birden çok kez fırlatmakla cooldown/armed state'i aşamaz.
- **65–75 — launch:** Tension ile ölçeklenen yukarı yönlü hız/impulse uygulanır. Bu fizik davranışının kuvvet/açı/timing'i gerçek mesh ve CharacterMovement ile PIE'de ayarlanmalıdır.
- **76:** Fonksiyon kapanışı.

**UE entegrasyon sınırı:** Bu doğrudan Unreal C++ kaynağında yapılan dar bir düzeltmedir; UBT veya PIE ile doğrulanmadı. Portable wm::TrapLifecycle ile UTrapMechanismComponent henüz birbirine bağlanmış değildir.

## 7. Build / test kaynak haritası

### Modules/CMakeLists.txt

- **16–20:** gameplay_quality.cpp, mevcut local_stone_ring_recognition.cpp ve yeni stone_fireplace_lifecycle.cpp, trap_lifecycle.cpp kütüphaneye eklenir.
- **21–36:** C++17 ve ilgili public include yolları. İki yeni header mevcut GameplayIntegration/include yolundan görünür.
- **44–49:** Test seçeneği açıksa test executable ve CTest kaydı burada tanımlıdır. Bu, testlerin çalıştığını değil yalnız derleme/çalıştırma için bağlandığını gösterir.

### Modules/tests/module_tests.cpp

- **1–28:** Ortak modül başlıkları; 21–22 şömine/tuzak header'ları. CHECK hata halinde test grubu false döndürür.
- **240–320 — TestSnapAndExcavation:** 252–271 yeni snap gate kaynakları; candidate budget/grid/kazı kontrolleri devam eder.
- **366–463 — TestStructuralIntegrity:** Yük/collapse; snapshot roundtrip, bozuk/dangling edge ve cycle reddi kaynakları. Geçersiz restore'un hedef graph state/revision'ını değiştirmemesi amaçlanır.
- **465–999 — diğer mevcut test grupları:** Çevre simülasyonu, raid scoring/spawn, chunk scheduler, material rule cache, plant/animal, crafting ve restore transaction testleri bu aralıkta kalır.
- **1001–1060 — TestStoneFireplaceLifecycle:** Ring, fuel, ignition/pot/heat; 995–1011 aralığı save/restore, güvenli NotBuilt geçişi, geometri yeniden doğrulanınca yangının yeniden başlaması ve bozuk yakıt kaydını reddetme kaynaklarıdır.
- **1062–1118 — TestTrapLifecycle / ring recognition başlangıcı:** Tuzak kaynak/tekrarlı event/cooldown; 1048–1068 save/restore sırasında relative cooldown ve event history; 1076–1081 consumed tuzak kalıcılık kaynakları. Halka tanıma testi 1120'de başlar.
- **1120–1151 — TestLocalStoneRingRecognition:** Geometri kabul/reddetme durumları.
- **1155–1185 — test table/runner:** 19 test group kayıtlıdır; bu kaynak sayısı PASS sayısı değildir.

## 7A. Ek dilim — save/restore sözleşmesi ve structural graph snapshot

### Şömine save record — yeni kaynak aralıkları
- Header satırları **45–51**: schema v1, saved revision, fuel, pot ve yükleme öncesi ateş yanıyor muydu bilgisi.
- Public API satırları **70–71**: CreateSaveRecord ve RestoreFromSaveRecord.
- Private pending intent satırı **87**: yükleme öncesi ateş yanıyorsa, live taş geometrisi tekrar doğrulanana kadar yeniden yakma kararı tutulur.
- C++ satırları **184–201 — CreateSaveRecord**: yalnız internal state tutarlıyken kayıt üretir.
- C++ satırları **202–234 — RestoreFromSaveRecord**: schema, revision/fuel sınırı ve revizyon taşmasını önce kontrol eder; bozuk kayıt state'i değiştirmez. Yakıt/kap geri yüklenir fakat saved ring geometry live world kanıtı sayılmaz. State NotBuilt ve heat 0 olur; ring source revision sıfırlanır. Save öncesi yanıyorsa pending resume intent saklanır. Live halka tekrar tanınırsa ve yakıt varsa yangın devam edebilir. Bu binary codec değil, taşınabilir restore contract'ıdır.

### Tuzak save record — yeni kaynak aralıkları
- Header satırları **31–54**: runtime snapshot, hasTriggered, schema v1 save record ve cooldown'ı kalan süre olarak saklama.
- Header satırları **86–88**: CreateSaveRecord/RestoreFromSaveRecord.
- Private state **101–104**: hasTriggered_ boolean, restore edilen son trigger zamanı negatif bir clock-relative değer olsa bile “hiç tetiklenmedi” sentinel'ıyla karıştırılmasını engeller.
- C++ **149–181 — CreateSaveRecord**: mutlak dünya saati yerine secondsSinceLastTrigger ve remainingCooldownSeconds kaydeder.
- C++ **182–245 — RestoreFromSaveRecord**: schema, trap identity, state/config tutarlılığı, event history ve clock değerlerini kontrol eder; yeni world clock'ta cooldown end'i yeniden üretir. Bozuk kayıt commit edilmez; başarılı restore revision'ı artırılır.
- Gerçek disk byte'larına çevirmek, save-slot sürüm geçişi ve UE SaveGame aktör kimliği eşlemesi hâlâ adapter/codec işidir.

### Structural graph snapshot — yeni kaynak aralıkları
- Header structural_integrity.hpp **37–42**: versioned graph snapshot (schemaVersion, saved graph revision, düğüm ve kenar listeleri); **58–61** snapshot create/restore public API.
- structural_integrity.cpp **208–229 — CreateSnapshot**: limitler uygulanır; unordered map içerikleri kararlı ID sırasıyla kayıt listesine çevrilir.
- structural_integrity.cpp **231–269 — RestoreSnapshot**: limit/schema/revision kontrolü; önce private candidate graph'a düğümler/kenarlar eklenir; dangling reference/duplicate/invalid node-edge ve aktif graph cycle kontrolü yapılır. Tüm kontroller bitmeden live map'ler değiştirilmez. Başarılı restore yeni revision'a geçer. Bu typed snapshot'tır; henüz byte codec veya UE SaveGame binding değildir.
- Bu yaklaşım bozuk snapshot'taki kenarın mevcut live graph'ı kısmen değiştirmesini önlemek için tasarlandı.

## 8. Claude Code için entegrasyon sırası

1. Bu raporu oku ve önce ilgili satır aralığını hedefle. Kaynağı baştan/tekrar taramak yerine aşağıdaki arayüzü temel al.
2. **Stone ring → Actor:** WorldScan/yakın alan sorgusundan LocalStoneSample oluştur; yalnız taş sınıfı ve geçerli ID'leri dahil et. Local query/samples maxStoneSamples ile sınırlandırılmalı. Kaynak revision'ı world/placement revizyonuyla ilişkilendir.
3. **Fireplace transaction:** Önce ReassessRing, sonra yakıt eklemede inventory cost staging + AddFuel; ancak kabul edilen miktar kadar item tüket. Ignite kararından sonra light/particle aç; Advance sonucunu fuel actor/inventory/heat provider'a uygula. Actor ve inventory güncellemeleri başarısız olursa rollback tanımla. Şu an bu adapter yazılmadı.
4. **Trap authority:** Ya portable TrapLifecycle UTrapMechanismComponent için tek kural otoritesi olsun ya da UE component aynen kalsın. İkisini paralel state authority yapma. UE side effect yalnız başarılı trigger sonrasında; event ID idempotency anahtarıyla uygulanmalı.
5. **Snap commit:** Scan provider gerçek targetCompatible, supportValid, socketOccupied verilerini üretsin. Preview ve commit aynı SnapCandidate kimliğini ve revision'ları kullansın; commit öncesi socket/collision/region/support yeniden sorgulansın. Ardından spawn, material debit ve graph edge create tek transaction olarak bağlansın.
6. **Support graph:** Actor için stable ID; connection için stable edge ID kullan. Piece kaldırıldığında RemoveEdge/RemoveNode; temporary disconnect için SetEdgeActive(false); yük ve zemin değişimlerinde SetNodeDemand/SetNodeGrounded. Assess sonucunu Unreal collapse/physics kararına adapter çevirsin.
7. Kaynaklar yeniden değiştiğinde bu rapordaki line number'lar kayabilir. Kod satır numarası güncellenmediyse bu rapor eski kabul edilir; sonraki değişiklikte raporu aynı commit/sonraki commit içinde güncelle.
8. Bu dilimde test çalıştırma talimatına uyuldu: test kodu yazıldı ama test/build çalıştırılmadı. Sonraki açık test aşamasında önce portable CMake/CTest, sonra Unreal 5.8 UBT, Automation ve PIE kanıtlarını ayrı ayrı raporla.

## 9. Açık iş / tamamlandı diye raporlanmaması gerekenler

- [x] Portable ring geometry recognizer mevcut.
- [x] Portable stone fireplace lifecycle state machine yazıldı.
- [x] Portable trap lifecycle state machine yazıldı.
- [x] Snap resolver occupied socket, target compatibility ve support gate'i içeriyor.
- [x] StructuralGraph mutation API'leri revision'a bağlı.
- [x] SpringTrap actor root ve trigger-before-launch akışı düzeltildi.
- [x] Bu değişiklikler için test kaynakları eklendi.
- [ ] Portable build/CTest sonucu alınmadı.
- [ ] Unreal 5.8 UBT/UHT derleme sonucu alınmadı.
- [ ] PIE'de gerçek taş halkası tanıma/şömine kurma/ısı uygulaması bağlanmadı.
- [ ] Portable trap model UE trap component'ine bağlanmadı.
- [ ] StructuralGraph UE UStructuralIntegritySubsystem için tek otorite haline getirilmedi.
- [x] Şömine ve tuzak için schema-versioned portable save record ve doğrulanan restore state geçişleri var.
- [x] StructuralGraph için versioned snapshot oluşturma ve candidate graph üzerinde doğrulayıp restore etme akışı var.
- [ ] Bu kayıtlar için binary byte codec, Unreal SaveGame serialization, crash recovery ve actor/inventory rollback kanıtı yok.
- [ ] Performance profili yok; performans iyileşti iddiası yapılmamalı.

Bu ayrım önemlidir: kod dilimleri ve test kaynakları mevcuttur; test sonucu ve motor entegrasyonu kanıtı henüz mevcut değildir.
