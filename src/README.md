# *felspar-io* Implementation

The contains three implementations:

* POSIX using `poll`
* Windows using `WSAPoll`
* Linux's io_uring

The `poll` and `WSAPoll` are similar, but not identical. The main part of the implementation is in:

* [`poll.hpp`](./poll.hpp) -- Common header containing completion tracking and common retry and cancellation code.
* [`poll.iops.cpp`](./poll.iops.cpp) -- Implementation of the individual IOP APIs.
* [`poll.warden.cpp`](./poll.warden.cpp) -- Implementation of the poll loop itself together with other code needed to have everything work.

The differences between `poll` and `WSAPoll` are handled through `#if` blocks.

There is a separate implementation `io_uring` in the following files:

* [`uring.hpp`](./uring.hpp) -- IOP delivery and completion header file.
* [`uring.iops.cpp`](./uring.iops.cpp) -- Implementation of the various IOPs.
* [`uring.warden.cpp`](./uring.warden.cpp) -- The io_uring submission and completion loop.


## Other files

* [`convenience.cpp`](./convenience.cpp) -- Contains a few helpers.
* [`posix.cpp`](./posix.cpp) -- Contains wrappers for some common POSIX APIs.
* [`tls.cpp`](./tls.cpp) -- Contains an implementation of TLS using OpenSSL.
* [`warden.cpp`](./warden.cpp) -- Common warden code (creating sockets and pipes).
