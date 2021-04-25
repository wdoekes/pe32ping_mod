#ifndef INCLUDED_PINGMON_UTIL_H
#define INCLUDED_PINGMON_UTIL_H

#include <Arduino.h>

/**
 * Return 111.222.33.44 if that's my external IP.
 */
String pingmon_util_http_whatsmyip(const String& whatsmyip_url);

#endif //INCLUDED_PINGMON_UTIL_H
