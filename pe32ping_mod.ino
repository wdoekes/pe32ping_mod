/**
 * pe32ping_example // ...
 */

/* In config.h, you should have:
const char wifi_ssid[] = "<ssid>";
const char wifi_password[] = "<password>";
const char mqtt_broker[] = "192.168.1.2";
const int  mqtt_port = 1883;
const char mqtt_topic[] = "some/topic";
const char pingmon_whatsmyip_url[] = "http://ifconfig.co";
const char pingmon_host0[] = "example.com";
*/
#include "config.h"

/* Include files specific to the platform (ESP8266, Arduino or TEST) */
#if defined(ARDUINO_ARCH_ESP8266)
# include <Arduino.h> /* Serial, pinMode, INPUT, OUTPUT, ... */
# include <SoftwareSerial.h>
# define HAVE_MQTT
# define HAVE_WIFI
#elif defined(ARDUINO_ARCH_AVR)
# include <Arduino.h> /* Serial, pinMode, INPUT, OUTPUT, ... */
# include <CustomSoftwareSerial.h>
# define SoftwareSerial CustomSoftwareSerial
# define SWSERIAL_7E1 CSERIAL_7E1
#elif defined(TEST_BUILD)
# include <Arduino.h>
# include <SoftwareSerial.h>
#else
# error Unsupported platform
#endif

/* Include files specific to Wifi/MQTT */
#ifdef HAVE_WIFI
# include <ESP8266WiFi.h>
# ifdef HAVE_MQTT
#  include <ArduinoMqttClient.h>
# endif
#endif

#include "PingMon.h"
#include "PingMonUtil.h"


static void ensure_wifi();
static void ensure_mqtt();

static void pingmon_init();
static void pingmon_update();
static void pingmon_publish();

#ifdef HAVE_WIFI
WiFiClient wifiClient;
#ifdef HAVE_MQTT
MqttClient mqttClient(wifiClient);
#endif //HAVE_MQTT
#endif //HAVE_WIFI

static PingMon pingmon;
const char guid[] = "577dcafe-d344-41d9-b633-f3f3dc900502";

static inline bool str_non_zero(const char *p) { return p && *p != '\0'; }


void setup()
{
  // Init Serial
  Serial.begin(115200);
  while (!Serial)
    delay(0);

  // Welcome message
  delay(200); // tiny sleep to avoid dupe log after double restart
  Serial.println(F("Booted pe32ping_example"));

  // Initial connect (if available)
  ensure_wifi();
  ensure_mqtt();

  pingmon_init();
}

void loop()
{
  pingmon_update();
}

static void ensure_wifi()
{
#ifdef HAVE_WIFI
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wifi_ssid, wifi_password);
    for (int i = 30; i >= 0; --i) {
      if (WiFi.status() == WL_CONNECTED) {
        break;
      }
      delay(1000);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Wifi UP on \"");
      Serial.print(wifi_ssid);
      Serial.print("\", Local IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.print("Wifi NOT UP on \"");
      Serial.print(wifi_ssid);
      Serial.println("\".");
    }
  }
#endif //HAVE_WIFI
}

static void ensure_mqtt()
{
#ifdef HAVE_MQTT
  mqttClient.poll();
  if (!mqttClient.connected()) {
    if (mqttClient.connect(mqtt_broker, mqtt_port)) {
      Serial.print("MQTT connected: ");
      Serial.println(mqtt_broker);
    } else {
      Serial.print("MQTT connection to ");
      Serial.print(mqtt_broker);
      Serial.print(" failed! Error code = ");
      Serial.println(mqttClient.connectError());
    }
  }
#endif
}

static void pingmon_init()
{
  /* Take Cloudflare DNS */
  pingmon.addTarget("dns.cfl", "1.1.1.1");
  /* Take Google DNS */
  pingmon.addTarget("dns.ggl", "8.8.8.8");
  /* Fetch external IP from whatsmyip/ifconfig.co service */
  pingmon.addTarget("ip.ext", []() {
    return pingmon_util_http_whatsmyip(pingmon_whatsmyip_url);
  });
  /* Fetch external Gateway, by replacing last octet of my IP with ".1" */
  pingmon.addTarget("gw.ext", []() {
    String ret = pingmon_util_http_whatsmyip(pingmon_whatsmyip_url);
    int pos = ret.lastIndexOf('.'); // take last '.'
    if (pos > 0) { ret.remove(pos + 1); ret += "1"; /* => x.x.x.1 */ }
    return ret;
  });
  pingmon.addTarget("gw.int", []() {
#ifdef HAVE_WIFI
    return WiFi.gatewayIP().toString();
#else
    return String("192.168.1.1");
#endif
  });
  if (str_non_zero(pingmon_host0)) {
    pingmon.addTarget("host.0", pingmon_host0);
  }
  pingmon.setPublish(pingmon_publish);
}

static void pingmon_update()
{
  pingmon.update();
}

static void pingmon_publish()
{
  ensure_wifi();
  ensure_mqtt();

#ifdef HAVE_MQTT
  // Use simple application/x-www-form-urlencoded format.
  mqttClient.beginMessage(mqtt_topic);
  mqttClient.print("device_id=");
  mqttClient.print(guid);
#endif

  for (unsigned i = 0; i < pingmon.getTargetCount(); ++i) {
    PingTarget& tgt = pingmon.getTarget(i);
    const PingStats& st = tgt.getStats();
    Serial.print(F("pushing ping: "));
    Serial.print(tgt.getId());
    Serial.print(F("="));
    Serial.print(st.responseTimeMs);
    if (st.loss) {
      Serial.print(F("/"));
      Serial.print((unsigned)st.loss);
    }
    Serial.println();
#ifdef HAVE_MQTT
    mqttClient.print("&ping.");
    mqttClient.print(tgt.getId());
    mqttClient.print("=");
    mqttClient.print(st.responseTimeMs);
    if (st.loss) {
      mqttClient.print("/");
      mqttClient.print((unsigned)st.loss);
    }
#endif
  }

#ifdef HAVE_MQTT
  mqttClient.endMessage();
#endif
}


#ifdef TEST_BUILD
int main()
{
  pingmon_init();
  pingmon_publish();
  return 0;
}
#endif //TEST_BUILD

/* vim: set ts=8 sw=2 sts=2 et ai: */
