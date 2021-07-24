#include <sys/socket.h>
#include <openssl/ssl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <unistd.h>
#include <string.h>

#define HOST "server.waldron.co.za"
#define PORT 4433

static SSL_CTX* create_context()
{
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = TLS_client_method();
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

    // Set CA cert to verify with
    if(!SSL_CTX_load_verify_locations(ctx, "./certs/ca-cert.pem", NULL)) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }
}

int create_socket() {
    int sockfd;
    char* tmp_ptr = NULL;
    struct  hostent* host;
    struct  sockaddr_in dest_addr;

    if ( (host = gethostbyname(HOST)) == NULL ) {
        printf("Cannot resolve hostname %s.\n",  HOST);
        abort();
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    dest_addr.sin_family=AF_INET;
    dest_addr.sin_port=htons(PORT);
    dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);;

    memset(&(dest_addr.sin_zero), '\0', 8);
    tmp_ptr = inet_ntoa(dest_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) == -1) {
        printf("Cannot connect to host %s [%s] on port %d.\n", HOST, tmp_ptr, PORT);
    }

    return sockfd;
}

int main(int arc, char **argv)
{
    SSL_CTX* ctx;
    SSL* ssl;
    int to_server = 0;

    ctx = create_context();
    configure_context(ctx);
    ssl = SSL_new(ctx);
    to_server = create_socket();

    SSL_set_fd(ssl, to_server);

    if ( SSL_connect(ssl) != 1 )
        printf("Cannot open TLS session to host %s.\n", HOST);
    else
        printf("Successfully opened TLS session to host %s.\n", HOST);

    SSL_free(ssl);
    close(to_server);
    SSL_CTX_free(ctx);    
}