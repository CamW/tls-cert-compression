#!/bin/bash
# Script to compile tls_server and tls_client with proper linking

# Exit on any error
set -e

echo "Starting the build process..."

# Define colors for better output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Step 1: Verify library paths
echo -e "${YELLOW}Step 1: Verifying library paths...${NC}"

SSL_LIB="./third_party/boringssl/build/libssl.a"
CRYPTO_LIB="./third_party/boringssl/build/libcrypto.a"
BROTLI_ENC="./third_party/brotli/build/libbrotlienc.so"
BROTLI_DEC="./third_party/brotli/build/libbrotlidec.so"
BROTLI_COMMON="./third_party/brotli/build/libbrotlicommon.so"

if [ -f "$SSL_LIB" ]; then
    echo -e "${GREEN}Found libssl.a${NC}"
else
    echo -e "${RED}Error: $SSL_LIB not found${NC}"
    exit 1
fi

if [ -f "$CRYPTO_LIB" ]; then
    echo -e "${GREEN}Found libcrypto.a${NC}"
else
    echo -e "${RED}Error: $CRYPTO_LIB not found${NC}"
    exit 1
fi

if [ -f "$BROTLI_ENC" ]; then
    echo -e "${GREEN}Found libbrotlienc.so${NC}"
else
    echo -e "${RED}Error: $BROTLI_ENC not found${NC}"
    exit 1
fi

if [ -f "$BROTLI_DEC" ]; then
    echo -e "${GREEN}Found libbrotlidec.so${NC}"
else
    echo -e "${RED}Error: $BROTLI_DEC not found${NC}"
    exit 1
fi

if [ -f "$BROTLI_COMMON" ]; then
    echo -e "${GREEN}Found libbrotlicommon.so${NC}"
else
    echo -e "${RED}Error: $BROTLI_COMMON not found${NC}"
    exit 1
fi

# Step 2: Clean previous builds
echo -e "${YELLOW}Step 2: Cleaning previous builds...${NC}"
rm -f tls_server tls_client

# Step 3: Compile server - NOTE: using proper linking method
echo -e "${YELLOW}Step 3: Compiling tls_server...${NC}"
# Compile to object file first
gcc -c -Wall -g -std=c18 -Werror -fno-omit-frame-pointer -fvisibility=hidden -Wunused -Wno-unused-value -O3 \
    -I./third_party/boringssl/include -I./third_party/brotli/c/include \
    -o tls_server.o tls_server.c

# Then link with all libraries
g++ -o tls_server tls_server.o $SSL_LIB $CRYPTO_LIB \
    -L./third_party/brotli/build -lbrotlienc -lbrotlidec -lbrotlicommon \
    -lpthread -ldl -lz -lstdc++

# Step 4: Compile client - using same approach
echo -e "${YELLOW}Step 4: Compiling tls_client...${NC}"
# Compile to object file first
gcc -c -Wall -g -std=c18 -Werror -fno-omit-frame-pointer -fvisibility=hidden -Wunused -Wno-unused-value -O3 \
    -I./third_party/boringssl/include -I./third_party/brotli/c/include \
    -o tls_client.o tls_client.c

# Then link with all libraries
g++ -o tls_client tls_client.o $SSL_LIB $CRYPTO_LIB \
    -L./third_party/brotli/build -lbrotlienc -lbrotlidec -lbrotlicommon \
    -lpthread -ldl -lz -lstdc++

# Step 5: Verify binaries were created
echo -e "${YELLOW}Step 5: Verifying binaries...${NC}"
if [ -f "tls_server" ] && [ -f "tls_client" ]; then
    echo -e "${GREEN}Build successful! Both tls_server and tls_client were built.${NC}"
    echo ""
    echo -e "${YELLOW}To run the server:${NC}"
    echo "LD_LIBRARY_PATH=./third_party/brotli/build:\$LD_LIBRARY_PATH ./tls_server"
    echo ""
    echo -e "${YELLOW}To run the client:${NC}"
    echo "LD_LIBRARY_PATH=./third_party/brotli/build:\$LD_LIBRARY_PATH ./tls_client"
else
    echo -e "${RED}Build failed! Check the errors above.${NC}"
    exit 1
fi

# Cleanup object files
rm -f tls_server.o tls_client.o