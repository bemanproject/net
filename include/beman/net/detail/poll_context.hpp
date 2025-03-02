// include/beman/net/detail/poll_context.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_POLL_CONTEXT
#define INCLUDED_BEMAN_NET_DETAIL_POLL_CONTEXT

// ----------------------------------------------------------------------------

#include <beman/net/detail/netfwd.hpp>
#include <beman/net/detail/container.hpp>
#include <beman/net/detail/context_base.hpp>
#include <beman/net/detail/sorted_list.hpp>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct poll_record;
struct poll_context;
} // namespace beman::net::detail

// ----------------------------------------------------------------------------

struct beman::net::detail::poll_record final {
    poll_record(::beman::net::detail::native_handle_type h) : handle(h) {}
    ::beman::net::detail::native_handle_type handle;
    bool                                     blocking{true};
};

// ----------------------------------------------------------------------------

struct beman::net::detail::poll_context final : ::beman::net::detail::context_base {
    using time_t       = ::std::chrono::system_clock::time_point;
    using timer_node_t = ::beman::net::detail::context_base::resume_at_operation;
    struct get_time {
        auto operator()(auto* t) const -> time_t { return ::std::get<0>(*t); }
    };
    using timer_priority_t = ::beman::net::detail::sorted_list<timer_node_t, ::std::less<>, get_time>;
    ::beman::net::detail::container<::beman::net::detail::poll_record> d_sockets;
    ::std::vector<::pollfd>                                            d_poll;
    ::std::vector<::beman::net::detail::io_base*>                      d_outstanding;
    timer_priority_t                                                   d_timeouts;
    ::beman::net::detail::context_base::task*                          d_tasks{};

    auto make_socket(int fd) -> ::beman::net::detail::socket_id override final { return this->d_sockets.insert(fd); }
    auto make_socket(int d, int t, int p, ::std::error_code& error) -> ::beman::net::detail::socket_id override final {
        int fd(::socket(d, t, p));
        if (fd < 0) {
            error = ::std::error_code(errno, ::std::system_category());
            return ::beman::net::detail::socket_id::invalid;
        }
        return this->make_socket(fd);
    }
    auto release(::beman::net::detail::socket_id id, ::std::error_code& error) -> void override final {
        ::beman::net::detail::native_handle_type handle(this->d_sockets[id].handle);
        this->d_sockets.erase(id);
        if (::close(handle) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto native_handle(::beman::net::detail::socket_id id) -> ::beman::net::detail::native_handle_type override final {
        return this->d_sockets[id].handle;
    }
    auto set_option(::beman::net::detail::socket_id id,
                    int                             level,
                    int                             name,
                    const void*                     data,
                    ::socklen_t                     size,
                    ::std::error_code&              error) -> void override final {
        if (::setsockopt(this->native_handle(id), level, name, data, size) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto bind(::beman::net::detail::socket_id       id,
              const ::beman::net::detail::endpoint& endpoint,
              ::std::error_code&                    error) -> void override final {
        if (::bind(this->native_handle(id), endpoint.data(), endpoint.size()) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }
    auto listen(::beman::net::detail::socket_id id, int no, ::std::error_code& error) -> void override final {
        if (::listen(this->native_handle(id), no) < 0) {
            error = ::std::error_code(errno, ::std::system_category());
        }
    }

    auto process_task() -> ::std::size_t {
        if (this->d_tasks) {
            auto* tsk{this->d_tasks};
            this->d_tasks = tsk->next;
            tsk->complete();
            return 1u;
        }
        return 0u;
    }
    auto process_timeout(const auto& now) -> ::std::size_t {
        if (!this->d_timeouts.empty() && ::std::get<0>(*this->d_timeouts.front()) <= now) {
            this->d_timeouts.pop_front()->complete();
            return 1u;
        }
        return 0u;
    }
    auto remove_outstanding(::std::size_t i) {
        if (i + 1u != this->d_poll.size()) {
            this->d_poll[i]        = this->d_poll.back();
            this->d_outstanding[i] = this->d_outstanding.back();
        }
        this->d_poll.pop_back();
        this->d_outstanding.pop_back();
    }
    auto to_milliseconds(auto duration) -> int {
        return int(::std::chrono::duration_cast<::std::chrono::milliseconds>(duration).count());
    }
    auto run_one() -> ::std::size_t override final {
        auto now{::std::chrono::system_clock::now()};
        if (0u < this->process_timeout(now) || 0 < this->process_task()) {
            return 1u;
        }
        if (this->d_poll.empty() && this->d_timeouts.empty()) {
            return ::std::size_t{};
        }
        while (true) {
            auto   next_time{this->d_timeouts.value_or(now)};
            int    timeout{now == next_time ? -1 : this->to_milliseconds(next_time - now)};
            nfds_t sz([](auto s) {
                if constexpr (::std::same_as<decltype(s), nfds_t>)
                    return s;
                else
                    return nfds_t(s);
            }(this->d_poll.size()));
            int    rc(::poll(this->d_poll.data(), sz, timeout));
            if (rc < 0) {
                switch (errno) {
                default:
                    return ::std::size_t();
                case EINTR:
                case EAGAIN:
                    break;
                }
            } else {
                for (::std::size_t i(this->d_poll.size()); 0 < i--;) {
                    if (this->d_poll[i].revents & (this->d_poll[i].events | POLLERR)) {
                        ::beman::net::detail::io_base* completion = this->d_outstanding[i];
                        this->remove_outstanding(i);
                        completion->work(*this, completion);
                        return ::std::size_t(1);
                    }
                }
                if (0u < this->process_timeout(::std::chrono::system_clock::now())) {
                    return 1u;
                }
            }
        }
        return ::std::size_t{};
    }
    auto wakeup() -> void {
        //-dk:TODO wake-up polling thread
    }

    auto add_outstanding(::beman::net::detail::io_base* completion) -> ::beman::net::detail::submit_result {
        auto id{completion->id};
        if (this->d_sockets[id].blocking ||
            completion->work(*this, completion) == ::beman::net::detail::submit_result::submit) {
            decltype(pollfd().events) events{};
            if (bool(completion->event & ::beman::net::event_type::in)) {
                events |= POLLIN;
            }
            if (bool(completion->event & ::beman::net::event_type::out)) {
                events |= POLLOUT;
            }
            this->d_poll.emplace_back(::pollfd{this->native_handle(id), events, short()});
            this->d_outstanding.emplace_back(completion);
            this->wakeup();
            return ::beman::net::detail::submit_result::submit;
        }
        return ::beman::net::detail::submit_result::ready;
    }

    auto cancel(::beman::net::detail::io_base* cancel_op, ::beman::net::detail::io_base* op) -> void override final {
        auto it(::std::find(this->d_outstanding.begin(), this->d_outstanding.end(), op));
        if (it != this->d_outstanding.end()) {
            this->remove_outstanding(std::size_t(::std::distance(this->d_outstanding.begin(), it)));
            op->cancel();
            cancel_op->cancel();
        } else if (this->d_timeouts.erase(op)) {
            op->cancel();
            cancel_op->cancel();
        } else {
            std::cerr << "ERROR: poll_context::cancel(): entity not cancelled!\n";
        }
    }
    auto schedule(::beman::net::detail::context_base::task* tsk) -> void override {
        tsk->next     = this->d_tasks;
        this->d_tasks = tsk;
    }
    auto accept(::beman::net::detail::context_base::accept_operation* completion)
        -> ::beman::net::detail::submit_result override final {
        completion->work = [](::beman::net::detail::context_base& ctxt, ::beman::net::detail::io_base* comp) {
            auto  id{comp->id};
            auto& cmp(*static_cast<accept_operation*>(comp));

            while (true) {
                int rc = ::accept(ctxt.native_handle(id), ::std::get<0>(cmp).data(), &::std::get<1>(cmp));
                if (0 <= rc) {
                    ::std::get<2>(cmp) = ctxt.make_socket(rc);
                    cmp.complete();
                    return ::beman::net::detail::submit_result::ready;
                } else {
                    switch (errno) {
                    default:
                        cmp.error(::std::error_code(errno, ::std::system_category()));
                        return ::beman::net::detail::submit_result::error;
                    case EINTR:
                        break;
                    case EWOULDBLOCK:
                        return ::beman::net::detail::submit_result::submit;
                    }
                }
            }
        };
        return this->add_outstanding(completion);
    }
    auto connect(::beman::net::detail::context_base::connect_operation* op)
        -> ::beman::net::detail::submit_result override {
        auto        handle{this->native_handle(op->id)};
        const auto& endpoint(::std::get<0>(*op));
        if (-1 == ::fcntl(handle, F_SETFL, O_NONBLOCK)) {
            op->error(::std::error_code(errno, ::std::system_category()));
            return ::beman::net::detail::submit_result::error;
        }
        if (0 == ::connect(handle, endpoint.data(), endpoint.size())) {
            op->complete();
            return ::beman::net::detail::submit_result::ready;
        }
        switch (errno) {
        default:
            op->error(::std::error_code(errno, ::std::system_category()));
            return ::beman::net::detail::submit_result::error;
        case EINPROGRESS:
        case EINTR:
            break;
        }

        op->context = this;
        op->work    = [](::beman::net::detail::context_base& ctxt, ::beman::net::detail::io_base* o) {
            auto hndl{ctxt.native_handle(o->id)};

            int         error{};
            ::socklen_t len{sizeof(error)};
            if (-1 == ::getsockopt(hndl, SOL_SOCKET, SO_ERROR, &error, &len)) {
                o->error(::std::error_code(errno, ::std::system_category()));
                return ::beman::net::detail::submit_result::error;
            }
            if (0 == error) {
                o->complete();
                return ::beman::net::detail::submit_result::ready;
            } else {
                o->error(::std::error_code(error, ::std::system_category()));
                return ::beman::net::detail::submit_result::error;
            }
        };

        return this->add_outstanding(op);
    }
    auto receive(::beman::net::detail::context_base::receive_operation* op)
        -> ::beman::net::detail::submit_result override {
        op->context = this;
        op->work    = [](::beman::net::detail::context_base& ctxt, ::beman::net::detail::io_base* o) {
            auto& completion(*static_cast<receive_operation*>(o));
            while (true) {
                auto rc{::recvmsg(ctxt.native_handle(o->id), &::std::get<0>(completion), ::std::get<1>(completion))};
                if (0 <= rc) {
                    ::std::get<2>(completion) = ::std::size_t(rc);
                    completion.complete();
                    return ::beman::net::detail::submit_result::ready;
                } else
                    switch (errno) {
                    default:
                        completion.error(::std::error_code(errno, ::std::system_category()));
                        return ::beman::net::detail::submit_result::error;
                    case ECONNRESET:
                    case EPIPE:
                        ::std::get<2>(completion) = 0u;
                        completion.complete();
                        return ::beman::net::detail::submit_result::ready;
                    case EINTR:
                        break;
                    case EWOULDBLOCK:
                        return ::beman::net::detail::submit_result::submit;
                    }
            }
        };
        return this->add_outstanding(op);
    }
    auto send(::beman::net::detail::context_base::send_operation* op) -> ::beman::net::detail::submit_result override {
        op->context = this;
        op->work    = [](::beman::net::detail::context_base& ctxt, ::beman::net::detail::io_base* o) {
            auto& completion(*static_cast<send_operation*>(o));

            while (true) {
                auto rc{::sendmsg(ctxt.native_handle(o->id), &::std::get<0>(completion), ::std::get<1>(completion))};
                if (0 <= rc) {
                    ::std::get<2>(completion) = ::std::size_t(rc);
                    completion.complete();
                    return ::beman::net::detail::submit_result::ready;
                } else
                    switch (errno) {
                    default:
                        completion.error(::std::error_code(errno, ::std::system_category()));
                        return ::beman::net::detail::submit_result::error;
                    case ECONNRESET:
                    case EPIPE:
                        ::std::get<2>(completion) = 0u;
                        completion.complete();
                        return ::beman::net::detail::submit_result::ready;
                    case EINTR:
                        break;
                    case EWOULDBLOCK:
                        return ::beman::net::detail::submit_result::submit;
                    }
            }
        };
        return this->add_outstanding(op);
    }
    auto resume_at(::beman::net::detail::context_base::resume_at_operation* op)
        -> ::beman::net::detail::submit_result override {
        if (::std::chrono::system_clock::now() < ::std::get<0>(*op)) {
            this->d_timeouts.insert(op);
            return ::beman::net::detail::submit_result::submit;
        } else {
            op->complete();
            return ::beman::net::detail::submit_result::ready;
        }
    }
};

// ----------------------------------------------------------------------------

#endif
