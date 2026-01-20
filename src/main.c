#include "common.h"

#include <signal.h>
#include <unistd.h>


typedef struct {
    log_level_t log_level;
    char *conf_path;
} args_t;


static void print_usage(FILE *f, char *name) {
    fprintf(f,
            "Usage: %s [-h] [-l LOG_LEVEL] <conf_path>\n"
            "\n"
            "  Arguments:\n"
            "    <conf_path>   Path to configuration file or - for stdin\n"
            "\n"
            "  Options:\n"
            "    -h            Display usage help and exit\n"
            "    -l LOG_LEVEL  Set logging level (DEBUG, INFO, WARNING, ERROR) "
            "(default INFO)\n",
            name);
}


static int parse_args(int argc, char *argv[], args_t *args) {
    args->log_level = LOG_LEVEL_INFO;

    int opt;
    while ((opt = getopt(argc, argv, "hl:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage(stdout, argv[0]);
            exit(0);

        case 'l':
            if (parse_log_level(optarg, &(args->log_level)) < 0) {
                print_usage(stderr, argv[0]);
                return -1;
            }
            break;

        default:
            print_usage(stderr, argv[0]);
            return -1;
        }
    }

    if (argc - optind != 1) {
        fprintf(stderr, "invalid number of required arguments\n");
        print_usage(stderr, argv[0]);
        return -1;
    }

    args->conf_path = argv[optind];

    return 0;
}


int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    app_conf_t *app_conf = NULL;
    app_t *app = NULL;

    args_t args;
    if (parse_args(argc, argv, &args) < 0)
        goto error;

    set_log_level(args.log_level);

    app_conf = create_app_conf(args.conf_path);
    if (!app_conf) {
        log_error("create_app_conf error");
        goto error;
    }

    app = create_app(app_conf);
    if (!app) {
        log_error("create_app error");
        goto error;
    }

    if (run_app(app) < 0) {
        log_error("run_app error");
        goto error;
    }

    int result = 0;
    goto cleanup;

error:
    result = 1;

cleanup:
    if (app)
        destroy_app(app);

    if (app_conf)
        destroy_app_conf(app_conf);

    return result;
}
