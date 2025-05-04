#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <brotli/encode.h>

#define COMPRESS_CHUNK_SIZE 4096
#define BROTLI_QUALITY BROTLI_MAX_QUALITY
#define BROTLI_LGWIN BROTLI_MAX_WINDOW_BITS

// Define this if it's not available in your BoringSSL version
#ifndef TLSEXT_cert_compression_brotli
#define TLSEXT_cert_compression_brotli 3
#endif

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

    // Use SSLv23_server_method() instead of TLS_server_method() for older BoringSSL
    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);

    if (!ctx) {
	    perror("Unable to create SSL context\n");
	    ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }

    return ctx;
}

int compress_cert(SSL* ssl, CBB* out, const uint8_t* in, size_t len_in)
{
    BrotliEncoderState* brotliState = BrotliEncoderCreateInstance(NULL, NULL, NULL);
    BrotliEncoderSetParameter(brotliState, BROTLI_PARAM_SIZE_HINT, len_in);
    BrotliEncoderSetParameter(brotliState, BROTLI_PARAM_QUALITY, BROTLI_QUALITY);
    if (BROTLI_LGWIN > BROTLI_MAX_WINDOW_BITS) {
        // Large window not compatible with RFC7932
        BrotliEncoderSetParameter(brotliState, BROTLI_PARAM_LGWIN, BROTLI_MAX_WINDOW_BITS);
    } else {
        BrotliEncoderSetParameter(brotliState, BROTLI_PARAM_LGWIN, BROTLI_LGWIN);
    }

    size_t available_in = len_in;
    size_t total_out = 0;

    for (;;) {
        const uint8_t* next_in = in + len_in - available_in;
        uint8_t* next_out = NULL;
        if (!CBB_reserve(out, &next_out, COMPRESS_CHUNK_SIZE)) {
            fputs("Unable to reserve space for compressed data\n", stderr);
        }
        size_t available_out = COMPRESS_CHUNK_SIZE;
        if (!BrotliEncoderCompressStream(brotliState,
            BROTLI_OPERATION_FINISH,
            &available_in, &next_in,
            &available_out, &next_out, &total_out)) {

            fputs("Unable to compress certificate\n", stderr);
            BrotliEncoderDestroyInstance(brotliState);
            return 0;
        }
        CBB_did_write(out, COMPRESS_CHUNK_SIZE - available_out);
        if (BrotliEncoderIsFinished(brotliState)) {
            BrotliEncoderDestroyInstance(brotliState);
            printf("Certificate compressed. [%zu->%zu] %f%%\n", len_in, total_out, (100.0 * total_out) / len_in);
            return 1;
        }
    }   
}

int decompress_cert(SSL* ssl, CRYPTO_BUFFER** out, size_t uncompressed_len, 
                            const uint8_t* in, size_t in_len)
{
    // Implementation
    printf("Decompress called\n");
    return 0;
}

void configure_context(SSL_CTX* ctx)
{
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    
    int compression_result = SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_brotli, 
                                                            compress_cert, decompress_cert);
    printf("Certificate compression setup result: %d (1=success, 0=failure)\n", compression_result);
    
    if (compression_result != 1) {
        ERR_print_errors_fp(stderr);
        printf("Failed to set up certificate compression. Error code: %u\n", ERR_get_error());
    }
    
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
        socklen_t len = sizeof(addr);  // Use socklen_t instead of uint
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