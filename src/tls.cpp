#include <felspar/io/tls.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>


/// ## `felspar::io::tls::impl`


struct felspar::io::tls::impl {
    impl() : ctx{SSL_CTX_new(TLS_method())}, ssl{SSL_new(ctx)} {
        /// TODO There should be some error handling here
        BIO_new_bio_pair(&ib, 0, &nb, 0);
        /// Give the internal BIO to `ssl`
        SSL_set_bio(ssl, ib, ib);
    }
    ~impl() {
        if (nb) { BIO_free(nb); }
        if (ssl) { SSL_free(ssl); }
        if (ctx) { SSL_CTX_free(ctx); }
    }
    SSL_CTX *ctx = nullptr;
    SSL *ssl = nullptr;
    /// `ib` is the internal BIO and `nb` is the network BIO
    BIO *ib = nullptr, *nb = nullptr;
    posix::fd fd;
};


/// ## `felspar::io::tls`


felspar::io::tls::tls() = default;
felspar::io::tls::tls(tls &&) = default;
felspar::io::tls::~tls() = default;
felspar::io::tls &felspar::io::tls::operator=(tls &&) = default;


auto felspar::io::tls::connect(
        io::warden &warden,
        sockaddr const *addr,
        socklen_t addrlen,
        std::optional<std::chrono::nanoseconds> timeout,
        felspar::source_location const &loc) -> warden::task<tls> {
    posix::fd fd = warden.create_socket(AF_INET, SOCK_STREAM, 0);
    co_await warden.connect(fd, addr, addrlen, timeout, loc);

    auto i = std::make_unique<impl>();

    co_return {};
}
