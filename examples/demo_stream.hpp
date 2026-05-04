// examples/demo_stream.hpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_EXAMPLES_DEMO_STREAM
#define INCLUDED_EXAMPLES_DEMO_STREAM

#include <beman/execution/execution.hpp>
#include <beman/net/net.hpp>
#include <beman/net/detail/repeat_effect_until.hpp>
#include <algorithm>
#include <functional>
#include <array>
#include <cstddef>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------------

namespace demo::stream {
namespace ex  = ::beman::execution;
namespace net = ::beman::net;

struct tcp {
    ::beman::net::ip::tcp::socket socket;
    template <typename Buffer>
    auto receive(Buffer buffer) {
        return net::async_receive(this->socket, buffer);
    }
};

struct memory_base {
    virtual ~memory_base()                                              = default;
    virtual auto add_receive_data(std::string_view data) -> std::size_t = 0;
};
struct context {
    using update_fun = std::function<bool(memory_base&)>;
    std::unordered_map<std::string, memory_base*>   streams;
    std::vector<std::pair<std::string, update_fun>> updates;
    std::size_t                                     next_index{};
    void                                            next() {
        auto&        update = updates[next_index];
        memory_base* stream = streams[update.first];
        if (stream == nullptr || update.second(*stream)) {
            this->updates.erase(this->updates.begin() + next_index);
        } else if (this->updates.size() == ++this->next_index) {
            this->next_index = 0u;
        }
    }
    void add(std::string name, update_fun fun) { this->updates.emplace_back(std::move(name), std::move(fun)); }
    auto async_run_one() {
        return ex::just() | ex::then([this] { this->next(); });
    }
    auto async_run() {
        return ex::just() |
               net::repeat_effect_until(ex::read_env(ex::get_scheduler) | ex::let_value([this](auto sched) {
                                            return ex::schedule(sched) |
                                                   ex::let_value([this] { return this->async_run_one(); });
                                        }),
                                        [this] { return this->updates.empty(); });
    }
};
struct memory {
    template <ex::receiver Receiver, typename Buffer>
    struct state : memory_base {
        using operation_state_concept = ex::operation_state_tag;
        Receiver receiver;
        Buffer   buffer;
        memory*  self;

        template <typename R, typename B>
        state(R&& r, B&& b, memory* s) : receiver(std::forward<R>(r)), buffer(std::forward<B>(b)), self(s) {}
        auto start() & noexcept {
            this->self->receive_state                   = this;
            this->self->ctxt->streams[this->self->name] = this;
        }
        auto add_receive_data(std::string_view data) -> std::size_t override {
            const auto& vec{this->buffer.data()};
            std::size_t n{std::min(data.size(), vec[0].iov_len)};
            std::copy(data.begin(), data.begin() + n, static_cast<char*>(vec[0].iov_base));
            this->self->ctxt->streams[this->self->name] = nullptr;
            ex::set_value(std::move(this->receiver), n);
            return n;
        }
    };
    template <typename Buffer>
    struct receive_sender {
        using sender_concept = ex::sender_tag;
        template <typename...>
        static consteval auto get_completion_signatures() {
            return ex::completion_signatures<ex::set_value_t(std::size_t)>{};
        }
        memory* self;
        Buffer  buffer;

        template <ex::receiver Receiver>
        state<std::remove_cvref_t<Receiver>, Buffer> connect(Receiver&& receiver) && {
            static_assert(ex::operation_state<state<std::remove_cvref_t<Receiver>, Buffer>>);
            return {std::forward<Receiver>(receiver), std::move(this->buffer), this->self};
        }
    };
    template <typename Buffer>
    auto receive(Buffer buffer) {
        static_assert(ex::sender<receive_sender<Buffer>>);
        return receive_sender<Buffer>{this, std::move(buffer)};
    }

    context*     ctxt;
    std::string  name;
    memory_base* receive_state{};
    memory(context& c, std::string n) : ctxt(&c), name(std::move(n)) {}
};

template <typename Stream>
struct basic_buffered {
    using buffer_t        = ::std::array<char, 1024>;
    using buffer_iterator = typename buffer_t::iterator;

    Stream          stream;
    buffer_t        buffer{};
    buffer_iterator it{buffer.begin()};
    buffer_iterator end{buffer.begin()};

    template <typename... Args>
    basic_buffered(Args&&... args) : stream(std::forward<Args>(args)...) {}
    static constexpr std::size_t length(char) { return 1u; }
    bool                         consume(auto& to, char sentinel) {
        auto end{::std::find(this->it, this->end, sentinel)};
        to.insert(to.end(), this->it, end);
        this->it = end;
        return end == this->end;
    }
    template <::std::size_t Size>
    static constexpr std::size_t length(const char (&)[Size]) {
        return Size - 1u;
    }
    template <::std::size_t Size>
    bool consume(auto& to, const char (&sentinel)[Size]) {
        auto end{::std::search(this->it, this->end, sentinel, sentinel + Size - 1)};
        bool rc(end == this->end);
        if (rc) {
            end -= std::min(std::size_t(std::distance(this->it, end)), Size);
        }
        to.insert(to.end(), this->it, end);
        this->it = end;
        return rc;
    }
    auto read(auto& to, const auto& sentinel) -> ex::task<bool> {
        while (this->consume(to, sentinel)) {
            if (std::size_t(std::distance(this->end, this->buffer.end())) < this->buffer.size() / 2) {
                this->end = std::move(this->it, this->end, this->buffer.begin());
                this->it  = this->buffer.begin();
            }
            std::size_t n{co_await this->stream.receive(net::buffer(std::string_view(this->end, this->buffer.end())))};
            if (n == 0) {
                co_return false;
            }
            this->end += n;
        }
        this->it += length(sentinel);

        co_return true;
    }
};
} // namespace demo::stream

namespace demo {
using demo::stream::basic_buffered;
using tcp_stream = demo::stream::basic_buffered<demo::stream::tcp>;
using mem_stream = demo::stream::basic_buffered<demo::stream::memory>;
using demo::stream::context;
using demo::stream::memory;
using demo::stream::memory_base;
} // namespace demo

// ----------------------------------------------------------------------------

#endif
