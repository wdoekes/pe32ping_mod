#include "PingMonUtil.h"

/* Previously, we used CRC8 instead of FNV-1a to compare previous URLs
 * (by value). But CRC is meant for guaranteed detection of at most N
 * bits, while a hash is meant for maximal distribution.
 * We expect many bits to be different, so a hash provides better
 * distribution / fewer collisions.
 * In short:
 * - checksum -> poor crc (LRC checksum is x8+x0 polynomial);
 * - crc -> guaranteed detection of rare bitflips (see Hamming Distance);
 * - hash -> uniform distribution. */
//#include "crc8.h"
#include "fnv1a.h"

/* Include files specific to the platform (ESP8266, Arduino or TEST) */
#if defined(ARDUINO_ARCH_ESP8266)
# define HAVE_HTTPCLIENT
# include <ESP8266HTTPClient.h>
#elif defined(ARDUINO_ARCH_AVR)
#elif defined(TEST_BUILD)
#else
# error Unsupported platform
#endif

/**
 * Return 111.222.33.44 if that's my external IP.
 */
String pingmon_util_http_whatsmyip(const String& whatsmyip_url)
{
    static String ret;
    static long last_update_ms = 0;
    static uint32_t cached_whatsmyip_url_crc;
    uint32_t whatsmyip_url_crc = fnv1a_32(whatsmyip_url.c_str()); // 0ms
    const long cache_time_ms = (15L * 60L * 1000L); // 15 minutes

    if (last_update_ms == 0 ||
          whatsmyip_url_crc != cached_whatsmyip_url_crc ||
          (millis() - last_update_ms) > cache_time_ms) {
#ifdef HAVE_HTTPCLIENT
        HTTPClient http;
        http.begin(whatsmyip_url);
        int httpCode = http.GET(); // 50ms
        if (httpCode >= 200 && httpCode < 300) {
            String body = http.getString();
            body.trim();
            if (body.length()) {
                ret = body;
            }
        }
        http.end();
#else
        ret = F("111.222.33.44");
#endif
        last_update_ms = millis();
        cached_whatsmyip_url_crc = whatsmyip_url_crc;
    }
    return ret;
}
