#include "common.h"

#include <string.h>

#include <openssl/err.h>


#define MSG_TYPE_CLIENT_HELLO 1
#define MSG_TYPE_SERVER_HELLO 2
#define MSG_TYPE_KEY_UPDATE 24


static void destroy_timer(uv_timer_t *timer);
static void renegotiate(ssl_t *ssl);
static int verify_cert(ssl_t *ssl, X509 *cert);
static int verify_crl(ssl_t *ssl);
static X509_CRL *load_crl(char *path);
static uint64_t get_next_crl_update_interval(ssl_t *ssl);
static void parse_hello_message(ssl_t *ssl, const uint8_t *data, size_t len);
static void parse_key_update_message(ssl_t *ssl, const uint8_t *data,
                                     size_t len);


static void on_timer_close(uv_handle_t *timer) {
    log_debug("on_timer_close");

    ssl_t *ssl = timer->data;

    if (ssl->renegotiate_timer == (uv_timer_t *)timer)
        ssl->renegotiate_timer = NULL;

    if (ssl->crl_update_timer == (uv_timer_t *)timer)
        ssl->crl_update_timer = NULL;

    destroy_timer((uv_timer_t *)timer);

    if (ssl->close_cb && !ssl->renegotiate_timer && !ssl->crl_update_timer)
        ssl->close_cb(ssl);
}


static void on_renegotiate_timer(uv_timer_t *timer) {
    log_debug("on_renegotiate_timer");

    ssl_t *ssl = timer->data;
    renegotiate(ssl);
}


static void on_crl_update_timer(uv_timer_t *timer) {
    log_debug("on_crl_update_timer");

    ssl_t *ssl = timer->data;

    X509_CRL *crl = load_crl(ssl->conf->remote_cert->crl->path);
    if (!crl)
        log_event(SECURITY_EVENT_TLS_NO_CRL, NULL);

    bool crl_match = ssl->crl && crl && X509_CRL_match(ssl->crl, crl) == 0;

    if (ssl->crl)
        X509_CRL_free(ssl->crl);

    ssl->crl = crl;

    if (!crl)
        return;

    if (uv_timer_start(ssl->crl_update_timer, on_crl_update_timer,
                       get_next_crl_update_interval(ssl) * 1000,
                       ssl->conf->remote_cert->crl->update_interval * 1000) <
        0) {
        log_error("uv_timer_start error");
        close_channel(ssl->endpoint->channel);
        return;
    }

    if (crl_match) {
        if (!verify_crl(ssl)) {
            log_error("verify_crl error");
            close_channel(ssl->endpoint->channel);
        }
        return;
    }

    renegotiate(ssl);
}


static int on_password(char *buf, int size, int rwflag, void *u) {
    strncpy(buf, (u ? (char *)u : ""), size);
    buf[size - 1] = '\0';
    return strlen(buf);
}


static int on_verify(int preverify_ok, X509_STORE_CTX *x509_ctx) {
    log_debug("on_verify");

    int err = X509_STORE_CTX_get_error(x509_ctx);
    int depth = X509_STORE_CTX_get_error_depth(x509_ctx);

    switch (err) {
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        log_event(SECURITY_EVENT_TLS_NO_CA_MATCH, NULL);
        return 0;

        // case X509_V_ERR_UNABLE_TO_GET_CRL:
        //     log_event(SECURITY_EVENT_TLS_NO_CRL, NULL);
        //     return 0;

    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        log_event(SECURITY_EVENT_TLS_SIG_V_FAILED, NULL);
        return 0;

    case X509_V_ERR_CERT_HAS_EXPIRED:
        log_event(SECURITY_EVENT_TLS_CERT_EXP, NULL);
        return 0;

        // case X509_V_ERR_CRL_HAS_EXPIRED:
        //     log_event(SECURITY_EVENT_TLS_CRL_EXP, NULL);
        //     return 0;

    case X509_V_ERR_CERT_REVOKED:
        log_event(SECURITY_EVENT_TLS_CERT_REVOKED, NULL);
        return 0;

    case X509_V_ERR_CA_MD_TOO_WEAK:
    case X509_V_ERR_UNSUPPORTED_SIGNATURE_ALGORITHM:
        log_event(SECURITY_EVENT_TLS_SIG_ALG_MISMATCH, NULL);
        return 0;
    }

    if (depth == 0) {
        ssl_t *ssl = SSL_get_app_data(X509_STORE_CTX_get_ex_data(
            x509_ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));

        X509 *cert = X509_STORE_CTX_get_current_cert(x509_ctx);
        if (!cert) {
            log_error("X509_STORE_CTX_get_current_cert error");
            return 0;
        }

        if (!verify_cert(ssl, cert))
            return 0;
    }

    if (!preverify_ok) {
        log_error("verify error:num=%d:%s:depth=%d\n", err,
                  X509_verify_cert_error_string(err), depth);
    }

    return preverify_ok;
}


static int on_client_session(SSL *s, SSL_SESSION *session) {
    log_debug("on_client_session");

    ssl_t *ssl = SSL_get_app_data(s);

    if (ssl->endpoint->channel->mapping->client_session)
        SSL_SESSION_free(ssl->endpoint->channel->mapping->client_session);

    ssl->endpoint->channel->mapping->client_session = SSL_SESSION_dup(session);

    return 0;
}


static void on_message(int write_p, int version, int content_type,
                       const void *buf, size_t len, SSL *a, void *arg) {
    ssl_t *ssl = arg;
    const uint8_t *data = buf;

    if (write_p)
        return;

    if (content_type == SSL3_RT_HANDSHAKE && len > 0) {
        switch (data[0]) {
        case MSG_TYPE_CLIENT_HELLO:
            if (ssl->endpoint->type == ENDPOINT_TYPE_SERVER)
                parse_hello_message(ssl, data, len);

            break;

        case MSG_TYPE_SERVER_HELLO:
            if (ssl->endpoint->type == ENDPOINT_TYPE_CLIENT)
                parse_hello_message(ssl, data, len);

            break;

        case MSG_TYPE_KEY_UPDATE:
            parse_key_update_message(ssl, data, len);
            break;
        }
    }
}


static void parse_hello_message(ssl_t *ssl, const uint8_t *data, size_t len) {
    size_t pos = 0;

    ssl->extensions = (ssl_extensions_t){.renegotiation_info = false,
                                         .signature_algorithms = false,
                                         .encrypt_then_mac = false};

    if (pos >= len)
        return;
    uint8_t msg_type = data[pos];
    pos += 1;

    pos += 3;      // skip length
    pos += 2;      // skip client/server version
    pos += 4 + 28; // skip random

    if (pos >= len)
        return;
    pos += 1 + data[pos]; // skip sessions id

    if (msg_type == MSG_TYPE_CLIENT_HELLO) {
        if (pos + 1 >= len)
            return;
        pos += 2 + (((uint16_t)data[pos] << 8) |
                    data[pos + 1]); // skip sipher suites

        if (pos >= len)
            return;
        pos += 1 + data[pos]; // skip compression methods

    } else {
        pos += 2; // skip cipher suite
        pos += 1; // skip compression method
    }

    pos += 2; // skip extension length

    while (true) {
        if (pos + 1 >= len)
            return;
        uint16_t ext_type = ((uint16_t)data[pos] << 8) | data[pos + 1];
        pos += 2;

        switch (ext_type) {
        case TLSEXT_TYPE_renegotiate:
            ssl->extensions.renegotiation_info = true;
            break;
        case TLSEXT_TYPE_signature_algorithms:
            ssl->extensions.signature_algorithms = true;
            break;
        case TLSEXT_TYPE_encrypt_then_mac:
            ssl->extensions.encrypt_then_mac = true;
            break;
        }

        if (pos + 1 >= len)
            return;
        pos += 2 + (((uint16_t)data[pos] << 8) |
                    data[pos + 1]); // skip extension data
    }
}


static void parse_key_update_message(ssl_t *ssl, const uint8_t *data,
                                     size_t len) {
    size_t pos = 0;

    pos += 1; // skip msg type
    pos += 3; // skip length

    if (pos >= len)
        return;
    uint8_t req = data[pos];
    pos += 1;

    if (req == SSL_KEY_UPDATE_REQUESTED)
        ssl->key_update_requested = true;
}


static void log_ssl_errors() {
    for (unsigned long error = ERR_get_error(); error;
         error = ERR_get_error()) {
        log_error("ssl error: %s", ERR_error_string(error, NULL));
    }
}


static BIO *create_bio() {
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        log_error("BIO_new error");
        return NULL;
    }

    BIO_set_retry_read(bio);
    BIO_set_mem_eof_return(bio, -1);

    return bio;
}


static uv_timer_t *create_timer(ssl_t *ssl) {
    uv_timer_t *timer = malloc(sizeof(uv_timer_t));
    if (!timer) {
        log_error("malloc error");
        goto error;
    }

    timer->data = ssl;

    if (uv_timer_init(ssl->endpoint->channel->mapping->app->loop, timer) < 0) {
        log_error("uv_timer_init error");
        goto error;
    }

    return timer;

error:
    if (timer)
        free(timer);

    return NULL;
}


static void close_timer(uv_timer_t *timer) {
    if (!uv_is_closing((uv_handle_t *)timer))
        uv_close((uv_handle_t *)timer, on_timer_close);
}


static void destroy_timer(uv_timer_t *timer) { free(timer); }


static void read_output_bio(ssl_t *ssl) {
    while (true) {
        buffer_t *buffer = pop_buffer(ssl->endpoint->channel->pool);
        if (!buffer)
            break;

        int ret = BIO_read(ssl->output_bio, buffer->data, BUFFER_SIZE);

        if (ret > 0) {
            buffer->offset = 0;
            buffer->len = ret;

            write_endpoint(ssl->endpoint, buffer, true);
            continue;
        }

        push_buffer(buffer);

        if (!BIO_should_retry(ssl->output_bio)) {
            log_error("BIO_read error");
            close_channel(ssl->endpoint->channel);
        }

        break;
    }
}


static void read_ssl(ssl_t *ssl) {
    while (true) {
        buffer_t *buffer = pop_buffer(ssl->endpoint->channel->pool);
        if (!buffer)
            break;

        int ret = SSL_read(ssl->ssl, buffer->data, BUFFER_SIZE);

        if (ret > 0) {
            buffer->offset = 0;
            buffer->len = ret;

            endpoint_t *other_endpoint = get_other_endpoint(ssl->endpoint);
            write_endpoint(other_endpoint, buffer, false);
            continue;
        }

        push_buffer(buffer);

        int error = SSL_get_error(ssl->ssl, ret);
        if (error != SSL_ERROR_WANT_READ) {
            if (error == SSL_ERROR_ZERO_RETURN) {
                log_debug("SSL_read zero return");

            } else {
                log_error("SSL_read error");
            }

            close_channel(ssl->endpoint->channel);
        }

        break;
    }
}


static void write_ssl(ssl_t *ssl) {
    while (ssl->output_plain_data) {
        buffer_t *buffer = ssl->output_plain_data;
        ssl->output_plain_data = buffer->next;

        size_t written;
        int ret = SSL_write_ex(ssl->ssl, buffer->data + buffer->offset,
                               buffer->len, &written);

        if (ret > 0) {
            if (written == buffer->len) {
                push_buffer(buffer);

            } else {
                buffer->offset += written;
                buffer->len -= written;
                buffer->next = ssl->output_plain_data;
                ssl->output_plain_data = buffer;
            }

            continue;
        }

        buffer->next = ssl->output_plain_data;
        ssl->output_plain_data = buffer;

        int error = SSL_get_error(ssl->ssl, ret);
        if (error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
            log_error("SSL_write error");
            close_channel(ssl->endpoint->channel);
        }

        break;
    }
}


static void process_handshake_error(ssl_t *ssl) {
    unsigned long error = ERR_get_error();
    log_error("SSL_do_handshake error: %s", ERR_error_string(error, NULL));

    int error_lib = ERR_GET_LIB(error);
    int error_reason = ERR_GET_REASON(error);

    if (error_lib == ERR_LIB_SSL) {
        if (error_reason == SSL_R_UNSUPPORTED_PROTOCOL ||
            error_reason == SSL_R_VERSION_TOO_LOW) {
            int client_version = SSL_client_version(ssl->ssl);

            if (client_version == TLS1_VERSION) {
                log_event(SECURITY_EVENT_TLS_DEPRECATED_VERSION, "TLSv1.0");

            } else if (client_version == TLS1_1_VERSION) {
                log_event(SECURITY_EVENT_TLS_DEPRECATED_VERSION, "TLSv1.1");

            } else {
                log_event(SECURITY_EVENT_TLS_DISALLOWED_VERSION, "%s",
                          SSL_get_version(ssl->ssl));
            }

        } else if (error_reason == SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE) {
            log_event(SECURITY_EVENT_TLS_NO_PEER_CERT, NULL);

        } else if (error_reason == SSL_R_NO_SHARED_CIPHER) {
            log_event(SECURITY_EVENT_TLS_DISALLOWED_CIPHER, NULL);
        }
    }
}


static int verify_cert(ssl_t *ssl, X509 *cert) {
    if (ssl->conf->remote_cert->allowed_cert_sn) {
        ASN1_INTEGER *cert_sn_asn1 = X509_get_serialNumber(cert);
        if (!cert_sn_asn1) {
            log_error("X509_get_serialNumber error");
            return 0;
        }

        BIGNUM *cert_sn_bn = ASN1_INTEGER_to_BN(cert_sn_asn1, NULL);
        if (!cert_sn_bn) {
            log_error("ASN1_INTEGER_to_BN error");
            return 0;
        }

        bool allowed_sn = false;

        for (size_t i = 0; i < ssl->conf->remote_cert->allowed_cert_sn_len;
             ++i) {
            if (BN_cmp(ssl->conf->remote_cert->allowed_cert_sn[i],
                       cert_sn_bn) == 0) {
                allowed_sn = true;
                break;
            }
        }

        BN_free(cert_sn_bn);

        if (!allowed_sn) {
            log_event(SECURITY_EVENT_TLS_NO_TRUSTED_CERT_MATCH, NULL);
            return 0;
        }
    }

    unsigned char *cert_bytes = NULL;
    int cert_len = i2d_X509(cert, &cert_bytes);
    if (cert_len < 0) {
        log_error("i2d_X509 error");
        return 0;
    }

    OPENSSL_free(cert_bytes);

    if (cert_len > ssl->conf->remote_cert->max_cert_len) {
        log_event(SECURITY_EVENT_TLS_CERT_SIZE_MISMATCH, "%d", cert_len);
        return 0;
    }

    EVP_PKEY *key = X509_get_pubkey(cert);
    if (!key) {
        log_error("X509_get_pubkey error");
        return 0;
    }

    int key_base_id = EVP_PKEY_base_id(key);
    int key_size = EVP_PKEY_bits(key);

    EVP_PKEY_free(key);

    if (key_base_id == EVP_PKEY_RSA) {
        if (key_size < 1024) {
            log_event(SECURITY_EVENT_TLS_SHORT_KEY, "%d", key_size);
            return 0;
        }

        if (key_size < 2048) {
            log_event(SECURITY_EVENT_TLS_SHORT_RSA_KEY, "%d", key_size);
            return 0;
        }

        if (key_size > 8192) {
            log_warning("RSA key length greater than 8192: %d", key_size);
        }

    } else if (key_base_id == EVP_PKEY_EC) {
        if (key_size < 256) {
            log_error("insufficient EC key length: %d", key_size);
            return 0;
        }
    }

    return 1;
}


static int verify_version(ssl_t *ssl) {
    int version = SSL_version(ssl->ssl);

    if (ssl->version == 0) {
        ssl->version = version;

    } else if (ssl->version != version) {
        log_event(SECURITY_EVENT_TLS_VERSION_CHANGE, "%s",
                  SSL_get_version(ssl->ssl));
        return 0;
    }

    return 1;
}


static int verify_cipher(ssl_t *ssl) {
    const SSL_CIPHER *cipher = SSL_get_current_cipher(ssl->ssl);
    if (!cipher) {
        log_error("SSL_get_current_cipher error");
        return 0;
    }

    int digest = SSL_CIPHER_get_digest_nid(cipher);

    if (digest == NID_sha1)
        log_event(SECURITY_EVENT_TLS_DEP_HASH, NULL);

    // char *description = SSL_CIPHER_description(cipher, NULL, 0);
    // log_debug("cipher: %s", description);
    // OPENSSL_free(description);

    return 1;
}


static int verify_extensions(ssl_t *ssl) {
    if (ssl->endpoint->type == ENDPOINT_TYPE_SERVER) {
        if (!ssl->extensions.signature_algorithms)
            log_event(SECURITY_EVENT_TLS_NO_SIG_ALGO_EXT, NULL);
    }

    return 1;
}


static int verify_crl(ssl_t *ssl) {
    if (!ssl->crl)
        return 1;

    X509 *cert = SSL_get_peer_certificate(ssl->ssl);
    if (!cert) {
        log_error("SSL_get_peer_certificate error");
        return 0;
    }

    const ASN1_TIME *next_update = X509_CRL_get0_nextUpdate(ssl->crl);
    if (next_update) {
        int cmp = ASN1_TIME_cmp_time_t(next_update, time(NULL));
        if (cmp == -1 || cmp == 0)
            log_event(SECURITY_EVENT_TLS_CRL_EXP, NULL);
    }

    X509_REVOKED *revoked;
    int result = X509_CRL_get0_by_cert(ssl->crl, &revoked, cert);
    X509_free(cert);
    if (result == 1) {
        log_event(SECURITY_EVENT_TLS_CERT_REVOKED, NULL);
        log_event(SECURITY_EVENT_TLS_SESSION_CLOSED_REV, NULL);
        return 0;
    }

    return 1;
}


static void renegotiate(ssl_t *ssl) {
    if (ssl->version == TLS1_2_VERSION) {
        if (SSL_renegotiate_abbreviated(ssl->ssl) != 1) {
            log_error("SSL_renegotiate error");
            close_channel(ssl->endpoint->channel);
            return;
        }

    } else if (ssl->version == TLS1_3_VERSION) {
        if (SSL_key_update(ssl->ssl, SSL_KEY_UPDATE_REQUESTED) != 1) {
            log_error("SSL_key_update error");
            close_channel(ssl->endpoint->channel);
            return;
        }

    } else {
        return;
    }

    process_ssl(ssl);
}


static X509_CRL *load_crl(char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        log_error("fopen error");
        return NULL;
    }

    X509_CRL *crl = PEM_read_X509_CRL(fp, NULL, on_password, NULL);
    fclose(fp);
    if (!crl)
        log_error("PEM_read_X509_CRL error");

    return crl;
}


static uint64_t get_next_crl_update_interval(ssl_t *ssl) {
    if (!ssl->conf->remote_cert->crl->update_interval || !ssl->crl)
        goto fallback;

    const ASN1_TIME *next_update = X509_CRL_get0_nextUpdate(ssl->crl);
    if (!next_update)
        goto fallback;

    int pday, psec;
    if (ASN1_TIME_diff(&pday, &psec, NULL, next_update) != 1)
        goto fallback;

    if (pday < 0 || psec < 0 || (pday == 0 && psec == 0))
        goto fallback;

    uint64_t interval = (uint64_t)pday * 86400 + psec;
    if (interval > ssl->conf->remote_cert->crl->update_interval)
        goto fallback;

    return interval;

fallback:
    return ssl->conf->remote_cert->crl->update_interval;
}


ssl_ctx_t *create_ssl_ctx(endpoint_type_t type, ssl_conf_t *conf) {
    ERR_clear_error();

    ssl_ctx_t *ctx = malloc(sizeof(ssl_ctx_t));
    if (!ctx) {
        log_error("malloc error");
        goto error;
    }

    ctx->type = type;
    ctx->conf = conf;
    ctx->ctx = NULL;

    ctx->ctx = SSL_CTX_new(type == ENDPOINT_TYPE_CLIENT ? TLS_client_method()
                                                        : TLS_server_method());
    if (!ctx->ctx) {
        log_error("SSL_CTX_new error");
        goto error;
    }

    if (SSL_CTX_set_app_data(ctx->ctx, ctx) != 1) {
        log_error("SSL_CTX_set_app_data error");
        goto error;
    }

    uint64_t options = SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    options |= SSL_OP_NO_SSLv2;
    options |= SSL_OP_NO_SSLv3;
#ifdef SSL_OP_ALLOW_CLIENT_RENEGOTIATION
    options |= SSL_OP_ALLOW_CLIENT_RENEGOTIATION;
#endif

    uint64_t result_options = SSL_CTX_set_options(ctx->ctx, options);
    if ((result_options & options) != options)
        log_warning("SSL_CTX_set_options result different from argument");

    if (SSL_CTX_set_cipher_list(ctx->ctx, CIPHER_LIST_V1_2) != 1) {
        log_error("SSL_CTX_set_cipher_list error");
        goto error;
    }

    if (SSL_CTX_set_ciphersuites(ctx->ctx, CIPHER_LIST_V1_3) != 1) {
        log_error("SSL_CTX_set_ciphersuites error");
        goto error;
    }

    if (SSL_CTX_set_min_proto_version(ctx->ctx, TLS1_2_VERSION) != 1) {
        log_error("SSL_CTX_set_min_proto_version error");
        goto error;
    }

    if (SSL_CTX_set_max_proto_version(ctx->ctx, TLS1_3_VERSION) != 1) {
        log_error("SSL_CTX_set_max_proto_version error");
        goto error;
    }

    if (type == ENDPOINT_TYPE_SERVER) {
        if (SSL_CTX_set_session_id_context(ctx->ctx,
                                           (const unsigned char *)SESSION_ID,
                                           sizeof(SESSION_ID)) != 1) {
            log_error("SSL_CTX_set_session_id_context error");
            goto error;
        }

        // SSL_CTX_set_max_early_data(ctx->ctx, 0xffff);

    } else {
        SSL_CTX_sess_set_new_cb(ctx->ctx, on_client_session);
    }

    SSL_CTX_set_session_cache_mode(ctx->ctx, SSL_SESS_CACHE_BOTH);

    if (conf->session_cache_timeout)
        SSL_CTX_set_timeout(ctx->ctx, conf->session_cache_timeout);

    SSL_CTX_set_post_handshake_auth(ctx->ctx, 0);

    if (conf->remote_cert) {
        int mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        SSL_CTX_set_verify(ctx->ctx, mode, on_verify);

        if (SSL_CTX_load_verify_locations(ctx->ctx, conf->remote_cert->ca_path,
                                          NULL) != 1) {
            log_error("SSL_CTX_load_verify_locations error");
            goto error;
        }

        if (SSL_CTX_set1_sigalgs_list(ctx->ctx, SIG_ALG_LIST) != 1) {
            log_error("SSL_CTX_set1_sigalgs_list error");
            goto error;
        }

        // X509_VERIFY_PARAM *verify_param = SSL_CTX_get0_param(ctx->ctx);
        // unsigned long verify_flags =
        // X509_VERIFY_PARAM_get_flags(verify_param); if
        // (X509_VERIFY_PARAM_set_flags(verify_param, X509_V_FLAG_CRL_CHECK) !=
        // 1) {
        //     log_error("X509_VERIFY_PARAM_set_flags error");
        //     goto error;
        // }

        // void X509_STORE_set_get_crl(X509_STORE *ctx,
        // X509_STORE_CTX_get_crl_fn get_crl);

    } else {
        SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_NONE, NULL);
    }

    if (conf->local_cert) {
        SSL_CTX_set_default_passwd_cb(ctx->ctx, on_password);
        SSL_CTX_set_default_passwd_cb_userdata(ctx->ctx,
                                               conf->local_cert->password);

        if (SSL_CTX_use_certificate_chain_file(
                ctx->ctx, conf->local_cert->cert_path) != 1) {
            log_error("SSL_CTX_use_certificate_chain_file error");
            log_event(SECURITY_EVENT_TLS_NO_LOCAL_CERT, NULL);
            goto error;
        }

        if (SSL_CTX_use_PrivateKey_file(ctx->ctx, conf->local_cert->key_path,
                                        SSL_FILETYPE_PEM) != 1) {
            log_error("SSL_CTX_use_PrivateKey_file error");
            goto error;
        }

        if (SSL_CTX_check_private_key(ctx->ctx) != 1) {
            log_error("SSL_CTX_check_private_key error");
            goto error;
        }
    }

    return ctx;

error:
    log_ssl_errors();

    if (ctx)
        destroy_ssl_ctx(ctx);

    return NULL;
}


void destroy_ssl_ctx(ssl_ctx_t *ctx) {
    if (ctx->ctx)
        SSL_CTX_free(ctx->ctx);

    free(ctx);
}


ssl_t *create_ssl(endpoint_t *endpoint, ssl_close_cb_t close_cb) {
    ERR_clear_error();

    ssl_t *ssl = malloc(sizeof(ssl_t));
    if (!ssl) {
        log_error("malloc error");
        goto error;
    }

    ssl->endpoint = endpoint;
    ssl->close_cb = NULL;
    ssl->conf = endpoint->conf->ssl;
    ssl->renegotiate_timer = NULL;
    ssl->crl_update_timer = NULL;
    ssl->input_bio = NULL;
    ssl->output_bio = NULL;
    ssl->ssl = NULL;
    ssl->crl = NULL;
    ssl->output_plain_data = NULL;
    ssl->active = false;
    ssl->shutdown = false;
    ssl->key_update_requested = false;
    ssl->version = 0;
    ssl->extensions = (ssl_extensions_t){.renegotiation_info = false,
                                         .signature_algorithms = false,
                                         .encrypt_then_mac = false};

    ssl->renegotiate_timer = create_timer(ssl);
    if (!ssl->renegotiate_timer) {
        log_error("create timer error");
        goto error;
    }

    ssl->crl_update_timer = create_timer(ssl);
    if (!ssl->crl_update_timer) {
        log_error("create timer error");
        goto error;
    }

    ssl->input_bio = create_bio();
    if (!(ssl->input_bio)) {
        log_error("create bio error");
        goto error;
    }

    ssl->output_bio = create_bio();
    if (!(ssl->output_bio)) {
        log_error("create bio error");
        goto error;
    }

    ssl->ssl = SSL_new(endpoint->type == ENDPOINT_TYPE_CLIENT
                           ? endpoint->channel->mapping->client_ssl_ctx->ctx
                           : endpoint->channel->mapping->server_ssl_ctx->ctx);
    if (!(ssl->ssl)) {
        log_error("SSL_new error");
        goto error;
    }

    SSL_set_bio(ssl->ssl, ssl->input_bio, ssl->output_bio);

    if (SSL_set_app_data(ssl->ssl, ssl) != 1) {
        log_error("SSL_set_app_data error");
        goto error;
    }

    long mode = SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_AUTO_RETRY;
    long result_mode = SSL_set_mode(ssl->ssl, mode);
    if (result_mode != mode)
        log_warning("SSL_set_mode result different from argument");

    if (endpoint->type == ENDPOINT_TYPE_CLIENT) {
        if (endpoint->channel->mapping->client_session) {
            if (SSL_set_session(ssl->ssl,
                                endpoint->channel->mapping->client_session) !=
                1) {
                log_warning("SSL_set_session error");
            }
        }

        SSL_set_connect_state(ssl->ssl);

    } else {
        SSL_set_accept_state(ssl->ssl);
    }

    if (ssl->conf->remote_cert && ssl->conf->remote_cert->crl) {
        ssl->crl = load_crl(ssl->conf->remote_cert->crl->path);
        if (!ssl->crl)
            log_event(SECURITY_EVENT_TLS_NO_CRL, NULL);
    }

    SSL_set_msg_callback(ssl->ssl, on_message);
    SSL_set_msg_callback_arg(ssl->ssl, ssl);

    ssl->close_cb = close_cb;

    return ssl;

error:
    log_ssl_errors();

    if (ssl) {
        if (!ssl->renegotiate_timer && !ssl->crl_update_timer)
            destroy_ssl(ssl);

        else
            close_ssl(ssl);
    }

    return NULL;
}


void close_ssl(ssl_t *ssl) {
    if (ssl->active && !ssl->shutdown) {
        ssl->shutdown = true;

        SSL_shutdown(ssl->ssl);
        read_output_bio(ssl);
    }

    if (ssl->renegotiate_timer)
        close_timer(ssl->renegotiate_timer);

    if (ssl->crl_update_timer)
        close_timer(ssl->crl_update_timer);
}


void destroy_ssl(ssl_t *ssl) {
    while (ssl->output_plain_data) {
        buffer_t *buffer = ssl->output_plain_data;
        ssl->output_plain_data = buffer->next;

        push_buffer(buffer);
    }

    if (ssl->ssl) {
        SSL_free(ssl->ssl);

    } else {
        if (ssl->input_bio)
            BIO_free(ssl->input_bio);

        if (ssl->output_bio)
            BIO_free(ssl->output_bio);
    }

    free(ssl);
}


void process_ssl(ssl_t *ssl) {
    ERR_clear_error();

    ssl->active = true;

init:
    if (!SSL_is_init_finished(ssl->ssl)) {
        int ret = SSL_do_handshake(ssl->ssl);

        if (ret == 1) {
            if (!verify_version(ssl)) {
                log_error("verify_version error");
                close_channel(ssl->endpoint->channel);
                return;
            }

            if (!verify_cipher(ssl)) {
                log_error("verify_cipher error");
                close_channel(ssl->endpoint->channel);
                return;
            }

            if (!verify_extensions(ssl)) {
                log_error("verify_extensions error");
                close_channel(ssl->endpoint->channel);
                return;
            }

            if (ssl->conf->remote_cert && ssl->conf->remote_cert->crl &&
                !verify_crl(ssl)) {
                log_error("verify_crl error");
                close_channel(ssl->endpoint->channel);
                return;
            }

            if (SSL_get_early_data_status(ssl->ssl) != SSL_EARLY_DATA_NOT_SENT)
                log_event(SECURITY_EVENT_TLS_EARLY_DATA, NULL);

            log_event(SECURITY_EVENT_TLS_HS_SUCCESS, NULL);

            log_debug("handshake done (session reuse: %d)",
                      SSL_session_reused(ssl->ssl));

            if (ssl->conf->renegotiate_interval) {
                if (uv_timer_start(ssl->renegotiate_timer, on_renegotiate_timer,
                                   ssl->conf->renegotiate_interval * 1000,
                                   ssl->conf->renegotiate_interval * 1000) <
                    0) {
                    log_error("uv_timer_start error");
                    close_channel(ssl->endpoint->channel);
                    return;
                }
            }

            if (ssl->conf->remote_cert && ssl->conf->remote_cert->crl &&
                ssl->conf->remote_cert->crl->update_interval &&
                !uv_is_active((uv_handle_t *)ssl->crl_update_timer)) {
                if (uv_timer_start(
                        ssl->crl_update_timer, on_crl_update_timer,
                        get_next_crl_update_interval(ssl) * 1000,
                        ssl->conf->remote_cert->crl->update_interval * 1000) <
                    0) {
                    log_error("uv_timer_start error");
                    close_channel(ssl->endpoint->channel);
                    return;
                }
            }

        } else {
            int err = SSL_get_error(ssl->ssl, ret);

            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                read_output_bio(ssl);

            } else {
                process_handshake_error(ssl);
                close_channel(ssl->endpoint->channel);
            }

            return;
        }
    }

    read_ssl(ssl);

    if (ssl->key_update_requested) {
        ssl->key_update_requested = false;

        if (SSL_key_update(ssl->ssl, SSL_KEY_UPDATE_NOT_REQUESTED) != 1) {
            log_error("SSL_key_update error");
            close_channel(ssl->endpoint->channel);
            return;
        }

        goto init;
    }

    write_ssl(ssl);

    read_output_bio(ssl);
}


void add_ssl_input_encrypted_data(ssl_t *ssl, buffer_t *buffer) {
    int res =
        BIO_write(ssl->input_bio, buffer->data + buffer->offset, buffer->len);

    push_buffer(buffer);

    if (res < 0) {
        log_error("BIO_write error");
        close_channel(ssl->endpoint->channel);
        return;
    }

    process_ssl(ssl);
}


void add_ssl_output_plain_data(ssl_t *ssl, buffer_t *buffer) {
    buffer_t **slot = &(ssl->output_plain_data);
    while (*slot)
        slot = &((*slot)->next);

    *slot = buffer;
    buffer->next = NULL;

    process_ssl(ssl);
}
