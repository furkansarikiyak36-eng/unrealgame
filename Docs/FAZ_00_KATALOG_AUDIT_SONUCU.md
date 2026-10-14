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
| Şema/kimlik hatası | 0 |
| U0 — denetim gerekli | 485 |
| Kanıtlanmış U1–U6 satırı | 0 |
| Kanıtlanmamış/haritalanmamış satır | 485 |

Bu raporun ana sonucu “485 mekanik başarısız” değildir. Sonuç, **485 mekanik için henüz satır bazlı çağrı zinciri ve doğrulama kanıtı kaydedilmediği** anlamına gelir. Dizin/sınıf adı veya genel test raporu tek başına belirli katalog maddesini U3–U6 seviyesine yükseltmez.

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
