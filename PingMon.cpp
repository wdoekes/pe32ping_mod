#include "PingMon.h"

#if defined(ARDUINO_ARCH_ESP8266)
# define HAVE_GLOBALPINGER
#endif

#ifdef HAVE_GLOBALPINGER
# include <Pinger.h> /* library: ESP8266-ping */
static Pinger pinger;
#endif

#ifdef HAVE_GLOBALPINGER
/* BEWARE: Not safe for multiple simultaneous PingMon instances! */
static PingTarget* _onPingResponseTarget;
static bool _onPingResponse(const PingerResponse& response);
#endif

const PingStats PingTarget::getStats() const
{
    float loss = 100;
    unsigned sent = 0;
    unsigned lost = 0;
    unsigned responseTimeMs = 0;
    unsigned ttl = 0;
    for (unsigned i = 0; i < _history; ++i) {
        if (_responseTimeMs[i] >= 0) {
            sent++;
            responseTimeMs += _responseTimeMs[i];
            ttl += _ttls[i];
        } else if (_responseTimeMs[i] == -1) {
            sent++;
            lost++;
        }
    }
    if (sent > lost) {
        loss = (float)lost * 100.0 / (float)sent;
        responseTimeMs = responseTimeMs / (sent - lost);
        ttl = ttl / (sent - lost);
    }
    if (lost == sent) {
        responseTimeMs = 999; /* all is gone */
    }
    return PingStats(loss, responseTimeMs, ttl);
}

bool PingTarget::update()
{
    unsigned timePassed = (millis() - _lastResponseMs) / 1000;
    if (timePassed > 120) {
        /* go every 2 or so minutes */
    } else if (!_totalResponses) {
        /* if we haven't even started yet */
    } else if ((_totalResponses % 3) != 0 && timePassed >= 1) {
        /* we're in the middle of a burst of three and a second has passed */
    } else {
        return false; /* else, we wait */
    }

#ifdef HAVE_GLOBALPINGER
    /* Yuck. This is not the greatest code. But the Pinger class
     * leaves me little room to do this properly. */
    if (_onPingResponseTarget != NULL) {
        /* Yes, you got that right. Only one PingMon instance is allowed. */
        Serial.println("ERROR: Programming error!");
    }
    _onPingResponseTarget = this;
    pinger.OnReceive(_onPingResponse);
    if (pinger.Ping(getHost(), 1 /* requests */, 1000 /* ms timeout */)) {
        /* Make it synchronous... */
        while (_onPingResponseTarget) {
            delay(10);
        }
        Serial.print("DEBUG: Done pinging ");
        Serial.print(getId());
        Serial.print(", ");
        Serial.println(getHost());
    } else {
        Serial.print("ERROR: Something went wrong with ping to ");
        Serial.print(getId());
        Serial.print(", ");
        Serial.println(getHost());
    }
#endif
    _lastResponseMs = millis();
    _totalResponses += 1;
    return true;
}

#ifdef HAVE_GLOBALPINGER
bool _onPingResponse(const PingerResponse& response) {
    PingTarget& tgt = *_onPingResponseTarget;
    // extern "C" {
    // # include <lwip/icmp.h> /* needed for icmp packet definitions */
    // }
    // response.DestIPAddress.toString().c_str(),
    // response.EchoMessageSize - sizeof(struct icmp_echo_hdr),
    if (response.ReceivedResponse) {
        tgt.addResponse(response.ResponseTime, response.TimeToLive);
    } else {
        tgt.addResponseTimeout();
    }
    _onPingResponseTarget = NULL; /* done */
    return false; /* (don't continue, but we only scheduled one anyway) */
}
#endif

// vim: set ts=8 sw=4 sts=4 et ai:
