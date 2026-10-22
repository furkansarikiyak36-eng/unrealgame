# Faz 6 — Tahkimat Desteği, Hasar ve Çöküş

**Dal:** feature/phase-06-shoring-support-damage-collapse  
**Taban:** feature/phase-05-shoring-pieces-planner  
**Hedef:** Unreal Engine 5.8 / C++  
**Doğrulama:** Kaynak/runner test adı eşleşmesi statik olarak kontrol edildi. UBT/UHT, Unreal Automation ve PIE bu ortamda çalıştırılmadı.

## Amaç

Faz 5'teki tahkimat parçalarına deterministik destek değerlendirmesi, hasar/tamir, başarısız elemanın değiştirilmesi ve destek kaybı sonrası çöküş davranışı eklemek. Voxel density hâlâ arazinin tek veri otoritesidir. Tahkimat ayrı aktörlerdir; voxel mesh'ine birleştirilmez.

## Eklenen dosya ve API'ler

| Dosya | Sorumluluk |
|---|---|
| Shoring/ShoringSupportResolver.h/.cpp | Saf C++ destek grafiği değerlendirmesi, kapasite/yük oranı, döngü ve destek kaybı kontrolü |
| Shoring/ShoringTypes.h | Hasar nedeni, destek koşulları, soyut load demand ve assessment sözleşmeleri |
| Shoring/PlaceableShoringPiece.h/.cpp | Health/durability, hasar, tamir ve çöküş state'i |
| Shoring/ExcavationShoringSubsystem.h/.cpp | Plan değerlendirmesi, hasar/tamir/değiştirme API'leri ve voxel invalidation köprüsü |
| Tests/ShoringLayoutPlannerTests.cpp | Kapasite eşikleri, destek kaybı zinciri, cycle rejection, hasar/tamir/değişim testleri |
| Scripts/run_phase6_tests.sh | Faz 1–6 dünya Automation testleri ve UE 5.8 build runner |

## Destek sözleşmesi

- Her parça benzersiz GUID ile tanımlanır. Primary socket ankrajı ilgili parça GUID'sine bağlıdır; bağlantı listesinin iki yönde aynı ilişkiyi göstermesi gerekir.
- Her destek zinciri bir Ground veya güncel VoxelSurface köküne ulaşmalıdır. Birbirine bağlı görünse bile yalnızca kendi kendini taşıyan socket döngüsü geçerli destek değildir.
- Density revision'a bağlı eski voxel ankrajı güncel destek olarak kabul edilmez.
- Girdi verileri GUID, anchor türü, bağlantı listesi, health oranı, planlayıcının soyut design-load değeri ve güncel yüzey durumunu içerir.
- Sonuç parça başına Stable, Warning, Overloaded, Unsupported, Failed veya NeedsRevalidation koşullarından birini verir.
- Support resolver UObjects'a bağlı değildir; aynı girdi aynı sonuç üretir. İşlem bütçesi 1–4096 parça ile sınırlıdır.

### Oyun içi kapasite modeli

Parça tipine göre nominal kapasite sabitleri kullanılır. Etkin kapasite, durability oranı ve bağlantı desteğiyle çarpılır. Yük / etkin kapasite oranı:

- 0,75 altında: Stable
- 0,75–1,00: Warning
- 1,00 ve üstü: Overloaded

Bu değerler denge/test amaçlı soyut oyun birimleridir. Gerçek ahşap dayanımı, zemin mekaniği, tahkimat tasarımı veya gerçek dünyada kazı güvenliği hesabı değildir. Faz 6’da kendiliğinden çalışan sürekli yük/hasar tick'i eklenmedi; durum değerlendirmesi hasar, tamir, plan doğrulama veya açık çağrı sırasında yapılır.

## Hasar, tamir ve değiştirme

- ApplyDamageToPiece, yalnızca aktif/hasarlı/uyarı/aşırı yük koşulundaki parçaya pozitif ve sonlu hasar uygular.
- Health %70 ve altına düştüğünde parça Damaged olur. Health sıfıra indiğinde Failed olur ve collision kapatılır.
- Failed bir parçayı tamir etmek yerine ReplaceFailedPiece kullanılır. Yeni parça yeni GUID alır; komşuların bağlantıları ve primary anchor referansları yeni GUID'ye güncellenir.
- RepairPiece, Failed olmayan Damaged parçanın health değerini geri getirir. Tamir, desteğin geometrik veya revision geçerliliğini atlamaz.
- Bağlı parçanın primary destek ankrajı Failed veya Unsupported elemana gidiyorsa bağımlı parça Unsupported olur. Bu durumda root collision fizik simülasyonu ve gravity ile çöküş hareketine geçer.
- Çökmüş elemanın çöküş flag'i kalıcıdır. Yalnızca yüzeyi tekrar doğrulamak çöken elemanı kendiliğinden yeniden sabitlemez; açık MovePiece / yeniden ankraj işlemi gerekir.
- Voxel değişimi yalnızca ilgili planları NeedsRevalidation'a alır. Yeniden doğrulama, scan ile voxel yüzey ankrajını yeniden sorgular ve destek grafiğini tekrar değerlendirir.

## StructureLink sınırı

Mevcut StructureLink kodu ACarvableLog / WoodStructure grafiğine özeldir. Tahkimat actor'larını sahte kütük düğümlerine dönüştürmedim. Tahkimat için GUID tabanlı ayrı ve açık bir destek grafiği ile IStructureSupportAdapter sözleşmesi kullanılır; eski kütük sistemiyle tam birleşik tek grafik Faz 6'nın bu diliminde yapılmadı. Bu sınır, sonraki entegrasyon adımında açıkça korunmalıdır.

## Test

Yeni eklenen testler:
- MyProject2.World.Shoring.SupportLoadThresholdsAreDeterministic
- MyProject2.World.Shoring.SupportLossPropagatesAndCyclesFailClosed
- MyProject2.World.Shoring.DamageRepairAndFailedSupportReplacement

Runner, Faz 1–5'in 20 testini de koruyarak toplam 23 test adı kontrol eder. Kaynaklardaki 23 ad ile runner'ın 23 adı statik olarak eşleşiyor; testlerin çalıştığı sonucu çıkarılamaz.

Windows Git Bash/MSYS ve Unreal Engine 5.8 kurulu ortamda, Unreal Editor kapalıyken:

~~~bash
bash Scripts/run_phase6_tests.sh
# Gerekirse:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase6_tests.sh
~~~

## Faz 6 kapanış kapıları

1. UE 5.8 Development Editor UBT/UHT build başarılı olmalı.
2. 23/23 dünya Automation testi gerçek çalışmada başarılı olmalı.
3. PIE içinde hasar → tamir → ölümcül hasar → bağlı desteğin düşmesi → başarısız elemanın GUID ile değiştirilmesi doğrulanmalı.
4. Çöken parçanın sırf revision güncellendi diye otomatik sabitlenmediği; yeniden yerleştirme/ankraj gerektirdiği kontrol edilmeli.
5. Eksik socket ve desteksiz cycle testleri gerçek build'de yeşil olmalı.
6. 100+ parçada support değerlendirmesi ve cascade maliyeti Unreal Insights ile ölçülmeli.

Faz 7; kazı/tahkimat akışına stamina, ıslanma/sıcaklık, ışık ve su maruziyeti entegrasyonudur.