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
