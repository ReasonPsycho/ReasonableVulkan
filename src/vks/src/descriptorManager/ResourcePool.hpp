#pragma once
#include <vector>
#include <memory>
#include "Handle.hpp"

namespace vks {

    template<typename T, typename Tag>
    class ResourcePool {
    public:
        struct Slot {
            std::unique_ptr<T> resource{nullptr};
            uint32_t generation{0};
            uint32_t nextFree{gfx::Handle<Tag>::INVALID_INDEX};
        };

        gfx::Handle<Tag> insert(std::unique_ptr<T> resource) {
            uint32_t index;
            if (firstFreeIndex != gfx::Handle<Tag>::INVALID_INDEX) {
                index = firstFreeIndex;
                firstFreeIndex = slots[index].nextFree;
            } else {
                index = static_cast<uint32_t>(slots.size());
                slots.emplace_back();
            }

            slots[index].resource = std::move(resource);
            return gfx::Handle<Tag>(index, slots[index].generation);
        }

        [[nodiscard]] T* get(gfx::Handle<Tag> handle) const {
            if (!handle.isValid() || handle.index >= slots.size()) return nullptr;
            const auto& slot = slots[handle.index];
            if (slot.generation != handle.generation) return nullptr; // Stale or mismatched generation
            return slot.resource.get();
        }

        [[nodiscard]] T* getByIndex(uint32_t index) const {
            if (index >= slots.size()) return nullptr;
            return slots[index].resource.get();
        }

        void erase(gfx::Handle<Tag> handle) {
            if (!handle.isValid() || handle.index >= slots.size()) return;
            auto& slot = slots[handle.index];
            if (slot.generation != handle.generation) return;

            slot.resource.reset();
            slot.generation = (slot.generation + 1) & 0x00000FFF; // Increment generation (12 bits)
            slot.nextFree = firstFreeIndex;
            firstFreeIndex = handle.index;
        }

        void clear() {
            slots.clear();
            firstFreeIndex = gfx::Handle<Tag>::INVALID_INDEX;
        }

        [[nodiscard]] size_t size() const {
            return slots.size();
        }

        [[nodiscard]] const std::vector<Slot>& getSlots() const {
            return slots;
        }

    private:
        std::vector<Slot> slots;
        uint32_t firstFreeIndex = gfx::Handle<Tag>::INVALID_INDEX;
    };

} // namespace vks
