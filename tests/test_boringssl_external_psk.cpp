#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstdio>

// This exercises BoringSSL itself. QUIC and lobby integration require separate tests.
static bool handshake(int scenario) {
    SSL_CTX* client_ctx = SSL_CTX_new(TLS_method());
    SSL_CTX* server_ctx = SSL_CTX_new(TLS_method());
    if (!client_ctx || !server_ctx) return false;
    SSL_CTX_set_min_proto_version(client_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(client_ctx, TLS1_3_VERSION);
    SSL_CTX_set_min_proto_version(server_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(server_ctx, TLS1_3_VERSION);
    SSL* client = SSL_new(client_ctx);
    SSL* server = SSL_new(server_ctx);
    const unsigned char key[32] = {1, 2, 3}, bad_key[32] = {1, 2, 4};
    const unsigned char id[] = {5, 6}, bad_id[] = {5, 7}, context[] = {7, 8};
    SSL_CREDENTIAL* client_credential = SSL_CREDENTIAL_new_pre_shared_key(key, sizeof key, id, sizeof id, EVP_sha256(), context, sizeof context);
    SSL_CREDENTIAL* server_credential = SSL_CREDENTIAL_new_pre_shared_key(scenario == 1 ? bad_key : key, sizeof key,
        scenario == 2 ? bad_id : id, sizeof id, EVP_sha256(), context, sizeof context);
    bool ready = client && server && client_credential && server_credential &&
        SSL_add1_credential(client, client_credential) &&
        (scenario == 3 || SSL_add1_credential(server, server_credential));
    SSL_CREDENTIAL_free(client_credential);
    SSL_CREDENTIAL_free(server_credential);
    BIO* left = nullptr;
    BIO* right = nullptr;
    ready = ready && BIO_new_bio_pair(&left, 0, &right, 0);
    if (ready) {
        SSL_set_bio(client, left, left);
        SSL_set_bio(server, right, right);
        SSL_set_connect_state(client);
        SSL_set_accept_state(server);
    }
    bool client_done = false, server_done = false, fatal = false;
    for (int i = 0; ready && !fatal && !(client_done && server_done) && i < 100; ++i) {
        if (!client_done) {
            const int result = SSL_do_handshake(client);
            if (result == 1) client_done = true;
            else { const int error = SSL_get_error(client, result); fatal = error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE; }
        }
        if (!server_done && !fatal) {
            const int result = SSL_do_handshake(server);
            if (result == 1) server_done = true;
            else { const int error = SSL_get_error(server, result); fatal = error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE; }
        }
    }
    const bool success = ready && client_done && server_done &&
        SSL_version(client) == TLS1_3_VERSION && SSL_version(server) == TLS1_3_VERSION;
    SSL_free(client); SSL_free(server); SSL_CTX_free(client_ctx); SSL_CTX_free(server_ctx);
    ERR_clear_error();
    return success;
}

int main() {
    const bool matching = handshake(0), wrong_key = handshake(1), wrong_identity = handshake(2), missing_server = handshake(3);
    std::printf("TLS 1.3 external PSK: matching=%d wrong_key=%d wrong_identity=%d missing_server=%d\n",
        matching, wrong_key, wrong_identity, missing_server);
    return matching && !wrong_key && !wrong_identity && !missing_server ? 0 : 1;
}
