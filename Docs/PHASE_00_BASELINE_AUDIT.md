# Faz 0 — Baseline Denetimi ve Kapatma Kontrolü

**Proje:** MyProject2 / Unreal Engine 5.8  
**İnceleme tarihi:** 22 Ekim 2026  
**Baseline:** `main` — `8acc90dce4a495b44b0b098006b9e75f9c8e77ff`  
**Çalışma dalı:** `feature/phase-01-world-contracts-lowmid`  
**Durum:** Repository ve statik inceleme yapıldı; gerçek UE 5.8 clean build kapısı bu ortamda çalıştırılamadı.

> **Durum özeti:** Faz 0'ın kaynak denetimi ve test altyapısı inceleme adımı tamamlandı. Fazı kesin olarak “PASS/tamamlandı” kapatmak için gereken temiz UE 5.8 derlemesi ve eski testlerin yeniden çalıştırılması hâlâ bekliyor. Bu iki durumu birbirine karıştırmıyoruz.

## 1. Adım adım ne yapıldı?

### Adım 1 — Baseline commit ve proje ayarları sabitlendi

- Ana dalın incelenen baseline SHA'sı kaydedildi: `8acc90dce4a495b44b0b098006b9e75f9c8e77ff`.
- `MyProject2.uproject` kontrol edildi: `EngineAssociation = 5.8`, bir Runtime `MyProject2` modülü ve proje eklenti listesi mevcut.
- `Source/MyProject2/MyProject2.Build.cs` kontrol edildi: `bUseUnity = false`; Core/Engine/Input, CableComponent, GeometryScriptingCore/GeometryFramework, PhysicsCore, AIModule, GameplayTasks, NavigationSystem, RenderCore ve RHI bağımlılıkları tanımlı.
- .uproject JSON içeriği kaynak üzerinden okunabildi; bu **UBT/UHT derleme testi yerine geçmez**.

### Adım 2 — Canonical sahipler belirlendi

| Alan | Canonical kaynak / bulgu | Faz kararı |
|---|---|---|
| Ağaç/kütük oynanışı | `AHarvestableActor`, `ACarvableLog`, `AForestCharacter` | Ana dalın fiziksel kütük akışını koru; alternatif `ALogActor` sistemini otomatik birleştirme. |
| Ahşap yapı desteği | `StructureLink` ve `WoodStructureLogic` | Voxel tahkimatı aynı destek sahibiyle adapter üzerinden entegre et; ikinci destek otoritesi kurma. |
| Çevresel chunk state'i | `UChunkState`, `WorldCore` W1–W6 | Su/toprak/kimya/biyokütle state'ini 3B voxel density ile eş tutma; aradaki dönüşümü ayrı adaptör yönetsin. |
| Sparse 3B kazı data core | `UExcavationVolumeSubsystem`, `FExcavationVolumeSnapshot`, `IExcavationBaseDensitySource` yalnızca `plan/phase-00-baseline` dalında | Mevcut aday çekirdeği sonraki fazda denetleyip seçici biçimde getir; ikinci bir density sahibi oluşturma. Bu API ana dalda yok. |
| World Scanner / Analyzer | PR #4–#5 taslakları | Kodu ana dala alınmış varsayma; ortak sözleşmeyle uyum ve UE doğrulaması gerekli. |
| Kütük Snap | PR #3 taslağı | Mevcut özel kütük placement kurallarını koruyup generic provider arkasına adapte et. |
| Generic WorldSnap | PR #6 taslağı | Yalnızca verilen adaylar arasından seçim yapar; trace, placement onayı veya yapısal desteği sahiplenmez. |
| Survival | `USurvivalComponent` ve genişletilmiş dal modülleri | Mevcut statları koru; kazı/yeraltı etkilerini ayrı adapter'larla ekle. |
| Persistence | `WorldPersistence` / ilgili subsystem'ler | Dünya hücre kaydının fiziksel aktör, anchor/bağlantı ve voxel mesh/collision durumunu sakladığını varsayma; E2E testle kanıtla. |
| Landscape Builder | Ana dalda canonical bir implementasyon doğrulanmadı | Yeni sınıflandırma haritası/Builder olarak oluştur; Scanner ile birleştirme. |
| Smooth voxel mesh/collision | Ana dalda uçtan uca runtime pipeline kanıtlanmadı | Density, mesher, collision update ve yayınlama ayrı sorumluluklar olarak tasarlanmalı. |
| Voxel kazı tahkimatı | Ana dalda voxel yüzeyine bağlanan tahkimat akışı kanıtlanmadı | Dikey dikme/kaplama/payanda aktörleri placement ve StructureLink ile entegre edilmeli. |

### Adım 3 — Önceki test kanıtları kontrol edildi

Ana daldaki `Docs/TEST_DURUMU.md` son çalıştırmaları şöyle kaydediyor:

| Test | Son kayıtlı sonuç | Zaman | Kanıt sınırı |
|---|---|---|---|
| Grafiksiz mekanik demo | 101 geçti, 0 kaldı | 14 Ekim 2026, 05:29 | Tarihli doküman kaydı; bu oturumda yeniden çalıştırılmadı. |
| `MyProject2.Wood` | 12 geçti, 0 kaldı | 14 Ekim 2026, 05:26 | Tarihli doküman kaydı; bu oturumda yeniden çalıştırılmadı. |

- Ana dal ağacında `Saved/TestRuns` mevcut değil.
- `.gitignore` `Saved/` klasörünü dışlıyor. Bu nedenle ham `Saved/TestRuns/*.log` dosyaları Git içinden bağımsız biçimde doğrulanamıyor; mevcut kanıt, `Docs/TEST_DURUMU.md` içindeki özet kayıt.
- Bu testler Scan/Snap/voxel/tahkimat gibi sonradan hazırlanmış taslakların test edildiği anlamına gelmez.

### Adım 4 — CI/Actions ve PR sınırları kontrol edildi

- Baseline commit için GitHub combined status içinde status kaydı bulunmadı.
- Baseline commit'e bağlı PR workflow run bulunmadı.
- Ana dal tree'sinde `.github/workflows/` bulunmuyor.
- PR #1 (sparse excavation core), PR #3–#6 (snap/scanner), PR #7 (tasarım) ve PR #8 (Faz 0/1 çalışma dalı) açık; bunların hiçbiri ana dala birleştirilmiş değil.
- Bu sebeple açık PR dosyaları baseline kodu gibi sayılamaz. Özellikle sparse 3B kazı çekirdeği `main` içinde değildir.

### Adım 5 — Test runner güvenilirliği incelendi ve iyileştirildi

Ana daldaki eski `Scripts/run_tests.sh` içinde şu riskler vardı:

1. Yalnızca `set -u` kullanılıyordu; `set -e` ve `pipefail` yoktu.
2. `Build.bat | grep` pipeline'ı Build.bat'in exit code'unu doğru yansıtmayabilirdi; build başarısızken sonraki adımlar devam edebilirdi.
3. Unreal test süreçlerinin exit code'u açık şekilde doğrulanmıyordu.
4. Sonuç log'u yoksa veya sonuç satırı üretilemediyse eski/eksik test kaydı başarılı sayılabilecek şekilde raporlanabilirdi.
5. Engine kök yolu sabitti ve hata teşhisi için build/test process log'u yeterince görünür değildi.

Bu çalışma dalındaki `Scripts/run_tests.sh` şu şekilde güncellendi:

- `set -Eeuo pipefail` ve süreç exit-code kontrolleri.
- `UE_ENGINE_ROOT` ile değiştirilebilir UE 5.8 yolu.
- Build sonucu başarısızsa son log satırlarını gösterip durma.
- Eski Engine test loglarını silip yalnızca yeni koşunun log'unu kabul etme.
- Wood sonucu için test/failure kaydı kontrolü.
- Mekanik testi için beklenen `SONUÇ: N geçti, M kaldı` satırını zorunlu kılma; başarısız veya eksik sonuçta nonzero exit.
- Başarılı koşudan sonra logları `Saved/TestRuns` altına saklama ve doküman üreticisini çalıştırma.

Güncellenen script içeriğine Linux shell ortamında `bash -n` uygulandı ve **sözdizimi kontrolü PASS** verdi. Ancak script **Windows Git Bash + UE 5.8 üzerinde çalıştırılmadı**; bu yalnızca shell syntax kontrolüdür, gerçek build/test sonucu değildir. Mevcut yerel test ortamı olmadan runner düzeltmesini “doğrulanmış” ilan etmiyoruz.

### Adım 6 — Bu oturumun gerçek ortamı kontrol edildi

- Bu çalışma ortamında `UnrealEditor-Cmd` ve `UnrealBuildTool` komutları bulunmuyor; bilinen UE kurulum dizinleri de yok.
- `git clone https://github.com/...` denemesi DNS/host erişimi nedeniyle başarısız oldu.
- Dolayısıyla buradan .uproject'i açmak, UHT/UBT derlemek, Unreal Automation Test çalıştırmak veya Editor/PIE görmek mümkün olmadı.
- GitHub API kaynak okuması ve dosya karşılaştırması yapılabildi; bu statik inceleme, UE çalıştırma ortamının yerine geçmez.

## 2. Bulunan kritik entegrasyon riskleri

1. **Kodun varlığı akışın varlığı değildir.** Bağımsız sınıf/modül, oyuncu eyleminden yetkili world-state değişimine kadar çalışan bir akış anlamına gelmez.
2. **Kütük modelinin çoğalması:** `ACarvableLog` ile alternatif `ALogActor` aynı canonical nesne sayılmamalı.
3. **Kazı data core vs render/collision:** Sparse density delta'sı görünür smooth mesh veya güncel collision değildir.
4. **Scanner vs placement:** Scanner hedefi ve yüzeyi bulur; Snap aday seçer; Placement çakışma/commit doğrular; Structure sistemi desteği yönetir.
5. **Dünya çevresel hücreleri vs 3B voxel:** 2B chunk içindeki su/nem/biyokütle alanları gerçek kazılmış 3B hacmin yerini tutmaz.
6. **Persistence kapsamı:** Dünya hücresi kaydıyla fiziksel kütük, taş, tahkimat, anchor ve bağlantı kaydı aynı şey değildir.
7. **Testlerin tazeliği:** 14 Ekim test özeti, 22 Ekim'de eklenen yeni kodların build/test başarısını kanıtlamaz.
8. **Profil ayarları ile gameplay:** LOW_MID/HIGH_END çözünürlük ayarı değişebilir; kaydedilmiş mantıksal world state ve placement kuralları profilden bağımsız kalmalı.

## 3. Faz 0 kontrol tablosu

| Kontrol | Sonuç | Kanıt |
|---|---|---|
| Baseline commit ve engine sürümü tanımlandı | **PASS — statik** | Main SHA ve `.uproject` EngineAssociation 5.8 |
| Build.cs ve bağımlılıklar incelendi | **PASS — statik** | Ana dal `MyProject2.Build.cs` |
| Canonical sistem sahiplikleri belirlendi | **PASS — statik** | Kaynak başlıkları ve mevcut kararlar |
| Ana dal ile taslak PR ayrımı yapıldı | **PASS — statik** | PR metadata / branch comparisons |
| Önceki test kaydı bulundu | **PASS — tarihli kayıt** | `Docs/TEST_DURUMU.md` |
| Güncel temiz UE 5.8 build | **BLOCKED** | Bu ortamda Unreal Engine yok |
| Baseline mekanik + Wood testlerini yeniden çalıştırma | **BLOCKED** | UE yok; Saved test log'ları main'de yok |
| Test runner hata yayılımını düzeltme | **CODED, UNVERIFIED** | Güncellenen `Scripts/run_tests.sh` |
| Temiz build + test log'larını repo artifact'ına alma | **PENDING** | UE makinesinde çalıştırılmalı ve loglar/özet eklenmeli |

## 4. Faz 0 kapatma ölçütü

Faz 0'ın statik envanter kısmı tamamlandı. Faz 0 ancak aşağıdaki motor kapısı da geçildiğinde **tamamlandı** olarak işaretlenebilir:

1. Çalışma dalını UE 5.8 kurulu Windows Git Bash ortamına al.
2. Editör kapalıyken `bash Scripts/run_tests.sh` çalıştır.
3. Build başarısızsa yeni runner durmalı ve `Saved/TestRuns/build_*.log` teşhis için korunmalı.
4. Build başarılıysa yeni Wood ve mekanik test log'ları oluşmalı; script iki testte de failure veya eksik sonuçta nonzero çıkmalı.
5. Test raporunun yeni tarihli sonuçları `Docs/TEST_DURUMU.md` içinde görülmeli; loglar gerektiğinde ayrı artifact olarak saklanmalı.
6. Hiçbir yeni World/voxel sistemi ana dal baseline'ını sessizce değiştirmemeli; baseline testleri ayrı clean checkout'ta da mümkünse çalıştırılmalı.

## 5. Faz 1'e etkisi

Faz 1 profil/interface kodu çalışma dalında mevcut fakat Faz 0 clean build kapısı geçilmeden “derlenebilir ve hazır” kabul edilmemeli. Önce güncellenen test runner, sonra aynı UE 5.8 oturumunda Faz 1 build ve `MyProject2.World.RuntimeProfile.DefaultProfilesValidate` testi çalıştırılmalı.

## Referanslar

- Ana test kaydı: [Docs/TEST_DURUMU.md](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Docs/TEST_DURUMU.md)
- Ana proje raporu: [Docs/DURUM_RAPORU.md](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Docs/DURUM_RAPORU.md)
- Proje kararları: [Docs/KARARLAR.md](https://github.com/furkansarikiyak36-eng/unrealgame/blob/main/Docs/KARARLAR.md)
- Sparse excavation core taslak dalı: [Faz 1 kazı spike](https://github.com/furkansarikiyak36-eng/unrealgame/blob/plan/phase-00-baseline/Docs/FAZ_01_KAZI_TECHNOLOJI_SPIKE.md)
