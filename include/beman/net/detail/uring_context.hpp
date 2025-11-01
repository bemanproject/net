// include/beman/net/detail/uring_context.hpp                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_URING_CONTEXT
#define INCLUDED_BEMAN_NET_DETAIL_URING_CONTEXT

#include <beman/net/detail/container.hpp>
#include <beman/net/detail/context_base.hpp>

#include <cassert>
#include <cstdint>
#include <system_error>
#include <tuple>
#include <liburing.h>

namespace beman::net::detail {

// io_context implementation based on liburing
struct uring_context final : context_base {
    static constexpr unsigned     QUEUE_DEPTH = 128;
    ::io_uring                    ring;
    container<native_handle_type> sockets;
    task*                         tasks       = nullptr;
    ::std::size_t                 submitting  = 0; // sqes not yet submitted
    ::std::size_t                 outstanding = 0; // cqes expected

    uring_context() {
        int flags = 0;
        int r     = ::io_uring_queue_init(QUEUE_DEPTH, &ring, flags);
        if (r < 0) {
            throw ::std::system_error(-r, ::std::system_category(), "io_uring_queue_init failed");
        }
    }
    ~uring_context() override { ::io_uring_queue_exit(&ring); }

    auto make_socket(int fd) -> socket_id override { return sockets.insert(fd); }

    auto make_socket(int d, int t, int p, ::std::error_code& error) -> socket_id override {
        int fd(::socket(d, t, p));
        if (fd < 0) {
            error = ::std::error_code(errno, ::std::system_category());
            return socket_id::invalid;
        }
        return make_socket(fd);
    }

    auto release(socket_id id, ::std::error_code& error) -> void override {
        const native_handle_type handle = sockets[id];
        sockets.erase(id);
        if (::close(handle) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }

    auto native_handle(socket_id id) -> native_handle_type override { return sockets[id]; }

    auto set_option(socket_id id, int level, int name, const void* data, ::socklen_t size, ::std::error_code& error)
        -> void override {
        if (::setsockopt(native_handle(id), level, name, data, size) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }

    auto bind(socket_id id, const endpoint& ep, ::std::error_code& error) -> void override {
        if (::bind(native_handle(id), ep.data(), ep.size()) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }

    auto listen(socket_id id, int no, ::std::error_code& error) -> void override {
        if (::listen(native_handle(id), no) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }

    auto submit() -> void {
        int r = ::io_uring_submit(&ring);
        if (r < 0) {
            throw ::std::system_error(-r, ::std::system_category(), "io_uring_submit failed");
        }
        assert(submitting >= r);
        submitting -= r;
        outstanding += r;
    }

    auto get_sqe(io_base* completion) -> ::io_uring_sqe* {
        auto sqe = ::io_uring_get_sqe(&ring);
        while (sqe == nullptr) {
            // if the submission queue is full, flush and try again
            submit();
            sqe = ::io_uring_get_sqe(&ring);
        }
        ::io_uring_sqe_set_data(sqe, completion);
        ++submitting;
        return sqe;
    }

    auto wait() -> ::std::tuple<int, io_base*> {
        ::io_uring_cqe* cqe = nullptr;
        int             r   = ::io_uring_wait_cqe(&ring, &cqe);
        if (r < 0) {
            throw ::std::system_error(-r, ::std::system_category(), "io_uring_wait_cqe failed");
        }

        assert(outstanding > 0);
        --outstanding;

        const int  res        = cqe->res;
        const auto completion = ::io_uring_cqe_get_data(cqe);
        ::io_uring_cqe_seen(&ring, cqe);

        return {res, static_cast<io_base*>(completion)};
    }

    auto run_one() -> ::std::size_t override {
        if (auto count = process_task(); count) {
            return count;
        }

        if (submitting) {
            // if we have anything to submit, batch the submit and wait in a
            // single system call. this allows io_uring_wait_cqe() below to be
            // served directly from memory
            unsigned wait_nr = 1;
            int      r       = ::io_uring_submit_and_wait(&ring, wait_nr);
            if (r < 0) {
                throw ::std::system_error(-r, ::std::system_category(), "io_uring_submit_and_wait failed");
            }
            assert(submitting >= r);
            submitting -= r;
            outstanding += r;
        }

        if (!outstanding) {
            // nothing to submit and nothing to wait on, we're done
            return 0;
        }

        // read the next completion, waiting if necessary
        auto [res, completion] = wait();

        if (completion) {
            // work() functions depend on res, so pass it in via 'extra'
            completion->extra.reset(&res);
            completion->work(*this, completion);
        }

        return 1;
    }

    auto cancel(io_base* cancel_op, io_base* op) -> void override {
        auto sqe   = get_sqe(nullptr);
        int  flags = 0;
        ::io_uring_prep_cancel(sqe, op, flags);

        // use io_uring_prep_cancel() for asynchronous cancellation of op.
        // cancel_op, aka sender_state::cancel_callback, lives inside of op's
        // operation state. op's completion may race with this cancellation,
        // causing that sender_state and its cancel_callback to be destroyed.
        // so we can't pass cancel_op to io_uring_sqe_set_data() and attach a
        // cancel_op->work() function to handle its completion in run_one().
        // instead, we just complete it here without waiting for the result
        cancel_op->complete();
    }

    auto schedule(task* t) -> void override {
        t->next = tasks;
        tasks   = t;
    }

    auto process_task() -> ::std::size_t {
        if (tasks) {
            auto* t = tasks;
            tasks   = t->next;
            t->complete();
            return 1u;
        }
        return 0u;
    }

    auto accept(accept_operation* op) -> submit_result override {
        op->work = [](context_base& ctx, io_base* io) {
            auto res = *static_cast<int*>(io->extra.get());
            if (res == -ECANCELED) {
                io->cancel();
                return submit_result::ready;
            } else if (res < 0) {
                io->error(::std::error_code(-res, ::std::system_category()));
                return submit_result::error;
            }
            auto op = static_cast<accept_operation*>(io);
            // set socket
            ::std::get<2>(*op) = ctx.make_socket(res);
            io->complete();
            return submit_result::ready;
        };

        auto sqe     = get_sqe(op);
        auto fd      = native_handle(op->id);
        auto addr    = ::std::get<0>(*op).data();
        auto addrlen = &::std::get<1>(*op);
        int  flags   = 0;
        ::io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        return submit_result::submit;
    }

    auto connect(connect_operation* op) -> submit_result override {
        op->work = [](context_base&, io_base* io) {
            auto res = *static_cast<int*>(io->extra.get());
            if (res == -ECANCELED) {
                io->cancel();
                return submit_result::ready;
            } else if (res < 0) {
                io->error(::std::error_code(-res, ::std::system_category()));
                return submit_result::error;
            }
            io->complete();
            return submit_result::ready;
        };

        auto  sqe  = get_sqe(op);
        auto  fd   = native_handle(op->id);
        auto& addr = ::std::get<0>(*op);
        ::io_uring_prep_connect(sqe, fd, addr.data(), addr.size());
        return submit_result::submit;
    }

    auto receive(receive_operation* op) -> submit_result override {
        op->work = [](context_base&, io_base* io) {
            auto res = *static_cast<int*>(io->extra.get());
            if (res == -ECANCELED) {
                io->cancel();
                return submit_result::ready;
            } else if (res < 0) {
                io->error(::std::error_code(-res, ::std::system_category()));
                return submit_result::error;
            }
            auto op = static_cast<receive_operation*>(io);
            // set bytes received
            ::std::get<2>(*op) = res;
            io->complete();
            return submit_result::ready;
        };

        auto sqe   = get_sqe(op);
        auto fd    = native_handle(op->id);
        auto msg   = &::std::get<0>(*op);
        auto flags = ::std::get<1>(*op);
        ::io_uring_prep_recvmsg(sqe, fd, msg, flags);
        return submit_result::submit;
    }

    auto send(send_operation* op) -> submit_result override {
        op->work = [](context_base&, io_base* io) {
            auto res = *static_cast<int*>(io->extra.get());
            if (res == -ECANCELED) {
                io->cancel();
                return submit_result::ready;
            } else if (res < 0) {
                io->error(::std::error_code(-res, ::std::system_category()));
                return submit_result::error;
            }
            auto op = static_cast<send_operation*>(io);
            // set bytes sent
            ::std::get<2>(*op) = res;
            io->complete();
            return submit_result::ready;
        };

        auto sqe   = get_sqe(op);
        auto fd    = native_handle(op->id);
        auto msg   = &::std::get<0>(*op);
        auto flags = ::std::get<1>(*op);
        ::io_uring_prep_sendmsg(sqe, fd, msg, flags);
        return submit_result::submit;
    }

    static auto make_timespec(auto dur) -> __kernel_timespec {
        auto sec = ::std::chrono::duration_cast<::std::chrono::seconds>(dur);
        dur -= sec;
        auto              nsec = ::std::chrono::duration_cast<::std::chrono::nanoseconds>(dur);
        __kernel_timespec ts;
        ts.tv_sec  = sec.count();
        ts.tv_nsec = nsec.count();
        return ts;
    }

    auto resume_at(resume_at_operation* op) -> submit_result override {
        auto at  = ::std::get<0>(*op);
        op->work = [](context_base&, io_base* io) {
            auto res = *static_cast<int*>(io->extra.get());
            auto op  = static_cast<resume_at_operation*>(io);
            if (res == -ECANCELED) {
                io->cancel();
                return submit_result::ready;
            } else if (res == -ETIME) {
                io->complete();
                return submit_result::ready;
            }
            io->error(::std::error_code(-res, ::std::system_category()));
            return submit_result::error;
        };

        auto     sqe   = get_sqe(op);
        auto     ts    = make_timespec(at.time_since_epoch());
        unsigned count = 0;
        unsigned flags = IORING_TIMEOUT_ABS | IORING_TIMEOUT_REALTIME;
        ::io_uring_prep_timeout(sqe, &ts, count, flags);

        // unlike other operations whose submissions can be batched in run_one(),
        // the timeout argument to io_uring_prep_timeout() is a pointer to memory
        // on the stack. this memory must remain valid until submit, so we either
        // have to call submit here or allocate heap memory to store it
        submit();
        return submit_result::submit;
    }
};

} // namespace beman::net::detail

#endif
