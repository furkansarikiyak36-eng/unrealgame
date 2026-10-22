# Faz 4 — Scan-to-Dig ve Voxel Üzerine Yerleştirme

**Branch:** feature/phase-04-scan-to-dig-placement  
**Base:** feature/phase-03-lowmid-smooth-voxel  
**Engine hedefi:** Unreal Engine 5.8  
**Durum:** Uygulama taslağı ve statik kaynak kontrolleri yapıldı. Unreal Build Tool/Unreal Header Tool, Automation ve PIE henüz çalıştırılmadı; PR doğrulama için taslak kalmalı.

## 1. Amaç ve veri akışı

Faz 4, Faz 2 scanner ve classification verisini Faz 3'ün authoritative density owner'ına bağlar. Yeni sistem UWorldVoxelInteractionSubsystem adıyla tek bir köprü sağlar; ikinci bir voxel-data owner veya ayrı bir collision authority oluşturmaz.

~~~text
Oyuncu kamera ray'i / yerleştirme sorgusu
        ↓
UWorldScanSubsystem::Trace
        ↓
RegionKind + bCanDig / bCanSupport / bCanSnap
        ↓
UWorldVoxelInteractionSubsystem
        ├── IVoxelEditService → UExcavationVolumeSubsystem → density revision
        └── IPlacementSurfaceProvider → güncel voxel anchor
                                      ↓
                       bounded dirty chunk queue
                                      ↓
                           mesh + collision publish
~~~

## 2. Scan-to-dig

### Oyuncu akışı

- Oyuncu mevcut Pickaxe aracıyla voxel yüzeyine vurduğunda AForestCharacter::PerformHit, ortak interaction subsystem'ine yönlenir. Yeni ve çakışan bir input sistemi eklenmedi; odun, taş ve diğer mevcut hit yolları korunur.
- Interaction subsystem kamera ışınını UWorldScanSubsystem ile yeniden tarar. Geometry hit olması tek başına yetki sayılmaz.
- İzin yalnızca scanner sonucu RegionKind == EditableVoxel ve bCanDig == true olduğunda verilir. Faz 2 policy'sine göre bu, World.Capability.Dig gerektirir; World.Capability.NoDig, transition/water/static sınıfları kazıyı reddeder.
- Generated voxel chunk actor artık World.Object.VoxelSurface, World.Region.EditableVoxel ve World.Capability.Dig işaretlerine sahiptir. Bir classification map kurulmuşsa onun statik/transition kararı actor tag'lerinin üzerine çıkar; tag'ler coarse classification'ı geçersiz kılamaz.
- Başlangıç yarıçapı 45 cm olan sphere carve, yüzey noktasının biraz içine merkezlenir. Radius 1–200 cm aralığında doğrulanır; her istek profile MaxDensitySamplesPerEdit, volume MaxCellsPerEdit ve request MaxSamples sınırlarının tamamına uymalıdır.

### Stale collision koruması

Her AExcavationTerrainChunkActor, görünür mesh ve collision ile yayımlanmış density revision'ını tutar. Etki eden kazı yeni revision oluşturduktan sonra eski collider dünyada bir süre kalabilir; fakat eski collider:
- başka bir kazıyı yetkilendiremez;
- placement preview için snap/support yüzeyi olamaz;
- placement click/commit sırasında geçerli hedef kabul edilemez.

Revision güncel değilse sistem yeni işlemi reddeder, chunk rebuild kuyruğunun güncel mesh + collision yayımlamasını bekler. Böylece fiziksel collider ile authoritative density farklıyken ghost placement veya yeni voxel edit yetkilendirilmez.

## 3. Authoritative edit sözleşmesi ve rollback

UWorldVoxelInteractionSubsystem, Faz 1/2'de tanımlanan IVoxelEditService sözleşmesini uygular:

- ApplyEdit(FWorldVoxelEditRequest, FWorldVoxelEditResult)
- RollbackEdit(OperationId, FWorldVoxelEditResult)

Sözleşme şunları doğrular:
- işlem GUID'si benzersiz ve request geçerli;
- Carve için hedef density 0.0, Fill için 1.0;
- doğrudan çağrılan edit'in merkez noktası bilinen EditableVoxel classification içinde; yalnızca scanner'dan o an alınmış onay, map'in henüz tanımadığı ama tag policy ile yetkilendirilmiş interaktif hedef için fallback sağlar;
- shape bounds'undan hesaplanan konservatif hücre tahmini, request/profile/volume bütçelerini aşmaz.

Asıl edit yine yalnızca UExcavationVolumeSubsystem üzerinden gerçekleşir. Sonuç, request transaction ID'si, değiştirilmiş hücre sayısı ve etkilenen/neighbor chunk'ları içerir; volume delegate'i mevcut dirty chunk queue'yu tetikler. Son 512 in-memory edit için rollback makbuzu tutulur. Bu receipt listesi persistence değildir; daha eski makbuzlar veya world reload sonrası rollback bu fazda desteklenmez.

## 4. Voxel surface placement

UWorldVoxelInteractionSubsystem, IPlacementSurfaceProvider sözleşmesini de uygular ve üç çağrı sunar:

- GatherCandidates: hit noktasını scanner ile yeniden sorgular; yalnızca güncel, editable, stable ve snap/support izinli World.Object.VoxelSurface hedefi için doğrudan anchor döndürür.
- BuildPlacementPreview: anchor'a göre transform üretir, source revision'ı kaydeder ve yerel oriented-box overlap kontrolü yapar.
- ValidatePlacementCommit: aynı query'yi mevcut revision altında tekrar değerlendirir; hedef, transform veya revision önizlemeden değişmişse commit reddedilir. Kullanıcıya dönük placer, bu doğrulama geçmeden actor spawn etmemeli veya kaynak tüketmemelidir.

Mevcut AForestCharacter build sistemi hâlâ kendi 25 cm grid placement davranışını kullanır; voxel surface üzerine yerleştirme öncesinde ValidateVoxelSurfaceHit ile güncel surface/revision doğrular ve mevcut collision overlap testini korur. Bu, mevcut taş/kütük/door/window actor üretimini yeni genel snap API'sine zorla taşımadan güvenli bir ilk vertical slice sağlar.

Bir placement invalid/stale olduğunda PlaceSelected mevcut akışta transform/overlap doğrulaması başarısız olduğu için actor spawn ve envanter azaltma aşamasına geçmez. Voxel collision, yerleştirme footprint'inin yüzey içine girmesini denetlemek üzere overlap sorgusunda tutulur.

## 5. Değişen dosyalar

| Dosya | Değişiklik |
|---|---|
| WorldCore/WorldVoxelInteractionSubsystem.h/.cpp | Scanner→edit yetkilendirmesi, bütçeli atomic edit, geçici rollback receipts, voxel anchor candidate, preview/commit revision ve overlap validation |
| WorldCore/ExcavationTerrainChunkActor.h/.cpp | Mesh + collision'ın temsil ettiği revision'ı saklama; World.Object.VoxelSurface, World.Region.EditableVoxel, World.Capability.Dig marker'ları |
| ForestCharacter.cpp | Mevcut Pickaxe hit akışına scan-to-dig bağlanması; build preview ve commit yolunda stale voxel surface doğrulaması |
| Tests/WorldVoxelInteractionTests.cpp | Edit/rollback/dirty chunks, duplicate operation, unknown region/invalid fill ve stale placement testleri |
| Scripts/run_phase4_tests.sh | Faz 1–4 zorunlu world Automation testleri ve UE 5.8 build runner |

## 6. Yeni Automation testleri

- MyProject2.World.VoxelInteraction.EditRollbackAndDirtyChunks
- MyProject2.World.VoxelInteraction.UnknownAndInvalidEditsFailClosed
- MyProject2.World.VoxelInteraction.PlacementRejectsStaleRevision

Runner, ayrıca Faz 1, 2 ve 3'ün 11 mevcut test adını arar; toplam **14 test adı** beklenir.

Windows Git Bash/MSYS, Unreal Engine 5.8 kurulu ve Unreal Editor kapalıyken:

~~~bash
bash Scripts/run_phase4_tests.sh
# Gerekirse özel motor yolu:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase4_tests.sh
~~~

Bu ortamda runner çalıştırılmadı. Dolayısıyla kodun derlendiği veya testlerin geçtiği söylenemez.

## 7. Fazı kapatmadan önce motor içi kontrol

1. UBT/UHT build ve 14 Automation testinin tümü başarılı olmalı.
2. EditableVoxel + Dig capability tag'li test alanında Pickaxe vurduğunda density edit oluşmalı; static, unknown, transition, water ve NoDig hedeflerde değişiklik olmamalı.
3. Kazıdan sonra mesh/collision yayımlanmadan önce tekrar vurma ve placement denemesi güvenli biçimde reddedilmeli; publish sonrasında ray yeni surface hit'e ulaşmalı.
4. Bir chunk sınırında kazı yapıp komşu chunk'ların güncellendiğini ve yerleştirme overlap kontrolünün terrain'e nüfuz eden transform'u reddettiğini doğrula.
5. Başarılı yerleştirme öncesi envanter sayısını kaydet; stale/occupied/invalid hedeflerde actor oluşmadığını ve resource miktarının değişmediğini doğrula.
6. Üst üste iki edit yapıp ilk edit rollback'inin daha yeni sahipliği bozmadığını doğrula.
7. Map/world teardown sırasında dirty queue ve worker sonuçlarının yaşam döngüsünü incele.

## 8. Faz 4 sınırları

- Rollback receipt'leri 512 son in-memory edit ile sınırlı; kalıcı rollback history/save-load Faz 8 kapsamındadır.
- Yeni genel placement preview API'si hazır; mevcut yapı item'ları kendi grid/snap davranışını korur ve bu fazda tüm plank/log/stone sistemlerini genel snap candidate API'sine taşımıyor.
- Bu faz su, toprak deformasyonu, support/collapse ve tahkimat davranışını uygulamaz; sadece scan/edit ve voxel placement safety köprüsünü kurar.
- Unreal compile, Automation ve PIE henüz çalıştırılmadı. Bu nedenle Faz 4 implementasyon taslağı hazır olabilir, fakat motor-doğrulamalı kapanış yapılmış değildir.
