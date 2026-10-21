# ADR-001 — Oyuncu envanteri için tek otorite ve geçiş sözleşmesi

- **Karar durumu:** Faz 0 baseline kararı — geçiş tasarımı onay kapısına tabi
- **Tarih:** 2026-10-14
- **İlgili bulgular:** P0-003, P0-004, P0-014, P0-015, P0-022, P0-027, P0-029
- **Geçiş fazı:** Faz 6 (taş/çubuk/envanter entegrasyonu); ön koşullar Faz 2 ve Faz 5

## 1. Gözlenen mevcut durum

1. AForestCharacter kendi TMap<EForestItem, int32> Inventory alanını ve 24-slot / 20-stack davranışını kullanıyor. GetItemCount, CanAddItem, AddItem, RemoveItem, build ve craft akışları bu kayda dayanıyor.
2. Taşımadaki büyük kütükler yalnızca genel item adedi değil, ACarvableLog aktör referansları (CarriedLogs) ve fiziksel durumla temsil ediliyor. Bu kimliği/state'i FInventoryItem içine dönüştürmek kayıplı olabilir.
3. Ayrı UInventoryComponent ise FName item ID, UInventoryItemData, ağırlık, slot/stack ve save/restore API'si sunuyor. InventoryUIComponent, CraftingUIComponent, QuickAccessComponent ve InventorySaveSubsystem bu yeni aileye bağlı.
4. İncelenen AForestCharacter constructor'ı yeni inventory/UI/quick-access bileşenlerini oluşturmuyor. Depodaki 33 .uasset içinde mesh/malzeme varlıkları bulunuyor; karakter envanterini bu bileşenlere bağlayan player/Inventory Blueprint bulunmadı.
5. Fizikselden genele köprü yalnızca ALogActor, AStoneActor ve AStickActor kabul ediyor; mevcut oynanışın otoriter ACarvableLog türünü dönüştürmüyor. Envanterden fiziksele köprü generic item ID'sine göre legacy actor spawn ediyor.

## 2. Faz 0 için karar

- Mevcut oynanabilir odun/yapı akışında AForestCharacter::Inventory ve fiziksel taşınan aktörler mevcut otoriter kaynak olarak kalır.
- Yeni UInventoryComponent familyası canlı karakter envanteriyle eşit/bağlıymış gibi gösterilmeyecek. Bir Blueprint veya kod adapter'ı kanıtlanmadan bileşenleri karaktere eklemek ve aynı eşya için iki stock tutmak yasak kabul edilir.
- UInventoryComponent içindeki kapasite preflight/rollback düzeltmesi yalnızca bu genel bileşenin kendi atomikliğini iyileştirir; mevcut oynanabilir envanteri birleştirmez ve ACarvableLog state kaybını çözmez.
- Ara çözüm olarak iki otoritenin her işlemde birbirine kopya yazmasına izin verilmez. Çift yazım resource duplication/loss riskidir.

## 3. Faz 6 geçiş kapısı

Birleştirme ancak aşağıdaki sözleşmeler kod, test ve UE 5.8 host loglarıyla sağlanınca yapılır:

1. **ID eşlemesi:** Her EForestItem için kararlı FName ItemID; bilinmeyen enum/ID dönüşümü fail-closed.
2. **Durum sınıfları:** Basit stack item ile fiziksel actor-item ayrılır. Hasarlı/kesilmiş/oyulmuş/bağlantılı ACarvableLog otomatik olarak genel item sayacına indirgenmez; transfer tasarımı ya aynı actor/state'i korur ya açıkça yasaklar.
3. **Tek otorite:** Pickup, craft, drop, build, quick-access ve save/load aynı item owner API'sini çağırır. İkinci TMap/slot listesinde gölge stok tutulmaz.
4. **İşlemsel transfer:** Spawn/attach/remove işlemleri tüm başarı, iptal, dolu kapasite, eksik class, geçersiz konum ve failed spawn koşullarında transaction/rollback ile yapılır.
5. **Kalıcılık:** Inventory item'ları, quick slots, fiziksel actor state'i ve dünya save sistemi versioned round-trip testini geçer; aynı aktör ikinci kez oluşturulmaz.
6. **Player acceptance:** UE 5.8 test haritasında taşınabilir kaynak pickup → inventory → craft/drop → save → world reload döngüsü; miktar bilançosu sıfır kayıp/sıfır çoğalma gösterir.

## 4. Şimdiki test kapsamı ve açık riskler

- Scripts/verify_test_logs.py ile loglar fail-closed doğrulanıyor; StickCraftTransaction (3), StonePlacement (5), StoneTrap (4) ve yeni Inventory.Core.Capacity (2) suite'leri için ayrı minimumlar var.
- Inventory kapasite testleri, MaxSlots gerçek slot dizisi boyutundan küçük kaldığında preflight ve mutasyon aralığının eşleşmesini hedefler.
- Henüz kanıtlanmayan: Unreal Engine 5.8 C++ build/Automation, gerçek player Blueprint bağlantısı, genel inventory'nin live AForestCharacter'a göçü ve tam pickup/drop/save/load entegrasyonu.

## 5. Faz 0 çıkış koşuluna etkisi

P0-022 ve ilgili kaynak transfer bulguları açık kalır. Bu ADR belirsizliği ve risk sınırını kayda geçirir; tek otoriteyi gerçekten uygulamış sayılmaz. Faz 6 iş kalemleri, Faz 2/5 ön koşulları ve UE 5.8 acceptance testleri tamamlanmadan bu bulgu kapatılmaz.
