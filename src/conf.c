#include "common.h"


static void destroy_crl_conf(crl_conf_t *crl) { free(crl); }


static crl_conf_t *create_crl_conf(json_t *json) {
    crl_conf_t *crl = malloc(sizeof(crl_conf_t));
    if (!crl) {
        log_error("malloc error");
        goto error;
    }

    crl->path = NULL;
    crl->update_interval = 0;

    if (!json_is_object(json)) {
        log_error("invalid crl type");
        goto error;
    }

    json_t *path_json = json_object_get(json, "path");
    if (!path_json) {
        log_error("crl missing path");
        goto error;
    }

    crl->path = (char *)json_string_value(path_json);
    if (!crl->path) {
        log_error("invalid path");
        goto error;
    }

    json_t *update_interval_json = json_object_get(json, "update_interval");
    if (update_interval_json) {
        json_int_t update_interval = json_integer_value(update_interval_json);
        if (update_interval < 1) {
            log_error("invalid update_interval");
            goto error;
        }

        crl->update_interval = update_interval;
    }

    return crl;

error:
    if (crl)
        destroy_crl_conf(crl);

    return NULL;
}


static void destroy_local_cert_conf(local_cert_conf_t *local_cert) {
    free(local_cert);
}


static local_cert_conf_t *create_local_cert_conf(json_t *json) {
    local_cert_conf_t *local_cert = malloc(sizeof(local_cert_conf_t));
    if (!local_cert) {
        log_error("malloc error");
        goto error;
    }

    local_cert->cert_path = NULL;
    local_cert->key_path = NULL;
    local_cert->password = NULL;

    if (!json_is_object(json)) {
        log_error("invalid local_cert type");
        goto error;
    }

    json_t *cert_path_json = json_object_get(json, "cert_path");
    if (!cert_path_json) {
        log_error("local_cert missing cert_path");
        goto error;
    }

    local_cert->cert_path = (char *)json_string_value(cert_path_json);
    if (!local_cert->cert_path) {
        log_error("invalid cert_path");
        goto error;
    }

    json_t *key_path_json = json_object_get(json, "key_path");
    if (!key_path_json) {
        log_error("local_cert missing key_path");
        goto error;
    }

    local_cert->key_path = (char *)json_string_value(key_path_json);
    if (!local_cert->key_path) {
        log_error("invalid key_path");
        goto error;
    }

    json_t *password_json = json_object_get(json, "password");
    if (password_json) {
        local_cert->password = (char *)json_string_value(password_json);
        if (!local_cert->password) {
            log_error("invalid password");
            goto error;
        }
    }

    return local_cert;

error:
    if (local_cert)
        destroy_local_cert_conf(local_cert);

    return NULL;
}


static void destroy_remote_cert_conf(remote_cert_conf_t *remote_cert) {
    if (remote_cert->allowed_cert_sn) {
        for (size_t i = 0; i < remote_cert->allowed_cert_sn_len; ++i) {
            if (remote_cert->allowed_cert_sn[i])
                BN_free(remote_cert->allowed_cert_sn[i]);
        }

        free(remote_cert->allowed_cert_sn);
    }

    if (remote_cert->crl)
        destroy_crl_conf(remote_cert->crl);

    free(remote_cert);
}


static remote_cert_conf_t *create_remote_cert_conf(json_t *json) {
    remote_cert_conf_t *remote_cert = malloc(sizeof(remote_cert_conf_t));
    if (!remote_cert) {
        log_error("malloc error");
        goto error;
    }

    remote_cert->ca_path = NULL;
    remote_cert->allowed_cert_sn = NULL;
    remote_cert->allowed_cert_sn_len = 0;
    remote_cert->max_cert_len = 8192;
    remote_cert->crl = NULL;

    if (!json_is_object(json)) {
        log_error("invalid remote_cert type");
        goto error;
    }

    json_t *ca_path_json = json_object_get(json, "ca_path");
    if (!ca_path_json) {
        log_error("remote_cert missing ca_path");
        goto error;
    }

    remote_cert->ca_path = (char *)json_string_value(ca_path_json);
    if (!remote_cert->ca_path) {
        log_error("invalid ca_path");
        goto error;
    }

    json_t *allowed_cert_sn_json = json_object_get(json, "allowed_cert_sn");
    if (allowed_cert_sn_json) {
        if (!json_is_array(allowed_cert_sn_json)) {
            log_error("invalid allowed_cert_sn type");
            goto error;
        }

        remote_cert->allowed_cert_sn_len =
            json_array_size(allowed_cert_sn_json);

        remote_cert->allowed_cert_sn =
            calloc(remote_cert->allowed_cert_sn_len, sizeof(BIGNUM *));
        if (!remote_cert->allowed_cert_sn) {
            log_error("malloc error");
            goto error;
        }

        size_t index;
        json_t *value;
        json_array_foreach(allowed_cert_sn_json, index, value) {
            char *value_str = (char *)json_string_value(value);
            if (!value_str) {
                log_error("invalid allowed_cert_sn");
                goto error;
            }

            if (BN_hex2bn(&(remote_cert->allowed_cert_sn[index]), value_str) ==
                0) {
                log_error("invalid allowed_cert_sn");
                goto error;
            }
        }
    }

    json_t *max_cert_len_json = json_object_get(json, "max_cert_len");
    if (max_cert_len_json) {
        json_int_t max_cert_len = json_integer_value(max_cert_len_json);
        if (max_cert_len < 1) {
            log_error("invalid max_cert_len");
            goto error;
        }

        remote_cert->max_cert_len = max_cert_len;
    }

    json_t *crl_json = json_object_get(json, "crl");
    if (crl_json) {
        remote_cert->crl = create_crl_conf(crl_json);
        if (!remote_cert->crl) {
            log_error("create_crl_conf error");
            goto error;
        }
    }

    return remote_cert;

error:
    if (remote_cert)
        destroy_remote_cert_conf(remote_cert);

    return NULL;
}


static void destroy_ssl_conf(ssl_conf_t *ssl) {
    if (ssl->local_cert)
        destroy_local_cert_conf(ssl->local_cert);

    if (ssl->remote_cert)
        destroy_remote_cert_conf(ssl->remote_cert);

    free(ssl);
}


static ssl_conf_t *create_ssl_conf(json_t *json) {
    ssl_conf_t *ssl = malloc(sizeof(ssl_conf_t));
    if (!ssl) {
        log_error("malloc error");
        goto error;
    }

    ssl->local_cert = NULL;
    ssl->remote_cert = NULL;
    ssl->renegotiate_interval = 0;
    ssl->session_cache_timeout = 0;

    if (!json_is_object(json)) {
        log_error("invalid ssl type");
        goto error;
    }

    json_t *local_cert_json = json_object_get(json, "local_cert");
    if (local_cert_json) {
        ssl->local_cert = create_local_cert_conf(local_cert_json);
        if (!ssl->local_cert) {
            log_error("create_local_cert_conf error");
            goto error;
        }
    }

    json_t *remote_cert_json = json_object_get(json, "remote_cert");
    if (remote_cert_json) {
        ssl->remote_cert = create_remote_cert_conf(remote_cert_json);
        if (!ssl->remote_cert) {
            log_error("create_remote_cert_conf error");
            goto error;
        }
    }

    json_t *renegotiate_interval_json =
        json_object_get(json, "renegotiate_interval");
    if (renegotiate_interval_json) {
        json_int_t renegotiate_interval =
            json_integer_value(renegotiate_interval_json);
        if (renegotiate_interval < 1) {
            log_error("invalid renegotiate_interval");
            goto error;
        }

        ssl->renegotiate_interval = renegotiate_interval;
    }

    json_t *session_cache_timeout_json =
        json_object_get(json, "session_cache_timeout");
    if (session_cache_timeout_json) {
        json_int_t session_cache_timeout =
            json_integer_value(session_cache_timeout_json);
        if (session_cache_timeout < 1) {
            log_error("invalid session_cache_timeout");
            goto error;
        }

        ssl->session_cache_timeout = session_cache_timeout;
    }


    return ssl;

error:
    if (ssl)
        destroy_ssl_conf(ssl);

    return NULL;
}


static void destroy_endpoint_conf(endpoint_conf_t *endpoint) {
    if (endpoint->ssl)
        destroy_ssl_conf(endpoint->ssl);

    free(endpoint);
}


static endpoint_conf_t *create_endpoint_conf(json_t *json) {
    endpoint_conf_t *endpoint = malloc(sizeof(endpoint_conf_t));
    if (!endpoint) {
        log_error("malloc error");
        goto error;
    }

    endpoint->host = NULL;
    endpoint->ssl = NULL;

    if (!json_is_object(json)) {
        log_error("invalid endpoint type");
        goto error;
    }

    json_t *host_json = json_object_get(json, "host");
    if (!host_json) {
        log_error("endpoint missing host");
        goto error;
    }

    endpoint->host = (char *)json_string_value(host_json);
    if (!endpoint->host) {
        log_error("invalid host");
        goto error;
    }

    json_t *port_json = json_object_get(json, "port");
    if (!port_json) {
        log_error("endpoint missing port");
        goto error;
    }

    json_int_t port = json_integer_value(port_json);
    if (port < 1 || port > 0xffff) {
        log_error("invalid port");
        goto error;
    }

    endpoint->port = port;

    json_t *ssl_json = json_object_get(json, "ssl");
    if (ssl_json) {
        endpoint->ssl = create_ssl_conf(ssl_json);
        if (!endpoint->ssl) {
            log_error("create_ssl_conf error");
            goto error;
        }
    }

    return endpoint;

error:
    if (endpoint)
        destroy_endpoint_conf(endpoint);

    return NULL;
}


static void destroy_mapping_conf(mapping_conf_t *mapping) {
    if (mapping->server)
        destroy_endpoint_conf(mapping->server);

    if (mapping->client)
        destroy_endpoint_conf(mapping->client);

    free(mapping);
}


static mapping_conf_t *create_mapping_conf(json_t *json) {
    mapping_conf_t *mapping = malloc(sizeof(mapping_conf_t));
    if (!mapping) {
        log_error("malloc error");
        goto error;
    }

    mapping->next = NULL;
    mapping->server = NULL;
    mapping->client = NULL;

    if (!json_is_object(json)) {
        log_error("invalid mapping type");
        goto error;
    }

    json_t *server_json = json_object_get(json, "server");
    if (!server_json) {
        log_error("mapping missing server");
        goto error;
    }

    mapping->server = create_endpoint_conf(server_json);
    if (!mapping->server) {
        log_error("create_endpoint_conf error");
        goto error;
    }

    if (mapping->server->ssl && !mapping->server->ssl->local_cert) {
        log_error("server endpoint requires local_cert");
        goto error;
    }

    json_t *client_json = json_object_get(json, "client");
    if (!client_json) {
        log_error("mapping missing client");
        goto error;
    }

    mapping->client = create_endpoint_conf(client_json);
    if (!mapping->client) {
        log_error("create_endpoint_conf error");
        goto error;
    }

    return mapping;

error:
    if (mapping)
        destroy_mapping_conf(mapping);

    return NULL;
}


app_conf_t *create_app_conf(char *path) {
    app_conf_t *app = malloc(sizeof(app_conf_t));
    if (!app) {
        log_error("malloc error");
        goto error;
    }

    app->json = NULL;
    app->mappings = NULL;

    if (strcmp(path, "-") == 0) {
        app->json = json_loadf(stdin, 0, NULL);

    } else {
        app->json = json_load_file(path, 0, NULL);
    }

    if (!app->json) {
        log_error("json_load_file error");
        goto error;
    }

    if (!json_is_object(app->json)) {
        log_error("invalid configuration type");
        goto error;
    }

    json_t *mappings_json = json_object_get(app->json, "mappings");
    if (!mappings_json) {
        log_error("configuration missing mappings");
        goto error;
    }

    if (!json_is_array(mappings_json)) {
        log_error("invalid mappings type");
        goto error;
    }

    if (json_array_size(mappings_json) < 1) {
        log_error("invalid number of mappings supplied in the config (<1)");
        goto error;
    }

    size_t i;
    json_t *mapping_json;
    mapping_conf_t **mapping_slot = &(app->mappings);

    json_array_foreach(mappings_json, i, mapping_json) {
        *mapping_slot = create_mapping_conf(mapping_json);
        if (!(*mapping_slot)) {
            log_error("create_mapping_conf error");
            goto error;
        }

        mapping_slot = &((*mapping_slot)->next);
    }

    return app;

error:
    if (app)
        destroy_app_conf(app);

    return NULL;
}


void destroy_app_conf(app_conf_t *app) {
    while (app->mappings) {
        mapping_conf_t *mapping = app->mappings;
        app->mappings = mapping->next;

        destroy_mapping_conf(mapping);
    }

    if (app->json)
        json_decref(app->json);

    free(app);
}
