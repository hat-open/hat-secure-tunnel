#include "common.h"


static buffer_t *create_buffer(buffer_pool_t *pool) {
    buffer_t *buffer = malloc(sizeof(buffer_t) + BUFFER_SIZE);
    if (!buffer) {
        log_error("malloc error");
        return NULL;
    }

    buffer->next = pool->buffers;
    buffer->pool = pool;
    buffer->write_req.data = buffer;

    return buffer;
}


static void destroy_buffer(buffer_t *buffer) { free(buffer); }


buffer_pool_t *create_buffer_pool(channel_t *channel) {
    buffer_pool_t *pool = malloc(sizeof(buffer_pool_t));
    if (!pool) {
        log_error("malloc error");
        goto error;
    }

    pool->channel = channel;
    pool->buffers = NULL;

    for (size_t i = 0; i < BUFFER_COUNT; ++i) {
        buffer_t *buffer = create_buffer(pool);
        if (!buffer) {
            log_error("create_buffer error");
            goto error;
        }

        pool->buffers = buffer;
    }

    return pool;

error:
    if (pool)
        destroy_buffer_pool(pool);

    return NULL;
}


void destroy_buffer_pool(buffer_pool_t *pool) {
    while (pool->buffers) {
        buffer_t *buffer = pool->buffers;
        pool->buffers = buffer->next;

        destroy_buffer(buffer);
    }

    free(pool);
}


void push_buffer(buffer_t *buffer) {
    buffer->next = buffer->pool->buffers;
    buffer->pool->buffers = buffer;

    if (!buffer->next)
        start_channel(buffer->pool->channel);
}


buffer_t *pop_buffer(buffer_pool_t *pool) {
    buffer_t *buffer = pool->buffers;

    if (buffer)
        pool->buffers = buffer->next;

    // can't call stop_channel in case of empty pool because this function is
    // called in on_handle_alloc

    return buffer;
}
