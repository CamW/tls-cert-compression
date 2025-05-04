CC=clang
CFLAGS=-Wall -g -std=c18 -Werror -fno-omit-frame-pointer -fvisibility=hidden -Wunused -Wno-unused-value -O3
INCLUDES=-I./third_party/boringssl/include -I./third_party/brotli/c/include

# BoringSSL libraries - updated with correct paths
SSL_LIB=./third_party/boringssl/build/libssl.a
CRYPTO_LIB=./third_party/boringssl/build/libcrypto.a

# Brotli path for shared libraries
BROTLI_PATH=-L./third_party/brotli/build

# Brotli shared libraries
BROTLI_LIBS=-lbrotlienc -lbrotlidec -lbrotlicommon

# System libraries
SYS_LIBS=-lpthread -ldl -lz

all: clean build_server build_client

build_server:
	@echo "Building server..."
	$(CC) $(CFLAGS) $(INCLUDES) -o tls_server tls_server.c $(SSL_LIB) $(CRYPTO_LIB) $(BROTLI_PATH) $(BROTLI_LIBS) $(SYS_LIBS)

build_client:
	@echo "Building client..."
	$(CC) $(CFLAGS) $(INCLUDES) -o tls_client tls_client.c $(SSL_LIB) $(CRYPTO_LIB) $(BROTLI_PATH) $(BROTLI_LIBS) $(SYS_LIBS)

# Build with verbose output from the linker
build_server_verbose:
	$(CC) $(CFLAGS) $(INCLUDES) -o tls_server tls_server.c $(SSL_LIB) $(CRYPTO_LIB) $(BROTLI_PATH) $(BROTLI_LIBS) $(SYS_LIBS) -v

clean:
	rm -f tls_server
	rm -f tls_client

# Verify libraries exist
verify:
	@echo "Checking if all required libraries exist..."
	@test -f $(SSL_LIB) || echo "ERROR: $(SSL_LIB) not found"
	@test -f $(CRYPTO_LIB) || echo "ERROR: $(CRYPTO_LIB) not found"
	@test -f ./third_party/brotli/build/libbrotlienc.so || echo "ERROR: libbrotlienc.so not found"
	@test -f ./third_party/brotli/build/libbrotlidec.so || echo "ERROR: libbrotlidec.so not found"
	@test -f ./third_party/brotli/build/libbrotlicommon.so || echo "ERROR: libbrotlicommon.so not found"
	@test -f $(SSL_LIB) && test -f $(CRYPTO_LIB) && \
	 test -f ./third_party/brotli/build/libbrotlienc.so && \
	 test -f ./third_party/brotli/build/libbrotlidec.so && \
	 test -f ./third_party/brotli/build/libbrotlicommon.so && \
	 echo "All libraries found!"

# Run script to set LD_LIBRARY_PATH for testing
run_server:
	LD_LIBRARY_PATH=./third_party/brotli/build:$$LD_LIBRARY_PATH ./tls_server

run_client:
	LD_LIBRARY_PATH=./third_party/brotli/build:$$LD_LIBRARY_PATH ./tls_client