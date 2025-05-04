# TLS Certificate Compression

This project demonstrates an implementation of TLS Certificate Compression using Brotli, following [RFC 8879](https://www.rfc-editor.org/rfc/rfc8879.html). Certificate compression reduces the amount of data exchanged during TLS handshakes, improving performance and reducing latency.

This implementation uses:
- **BoringSSL** - Google's fork of OpenSSL for TLS functionality
- **Brotli** - A compression algorithm developed by Google that provides good compression ratios

## Building

This project requires the following dependencies:
- BoringSSL (as a Git submodule)
- Brotli (as a Git submodule)
- A C compiler with C18 support
- A C++ compiler with C++17 support (for linking with BoringSSL)

### Setup and build:

```bash
# Clone the repository with submodules
git clone --recursive git@github.com:CamW/tls-cert-compression.git
cd tls-cert-compression

# Build BoringSSL
cd third_party/boringssl
mkdir -p build
cd build
cmake ..
make
cd ../../..

# Build Brotli
cd third_party/brotli
mkdir -p build
cd build
cmake ..
make
cd ../../..

# Build the project
chmod +x compile-script.sh
./compile-script.sh
```

## Usage

### Server:

```bash
LD_LIBRARY_PATH=./third_party/brotli/build:$LD_LIBRARY_PATH ./tls_server
```

The server listens on port 4433 by default and will use certificate compression if negotiated with the client.

### Client:

```bash
LD_LIBRARY_PATH=./third_party/brotli/build:$LD_LIBRARY_PATH ./tls_client [hostname] [port]
```

The client connects to `server.waldron.co.za:4433` by default if no hostname and port are specified. 

**Important:** You'll need to add an entry for `server.waldron.co.za` in your `/etc/hosts` file (or equivalent) pointing to your local machine (127.0.0.1) for the hostname resolution to work properly:

```
# Add to /etc/hosts
127.0.0.1 server.waldron.co.za
```

## How It Works

1. The client advertises support for certificate compression in its ClientHello message (extension type 27)
2. If the server supports the same compression algorithm, it compresses its certificate
3. The server sends a CompressedCertificate message instead of a standard Certificate message
4. The client decompresses the certificate and proceeds with the TLS handshake

Unlike many TLS extensions, certificate compression is unidirectional and doesn't require the server to acknowledge support in the ServerHello message.

## Testing

To verify that certificate compression is working:

1. Run the server and client
2. Check the server output for a message like: `Certificate compressed. [835->728] 87.185629%`
3. Check the client output for a message like: `Certificate decompressed successfully. [728->835] 87.185629%`

You can also capture the TLS traffic using Wireshark and verify the presence of extension type 27 in the ClientHello message.

## References

- [RFC 8879: TLS Certificate Compression](https://www.rfc-editor.org/rfc/rfc8879.html)
- [BoringSSL](https://boringssl.googlesource.com/boringssl/)
- [Brotli Compression](https://github.com/google/brotli)