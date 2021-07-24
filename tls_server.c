#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "zlib.h"

int create_socket(int port)
{
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	    perror("Unable to create socket");
	    exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	    perror("Unable to bind");
	    exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0) {
	    perror("Unable to listen");
	    exit(EXIT_FAILURE);
    }

    return s;
}

SSL_CTX* create_context()
{
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = TLS_server_method();
    ctx = SSL_CTX_new(method);

    if (!ctx) {
	    perror("Unable to create SSL context");
	    ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }

    return ctx;
}

static int compress_cert(SSL* ssl, CBB* out, const uint8_t* in, size_t in_len)
{
    // Implementation
    printf("Compress called\n");
    return 0;
}

static int decompress_cert(SSL* ssl, CRYPTO_BUFFER** out, size_t uncompressed_len, 
                            const uint8_t* in, size_t in_len)
{
    // Implementation
    printf("Decompress called\n");
    return 0;
}

static void enable_compression(SSL_CTX* ctx) {
    if (!(SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_zlib,  compress_cert, decompress_cert)))
        perror("Unable to register ZLIB for certificate compression\n");
}

static void configure_context(SSL_CTX* ctx)
{
    // Cert compression requires TLSv1.3
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    enable_compression(ctx);
    SSL_CTX_set_ecdh_auto(ctx, 1);


    if (SSL_CTX_use_certificate_file(ctx, "certs/server-cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "certs/server-key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv)
{
    int sock;
    SSL_CTX* ctx;

    ctx = create_context();
    configure_context(ctx);

    sock = create_socket(4433);

    while(1) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);
        SSL* ssl;
        const char reply[] = "test\n";

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        } else {
            printf("Accepted socket\n");
        }

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        }
        else {
            SSL_write(ssl, reply, strlen(reply));
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sock);
    SSL_CTX_free(ctx);
}

