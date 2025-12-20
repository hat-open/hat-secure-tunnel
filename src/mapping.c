#include "common.h"


static void on_channel_close(channel_t *channel) {
    log_debug("on_channel_close");

    channel_t **slot = &(channel->mapping->channels);
    while (*slot && *slot != channel)
        slot = &((*slot)->next);

    if (*slot)
        *slot = channel->next;

    destroy_channel(channel);
}


static void on_connection(uv_stream_t *s, int status) {
    log_debug("on_connection");

    mapping_t *mapping = s->data;

    channel_t *channel = create_channel(mapping, on_channel_close);
    if (!channel) {
        log_error("create_channel error");
        return;
    }

    channel->next = mapping->channels;
    mapping->channels = channel;
}


static void destroy_server(uv_tcp_t *server) { free(server); }


static uv_tcp_t *create_server(mapping_t *mapping, struct sockaddr *addr) {
    uv_tcp_t *server = malloc(sizeof(uv_tcp_t));
    if (!server) {
        log_error("malloc error");
        goto error;
    }

    server->data = mapping;

    if (uv_tcp_init(mapping->app->loop, server) < 0) {
        log_error("uv_tcp_init error");
        goto error;
    }

    if (uv_tcp_bind(server, addr, 0) < 0) {
        log_error("uv_tcp_bind error");
        goto error;
    }

    if (uv_listen((uv_stream_t *)server, SERVER_BACKLOG, on_connection) < 0) {
        log_error("uv_listen error");
        goto error;
    }

    return server;

error:
    if (server)
        destroy_server(server);

    return NULL;
}


static void close_server(uv_tcp_t *server) {
    if (!uv_is_closing((uv_handle_t *)server))
        uv_close((uv_handle_t *)server, NULL);
}


mapping_t *create_mapping(mapping_conf_t *conf, app_t *app) {
    mapping_t *mapping = malloc(sizeof(mapping_t));
    if (!mapping) {
        log_error("malloc error");
        goto error;
    }

    mapping->next = NULL;
    mapping->conf = conf;
    mapping->app = app;
    mapping->server = NULL;
    mapping->channels = NULL;
    mapping->server_ssl_ctx = NULL;
    mapping->client_ssl_ctx = NULL;
    mapping->client_session = NULL;

    char service[6];
    snprintf(service, sizeof(service), "%d", conf->server->port);

    struct addrinfo *info;
    if (getaddrinfo(conf->server->host, service, NULL, &info) || !info) {
        log_error("getaddrinfo error");
        goto error;
    }

    mapping->server = create_server(mapping, info->ai_addr);
    freeaddrinfo(info);
    if (!mapping->server) {
        log_error("create_server error");
        goto error;
    }

    if (conf->server->ssl) {
        mapping->server_ssl_ctx =
            create_ssl_ctx(ENDPOINT_TYPE_SERVER, conf->server->ssl);
        if (!mapping->server_ssl_ctx) {
            log_error("create_ssl_ctx error");
            goto error;
        }
    }

    if (conf->client->ssl) {
        mapping->client_ssl_ctx =
            create_ssl_ctx(ENDPOINT_TYPE_CLIENT, conf->client->ssl);
        if (!mapping->client_ssl_ctx) {
            log_error("create_ssl_ctx error");
            goto error;
        }
    }

    return mapping;

error:
    if (mapping)
        destroy_mapping(mapping);

    return NULL;
}


void close_mapping(mapping_t *mapping) {
    for (channel_t *channel = mapping->channels; channel;
         channel = channel->next)
        close_channel(channel);

    close_server(mapping->server);
}


void destroy_mapping(mapping_t *mapping) {
    while (mapping->channels) {
        channel_t *channel = mapping->channels;
        mapping->channels = channel->next;

        destroy_channel(channel);
    }

    if (mapping->server_ssl_ctx)
        destroy_ssl_ctx(mapping->server_ssl_ctx);

    if (mapping->client_ssl_ctx)
        destroy_ssl_ctx(mapping->client_ssl_ctx);

    if (mapping->client_session)
        SSL_SESSION_free(mapping->client_session);

    if (mapping->server)
        destroy_server(mapping->server);

    free(mapping);
}
