#include <unistd.h>

#include <openssl/ssl.h>


typedef struct {
    uint64_t next_update;
    char *key_path;
} args_t;


static void print_usage(FILE *f, char *name) {
    fprintf(f,
            "Usage: %s [-h] [-u SECONDS] <key_path>\n"
            "\n"
            "Create CRL containing revocation of certificates read from stdin. "
            "Resulting CRL is written to stdout.\n"
            "\n"
            "  Arguments:\n"
            "    <key_path>   Private key used for signing CRL\n"
            "\n"
            "  Options:\n"
            "    -h            Display usage help and exit\n"
            "    -u SECONDS    Set nextUpdate field to SECONDS into future\n",
            name);
}


static int parse_args(int argc, char *argv[], args_t *args) {
    args->next_update = 0;

    int opt;
    while ((opt = getopt(argc, argv, "hl:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage(stdout, argv[0]);
            exit(0);

        case 'u':
            args->next_update = atoi(optarg);
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

    args->key_path = argv[optind];

    return 0;
}


static void set_last_update(X509_CRL *crl, time_t t) {
    ASN1_TIME *t_asn1 = ASN1_TIME_set(NULL, t);
    X509_CRL_set1_lastUpdate(crl, t_asn1);
    ASN1_STRING_free(t_asn1);
}


static void set_next_update(X509_CRL *crl, time_t t) {
    ASN1_TIME *t_asn1 = ASN1_TIME_set(NULL, t);
    X509_CRL_set1_nextUpdate(crl, t_asn1);
    ASN1_STRING_free(t_asn1);
}


static void sign(X509_CRL *crl, char *key_path) {
    FILE *f = fopen(key_path, "r");
    EVP_PKEY *key = PEM_read_PrivateKey(f, NULL, NULL, NULL);
    fclose(f);

    X509_CRL_sign(crl, key, EVP_sha256());

    EVP_PKEY_free(key);
}


int main(int argc, char *argv[]) {
    args_t args;
    if (parse_args(argc, argv, &args) < 0)
        return 1;

    X509_CRL *crl = X509_CRL_new();

    sign(crl, args.key_path);

    time_t now = time(NULL);
    set_last_update(crl, now);

    if (args.next_update)
        set_next_update(crl, now + args.next_update);

    while (1) {
        X509 *cert = PEM_read_X509(stdin, NULL, NULL, NULL);
        if (!cert)
            break;

        X509_REVOKED *rev = X509_REVOKED_new();
        X509_REVOKED_set_serialNumber(rev, X509_get_serialNumber(cert));

        X509_CRL_add0_revoked(crl, rev);

        X509_free(cert);
    }

    PEM_write_X509_CRL(stdout, crl);

    X509_CRL_free(crl);

    return 0;
}
