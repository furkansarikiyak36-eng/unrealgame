# Faz 9 — Dikey Dilim Entegrasyonu ve Taş Aktörü Kalıcılığı

**Dal:** `feature/phase-09-vertical-slice-stone-persistence`  
**Taban:** `feature/phase-08-world-persistence`  
**Hedef:** Unreal Engine 5.8 / C++  
**Doğrulama:** Kaynak/runner test adları ve statik yapı kontrolü yapılır. UE 5.8 UBT/UHT, gerçek Automation ve PIE bu ortamda çalıştırılmaz.

## Kapsam

Yeni kayıt şeması `UWorldSaveGame : ULogSaveGame` olarak tanımlandı. Eski `ULogSaveGame` slotları okunabilir kalırken Faz 9 kaydı tek slota aşağıdaki durumları yazar:

- **Voxel:** `UExcavationVolumeSubsystem::ExportSnapshot` ile sparse density override'ları, grid hücre boyutu, chunk ölçüsü ve baseline density.
- **Tahkimat:** parça GUID/plan GUID, Blueprint aktör sınıfı, transform, parça türü, support anchor/socket bağlantıları, design load, health/max health, son hasar sebebi ve collapse latch.
- **Oyuncu:** transform ve respawn transform'u; mevcut envanter item/count dizileri; health, stamina, hunger, thirst ve starvation/exhaustion sayaçları; Faz 7 sıcaklık/ıslaklık/termal-karanlık-su maruziyet snapshot'ı.
- **Eski log aktörleri:** önceki `ULogSaveGame.Logs` dizisi aynı slotta korunur.
- **Taş aktörleri:** loose/installed/falling taşların Blueprint sınıfı, definition, transform, health, structural state, shape ve dimensions kaydedilir. Carried/dragging ve fracture/terminal durumlar kayıt sırasında reddedilir.

## Save işlemi

1. Voxel ve shoring alt sistemleri snapshot üretir.
2. Bütün oyuncu değerleri ve item slot limiti doğrulanır.
3. Fiziksel parça omuzda veya halatla sürükleniyorsa kayıt reddedilir; henüz bu carry state'i şemasına koyulmamış aktörleri sessizce kaybetmez.
4. `SaveGameToSlot` tek bir version-2 `UWorldSaveGame` yazar. Kayıt başarısızsa var olan slotun başarıyla kaydedildiği iddia edilmez.

## Load transaction

1. Slot ve schema/map kimliği doğrulanır.
2. Voxel hücreleri, shoring GUID/durability grafiği, oyuncu stat'ları ve envanter satırları **dünyaya uygulanmadan önce** preflight edilir.
3. Log replacement aktörleri geçici olarak oluşturulur; spawn başarısızsa mevcut loglar korunur.
4. Voxel state atomik snapshot import'uyla değiştirilir. Shoring aktörleri staged edilir ve tümü kurulmadan mevcut shoring kaldırılmaz.
5. Shoring import başarısız olursa eski voxel snapshot geri yüklenir. Oyuncu state restore aşaması başarısız olursa önceki oyuncu, shoring ve voxel snapshot'ları geri uygulanır.
6. Yalnızca başarılı commit sonunda mevcut log aktörleri silinir ve staged log aktörleri kalıcı kalır.
7. Yüklenen tahkimat parçaları `NeedsRevalidation` kalır. Voxel mesh/collision yayınlandıktan sonra Blueprint/C++ `UExcavationShoringSubsystem::RevalidateAllPlans` çağırmalıdır; geçici collision rebuild boşluğu varsa işlem daha sonra tekrar denenmelidir.

### Güvenlik: destekler load sonrası otomatik aktif sayılmaz

Voxel import density revision'ı değiştirir. Bu nedenle yüklenen tahkimatlar collision kapalı `NeedsRevalidation` durumunda başlar; GUID grafiği restore edilir fakat voxel yüzey ankrajları yeniden tarama/doğrulamadan geçmeden güvenilir destek haline gelmez. Daha önce çökmüş parçanın collapse latch'i snapshot'ta tutulur; sırf yeniden yüklendi diye otomatik dikilmez.

## Geriye uyumluluk ve açık sınırlar

- Eski `ULogSaveGame` slotu açılırsa yalnızca log aktörleri yüklenir; yeni voxel/shoring/player state'i uydurulmaz.
- Tam kayıt map adıyla ilişkilidir; farklı map üzerinde restore reddedilir.
- Sahnedeki log-only `ALogActor` aktörleri korunur. Aktif `ACarvableLog`/ağaç/taşıma sistemindeki her fiziksel aktör bu şemaya dahil edilmedi. Bu faz, mevcut eski log-save payload'ıyla smooth voxel, shoring ve player state'ini tek sürümlü slotta birleştirir; bu fiziksel aktör ailesi için ek world actor adapter'ları ayrıca gereklidir.
- Çevre sample'ı hâlâ Blueprint/volume sağlayıcısından gelir. Kaydedilen exposure durumunu yeniden yüklemek, çevre sağlayıcıyı otomatik oluşturmaz.
- Çok büyük override veya shoring kümeleri sabit bütçelerle reddedilir; kayıtta sessiz kırpma yapılmaz.

## Yeni Automation testleri

- `MyProject2.World.Persistence.StoneRecordsValidatePersistentStates`

- `MyProject2.World.Persistence.VoxelSnapshotRoundTrip`
- `MyProject2.World.Persistence.ShoringSnapshotRejectsDuplicateIds`
- `MyProject2.World.Persistence.PlayerSnapshotRejectsInvalidStats`
- `MyProject2.World.Persistence.VersionedSaveSchema`

Faz 9 runner önceki test adlarını korur ve taş kalıcılığı için bir test ekleyerek toplam 31 test bekler. Bu sayım kaynak/runner isim eşleşmesidir; gerçek UE Automation testlerinin geçtiğini ispatlamaz.

Windows Git Bash/MSYS + UE 5.8, Editor kapalıyken:
```bash
bash Scripts/run_phase9_tests.sh
# Gerekirse:
UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase9_tests.sh
```

## Kapanış kapıları

1. UE 5.8 Development Editor UBT/UHT build başarılı olmalı.
2. 31/31 Automation testi gerçek çalışmada başarılı olmalı.
3. PIE: kazı → tahkimat → hasar/çöküş → kaydet → restart/restore → revalidate zincirini görsel olarak doğrula.
4. Çakışma/failure enjekte edilerek staged log/shoring spawn ve rollback yolları doğrulanmalı.
5. Voxel chunk collision yeniden yayımlanmadan tahkimat parçalarının aktif collision almadığı doğrulanmalı.
6. Envanter, stats, respawn transform ve exposure timer'larının round-trip testi yapılmalı.
7. 10k / 100k voxel override ve 100+ tahkimat parçalı Save/Load süresi, dosya boyutu ve frame spike'ları Unreal Insights/profiling ile ölçülmeli.
