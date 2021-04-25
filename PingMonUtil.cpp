#include "PingMonUtil.h"

#include "crc8.h"

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
    static char cached_whatsmyip_url_crc;
    char whatsmyip_url_crc = crc8(whatsmyip_url.c_str());
    const long cache_time_ms = (15L * 60L * 1000L); // 15 minutes

    if (last_update_ms == 0 ||
          whatsmyip_url_crc != cached_whatsmyip_url_crc ||
          (millis() - last_update_ms) > cache_time_ms) {
#ifdef HAVE_HTTPCLIENT
        HTTPClient http;
        http.begin(whatsmyip_url);
        int httpCode = http.GET();
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
