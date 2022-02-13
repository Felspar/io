# Felspar IO

**C++ coroutine based asynchronous IO**


**felspar-io** is a C++20 library to help you use coroutines to perform input and output operations (IOPs). *The library should not yet be considered stable*


## Using the library

The library is designed to be easy to use from CMake projects. To use it add it in your source tree somewhere and then use `add_subdirectory` to include it in your project. The library also requires the use of some other libraries which can get by using an `include`. So if you have the library in a folder called `felspar-io`, you can add the following to your `CMakeLists.txt`:

```cmake
add_subdirectory(felspar-io)
include(felspar-io/requirements.cmake)
```

To make use of the library add `felspar-io` in your `target_link_libraries` directive.

```cmake
target_link_libraries(your-app otherlib1 otherlib2 felspar-io)
```


## I/O functionality

To echo data received on a socket the following code works:

```cpp
#include <felspar/io.hpp>

felspar::coro::task<void> echo_connection(
        felspar::io::warden &ward, felspar::posix::fd sock) {
    std::array<std::byte, 256> buffer;
    while (auto bytes = co_await ward.read_some(sock, buffer, 20s)) {
        std::span writing{buffer};
        auto written = co_await felspar::io::write_all(
                ward, sock, writing.first(bytes), 20s);
    }
}
```

A coroutine for running an accept loop for an echo server would look like this:

```cpp
#include <felspar/coro/start.hpp>

felspar::coro::task<void>
        echo_server(felspar::io::warden &ward, std::uint16_t port) {
    auto fd = ward.create_socket(AF_INET, SOCK_STREAM, 0);
    felspar::posix::bind_to_any_address(fd, port);

    int constexpr backlog = 64;
    if (::listen(fd.native_handle(), backlog) == -1) {
        throw felspar::stdexcept::system_error{
                errno, std::generic_category(), "Calling listen"};
    }

    felspar::coro::starter<felspar::coro::task<void>> co;
    for (auto acceptor = felspar::io::accept(ward, fd);
            auto cnx = co_await acceptor.next();) {
        co.post(echo_connection, ward, felspar::posix::fd{*cnx});
        co.gc(); // garbage collect completed coroutines ignoring errors
    }
}
```

The library is built around the notion of "wardens". There is an abstract `felspar::io::warden` type that provides an API for various IOPs, and in the future, polymorphic allocation for memory required to execute the IOPs and coroutines that make use of them.
