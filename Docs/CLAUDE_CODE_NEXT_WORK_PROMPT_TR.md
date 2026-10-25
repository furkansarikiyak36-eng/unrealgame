# Claude Code — Sonraki Geliştirme Görevi (Kopyala / Yapıştır)

**Repo:** furkansarikiyak36-eng/unrealgame  
**Dal:** feature/sotf-mechanics-expansion

Bu repoda yalnızca plan veya analiz raporu üretme. Mevcut kodu hedefli biçimde incele, en yüksek öncelikli eksik kabul koşulunu seç ve gerçek kod değişikliği yap.

## Sabit kurallar

- Yalnızca feature/sotf-mechanics-expansion dalında çalış. main dalına yazma veya merge yapma.
- Kullanıcının mevcut talimatı: test/build çalıştırma. CMake/CTest, UBT/UHT, Unreal Automation ve PIE çalıştırma. Test kaynağı ekleyebilirsin ama çalıştırma.
- Unreal Engine 5.8 ve C++ hedefini koru.
- Mevcut işlevleri sırf farklı tasarlamak için yeniden yazma.
- Yalnızca plan bırakma: seçilen tek dikey dilimin kodunu uygula ve commit et.
- Var olmayan entegrasyonu tamamlanmış gibi anlatma. Portable C++ modüllerle Unreal kodu ayrıysa bu sınırı koru ve raporla.
- Her kod değişikliği için ilgili handoff/manifest satır aralıklarını güncelle.

## Önce okuyacağın belgeler — tamamını değil, ilgili bölümlerini oku

1. Docs/CLAUDE_CODE_START_HERE.md — çalışma yöntemi ve sahiplik sınırları.
2. Docs/CLAUDE_CODE_SOTF_CHANGE_MANIFEST.md — hedef mekanik için dosyaların listesi.
3. Docs/CLAUDE_CODE_SOTF_LINE_BY_LINE_HANDOFF.md — yalnızca hedef mekanik bölümündeki işleyiş, fonksiyonlar ve açık entegrasyonlar.
4. Docs/SOTF_MECHANICS_IMPLEMENTATION_BACKLOG.md — Workstream A–H içinden ilgili kabul koşulu.
5. Docs/SOTF_MECHANICS_EXPANSION_PLAN.md — yalnızca o mekanik ailesinin tasarım ve kabul maddeleri.

Ardından yalnızca hedef işin mevcut kaynak dosyalarını ve doğrudan çağırdığı fonksiyonları oku. ForestCharacter.cpp gibi büyük dosyaların tamamını yazdırma/yeniden inceleme. Belgelerdeki satır numarası değişmişse ilgili sembolü bulup güncel aralığı kullan.

## İlk geliştirme önceliği: gerçek UE yerleşim zincirindeki açıkları kapat

Daha yüksek öncelikli ve henüz tamamlanmamış bir Aşama A (WorldScan) kabul koşulu tespit edersen önce onu ele al; aksi halde yerleşim/snap entegrasyonundan devam et. Bu bir kod görevidir; yalnızca “eksikler şunlar” diye rapor yazıp bırakma.

### İlk bakılacak dosyalar

**Tasarım/kabul sözleşmesi**
- Modules/PlacementSnap/SPEC.md — adaydan commit'e kadar olan akış ve atomiklik sözleşmesi.
- Docs/SOTF_MECHANICS_IMPLEMENTATION_BACKLOG.md — özellikle Workstream A (satır 26–34) ve B (36–44).
- Docs/SOTF_MECHANICS_EXPANSION_PLAN.md — özellikle ortak zincir (satır 41–50), snap/tarama (63–69) ve iki uçlu yerleşim (71–78).

**UE gerçek çalışma yolu**
- Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTypes.h — aday ve sorgu veri sözleşmesi.
- Plugins/WorldSnap/Source/WorldSnap/Private/WorldSnapResolver.cpp — genel aday eleme/sıralama.
- Plugins/WorldSnap/Source/WorldSnap/Public/WorldSnapTwoAnchorSolver.h ve Private/WorldSnapTwoAnchorSolver.cpp — iki uçlu geometri ve anchor hazır olma kuralları.
- Source/MyProject2/ForestCharacter.cpp — yalnızca generic log snap için yaklaşık 1518–1545; destek preview için 1683 sonrası; zemin+direk iki uçlu yol için 1747–1814. Gerekirse giriş/commit fonksiyonlarını sembol aramasıyla bul.
- Source/MyProject2/StructureLink.h ve StructureLink.cpp — özellikle Features, Settle, WouldBeSupported ve doğrudan çağırdıkları fonksiyonlar.
- Source/MyProject2/Phase2_Transport/CarvableLogJackingComponent.h/.cpp — yalnızca destek/yükseltilen uç davranışı değişecekse.

**Portable referans**
- Modules/PlacementSnap/include/world_mechanics/placement_snap.hpp
- Modules/PlacementSnap/include/world_mechanics/world_snap_two_anchor_solver.hpp
- Modules/PlacementSnap/src/world_snap_two_anchor_solver.cpp
- Modules/tests/module_tests.cpp — yalnızca ilgili test fonksiyonunun kaynak aralığı; çalıştırma.

Bu liste bir okuma zorunluluğudur ama tüm dosyaları her görevde okuma: yalnızca gerçekten değişecek sözleşme ve doğrudan çağrı zincirini yükle.

### Kodda kapatılacak kabul koşulları

1. **Sağlayıcı/adayı bulma:** Raycast yalnızca ilk gözlem/hedef seçimidir. Yakındaki uygun snap/anchor noktaları sınırlı ve deterministik sorguyla gelmeli; her karede global Actor taraması olmamalı.
2. **Sınıflandırma ve uyumluluk:** Adayın ID'si, türü, kullanılabilirliği, yuva doluluğu, desteği/yük kapasitesi ve ilgili revision kanıtı açıkça temsil edilmeli. Varsayılan true değerler gerçek world provider tarafından doldurulmadan doğrulama sayılmaz.
3. **Preview ve commit tutarlılığı:** Preview'da çözülen transform ve seçilen anchor'lar commit anında tekrar doğrulanmalı. Anchor hareketi/kaybı, dolu socket, stale revision veya collision değişmişse eski preview reddedilmeli.
4. **Tam çarpışma denetimi:** Sadece hedef noktasındaki trace değil; parça footprint/collision hacmi için gerçek overlap kontrolü yapılmalı.
5. **Atomik commit / rollback:** Yapı parçası spawn, kaynak/inventory debit ve support-graph connection birlikte başarılı olmalı. Bir adım başarısız olursa staged actor, kaynak ve graph ilişkisi geri alınmalı. Var olan transaction modeliyle uyumlu çalış.
6. **Tek durum sahibi:** UE gerçek runtime'ı ve portable referans modelini eşzamanlı ikinci otorite haline getirme.
7. **Bütçeler:** Aday ve pair sayısına üst sınır koy; limit aşımını sessizce geçerli sonuç gibi döndürme. Deterministik tie-break koru.

Bunların hepsini tek commit'e sığdırmaya çalışma. Mevcut kodda ilk eksik kalan kabul koşulunu bul, bir dikey dilim olarak uygula, sıradaki eksikleri handoff'a kaydet.

## Değişiklik yaparken

- Önce hedef sembolün güncel satırlarını ve çağrılarını dar aralıkta oku.
- Sadece gerekli dosyaları değiştir; ilgisiz formatlama/refactor yapma.
- Uygun başarı, ret, sınır, stale revision ve rollback senaryoları için test kaynağı ekle veya mevcut test kaynağını güncelle. ÇALIŞTIRMA.
- Derleme gerektiren Unreal header/UHT etkilerini raporda açıkça belirt; derlemenin başarılı olduğunu iddia etme.
- Handoff ve manifestte şu bilgileri güncelle: dosya/fonksiyon/satır aralığı, davranış, çağrı yolu, state authority, rollback ve revision kuralları, test kaynağı (çalıştırılmadı), kalan UE entegrasyonu.

## Son cevap formatı

1. **Uygulanan:** gerçek dosyalar + fonksiyonlar ve ne değişti.
2. **Kullanılan çağrı yolu:** hangi UE bileşeni bu kodu çağırıyor; portable ise bunu açıkça ayır.
3. **Sağlamlık:** stale revision, occupied socket, collision, rollback ve bütçenin hangi noktada ele alındığı.
4. **Belgeler:** handoff/manifestte güncellenen aralıklar.
5. **Commit:** SHA ve dal.
6. **Doğrulama:** “Test/build çalıştırılmadı (talimat gereği).” Test sonucu veya performans kazanımı uydurma.
7. **Kalan en yakın iş:** yalnızca sıradaki somut kabul koşulu.

Başla: belgelerin ilgili bölümlerini oku, mevcut kodda ilk tamamlanmamış kabul koşulunu tespit et ve raporla yetinmeden o dilimi uygula.
