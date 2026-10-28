#include "common.h"


static void destroy_handle(uv_tcp_t *handle);
static void destroy_getaddrinfo_req(uv_getaddrinfo_t *req);
static void close_handle(uv_tcp_t *handle);


static void on_handle_alloc(uv_handle_t *h, size_t suggested_size,
                            uv_buf_t *buf) {
    log_debug("on_handle_alloc");

    endpoint_t *endpoint = h->data;

    buffer_t *buffer = pop_buffer(endpoint->channel->pool);
    if (!buffer) {
        *buf = uv_buf_init(NULL, 0);
        return;
    }

    *buf = uv_buf_init(buffer->data, BUFFER_SIZE);
}


static void on_handle_read(uv_stream_t *s, ssize_t nread, const uv_buf_t *buf) {
    log_debug("on_handle_read");

    endpoint_t *endpoint = s->data;

    buffer_t *buffer =
        (buf->base ? (buffer_t *)(buf->base - sizeof(buffer_t)) : NULL);

    if (nread == UV_ENOBUFS) {
        stop_channel(endpoint->channel);
        return;
    }

    if (nread > 0) {
        log_debug("received %ld bytes", nread);

        buffer->offset = 0;
        buffer->len = nread;

        if (endpoint->ssl) {
            add_ssl_input_encrypted_data(endpoint->ssl, buffer);

        } else {
            endpoint_t *other_endpoint = get_other_endpoint(endpoint);
            write_endpoint(other_endpoint, buffer, false);
        }

        return;
    }

    if (buffer)
        push_buffer(buffer);

    if (nread == 0)
        return;

    if (nread == UV_EOF)
        log_debug("eof received");

    else
        log_error("on_uv_read error");

    close_channel(endpoint->channel);
}


static void on_handle_write(uv_write_t *req, int status) {
    log_debug("on_handle_write");

    buffer_t *buffer = req->data;
    push_buffer(buffer);

    if (status < 0) {
        log_error("write error");
        close_channel(buffer->pool->channel);
    }
}


static void on_handle_close(uv_handle_t *handle) {
    log_debug("on_handle_close");

    endpoint_t *endpoint = handle->data;

    endpoint->handle = NULL;
    destroy_handle((uv_tcp_t *)handle);

    if (endpoint->close_cb && !endpoint->getaddrinfo_req && !endpoint->ssl)
        endpoint->close_cb(endpoint);
}


static void on_handle_shutdown(uv_shutdown_t *req, int status) {
    log_debug("on_handle_shutdown");

    endpoint_t *endpoint = req->data;

    close_handle(endpoint->handle);
}


static void on_ssl_close(ssl_t *ssl) {
    log_debug("on_ssl_close");

    endpoint_t *endpoint = ssl->endpoint;

    endpoint->ssl = NULL;
    destroy_ssl(ssl);

    if (endpoint->close_cb && !endpoint->handle && !endpoint->getaddrinfo_req)
        endpoint->close_cb(endpoint);
}


static void on_connect(uv_connect_t *req, int status) {
    log_debug("on_connect");

    endpoint_t *endpoint = req->data;

    if (status < 0) {
        log_error("on_connect error");
        goto error;
    }

    start_channel(endpoint->channel);

    return;

error:
    close_channel(endpoint->channel);
}


static void on_getaddrinfo(uv_getaddrinfo_t *req, int status,
                           struct addrinfo *res) {
    log_debug("on_getaddrinfo");

    endpoint_t *endpoint = req->data;

    destroy_getaddrinfo_req(req);
    endpoint->getaddrinfo_req = NULL;

    if (status < 0) {
        log_error("on_getaddrinfo error");
        goto error;
    }

    uv_tcp_connect(&(endpoint->connect_req), endpoint->handle, res->ai_addr,
                   on_connect);
    uv_freeaddrinfo(res);

    return;

error:
    if (endpoint->handle || endpoint->ssl) {
        close_endpoint(endpoint);

    } else if (endpoint->close_cb) {
        endpoint->close_cb(endpoint);
    }
}


static uv_tcp_t *create_handle(endpoint_t *endpoint) {
    uv_tcp_t *handle = malloc(sizeof(uv_tcp_t));
    if (!handle) {
        log_error("malloc error");
        goto error;
    }

    handle->data = endpoint;

    if (uv_tcp_init(endpoint->channel->mapping->app->loop, handle) < 0) {
        log_error("uv_tcp_init error");
        goto error;
    }

    return handle;

error:
    if (handle)
        destroy_handle(handle);

    return NULL;
}


static void close_handle(uv_tcp_t *handle) {
    if (!uv_is_closing((uv_handle_t *)handle))
        uv_close((uv_handle_t *)handle, on_handle_close);
}


static void destroy_handle(uv_tcp_t *handle) { free(handle); }


static uv_getaddrinfo_t *create_getaddrinfo_req(endpoint_t *endpoint) {
    uv_getaddrinfo_t *req = malloc(sizeof(uv_getaddrinfo_t));
    if (!req) {
        log_error("malloc error");
        goto error;
    }

    req->data = endpoint;

    char service[6];
    snprintf(service, sizeof(service), "%d", endpoint->conf->port);

    if (uv_getaddrinfo(endpoint->channel->mapping->app->loop, req,
                       on_getaddrinfo, endpoint->conf->host, service,
                       NULL) < 0) {
        log_error("uv_getaddrinfo error");
        goto error;
    }

    return req;

error:
    if (req)
        destroy_getaddrinfo_req(req);

    return NULL;
}


static void cancel_getaddrinfo_req(uv_getaddrinfo_t *req) {
    uv_cancel((uv_req_t *)req);
}


static void destroy_getaddrinfo_req(uv_getaddrinfo_t *req) { free(req); }


endpoint_t *create_endpoint(endpoint_type_t type, channel_t *channel,
                            endpoint_close_cb_t close_cb) {
    endpoint_t *endpoint = malloc(sizeof(endpoint_t));
    if (!endpoint) {
        log_error("malloc error");
        goto error;
    }

    endpoint->type = type;
    endpoint->channel = channel;
    endpoint->close_cb = NULL;
    endpoint->conf = NULL;
    endpoint->handle = NULL;
    endpoint->getaddrinfo_req = NULL;
    endpoint->ssl = NULL;
    endpoint->reading = false;
    endpoint->connect_req.data = endpoint;
    endpoint->shutdown_req.data = endpoint;

    endpoint->handle = create_handle(endpoint);
    if (!endpoint->handle) {
        log_error("create_handle error");
        goto error;
    }

    if (type == ENDPOINT_TYPE_CLIENT) {
        endpoint->conf = channel->mapping->conf->client;

        endpoint->getaddrinfo_req = create_getaddrinfo_req(endpoint);
        if (!endpoint->getaddrinfo_req) {
            log_error("create_getaddrinfo_req error");
            goto error;
        }

    } else {
        endpoint->conf = channel->mapping->conf->server;

        if (uv_accept((uv_stream_t *)channel->mapping->server,
                      (uv_stream_t *)endpoint->handle) < 0) {
            log_error("uv_accept error");
            goto error;
        }
    }

    if (endpoint->conf->ssl) {
        endpoint->ssl = create_ssl(endpoint, on_ssl_close);
        if (!endpoint->ssl) {
            log_error("create_ssl error");
            goto error;
        }
    }

    endpoint->close_cb = close_cb;

    return endpoint;

error:
    if (endpoint) {
        if (!endpoint->handle && !endpoint->getaddrinfo_req && !endpoint->ssl)
            destroy_endpoint(endpoint);

        else
            close_endpoint(endpoint);
    }

    return NULL;
}


void close_endpoint(endpoint_t *endpoint) {
    if (endpoint->ssl)
        close_ssl(endpoint->ssl);

    if (endpoint->getaddrinfo_req)
        cancel_getaddrinfo_req(endpoint->getaddrinfo_req);

    if (endpoint->handle) {
        if (uv_shutdown(&(endpoint->shutdown_req),
                        (uv_stream_t *)endpoint->handle,
                        on_handle_shutdown) < 0)
            close_handle(endpoint->handle);
    }
}


void destroy_endpoint(endpoint_t *endpoint) { free(endpoint); }


void start_endpoint(endpoint_t *endpoint) {
    if (!endpoint->handle || endpoint->reading ||
        uv_is_closing((uv_handle_t *)endpoint->handle))
        return;

    if (uv_read_start((uv_stream_t *)endpoint->handle, on_handle_alloc,
                      on_handle_read) < 0) {
        log_error("uv_read_start error");
        close_channel(endpoint->channel);
        return;
    }

    endpoint->reading = true;

    if (endpoint->ssl)
        process_ssl(endpoint->ssl);
}


void stop_endpoint(endpoint_t *endpoint) {
    if (!endpoint->handle || !endpoint->reading)
        return;

    uv_read_stop((uv_stream_t *)endpoint->handle);
    endpoint->reading = false;
}


void write_endpoint(endpoint_t *endpoint, buffer_t *buffer, bool skip_ssl) {
    if (!endpoint || !endpoint->handle ||
        uv_is_closing((uv_handle_t *)endpoint->handle)) {
        push_buffer(buffer);
        return;
    }

    if (endpoint->ssl && !skip_ssl) {
        add_ssl_output_plain_data(endpoint->ssl, buffer);
        return;
    }

    uv_buf_t buf = {.base = buffer->data + buffer->offset, .len = buffer->len};
    if (uv_write(&(buffer->write_req), (uv_stream_t *)endpoint->handle, &buf, 1,
                 on_handle_write) < 0) {
        log_error("write error");
        close_channel(endpoint->channel);
    }
}


endpoint_t *get_other_endpoint(endpoint_t *endpoint) {
    return (endpoint->type == ENDPOINT_TYPE_CLIENT ? endpoint->channel->server
                                                   : endpoint->channel->client);
}
