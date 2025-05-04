#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <brotli/decode.h>

char* hostname = "server.waldron.co.za";
int port = 4433;

// Define this if it's not available in your BoringSSL version
#ifndef TLSEXT_cert_compression_brotli
#define TLSEXT_cert_compression_brotli 3
#endif

SSL_CTX* create_context()
{
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    // Use SSLv23_client_method() instead of TLS_client_method() for older BoringSSL
    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    
    if (!ctx) {
	    perror("Unable to create SSL context");
	    ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }

    return ctx;
}

int compress_cert(SSL* ssl, CBB* out, const uint8_t* in, size_t in_len)
{
    // Implementation
    printf("Compress called\n");
    return 0;
}

int decompress_brotli_cert(SSL* ssl, CRYPTO_BUFFER** out, size_t uncompressed_len, 
                            const uint8_t* in, size_t in_len)
{
    uint8_t* data;
    if (!((*out) = CRYPTO_BUFFER_alloc(&data, uncompressed_len))) {
        fprintf(stderr, "Unable to allocate decompression buffer [%zu].", uncompressed_len);
        return 0;
    }

    // Write compressed certificate data to file.
    FILE* cert_data_fd;
    cert_data_fd = fopen("comp_cert_data_zstd.bin","wb");
    fwrite(in, in_len, 1, cert_data_fd);
    fclose(cert_data_fd);

    size_t output_size = uncompressed_len;
    if (BrotliDecoderDecompress(in_len, in, &output_size, data) !=
            BROTLI_DECODER_RESULT_SUCCESS || output_size != uncompressed_len) {
        fprintf(stderr, "Unexpected length of decompressed data or failure to decompress. [%zu/%zu]" , output_size, uncompressed_len);
        return 0;
    }

    printf("Certificate decompressed successfully. [%zu->%zu] %f%%\n", in_len, uncompressed_len, (100.0 * in_len) / uncompressed_len);
    return 1;
}

void configure_context(SSL_CTX* ctx)
{
    // Use SSL_CTX_set_options instead of SSL_CTX_set_min_proto_version for older BoringSSL
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    
    SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_brotli, compress_cert, decompress_brotli_cert);
    
    SSL_CTX_set_ecdh_auto(ctx, 1);

    // Set CA cert to verify with
    if(!SSL_CTX_load_verify_locations(ctx, "./certs/ca-cert.pem", NULL)) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }
}

int create_socket() {
    int sockfd;
    char* tmp_ptr = NULL;
    struct hostent* host;
    struct sockaddr_in dest_addr;

    if ((host = gethostbyname(hostname)) == NULL) {
        fprintf(stderr, "Cannot resolve hostname %s.\n", hostname);
        abort();
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    // Use h_addr_list[0] instead of h_addr (which is deprecated)
    dest_addr.sin_addr.s_addr = *(long*)(host->h_addr_list[0]);

    memset(&(dest_addr.sin_zero), '\0', 8);
    tmp_ptr = inet_ntoa(dest_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*) &dest_addr, sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Cannot connect to host %s [%s] on port %d.\n", hostname, tmp_ptr, port);
    }

    return sockfd;
}

int main(int arc, char** argv)
{
    SSL_CTX* ctx;
    SSL* ssl;
    int to_server = 0;

    if (arc > 1) {
        hostname = argv[1];
    }
    if (arc > 2) {
        port = atoi(argv[2]);
    }

    ctx = create_context();
    configure_context(ctx);
    ssl = SSL_new(ctx);
    to_server = create_socket();

    SSL_set_fd(ssl, to_server);

    if (SSL_connect(ssl) != 1)
        fprintf(stderr, "Cannot open TLS session to host %s:%d.\n", hostname, port);
    else
        printf("Successfully opened TLS session to host %s:%d.\n", hostname, port);

    SSL_free(ssl);
    close(to_server);
    SSL_CTX_free(ctx);    
}