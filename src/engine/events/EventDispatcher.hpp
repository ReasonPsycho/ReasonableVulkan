#pragma once

#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include <algorithm>

namespace engine::events {

    using ListenerID = uint32_t;

    class IEventSink {
    public:
        virtual ~IEventSink() = default;
        virtual void ClearQueue() = 0;
        virtual void ProcessQueue() = 0;
        virtual size_t GetQueueSize() const = 0;
        virtual size_t GetListenerCount() const = 0;
    };

    template <typename TEvent>
    class EventSink : public IEventSink {
    public:
        using Callback = std::function<void(const TEvent&)>;

        struct ListenerEntry {
            ListenerID id;
            Callback callback;
        };

        ListenerID Subscribe(Callback callback) {
            ListenerID id = nextListenerId++;
            listeners.push_back({id, std::move(callback)});
            return id;
        }

        void Unsubscribe(ListenerID id) {
            std::erase_if(listeners, [id](const ListenerEntry& entry) {
                return entry.id == id;
            });
        }

        // Immediate dispatch
        void Publish(const TEvent& event) {
            // Copy listeners or iterate with index to safely handle unsubscriptions during dispatch
            auto currentListeners = listeners;
            for (const auto& listener : currentListeners) {
                listener.callback(event);
            }
        }

        // Deferred / Queue-based dispatch
        void Enqueue(const TEvent& event) {
            queue.push_back(event);
        }

        void ProcessQueue() override {
            if (queue.empty()) return;
            // Swap to temporary buffer to safely support re-entrant emissions
            std::vector<TEvent> currentQueue;
            currentQueue.swap(queue);

            for (const auto& ev : currentQueue) {
                Publish(ev);
            }
        }

        void ClearQueue() override {
            queue.clear();
        }

        size_t GetQueueSize() const override {
            return queue.size();
        }

        size_t GetListenerCount() const override {
            return listeners.size();
        }

    private:
        std::vector<ListenerEntry> listeners;
        std::vector<TEvent> queue;
        ListenerID nextListenerId{1};
    };

    class EventDispatcher;

    template <typename TEvent>
    class ScopedEventListener {
    public:
        ScopedEventListener() = default;
        ScopedEventListener(EventDispatcher* dispatcher, ListenerID id)
            : dispatcher(dispatcher), id(id) {}

        ~ScopedEventListener() {
            Reset();
        }

        ScopedEventListener(ScopedEventListener&& other) noexcept
            : dispatcher(other.dispatcher), id(other.id) {
            other.dispatcher = nullptr;
            other.id = 0;
        }

        ScopedEventListener& operator=(ScopedEventListener&& other) noexcept {
            if (this != &other) {
                Reset();
                dispatcher = other.dispatcher;
                id = other.id;
                other.dispatcher = nullptr;
                other.id = 0;
            }
            return *this;
        }

        ScopedEventListener(const ScopedEventListener&) = delete;
        ScopedEventListener& operator=(const ScopedEventListener&) = delete;

        void Reset();

        ListenerID GetID() const { return id; }
        bool IsValid() const { return dispatcher != nullptr && id != 0; }

    private:
        EventDispatcher* dispatcher{nullptr};
        ListenerID id{0};
    };

    class EventDispatcher {
    public:
        template <typename TEvent>
        ListenerID Subscribe(typename EventSink<TEvent>::Callback callback) {
            return GetOrCreateSink<TEvent>()->Subscribe(std::move(callback));
        }

        template <typename TEvent>
        ScopedEventListener<TEvent> ScopedSubscribe(typename EventSink<TEvent>::Callback callback) {
            ListenerID id = Subscribe<TEvent>(std::move(callback));
            return ScopedEventListener<TEvent>(this, id);
        }

        template <typename TEvent>
        void Unsubscribe(ListenerID id) {
            auto it = sinks.find(std::type_index(typeid(TEvent)));
            if (it != sinks.end()) {
                static_cast<EventSink<TEvent>*>(it->second.get())->Unsubscribe(id);
            }
        }

        template <typename TEvent>
        void Publish(const TEvent& event) {
            auto it = sinks.find(std::type_index(typeid(TEvent)));
            if (it != sinks.end()) {
                static_cast<EventSink<TEvent>*>(it->second.get())->Publish(event);
            }
        }

        template <typename TEvent>
        void Enqueue(const TEvent& event) {
            GetOrCreateSink<TEvent>()->Enqueue(event);
        }

        template <typename TEvent>
        void ProcessQueue() {
            auto it = sinks.find(std::type_index(typeid(TEvent)));
            if (it != sinks.end()) {
                it->second->ProcessQueue();
            }
        }

        void ProcessAllQueues() {
            for (auto& [typeIdx, sink] : sinks) {
                sink->ProcessQueue();
            }
        }

        template <typename TEvent>
        void ClearQueue() {
            auto it = sinks.find(std::type_index(typeid(TEvent)));
            if (it != sinks.end()) {
                it->second->ClearQueue();
            }
        }

        void ClearAllQueues() {
            for (auto& [typeIdx, sink] : sinks) {
                sink->ClearQueue();
            }
        }

        template <typename TEvent>
        size_t GetQueueSize() const {
            auto it = sinks.find(std::type_index(typeid(TEvent)));
            if (it != sinks.end()) {
                return it->second->GetQueueSize();
            }
            return 0;
        }

        template <typename TEvent>
        size_t GetListenerCount() const {
            auto it = sinks.find(std::type_index(typeid(TEvent)));
            if (it != sinks.end()) {
                return it->second->GetListenerCount();
            }
            return 0;
        }

        template <typename TEvent>
        bool HasListeners() const {
            return GetListenerCount<TEvent>() > 0;
        }

        void Reset() {
            sinks.clear();
        }

    private:
        template <typename TEvent>
        EventSink<TEvent>* GetOrCreateSink() {
            auto typeIdx = std::type_index(typeid(TEvent));
            auto it = sinks.find(typeIdx);
            if (it == sinks.end()) {
                auto sink = std::make_unique<EventSink<TEvent>>();
                auto* ptr = sink.get();
                sinks[typeIdx] = std::move(sink);
                return ptr;
            }
            return static_cast<EventSink<TEvent>*>(it->second.get());
        }

        std::unordered_map<std::type_index, std::unique_ptr<IEventSink>> sinks;
    };

    template <typename TEvent>
    inline void ScopedEventListener<TEvent>::Reset() {
        if (dispatcher && id != 0) {
            dispatcher->Unsubscribe<TEvent>(id);
            dispatcher = nullptr;
            id = 0;
        }
    }

} // namespace engine::events
