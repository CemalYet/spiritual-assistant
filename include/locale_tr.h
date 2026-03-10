/**
 * @file locale_tr.h
 * @brief Turkish locale arrays — single source of truth.
 *
 * Used by display_ticker (date formatting) and lvgl_display (prayer date).
 * Header-only, constexpr, no object-file cost.
 */

#pragma once

namespace LocaleTR
{
    constexpr const char *MONTHS[] = {
        "Ocak", "Subat", "Mart", "Nisan", "Mayis", "Haziran",
        "Temmuz", "Agustos", "Eylul", "Ekim", "Kasim", "Aralik"};

    constexpr const char *DAYS[] = {
        "Pazar", "Pazartesi", "Sali", "Carsamba",
        "Persembe", "Cuma", "Cumartesi"};

    /// Turkish-aware UTF-8 uppercase converter.
    /// Returns pointer to internal static buffer — valid until next call.
    /// Double-buffered: safe for `func(toUpperTR(a), toUpperTR(b))`.
    inline const char *toUpperTR(const char *src)
    {
        static char bufs[2][80];
        static int idx = 0;
        char *buf = bufs[idx];
        idx = 1 - idx;
        int j = 0;
        for (int i = 0; src[i] && j < 77;)
        {
            uint8_t c = (uint8_t)src[i];
            if (c == 'i')
            { // Turkish i → İ (0xC4 0xB0)
                buf[j++] = (char)0xC4;
                buf[j++] = (char)0xB0;
                i++;
            }
            else if (c == 0xC4 && (uint8_t)src[i + 1] == 0xB1)
            { // ı → I
                buf[j++] = 'I';
                i += 2;
            }
            else if (c == 0xC5 && (uint8_t)src[i + 1] == 0x9F)
            { // ş → Ş
                buf[j++] = (char)0xC5;
                buf[j++] = (char)0x9E;
                i += 2;
            }
            else if (c == 0xC4 && (uint8_t)src[i + 1] == 0x9F)
            { // ğ → Ğ
                buf[j++] = (char)0xC4;
                buf[j++] = (char)0x9E;
                i += 2;
            }
            else if (c == 0xC3 && (uint8_t)src[i + 1] == 0xB6)
            { // ö → Ö
                buf[j++] = (char)0xC3;
                buf[j++] = (char)0x96;
                i += 2;
            }
            else if (c == 0xC3 && (uint8_t)src[i + 1] == 0xBC)
            { // ü → Ü
                buf[j++] = (char)0xC3;
                buf[j++] = (char)0x9C;
                i += 2;
            }
            else if (c == 0xC3 && (uint8_t)src[i + 1] == 0xA7)
            { // ç → Ç
                buf[j++] = (char)0xC3;
                buf[j++] = (char)0x87;
                i += 2;
            }
            else if (c >= 'a' && c <= 'z')
            { // ASCII lowercase
                buf[j++] = (char)(c - 32);
                i++;
            }
            else if (c >= 0xC0)
            { // Other multi-byte UTF-8 — copy as-is
                buf[j++] = (char)c;
                i++;
                while (src[i] && ((uint8_t)src[i] & 0xC0) == 0x80 && j < 78)
                    buf[j++] = src[i++];
            }
            else
            {
                buf[j++] = (char)c;
                i++;
            }
        }
        buf[j] = '\0';
        return buf;
    }

} // namespace LocaleTR
