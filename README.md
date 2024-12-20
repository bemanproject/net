# beman.net29: Senders For Network Operations

<img src="https://github.com/bemanproject/beman/blob/main/images/logos/beman_logo-beman_library_under_development.png" style="width:5%; height:auto;">

`beman.net29` provides senders for asynchronous network operations.
It is based on [P2762R2](https://wg21.link/P2762R2). Both the proposal
and the implementation are in an early state although basic
functionality is provided.

`beman.net29` builds on top of [`beman.execution26`](https://github.com/bemanproject/execution26). The
implementation should allow multiple ways how to support asynchronous
operations although currently the only implemented approach is based
on `poll(2)`. Implementations using `kqueue(2)`, `epoll(2)`,
`io_uring`, and IOCP hopefully follow soon. It should also be
possible use various libraries for asynchronous operations like
`libevent`, `libuv`, etc.

**Implements**: [Sender/Receiver Interface for Networking (P2762R2)](https://wg21.link/P2762R2)

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/BEMAN_LIBRARY_MATURITY_MODEL.md#under-development-and-not-yet-ready-for-production-use)

## Help Welcome!

There are plenty of things which need to be done. Some of these
are listed in the [issues section](https://github.com/bemanproject/net29/issues).
Other contributions are, of course, also welcome. Aside from contributing
code the project welcomes reports on usage experience, code reviews, defect
reports, features requests, etc. There are plenty contribution opportunities.

If you want to contribute, you won't be left alone! Please e-mail
[Dietmar Kühl](mailto:dietmar.kuehl@me.com) if want help or mentoring
for contributing to the library.

## State of the Proposal

At the February 2024/Tokyo meeting the Networking SG4 discussion
recommended _not_ to provide a low-level interface for networking!
Instead, a [IETF TAPS](https://datatracker.ietf.org/wg/taps/documents/)
based implementation should be used. As a result, the shape of the
interface will be different. However, even if the public interface
is using [IETF TAPS](https://datatracker.ietf.org/wg/taps/documents/)
the implementation will use a lower-level interface which can likely
use something akin to the current interface.

## Building

Currently, the interfaces are entirely implemented in headers, i.e.,
there isn't a library needed. However, using the library `libbeman_net29`
should make the relevant paths to headers available.  The following
instructions build the library and the examples (required libraries are
obtained and build using these instructions):

    cmake -B build
    cmake --build build

The code is working on MacOS building with the system's
[`clang`](https://clang.llvm.org/) and recent [`gcc`](http://gcc.gnu.org/).
It should hopefully also work on Linux but it isn't tested, yet.
Windows isn't supported, yet, primarily because the relevant class
for the networking operations are different.