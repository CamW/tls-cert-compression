CC=gcc
CFLAGS=-Wall -g -std=c18 #-v
#LDFLAGS="-Wl,-rpath,/usr/local/lib"
#LDFLAGS=-L/usr/local/Cellar/openssl@1.1/1.1.1k/lib/ -I/usr/local/Cellar/openssl@1.1/1.1.1k/include/ -lssl -lcrypto
LDFLAGS=-L./third_party/boringssl/src/build/ssl -L./third_party/boringssl/src/build/crypto -I./third_party/boringssl/src/include/ -lssl -lcrypto -lz

all: clean build_server build_client

build_server:
	$(CC) $(CFLAGS) $(LDFLAGS) -o tls_server tls_server.c

build_client:
	$(CC) $(CFLAGS) $(LDFLAGS) -o tls_client tls_client.c

clean:
	rm -f tls_server
	rm -f tls_client