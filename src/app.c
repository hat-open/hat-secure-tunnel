#include "common.h"


static void close_app(app_t *app);


static void on_signal(uv_signal_t *s, int signum) {
    log_debug("on_signal");

    app_t *app = s->data;
    close_app(app);
}


static uv_signal_t *create_signal_handle(app_t *app) {
    uv_signal_t *handle = malloc(sizeof(uv_signal_t));
    if (!handle) {
        log_error("malloc error");
        goto error;
    }

    handle->data = app;

    if (uv_signal_init(app->loop, handle) < 0) {
        log_error("uv_signal_init error");
        goto error;
    }

    return handle;

error:
    if (handle)
        free(handle);

    return NULL;
}


static int start_signal_handle(uv_signal_t *handle, int signum) {
    return uv_signal_start(handle, on_signal, signum);
}


static void close_signal_handle(uv_signal_t *handle) {
    if (!uv_is_closing((uv_handle_t *)handle))
        uv_close((uv_handle_t *)handle, NULL);
}


static void destroy_signal_handle(uv_signal_t *handle) { free(handle); }


static uv_loop_t *create_loop(app_t *app) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    if (!loop) {
        log_error("malloc error");
        goto error;
    }

    loop->data = app;

    if (uv_loop_init(loop) < 0) {
        log_error("iuv_loop_init error");
        goto error;
    }

    return loop;

error:
    if (loop)
        free(loop);

    return NULL;
}


static void destroy_loop(uv_loop_t *loop) {
    uv_loop_close(loop);
    free(loop);
}


static void close_app(app_t *app) {
    for (mapping_t *mapping = app->mappings; mapping; mapping = mapping->next)
        close_mapping(mapping);

    close_signal_handle(app->sigint_handle);
    close_signal_handle(app->sigterm_handle);
}


app_t *create_app(app_conf_t *app_conf) {
    app_t *app = malloc(sizeof(app_t));
    if (!app) {
        log_error("malloc error");
        goto error;
    }

    app->conf = app_conf;
    app->loop = NULL;
    app->sigint_handle = NULL;
    app->sigterm_handle = NULL;
    app->mappings = NULL;

    app->loop = create_loop(app);
    if (!app->loop) {
        log_error("create_loop error");
        goto error;
    }

    app->sigint_handle = create_signal_handle(app);
    if (!app->sigint_handle) {
        log_error("create_signal_handle error");
        goto error;
    }

    app->sigterm_handle = create_signal_handle(app);
    if (!app->sigterm_handle) {
        log_error("create_signal_handle error");
        goto error;
    }

    if (start_signal_handle(app->sigint_handle, SIGINT) < 0) {
        log_error("start_signal_handle error");
        goto error;
    }

    if (start_signal_handle(app->sigterm_handle, SIGTERM) < 0) {
        log_error("start_signal_handle error");
        goto error;
    }

    mapping_t **mapping_slot = &(app->mappings);
    for (mapping_conf_t *mapping_conf = app_conf->mappings; mapping_conf;
         mapping_conf = mapping_conf->next) {
        mapping_t *mapping = create_mapping(mapping_conf, app);
        if (!mapping) {
            log_error("create mapping error");
            goto error;
        }

        *mapping_slot = mapping;
        mapping_slot = &(mapping->next);
    }

    return app;

error:
    if (app)
        destroy_app(app);

    return NULL;
}


int run_app(app_t *app) {
    if (uv_run(app->loop, UV_RUN_DEFAULT)) {
        log_error("loop stopped with active handles");
        return -1;
    }

    return 0;
}


void destroy_app(app_t *app) {
    while (app->mappings) {
        mapping_t *mapping = app->mappings;
        app->mappings = mapping->next;

        destroy_mapping(mapping);
    }

    if (app->sigint_handle)
        destroy_signal_handle(app->sigint_handle);

    if (app->sigterm_handle)
        destroy_signal_handle(app->sigterm_handle);

    if (app->loop)
        destroy_loop(app->loop);

    free(app);
}
