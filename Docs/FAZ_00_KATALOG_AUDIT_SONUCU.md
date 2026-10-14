# Faz 0 — Mekanik kataloğu ilk audit sonucu

**Durum:** ENGEL VAR — Faz 0 açık  
**Beklenen satır:** 485  
**Kaynak:** Çalışma dosyasındaki `MEKANIK_KATALOG_TAKIP_CSV.csv`  
**Denetleyici:** `Scripts/audit_mechanic_catalog.py`

## Sonuç

| Ölçüt | Sonuç |
|---|---:|
| Katalog satırı | 485 |
| Benzersiz ID | 485 |
| Yinelenen ID | 0 |
| Şema/kimlik/kanıt alanı hatası | 485 |
| U0 — denetim gerekli | 485 |
| Kanıtlanmış U1–U6 satırı | 0 |
| Kanıtlanmamış/haritalanmamış satır | 485 |

Bu raporun ana sonucu “485 mekanik başarısız” değildir. Sonuç, **485 mekanik için henüz satır bazlı çağrı zinciri ve doğrulama kanıtı kaydedilmediği** anlamına gelir. İlk sıkı denetimde 485 satırın tamamında `Bagimlilik_Bloker` ve `Acik_Notlar` alanları boştu; bu nedenle satır başına kanıt/meta veri boşluğu sayıldı. Dizin/sınıf adı veya genel test raporu tek başına belirli katalog maddesini U3–U6 seviyesine yükseltmez.

Denetleyici için 9 birim test izole Python ortamında geçti (kaynak-konum zorunluluğu, açıklanmamış blocker/not alanı, U0, bilinmeyen seviye, duplicate ID ve eksik şema dahil). Unreal Engine derlemesi veya GitHub Actions sonucu bu testin parçası değildir.

## Sonraki denetim düzeni

Her satır için sırayla:
1. Mekanik ID'nin sahibi olan sınıf/subsystem ve çağrı zinciri bulunur.
2. Sonucun otoriter dünya state'inde nerede değiştiği kaydedilir.
3. Olumlu, olumsuz ve sınır koşulu testleriyle eşleştirilir.
4. Editör/oyun içi görsel kanıtı ayrı işaretlenir.
5. Save/load, uzun oturum ve performans gereksinimleri U6 için ölçülür.
6. Bilinmeyenler tahminle tamamlanmaz; blocker olarak yazılır.

Katalog denetleyicisinin keşif ve sıkı modları:

```bash
python Scripts/audit_mechanic_catalog.py <katalog.csv> --expected-count 485 --json-out Saved/TestRuns/catalog_audit.json
python Scripts/audit_mechanic_catalog.py <katalog.csv> --expected-count 485 --strict
```

İlk komut keşif raporu üretir. İkinci komut kanıt boşluğu, eksik/tekrarlı ID veya beklenmeyen satır sayısı olduğunda sıfır olmayan çıkış vermelidir. Faz 1 ancak tüm katalog satırları gözden geçirilip denetim kapsamı yeterli kanıta bağlandığında başlar.
 

## Sonraki ilerleme — LOG alanının satır satır eşleştirilmesi

Repo dalına `Docs/FAZ_00_LOG_KATALOG_ESLESTIRME.csv` eklendi. Bu dosya LOG-001–LOG-076 kayıtlarının her biri için katalog ID'sini, kaynak yolu/adayı, satır özelindeki blocker'ı ve eksik test/editör kanıtını ayrı satırda tutar.

- LOG satırları: **76/76 kaynak denetim tablosuna eşlendi**.
- Kanıt seviyesi: **59 satır U2** (kaynakta yerel davranış izi var) ve **17 satır U0** (bu incelemede uygulanmış davranış zinciri bulunamadı ya da özellik ayrı geliştirme gerektiriyor). U2, otomatik test veya editör kabulü değildir; 59 U2 satırının da row-specific test/editör kanıtı bekliyor.
- Bazı ihtiyaçlar için kaynak yolu bulundu; bazıları “incelenen akışta bulunmadı” olarak açıkça işaretlendi (ör. zipline hattı, sal/su üstü kullanım, makine yapımı, genel savunma tuzağı). Bunlar tamamlandı olarak sayılmaz.
- CI, LOG (76), INVENTORY (32), RAID (63), WORLD (35) ve MATERIAL (38) eşleştirme dosyalarını keşif modunda denetler. Beklenen satır sayısı, zorunlu şema, ID kimliği ve açık blocker/not alanları kontrol edilir. Bilinen U0 satırları “geçti” diye gizlenmez.

Yeni eşleştirmelerle birlikte 5 katalog ailesinde **244/485 satır** satır bazlı haritaya alındı: LOG 76, INVENTORY 32, RAID 63, WORLD 35, MATERIAL 38. Geçici kaynak seviyesi dağılımı bu haritalarda U2=163, U1=45, U0=36; U4/U5/U6 olarak işaretlenen satır yok. Kalan **241 satır** STONE 96, STICK 76, FARMING 57 ve CORE 12 ailelerinde. 244 satırın da isimli test ve editör kanıtları ayrı ayrı kapatılmalıdır; bu nedenle Faz 0 çıkış kapısı kapalıdır.
