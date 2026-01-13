#include "common.h"

#include <string.h>


static int log_level = LOG_LEVEL_INFO;


static char *get_log_level_str(log_level_t level) {
    switch (level) {
    case LOG_LEVEL_DEBUG:
        return "DEBUG";

    case LOG_LEVEL_INFO:
        return "INFO";

    case LOG_LEVEL_WARNING:
        return "WARNING";

    case LOG_LEVEL_ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}


static char *get_event_str(security_event_t event) {
    switch (event) {
    case SECURITY_EVENT_TLS_HS_SUCCESS:
        return "notice: TLS handshake successfully performed";

    case SECURITY_EVENT_TLS_DEPRECATED_VERSION:
        return "alarm: unsecure communication: deprecated TLS version proposed";

    case SECURITY_EVENT_TLS_DISALLOWED_VERSION:
        return "alarm: unsecure communication: disallowed TLS version proposed";

    case SECURITY_EVENT_TLS_VERSION_CHANGE:
        return "alarm: requested TLS Version change in ongoing communication "
               "detected";

    case SECURITY_EVENT_TLS_NO_PEER_CERT:
        return "alarm: peer certificate unavailable";

    case SECURITY_EVENT_TLS_NO_LOCAL_CERT:
        return "alarm: endpoint certificate unavailable";

    case SECURITY_EVENT_TLS_SESSION_CLOSED_REV:
        return "alarm: session terminated due to revoked certificate";

    case SECURITY_EVENT_TLS_DISALLOWED_CIPHER:
        return "alarm: disallowed TLSv1.2 cipher suite proposed";

    case SECURITY_EVENT_TLS_CERT_SIZE_MISMATCH:
        return "alarm: TLS certificate size exceeded";

    case SECURITY_EVENT_TLS_NO_CA_MATCH:
        return "alarm: certificate validation: CA certificate not available";

    case SECURITY_EVENT_TLS_NO_TRUSTED_CERT_MATCH:
        return "alarm: certificate validation: trusted individual certificate "
               "from authorized CA not available";

    case SECURITY_EVENT_TLS_CERT_REVOKED:
        return "alarm: certificate validation: revoked certificate";

    case SECURITY_EVENT_TLS_NO_CRL:
        return "warning: local CRL not accessible";

    case SECURITY_EVENT_TLS_CRL_EXP:
        return "warning: CRL expired";

    case SECURITY_EVENT_TLS_CERT_EXP:
        return "alarm: certificate validation: expired certificate";

    case SECURITY_EVENT_TLS_SIG_ALG_MISMATCH:
        return "alarm: certificate validation: signature algorithms not "
               "supported";

    case SECURITY_EVENT_TLS_SIG_V_FAILED:
        return "alarm: certificate validation: failed verification";

    case SECURITY_EVENT_TLS_SHORT_RSA_KEY:
        return "alarm: insufficient RSA key length (<2048 bit)";

    case SECURITY_EVENT_TLS_SHORT_KEY:
        return "alarm: insufficient key length";

    default:
        return "unknown";
    }
}


log_level_t get_log_level() { return log_level; }


void set_log_level(log_level_t level) { log_level = level; }


void log_log(log_level_t level, char *fmt, ...) {
    if (level < log_level)
        return;

    fprintf(stderr, "[%s] ", get_log_level_str(level));

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}


void log_event(security_event_t event, char *fmt, ...) {
    fprintf(stderr, "[EVENT] %s", get_event_str(event));

    if (fmt) {
        fprintf(stderr, ": ");

        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }

    fprintf(stderr, "\n");
}


int parse_log_level(char *level_str, log_level_t *level) {
    if (strcmp(level_str, "DEBUG") == 0) {
        *level = LOG_LEVEL_DEBUG;

    } else if (strcmp(level_str, "INFO") == 0) {
        *level = LOG_LEVEL_INFO;

    } else if (strcmp(level_str, "WARNING") == 0) {
        *level = LOG_LEVEL_WARNING;

    } else if (strcmp(level_str, "ERROR") == 0) {
        *level = LOG_LEVEL_ERROR;

    } else {
        return -1;
    }

    return 0;
}
