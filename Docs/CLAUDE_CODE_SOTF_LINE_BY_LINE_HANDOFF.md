# Sons of the Forest Mekanikleri — Claude Code Satır Analizi ve Devam Raporu

**Dal:** feature/sotf-mechanics-expansion  
**Motor hedefi:** Unreal Engine 5.8 / C++  
**Bu raporun amacı:** Claude Code sonraki oturumda önce bu dosyayı okuyup yapılan değişikliklerin sorumluluğunu, çağrı sırasını ve henüz bağlanmamış parçaları anlayabilsin. Bu rapor, kaynak dosyalarını tekrar baştan tarama ihtiyacını azaltan bir kod-konum ve davranış haritasıdır.  
**Test durumu:** Bu değişiklikler sırasında CMake, CTest, Unreal Build Tool (UBT), Automation veya PIE çalıştırılmadı. Test kaynakları eklendi/değiştirildi, başarı durumu doğrulanmış değildir.  
**Dal güvenliği:** Tüm commit'ler yalnızca feature/sotf-mechanics-expansion dalına yazıldı. main dalına birleştirme/değişiklik yapılmadı.

## 1. Bu dilimde değişen dosyalar

| Dosya | Değişiklik | Kod sınırı |
|---|---|---|
| Modules/GameplayIntegration/include/world_mechanics/stone_fireplace_lifecycle.hpp | Yeni | Motor bağımsız taş halka–şömine durum sözleşmesi |
| Modules/GameplayIntegration/src/stone_fireplace_lifecycle.cpp | Yeni | Halka, yakıt, ateş, ısı ve pişirme kabı geçişleri |
| Modules/GameplayIntegration/include/world_mechanics/trap_lifecycle.hpp | Yeni | Motor bağımsız tuzak state-machine sözleşmesi |
| Modules/GameplayIntegration/src/trap_lifecycle.cpp | Yeni | Armed/cooldown/consumed, event dedupe ve revision kontrolü |
| Modules/PlacementSnap/include/world_mechanics/placement_snap.hpp | Güncellendi | Doluluk, hedef uyumu, destek geçerliliği ve explicit socket filtresi |
| Modules/StructuralIntegrity/include/world_mechanics/structural_integrity.hpp | Güncellendi | Destek kenarı/düğüm mutasyon API'leri |
| Modules/StructuralIntegrity/src/structural_integrity.cpp | Güncellendi | Kenar kaldırma/kapama, yük/zemin/kapasite değişimini revision'a bağlama |
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
- Bu rapor commit'i ayrıca aynı dalda oluşturulur.

## 2. Satır analizi: şömine yaşam döngüsü

### stone_fireplace_lifecycle.hpp — toplam 81 satır

- **1–6:** Header koruması ve ortak sözleşmeler. core.hpp Result/ErrorCode/Revision/Seconds tiplerini, taş halka header'ı ise geometri tanıma veri tiplerini sağlar.
- **8–14:** wm namespace'i ve StoneFireplaceState. NotBuilt halka geçersiz/yok; Ready geçerli halka var ama ateş yanmıyor; Burning yalnız yakıt varken aktif yanma.
- **16–23:** StoneFireplaceConfig. Halkanın tanıma eşikleri dışarıdan ayarlanabilir; yakıt kapasitesi, yakıt tüketimi, nominal ısı, maksimum tek zaman adımı ve tek taramada kabul edilen taş örneği sayısı da sınırlandırılır.
- **25–34:** StoneFireplaceSnapshot. Save/UI/adaptör tarafına yayınlanabilir durum: runtime revision'ı, halka kaynak revision'ı, state, halka tanıma sonucu, pişirme kabı varlığı, yakıt/ısı ve geometrik değerlendirme. Bu bir save dosyası codec'i değildir.
- **36–43:** StoneFireplaceTick. Advance çağrısının sonuçlarını raporlar; tüketilen ve kalan yakıtı, ısıyı, yakıt bitişini ve işlemden sonraki revision'ı verir.
- **45–47:** Motor sınırı açıkça belirtilir. Bu sınıf Actor spawn etmez, trace atmaz, inventory tüketmez ve partikül/light yaratmaz. Bunlar Unreal adaptörünün işidir.
- **49–54:** ReassessRing: yerel taş örnekleri ve kaynak revision'ı alır.
- **56–61:** Yakıt ekleme, ateş yakma/söndürme, kap koyma ve zaman ilerletme API'leri. Durum değiştiren çağrılar expectedStateRevision alır; eski preview/UI isteği state'i yanlışlıkla değiştiremez.
- **63–64:** Salt-okunur snapshot ve revision sorgusu.
- **66–77:** Dahili doğrulama, ısı hesabı ve otoritatif state. stateRevision_ her kabul edilen durum değişikliğinde ilerler; ringSourceRevision_ geometri girdisinin eskiliğini engeller.

### stone_fireplace_lifecycle.cpp — toplam 201 satır

- **8–9 — constructor:** Yalnızca ayarları saklar; dünyada bir şey spawn etmez.
- **11–35 — ConfigValid:** Halkanın min/max taş sayısı ve yarıçapı, toleranslar, yakıt kapasitesi/tüketimi, ısı, zaman adımı ve örnek limiti doğrulanır. Eklenen kontrol, merkez boşluk yarıçapının minimum halka yarıçapından küçük olmasını zorunlu kılar; aksi durumda tanım geometrik olarak kendi kendisiyle çelişirdi.
- **37–68 — ReassessRing:**
  1. Ayar, merkez koordinatları ve nonzero kaynak revision'ı doğrulanır.
  2. Örnek sayısı maxStoneSamples sınırını aşarsa state'e dokunmadan BudgetExceeded döner.
  3. sourceRevision <= ringSourceRevision_ reddedilir; eski/aynı gözlem state'i geri saramaz.
  4. LocalStoneRingRecognizer::Assess geometrik sonucu üretir.
  5. Yalnız çağrı seviyesi doğrulamaları geçince değerlendirme ve source revision atomik olarak saklanır.
  6. Halka geçersizleşirse NotBuilt olur ve ısı kesilir. Halka geçerliyse NotBuilt → Ready geçişi yapılır. Mevcut yanan halka yeni ölçümde de geçerliyse Burning sürer.
  7. Runtime revision bir artırılır. Geometrik olarak geçersiz halka bir API hatası değildir; geçerli bir gözlem sonucudur ve recognized=false döner.
- **70–89 — AddFuel:** Pozitif/sonlu yakıt, geçerli config ve doğru state revision aranır. Tanınmış halka yoksa yakıt eklenemez. Kabul edilen miktar kalan kapasiteyle sınırlıdır. Hiç yer kalmadıysa hata döner; kabul edilmeyen miktar yakıt state'ine yazılmaz.
- **91–110 — Ignite:** Eski revision reddedilir; halka tanınmamışsa veya yakıt yoksa ateş yakılmaz. Zaten yanıyorsa Success(false) idempotent no-op'tur. Başarılı yakma state'i Burning yapar ve revision'ı artırır.
- **112–122 — Extinguish:** Eski revision reddedilir. Yanma yoksa no-op; yanma varsa geçerli halka için Ready, geçersiz halka için NotBuilt durumuna geçer. Kalan yakıt silinmez.
- **124–137 — PlaceCookingPot:** Sadece geçerli halkada ve ateş yanarken kap eklenebilir. Zaten kap varsa ikinci bir kap yaratma kararı üretmez. Kap state'i ateş sönünce otomatik silinmez; dünya adaptörü fiziksel objeyi ayrı yönetmelidir.
- **139–177 — Advance:** Negatif/NaN zaman adımı, config hatası, maksimum yakalama adımından büyük delta ve stale revision reddedilir. Yanma yoksa veya delta sıfırsa state değiştirmeyen snapshot sonuçlanır. Yanarken yakıt deltaSeconds * fuelUnitsPerSecond kadar azalır. Yakıt bittiğinde sıfıra sabitlenir ve state yanmıyor durumuna geçer. Yakıt/ısı hesaplaması deterministiktir; rastgele ışık titremesi bu saf mekanik modelde bulunmaz.
- **179–190 — Snapshot:** Otoritatif state'i kopyalayarak verir; çağıran taraf internal state'e doğrudan yazamaz.
- **192–198 — CurrentHeatOutput:** Ateş yanmıyorsa veya halka geçersizse ısı sıfırdır. Aksi durumda nominalHeatOutput × clamp(fuel/maxFuel, 0..1) hesaplanır.
- **200:** Namespace kapanışı.

### Şömine adaptöründe henüz yapılmayanlar

Portable state machine, mevcut AStoneFireplace Actor'una henüz bağlanmadı. StoneFireplace.cpp şu an kendi CurrentFuel, bIsLit, light/particle ve Tick tüketimini yönetiyor. İki state machine'i birden otorite olarak çalıştırmayın. Unreal uyarlamasında ya Actor, portable modelin komut/sonuçlarını kullanmalı ya da bir adaptör bu modelin kararlarını tek yerden uygulatmalı. Taş örnekleri bounded yerel query'den ve stabil ID ile sağlanmalı; bütün dünyadaki tüm Actor'ları her Tick taramak kabul edilmez. Inventory yakıtı yalnız AddFuel kabul ettikten sonra tüketilmeli; Actor spawn/etki işlemleri başarısız olduğunda rollback sözleşmesi tanımlanmalı.

## 3. Satır analizi: tuzak yaşam döngüsü

### trap_lifecycle.hpp — toplam 90 satır

- **1–5:** Ortak Result/revision/time sözleşmesi ve namespace.
- **7–12:** Kaynağa göre olay türü: basınç, tripwire, yakınlık, manuel.
- **14–19:** Yaşam döngüsü: Disarmed, Armed, CoolingDown, Consumed.
- **21–29:** Konfigürasyon. Her tuzağın kararlı ID'si; izin verilen trigger kaynağı; cooldown, sıfır cooldown için en düşük tekrar-arming aralığı, damage ve tek kullanımlık olma kuralı.
- **31–40:** Snapshot; son kabul edilen event/instigator kimliği, zaman, cooldown bitişi ve trigger sayısı.
- **42–48:** Trigger isteği. eventId tuzak için monoton artmalıdır; expectedStateRevision gözlemin hazırlandığı state'i temsil eder.
- **50–60:** Trigger kararı. Adapter'ın uygulayacağı damage ve event ID'si, sonuç state'i/revision'ı ve cooldown bitişini taşır.
- **62–64:** Sınır açıklaması: damage, ses, partikül ve fizik etkisi bu sınıfta yapılmaz.
- **66–74:** Arm, Disarm, TryTrigger, Advance, Snapshot public API.
- **76–87:** Özel doğrulama ve state alanları.

### trap_lifecycle.cpp — toplam 161 satır

- **8–9 — constructor:** Konfigürasyonu alır; dünya işlemi yapmaz.
- **11–26 — ConfigValid:** Tuzağın ID'si, enum kaynağı, cooldown aralığı, minimum tekrar aralığı, damage ve maksimum cooldown sonlu/sınır içinde olmalıdır.
- **28–47 — Arm:** Config ve revision kontrolü; Consumed tuzak tekrar kurulamaz, zaten Armed ise no-op, CoolingDown iken erken kurma reddedilir. Yalnız Disarmed → Armed gerçek state değişimi revision artırır.
- **49–65 — Disarm:** Config/revision doğrular. Consumed değiştirilemez; zaten kapalıysa no-op. Diğer durumlar Disarmed olur. Bu çağrı cooldown timer'ını iptal etmek isteyen Unreal adaptörü için authoritative karar noktasıdır.
- **67–123 — TryTrigger:**
  1. Config, event ID ve zaman doğrulanır.
  2. Beklenen revision mevcut değilse StaleRevision.
  3. Event ID, son kabul edilmiş event ID'den büyük değilse tekrar/sıra dışı olay Conflict ile reddedilir.
  4. Trigger kaynağı uyuşmuyorsa Unsupported; tuzak Armed değilse Conflict.
  5. Zamanın geri gitmediği kontrol edilir.
  6. Gerçek cooldown, yapılandırılan cooldown ile minimum rearm süresinin maksimumudur; böylece 0 cooldown olsa dahi aynı anda yinelenen overlap callback'leri hemen tekrar tetikleyemez.
  7. Event ID, tetikleme zamanı ve instigator tek kabul işlemi olarak kaydedilir; trigger sayısı ve state revision artırılır.
  8. Tek kullanımlı tuzak Consumed, diğer tuzak CoolingDown olur.
  9. Dönen karar, adaptörün damage uygulayıp uygulamaması gerektiğini bildirir. instigatorId == 0 ise çevresel/kurbanı olmayan event damage uygulamaz.
- **128–145 — Advance:** Zaman/config/revision kontrolü. Cooldown bitmediyse no-op; bitmişse CoolingDown → Armed ve revision artışı.
- **147–159 — Snapshot:** Kalıcılık/diagnostics katmanına okunur state sunar. Snapshot tek başına disk serialization değildir.

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

- **1–28:** Ortak modül başlıkları; 21–22 yeni şömine/tuzak header'ları. CHECK hata halinde ilgili test grubunu false döndürür.
- **240–320 — TestSnapAndExcavation:** 252–271 yeni kontrol kaynakları: preview sonrası dolu socket, hedef uyumsuzluğu, destek kaybı; candidate-level occupancy/compatibility/support reddi; explicit socket zorunluluğu. 272 sonrası mevcut candidate bütçesi, eşik, plank grid ve excavate testleri sürer.
- **366–428 — TestStructuralIntegrity:** Mevcut yük ve collapse testleri; 394–428 ek bölüm yük update'inin revision artırdığını, destek kenarı silmenin alt düğümü unsupported yaptığını, nonexistent edge'i reddettiğini, edge disable/enable ve grounded/capacity güncellemelerini kontrol eder.
- **966–1007 — TestStoneFireplaceLifecycle:** Geçerli 8 taşlı ring; erken ignition reddi; stale ring revision; yakıt ekleme; stale ignition; ateş ve kap; 1.5 s içinde 3 yakıt birimi tüketme/135 heat; halkanın açılmasıyla ateş/ısı işlevinin kapanması fakat yakıt ve kap state'inin korunması.
- **1009–1037 — TestTrapLifecycle:** Yanlış kaynak reddi; tek event'in kabulü; aynı event ID replay reddi; cooldown öncesi/sonrası; yeniden tetik; consumed tuzağın rearm edilememesi.
- **1039–1070 — TestLocalStoneRingRecognition:** Önceden eklenmiş halka geometrisi kabul/reddetme durumları.
- **1074–1104 — test table/runner:** Test grupları bu tabloda kayıtlıdır. Bu dilimde kaynak kapsamı 17'den 19 gruba çıktı. Bu yalnızca kayıt sayısıdır; PASS sayısı değildir.

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
- [ ] Save/load serialization, crash recovery ve gerçek resource/actor rollback kanıtı yok.
- [ ] Performance profili yok; performans iyileşti iddiası yapılmamalı.

Bu ayrım önemlidir: kod dilimleri ve test kaynakları mevcuttur; test sonucu ve motor entegrasyonu kanıtı henüz mevcut değildir.
