#include "common.h"


static void on_endpoint_close(endpoint_t *endpoint) {
    log_debug("on_endpoint_close");

    channel_t *channel = endpoint->channel;

    if (channel->server == endpoint)
        channel->server = NULL;

    if (channel->client == endpoint)
        channel->client = NULL;

    destroy_endpoint(endpoint);

    if (channel->close_cb && !channel->server && !channel->client)
        channel->close_cb(channel);
}


channel_t *create_channel(mapping_t *mapping, channel_close_cb_t close_cb) {
    channel_t *channel = malloc(sizeof(channel_t));
    if (!channel) {
        log_error("malloc error");
        goto error;
    }

    channel->next = NULL;
    channel->mapping = mapping;
    channel->close_cb = NULL;
    channel->pool = NULL;
    channel->server = NULL;
    channel->client = NULL;

    channel->pool = create_buffer_pool(channel);
    if (!channel->pool) {
        log_error("create_buffer_pool error");
        goto error;
    }

    channel->server =
        create_endpoint(ENDPOINT_TYPE_SERVER, channel, on_endpoint_close);
    if (!channel->server) {
        log_error("create_endpoint error");
        goto error;
    }

    channel->client =
        create_endpoint(ENDPOINT_TYPE_CLIENT, channel, on_endpoint_close);
    if (!channel->client) {
        log_error("create_endpoint error");
        goto error;
    }

    channel->close_cb = close_cb;

    return channel;

error:
    if (channel) {
        if (!channel->server && !channel->client)
            destroy_channel(channel);

        else
            close_channel(channel);
    }

    return NULL;
}


void close_channel(channel_t *channel) {
    if (channel->server)
        close_endpoint(channel->server);

    if (channel->client)
        close_endpoint(channel->client);
}


void destroy_channel(channel_t *channel) {
    if (channel->pool)
        destroy_buffer_pool(channel->pool);

    free(channel);
}


void start_channel(channel_t *channel) {
    if (channel->server)
        start_endpoint(channel->server);

    if (channel->client)
        start_endpoint(channel->client);
}


void stop_channel(channel_t *channel) {
    if (channel->server)
        stop_endpoint(channel->server);

    if (channel->client)
        stop_endpoint(channel->client);
}
