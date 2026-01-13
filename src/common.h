#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#include <jansson.h>
#include <openssl/ssl.h>
#include <uv.h>


#define SERVER_BACKLOG 32

#define BUFFER_COUNT 256
#define BUFFER_SIZE 1024

#define CHIPER_LIST_V1_2                                                         \
    "AES128-GCM-SHA256:"             /*TLS_RSA_WITH_AES_128_GCM_SHA256*/         \
    "DHE-RSA-AES128-GCM-SHA256:"     /*TLS_DHE_RSA_WITH_AES_128_GCM_SHA256*/     \
    "DHE-RSA-AES256-GCM-SHA384:"     /*TLS_DHE_RSA_WITH_AES_256_GCM_SHA384*/     \
    "ECDHE-RSA-AES128-GCM-SHA256:"   /*TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256*/   \
    "ECDHE-RSA-AES256-GCM-SHA384:"   /*TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384*/   \
    "ECDHE-ECDSA-AES128-GCM-SHA256:" /*TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256*/ \
    "ECDHE-ECDSA-AES256-GCM-SHA384" /*TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384*/

#define CHIPER_LIST_V1_3                                                       \
    "TLS_AES_128_GCM_SHA256:"                                                  \
    "TLS_AES_256_GCM_SHA384:"                                                  \
    "TLS_CHACHA20_POLY1305_SHA256:"                                            \
    "TLS_AES_128_CCM_SHA256:"                                                  \
    "TLS_AES_128_CCM_8_SHA256"

#define SESSION_ID "HAT-IEC62351"

#define log_debug(...) log_log(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(...) log_log(LOG_LEVEL_INFO, __VA_ARGS__)
#define log_warning(...) log_log(LOG_LEVEL_WARNING, __VA_ARGS__)
#define log_error(...) log_log(LOG_LEVEL_ERROR, __VA_ARGS__)

#define SECURITY_EVENT_ID(major, minor) (((major) << 8) | (minor))


typedef struct app_conf_t app_conf_t;
typedef struct mapping_conf_t mapping_conf_t;
typedef struct endpoint_conf_t endpoint_conf_t;
typedef struct ssl_conf_t ssl_conf_t;

typedef struct app_t app_t;
typedef struct mapping_t mapping_t;
typedef struct channel_t channel_t;
typedef struct endpoint_t endpoint_t;
typedef struct ssl_ctx_t ssl_ctx_t;
typedef struct ssl_t ssl_t;

typedef struct buffer_pool_t buffer_pool_t;
typedef struct buffer_t buffer_t;

typedef void (*channel_close_cb_t)(channel_t *channel);
typedef void (*endpoint_close_cb_t)(endpoint_t *endpoint);
typedef void (*ssl_close_cb_t)(ssl_t *ssl);


typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} log_level_t;

typedef enum {
    SECURITY_EVENT_TLS_HS_SUCCESS = SECURITY_EVENT_ID(1, 1),
    SECURITY_EVENT_TLS_DEPRECATED_VERSION = SECURITY_EVENT_ID(1, 2),
    SECURITY_EVENT_TLS_DISALLOWED_VERSION = SECURITY_EVENT_ID(1, 4),
    SECURITY_EVENT_TLS_VERSION_CHANGE = SECURITY_EVENT_ID(1, 5),
    SECURITY_EVENT_TLS_NO_PEER_CERT = SECURITY_EVENT_ID(1, 6),
    SECURITY_EVENT_TLS_NO_LOCAL_CERT = SECURITY_EVENT_ID(1, 7),
    SECURITY_EVENT_TLS_SESSION_CLOSED_REV = SECURITY_EVENT_ID(1, 8),
    SECURITY_EVENT_TLS_DISALLOWED_CIPHER = SECURITY_EVENT_ID(1, 9),
    SECURITY_EVENT_TLS_CERT_SIZE_MISMATCH = SECURITY_EVENT_ID(2, 1),
    SECURITY_EVENT_TLS_NO_CA_MATCH = SECURITY_EVENT_ID(2, 2),
    SECURITY_EVENT_TLS_NO_TRUSTED_CERT_MATCH = SECURITY_EVENT_ID(2, 3),
    SECURITY_EVENT_TLS_CERT_REVOKED = SECURITY_EVENT_ID(2, 4),
    SECURITY_EVENT_TLS_NO_CRL = SECURITY_EVENT_ID(2, 5),
    SECURITY_EVENT_TLS_CRL_EXP = SECURITY_EVENT_ID(2, 6),
    SECURITY_EVENT_TLS_CERT_EXP = SECURITY_EVENT_ID(2, 8),
    SECURITY_EVENT_TLS_SIG_ALG_MISMATCH = SECURITY_EVENT_ID(2, 9),
    SECURITY_EVENT_TLS_SIG_V_FAILED = SECURITY_EVENT_ID(2, 10),
    SECURITY_EVENT_TLS_SHORT_RSA_KEY = SECURITY_EVENT_ID(2, 11),
    SECURITY_EVENT_TLS_SHORT_KEY = SECURITY_EVENT_ID(2, 13),
    SECURITY_EVENT_TLS_DEP_HASH = SECURITY_EVENT_ID(2, 14)
} security_event_t;

typedef enum { ENDPOINT_TYPE_CLIENT, ENDPOINT_TYPE_SERVER } endpoint_type_t;


struct app_conf_t {
    json_t *json;
    mapping_conf_t *mappings;
};

struct mapping_conf_t {
    mapping_conf_t *next;
    endpoint_conf_t *server;
    endpoint_conf_t *client;
};

struct endpoint_conf_t {
    char *host;
    uint16_t port;
    ssl_conf_t *ssl;
};

struct ssl_conf_t {
    char *cert_path;
    char *key_path;
    char *ca_path;
    char *password;
    char *crl_path;
    uint64_t renegotiate_interval;
    uint64_t crl_interval;
    uint64_t session_cache_timeout;
    BIGNUM **allowed_cert_sn;
    size_t allowed_cert_sn_len;
};

struct app_t {
    app_conf_t *conf;
    uv_loop_t *loop;
    uv_signal_t *sigint_handle;
    uv_signal_t *sigterm_handle;
    mapping_t *mappings;
};

struct mapping_t {
    mapping_t *next;
    mapping_conf_t *conf;
    app_t *app;
    uv_tcp_t *server;
    channel_t *channels;
    ssl_ctx_t *server_ssl_ctx;
    ssl_ctx_t *client_ssl_ctx;
    SSL_SESSION *client_session;
};

struct channel_t {
    channel_t *next;
    mapping_t *mapping;
    channel_close_cb_t close_cb;
    buffer_pool_t *pool;
    endpoint_t *server;
    endpoint_t *client;
};

struct endpoint_t {
    endpoint_type_t type;
    channel_t *channel;
    endpoint_close_cb_t close_cb;
    endpoint_conf_t *conf;
    uv_tcp_t *handle;
    uv_getaddrinfo_t *getaddrinfo_req;
    ssl_t *ssl;
    uv_connect_t connect_req;
};

struct ssl_ctx_t {
    endpoint_type_t type;
    ssl_conf_t *conf;
    SSL_CTX *ctx;
};

struct ssl_t {
    endpoint_t *endpoint;
    ssl_close_cb_t close_cb;
    ssl_conf_t *conf;
    uv_timer_t *renegotiate_timer;
    uv_timer_t *crl_timer;
    BIO *input_bio;
    BIO *output_bio;
    SSL *ssl;
    X509_CRL *crl;
    buffer_t *output_plain_data;
    int version;
};

struct buffer_pool_t {
    channel_t *channel;
    buffer_t *buffers;
};

struct buffer_t {
    buffer_t *next;
    buffer_pool_t *pool;
    uv_write_t write_req;
    size_t offset;
    size_t len;
    char data[];
};


log_level_t get_log_level();
void set_log_level(log_level_t level);
void log_log(log_level_t level, char *fmt, ...);
void log_event(security_event_t event, char *fmt, ...);
int parse_log_level(char *level_str, log_level_t *level);

app_conf_t *create_app_conf(char *path);
void destroy_app_conf(app_conf_t *app);

app_t *create_app(app_conf_t *app_conf);
int run_app(app_t *app);
void destroy_app(app_t *app);

mapping_t *create_mapping(mapping_conf_t *conf, app_t *app);
void close_mapping(mapping_t *mapping);
void destroy_mapping(mapping_t *mapping);

channel_t *create_channel(mapping_t *mapping, channel_close_cb_t close_cb);
void close_channel(channel_t *channel);
void destroy_channel(channel_t *channel);
void start_channel(channel_t *channel);
void stop_channel(channel_t *channel);

endpoint_t *create_endpoint(endpoint_type_t type, channel_t *channel,
                            endpoint_close_cb_t close_cb);
void close_endpoint(endpoint_t *endpoint);
void destroy_endpoint(endpoint_t *endpoint);
void start_endpoint(endpoint_t *endpoint);
void stop_endpoint(endpoint_t *endpoint);
void write_endpoint(endpoint_t *endpoint, buffer_t *buffer, bool skip_ssl);
endpoint_t *get_other_endpoint(endpoint_t *endpoint);

ssl_t *create_ssl(endpoint_t *endpoint, ssl_close_cb_t close_cb);
void close_ssl(ssl_t *ssl);
void destroy_ssl(ssl_t *ssl);
void process_ssl(ssl_t *ssl);
void add_ssl_input_encrypted_data(ssl_t *ssl, buffer_t *buffer);
void add_ssl_output_plain_data(ssl_t *ssl, buffer_t *buffer);

ssl_ctx_t *create_ssl_ctx(endpoint_type_t type, ssl_conf_t *conf);
void destroy_ssl_ctx(ssl_ctx_t *ctx);

buffer_pool_t *create_buffer_pool(channel_t *channel);
void destroy_buffer_pool(buffer_pool_t *pool);
void push_buffer(buffer_t *buffer);
buffer_t *pop_buffer(buffer_pool_t *pool);

#endif
