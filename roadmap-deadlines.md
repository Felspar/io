# Roadmap: Deadline-based lower-level IO APIs

## Context
Every end-user API currently takes a `std::optional<std::chrono::nanoseconds>` timeout, and composed operations (e.g. `read_exactly`, `write_all`, `connect`-by-hostname, `tls::connect`) pass that same timeout to each sub-operation — so the effective timeout resets on every retry and is effectively unbounded. We will give every timed API both a **deadline** overload and a **timeout** overload. The **deadline** overload is the canonical one: it takes `std::optional<std::chrono::steady_clock::time_point>` and carries the `= {}` default (so a no-argument call means "no deadline / run forever"). The **timeout** overload takes a plain `std::chrono::nanoseconds` (no `std::optional`, no default — you only pass a timeout when you actually have one), converts it to a deadline *immediately* on entry, and delegates. A single deadline is then threaded through composed operations so the user-supplied timeout bounds the whole operation.

---

## Phase 1: Deadline foundation and warden core — the warden speaks deadlines internally, timeout overloads delegate, all existing behaviour preserved

### Chunk 1.1: Introduce the `deadline` type and conversion helper

**Tests:**
* [x] Add `test/headers/deadline.cpp` (single `#include`) to the header-compile test list in `test/headers/CMakeLists.txt`.
* [x] Unit test (new `test/run/deadline.cpp`, registered in `test/run/CMakeLists.txt`): `deadline_from(20ms)` returns a time point in `[now+19ms, now+21ms]` (mirrors the jitter tolerance style in `test/run/timers.cpp`).

**Implementation:**
* [x] New `include/felspar/io/deadline.hpp`: `using deadline = std::chrono::steady_clock::time_point;` and an inline `deadline deadline_from(std::chrono::nanoseconds timeout)` that returns `now() + timeout`. The helper takes a plain duration — absence ("no timeout") is never converted; it is represented by an empty `std::optional<deadline>` at the call site instead.
* [x] Include `deadline.hpp` from `include/felspar/io/warden.hpp` so the type is visible everywhere.
* [x] Add `deadline.hpp` to `include/felspar/io.hpp`.

---

### Chunk 1.2: Convert the `do_*` IOP virtuals and completions to deadlines

**Tests:**
* [x] Existing timeout regression tests stay green: `timers` (`write/poll`, `accept/poll`, and the io_uring variants) and `connect` in `test/run/timers.cpp` / `test/run/timers.connect.cpp`.
* [x] Existing `test/run/cancel.cpp` stays green (verifies IOP destruction/cancel still cancels timeouts).

**Implementation:**
* [x] In `include/felspar/io/warden.hpp` change the protected virtuals `do_read_some`, `do_write_some`, `do_accept`, `do_connect`, `do_read_ready`, `do_write_ready` to take `std::optional<deadline>` instead of `std::optional<std::chrono::nanoseconds>`. Keep `do_close` (no timeout) and `do_sleep` (handled in 1.4) unchanged.
* [x] Update the public warden timeout methods to convert inline before calling the `do_*` virtual (no new public overload yet). These methods still take `std::optional<std::chrono::nanoseconds>` at this stage, so guard the conversion: `timeout ? std::optional<deadline>{deadline_from(*timeout)} : std::nullopt`. (Once Chunk 1.3 makes the timeout overload a plain `std::chrono::nanoseconds`, this collapses to a bare `deadline_from(timeout)`.)
* [x] `src/poll.hpp`: rename the completion's `timeout` field to `deadline` (type `std::optional<deadline>`); `insert_timeout()` inserts the stored deadline directly into `self->timeouts` (drop the `now() + *timeout` calculation — the multimap is already deadline-keyed).
* [x] `src/uring.hpp`: store `std::optional<deadline>` in both `completion<R>` and `completion<void>`; in `setup_timeout()` compute the link-timeout `kts` from `*deadline - std::chrono::steady_clock::now()` (clamp negative to zero so an already-passed deadline fires immediately).
* [x] Update completion constructors/`do_*` definitions in `src/poll.iops.cpp` and `src/uring.iops.cpp` to pass the deadline through; `sleep_completion` constructs its base with `now() + ns` for now; `close_completion` keeps `{}`.
* [x] Update the forwarding overrides in `include/felspar/io/allocator.hpp` to the new `std::optional<deadline>` signatures.
* [x] Update the override declarations in `include/felspar/io/warden.poll.hpp` and `include/felspar/io/warden.uring.hpp`.

---

### Chunk 1.3: Add deadline overloads to the public warden IOP methods

**Tests:**
* [ ] New test (extend `test/run/timers.cpp`): `co_await ward.accept(fd, deadline)` with a deadline a few ms in the future throws `felspar::io::timeout`, for both `poll_warden` and `uring_warden`.
* [ ] New test: a deadline already in the past (`steady_clock::now() - 1ms`) times out essentially immediately.

**Implementation:**
* [ ] In `include/felspar/io/warden.hpp`, for `read_some`, `write_some`, `accept`, `connect`, `read_ready`, `write_ready` (both the `socket_descriptor` and `posix::fd` variants), make the **deadline** overload canonical: `std::optional<deadline> = {}`, carrying the default. Calling the `do_*` virtual happens here.
* [ ] Change each **timeout** overload to take a plain `std::chrono::nanoseconds` (drop the `std::optional` and the default); its body is `return <name>(fd, …, deadline_from(timeout), loc);`.
* [ ] Fix the one public call site that passes an explicit `{}` ahead of a positional `source_location`: in `src/convenience.cpp` the streaming `accept` does `ward.accept(fd, {}, loc)` — change `{}` to an explicitly-typed `std::optional<deadline>{}` so it binds the deadline overload unambiguously (semantics unchanged — still "no timeout"). The internal `completion<void>{s, {}, loc}` constructors are single-type and need no change.

---

### Chunk 1.4: `sleep_until` deadline overload

**Tests:**
* [ ] New test (extend `test/run/timers.cpp`): `sleep_until(now() + 20ms)` sleeps roughly 20ms (same jitter window as the existing `short_sleep` test), for both wardens.

**Implementation:**
* [ ] In `include/felspar/io/warden.hpp` add `sleep_until(deadline, std::source_location)`; change `do_sleep` to take a `deadline`.
* [ ] Make `sleep(std::chrono::nanoseconds ns)` compute `now() + ns` and delegate to `sleep_until`.
* [ ] Update `do_sleep` in `warden.poll.hpp`/`warden.uring.hpp`/`allocator.hpp` and the definitions in `src/poll.iops.cpp` (store deadline directly) and `src/uring.iops.cpp` (`io_uring_prep_timeout` from `deadline - now()`).

---

## Phase 2: Deadline-consistent end-user APIs — composed operations bound the whole operation by a single deadline

### Chunk 2.1: `read.hpp` deadline overloads and composed threading

**Tests:**
* [ ] New test (`test/run/`): a `read_exactly` that requires several partial reads from a slow/loopback pipe (pattern from `accept_writer`/`write_forever` in `test/run/timers.cpp`) with a single timeout times out after ≈ that timeout total — not N × timeout.
* [ ] `read_until_lf_strip_cr` with a deadline times out across multiple `do_read_some` calls.

**Implementation:**
* [ ] In `include/felspar/io/read.hpp` add `std::optional<deadline>` overloads of the free `read_some`, `read_exactly` (all three signatures), `read_buffer::do_read_some`, and `read_until_lf_strip_cr`.
* [ ] Each timeout overload converts once via `deadline_from` and delegates; the deadline overloads loop passing the *same* deadline to every inner call.

---

### Chunk 2.2: `write.hpp` deadline overloads and composed threading

**Tests:**
* [ ] New test: `write_all` to a blocked socket (extend the `write_forever` setup in `test/run/timers.cpp`) with one timeout bounds the entire write loop, not each `write_some`.

**Implementation:**
* [ ] In `include/felspar/io/write.hpp` add `std::optional<deadline>` overloads of the free `write_some` and all `write_all` signatures; timeout overloads convert once and delegate; `write_all`'s deadline overload threads the single deadline through the loop.

---

### Chunk 2.3: `connect.hpp` deadline overloads and composed threading

**Tests:**
* [ ] New test: `connect`-by-hostname against a host resolving to multiple addresses with a short deadline fails within ≈ the deadline overall (mirrors `test/run/timers.connect.cpp`), rather than resetting the timeout per address.

**Implementation:**
* [ ] In `include/felspar/io/connect.hpp` add `std::optional<deadline>` overloads of the templated `connect(sock, addr, addrlen, ...)` and the `connect(hostname, port, ...)` declaration.
* [ ] In `src/convenience.cpp` implement the `connect`-by-hostname deadline overload so the deadline is computed once and shared across the `addrinfo` attempt loop; the timeout overload delegates.

---

### Chunk 2.4: `tls.hpp` deadline overloads and composed threading

**Tests:**
* [ ] Header test `test/headers/tls.cpp` still compiles with the new overloads.
* [ ] Existing `test/run/tls.tests.cpp` stays green; if it already exercises a timeout, add a deadline-overload variant.

**Implementation:**
* [ ] In `include/felspar/io/tls.hpp` add `std::optional<deadline>` overloads of `tls::connect` (both forms), `tls::read_some`, `tls::write_some`, and the free-standing `read_some`/`write_some` wrappers.
* [ ] In `src/tls.cpp` thread a single deadline through `service_operation`, `bio_read`, `bio_write`, and the `connect`-by-hostname `addrinfo` loop; timeout overloads convert once and delegate.

---

## Notes
- **Backward compatibility is a hard constraint.** All of `examples/` and the existing test suite must compile unchanged. This holds because the **deadline** overload now carries the `= {}` default, so a no-argument call (e.g. `ward.connect(fd, addr, len)`) binds it with `nullopt` — exactly the old "no timeout" meaning. A call with a duration (e.g. `ward.accept(fd, 10ms)`) binds the **timeout** overload, and a call with a time point binds the deadline overload; `nanoseconds` and `steady_clock::time_point` are unrelated types so these never collide.
- **The one ambiguous shape** is an explicit braced `{}` argument followed by a positional later argument: `{}` is viable for both `std::chrono::nanoseconds` (→ `0ns`) and `std::optional<deadline>` (→ `nullopt`). The only such public call site is the streaming `accept` in `src/convenience.cpp`; it is fixed in Chunk 1.3 by passing an explicitly-typed `std::optional<deadline>{}`. New code should avoid bare `{}` in the timeout/deadline slot.
- **Why the timeout overload drops `std::optional`.** "No timeout" is now expressed by calling the deadline overload with no argument, so the timeout overload never needs to represent absence — a plain `std::chrono::nanoseconds` is sufficient and clearer at call sites.
- **Conversion happens once, at entry.** Every timeout overload's first action is `deadline_from(timeout)`; it never passes a raw timeout downward. This is the single behavioural guarantee that makes composed timeouts consistent.
- **Phase ordering.** Phase 1 is a behaviour-preserving internal refactor (shippable on its own — the warden simply speaks deadlines). The observable behavioural change lands in Phase 2, where composed operations stop resetting the timeout per sub-call. The Phase 2 chunks are mutually independent and can ship individually.
- **uring timeouts** are recomputed as a relative `deadline - now()` timespec at submission time (clamped at zero), preserving current `io_uring_prep_link_timeout` semantics; switching to `IORING_TIMEOUT_ABS` against `CLOCK_MONOTONIC` is a possible later refinement but is out of scope here.
- **Streaming `accept`** (`warden::stream<socket_descriptor>` in `accept.hpp`/`convenience.cpp`) is intentionally left timeout-free: a single deadline doesn't fit a long-lived generator, and a per-accept timeout is not a deadline. Single-shot `accept` on the warden gains both overloads in Phase 1.
- The `allocator` warden (`include/felspar/io/allocator.hpp`) forwards every `do_*`, so it must be updated in lock-step whenever a virtual signature changes (Chunks 1.2 and 1.4).
- Network-dependent tests (Chunks 2.3/2.4) will be as flaky as the existing `timers.connect.cpp`; prefer the deterministic loopback/pipe patterns from `timers.cpp` for the bounded-deadline assertions where possible.
