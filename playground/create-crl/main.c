#include <openssl/ssl.h>


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
    X509_REVOKED *revs[] = {};

    X509_CRL *crl = X509_CRL_new();

    time_t now = time(NULL);

    set_last_update(crl, now);
    set_next_update(crl, now + 10);
    sign(crl, argv[1]);

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
