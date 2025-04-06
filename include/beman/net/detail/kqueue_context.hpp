// include/beman/net/detail/kqueue_context.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_NET_DETAIL_QUEUE_CONTEXT
#define INCLUDED_BEMAN_NET_DETAIL_QUEUE_CONTEXT

// ----------------------------------------------------------------------------

#include "beman/net/detail/event_type.hpp"
#include "beman/net/detail/io_base.hpp"
#include <array>
#include <beman/net/detail/netfwd.hpp>
#include <beman/net/detail/container.hpp>
#include <beman/net/detail/context_base.hpp>
#include <beman/net/detail/sorted_list.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <sys/event.h>
#include <map>
#include <vector>
#include <cassert>
#include <optional>

// ----------------------------------------------------------------------------

namespace beman::net::detail {
struct kqueue_record;
struct kqueue_context;
} // namespace beman::net::detail

// ----------------------------------------------------------------------------

struct beman::net::detail::kqueue_record final {
    kqueue_record(::beman::net::detail::native_handle_type h) : handle(h) {}
    ::beman::net::detail::native_handle_type handle;
    bool                                     blocking{true};
};

// ----------------------------------------------------------------------------

struct beman::net::detail::kqueue_context final : ::beman::net::detail::context_base {
    static constexpr size_t event_buffer_size = 10;
    using time_t                              = ::std::chrono::system_clock::time_point;
    using timer_node_t                        = ::beman::net::detail::context_base::resume_at_operation;
    using event_key_t                         = ::std::tuple<uintptr_t, int16_t>;
    struct get_time {
        auto operator()(auto* t) const -> time_t { return ::std::get<0>(*t); }
    };
    using timer_priority_t = ::beman::net::detail::sorted_list<timer_node_t, ::std::less<>, get_time>;
    using kevent_t         = struct ::kevent;

    struct io_event {
        struct ::kevent                event;
        ::beman::net::detail::io_base* operation;
    };

#if TOBEDELETE
    constexpr std::array<std::optional<int16_t>, 2> to_kqueue_filter(::beman::net::event_type event_type) {
        switch (event_type) {
        default:
            return {};
        case ::beman::net::event_type::in:
            return {{{EVFILT_READ}, {}}};
        case ::beman::net::event_type::out:
            return {{{EVFILT_WRITE}, {}}};
        case ::beman::net::event_type::in_out:
            return {{{EVFILT_READ}, {EVFILT_WRITE}}};
        }
    }
#endif

    constexpr auto to_native_filter(::beman::net::event_type event_type) -> std::span<const int16_t> {
        static constexpr std::array<int16_t, 1> read_filter      = {EVFILT_READ};
        static constexpr std::array<int16_t, 1> write_filter     = {EVFILT_WRITE};
        static constexpr std::array<int16_t, 2> readwrite_filter = {EVFILT_READ, EVFILT_WRITE};

        switch (event_type) {
        case ::beman::net::event_type::in:
            return {read_filter};
        case ::beman::net::event_type::out:
            return {write_filter};
        case ::beman::net::event_type::in_out:
            return {readwrite_filter};
        case ::beman::net::event_type::none:
            return {};
        }
    }

    ::beman::net::detail::container<::beman::net::detail::kqueue_record>  d_sockets;
    ::std::map<event_key_t, std::vector<::beman::net::detail::socket_id>> d_event;
    ::beman::net::detail::container<::beman::net::detail::io_base*>       d_outstanding;
    timer_priority_t                                                      d_timeouts;
    ::beman::net::detail::context_base::task*                             d_tasks{};
    const int d_queue = kqueue(); // TODO: is this a good practise to put it here?

    // constexpr beman::net::event_type to_event_kind(short events) {
    //     switch (events & (EVFILT_READ | EVFILT_WRITE)) {
    //     default:
    //         return ::beman::net::event_type::none;
    //     case EVFILT_READ:
    //         return ::beman::net::event_type::out;
    //     case EVFILT_WRITE:
    //         return ::beman::net::event_type::in;
    //         // case EVFILT_READ | EVFILT_WRITE: return toy::event_kind::both;
    //     }
    // }
    //
    // constexpr short int to_kqueue(::beman::net::event_type events) {
    //     switch (events) {
    //     default:
    //     case ::beman::net::event_type::none:
    //         return 0;
    //     case toy::event_kind::read:
    //         return EVFILT_READ;
    //     case toy::event_kind::write:
    //         return EVFILT_WRITE;
    //     case toy::event_kind::both:
    //         return EVFILT_READ | EVFILT_WRITE;
    //     }
    // }
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
#if TOBEDELETE
    auto remove_outstanding(::beman::net::event_type event_type, intptr_t ident) {
        auto filters = to_native_filter(event_type);
        for (const auto f : filters) {
            // const event_key_t key {}
        }
    }
#endif
    auto remove_outstanding(::beman::net::detail::socket_id outstanding_id) {
        auto&      completion    = d_outstanding[outstanding_id];
        const auto native_handle = this->native_handle(completion->id);
        const auto filters       = this->to_native_filter(completion->event);
        for (const auto f : filters) {
            const event_key_t key{native_handle, f};
            auto              event_it = d_event.find(key);
            if (event_it == d_event.end()) {
                continue;
            }
            auto& event_completions = event_it->second;
            auto  socket_it         = std::find(event_completions.begin(), event_completions.end(), outstanding_id);
            if (socket_it == event_completions.end()) {
                continue;
            }
            event_completions.erase(socket_it);

            if (1 > event_completions.size()) {
                kevent_t evt;
                EV_SET(&evt, native_handle, f, EV_DELETE, 0, 0, nullptr);
                kevent(d_queue, &evt, 1, nullptr, 0, NULL);
            }
        }
    }
    auto to_milliseconds(auto duration) -> int {
        return int(::std::chrono::duration_cast<::std::chrono::milliseconds>(duration).count());
    }
    auto run_one() -> ::std::size_t override final {
        auto now{::std::chrono::system_clock::now()};
        if (0u < this->process_timeout(now) || 0 < this->process_task()) {
            return 1u;
        }
        if (this->d_event.empty() && this->d_timeouts.empty()) {
            return ::std::size_t{};
        }
        auto                                    next_time{this->d_timeouts.value_or(now)};
        std::array<kevent_t, event_buffer_size> evt_buffer;
        timespec                                timeout;
        if (now != next_time) {
            auto milliseconds = this->to_milliseconds(next_time - now);
            timeout.tv_sec    = milliseconds / 1000;
            timeout.tv_nsec   = 1000000 * (milliseconds % 1000);
        }

        auto n = ::kevent(
            this->d_queue, nullptr, 0, evt_buffer.data(), evt_buffer.size(), now == next_time ? nullptr : &timeout);

        ::std::size_t ncompleted{0};
        if (n < 0) {
            // TODO: kevent error handling
            return 0;
        }

        while (0 < n) {
            --n;

            auto buffer_idx = size_t(n);

            event_key_t evt_key{evt_buffer[buffer_idx].ident, evt_buffer[buffer_idx].filter};
            auto        outstanding_evt = d_event.find(evt_key);
            if (d_event.end() == outstanding_evt || outstanding_evt->second.size() == 0) {
                kevent_t evt;
                EV_SET(&evt, std::get<0>(evt_key), std::get<1>(evt_key), EV_DELETE, 0, 0, nullptr);
                kevent(d_queue, &evt, 1, nullptr, 0, nullptr);
            }

            auto completion = d_outstanding[outstanding_evt->second.back()];

            this->remove_outstanding(outstanding_evt->second.back());
            completion->work(*this, completion);
            ++ncompleted;
        }

        return ncompleted;
    }
    auto wakeup() -> void {
        //-dk:TODO wake-up polling thread
    }

    auto add_outstanding(::beman::net::detail::io_base* completion) -> ::beman::net::detail::submit_result {
        auto id{completion->id};
        if (this->d_sockets[id].blocking ||
            completion->work(*this, completion) == ::beman::net::detail::submit_result::submit) {
            const auto native_handle  = this->native_handle(completion->id);
            auto       filters        = to_native_filter(completion->event);
            auto       outstanding_id = d_outstanding.insert(completion);
            for (const auto f : filters) {
                const event_key_t key{native_handle, f};
                d_event[key].emplace_back(outstanding_id);
                kevent_t evt;
                EV_SET(&evt, std::get<0>(key), std::get<1>(key), EV_ADD, 0, 0, nullptr);
                kevent(d_queue, &evt, 1, nullptr, 0, NULL);
            }
            this->wakeup();
            return ::beman::net::detail::submit_result::submit;
        }
        return ::beman::net::detail::submit_result::ready;
    }

    auto cancel(::beman::net::detail::io_base* cancel_op, ::beman::net::detail::io_base* op) -> void override final {
        auto f = this->d_outstanding.find(op);

        if (f) {
            this->remove_outstanding(*f);
            op->cancel();
            cancel_op->cancel();
        } else if (this->d_timeouts.erase(op)) {
            op->cancel();
            cancel_op->cancel();
        } else {
            std::cerr << "ERROR: kqueue_context::cancel(): entity not cancelled!\n";
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
