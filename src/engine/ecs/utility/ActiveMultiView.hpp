#ifndef REASONABLEVULKAN_ACTIVEMULTIVIEW_HPP
#define REASONABLEVULKAN_ACTIVEMULTIVIEW_HPP

#include <tuple>
#include <type_traits>
#include <cstddef>
#include <iterator>
#include "Entity.hpp"
#include "Reflection.hpp"
#include "../Types.h"
#include "../componentArrays/ComponentArray.h"
#include "../componentArrays/IntegralComponentArray.h"

namespace engine::ecs {

    class Scene;

    template <typename Primary, typename... Secondaries>
    class ActiveMultiView {
    public:
        using ValueType = std::tuple<Entity, Primary&, Secondaries&...>;

        class Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = ValueType;

            Iterator(ActiveMultiView* view, size_t index)
                : m_View(view), m_Index(index) {
                if (m_View && m_Index < m_View->GetPrimarySize()) {
                    if (!m_View->IsValidCandidate(m_Index)) {
                        AdvanceToNextValid();
                    }
                }
            }

            bool operator==(const Iterator& other) const {
                return m_Index == other.m_Index;
            }

            bool operator!=(const Iterator& other) const {
                return !(*this == other);
            }

            Iterator& operator++() {
                ++m_Index;
                AdvanceToNextValid();
                return *this;
            }

            Iterator operator++(int) {
                Iterator temp = *this;
                ++(*this);
                return temp;
            }

            ValueType operator*() const {
                Entity entity = m_View->GetEntityAt(m_Index);
                return std::tuple<Entity, Primary&, Secondaries&...>(
                    entity,
                    m_View->template GetComponentFromPrimary<Primary>(m_Index),
                    m_View->m_Scene->template GetComponent<Secondaries>(entity)...
                );
            }

        private:
            void AdvanceToNextValid() {
                const size_t primarySize = m_View->GetPrimarySize();
                while (m_Index < primarySize) {
                    if (m_View->IsValidCandidate(m_Index)) {
                        return;
                    }
                    ++m_Index;
                }
                m_Index = primarySize;
            }

            ActiveMultiView* m_View = nullptr;
            size_t m_Index = 0;
        };

        explicit ActiveMultiView(Scene* scene)
            : m_Scene(scene) {
            if (m_Scene) {
                if constexpr (has_annotation<Integral>(^^Primary)) {
                    m_PrimaryIntegralArray = m_Scene->template GetIntegralComponentArray<Primary>().get();
                } else {
                    m_PrimaryStandardArray = m_Scene->template GetComponentArray<Primary>().get();
                }
            }
        }

        Iterator begin() {
            if (!m_Scene || !HasValidPrimaryStorage() || GetPrimarySize() == 0) {
                return end();
            }
            return Iterator(this, 0);
        }

        Iterator end() {
            return Iterator(this, GetPrimarySize());
        }

    private:
        friend class Iterator;

        bool HasValidPrimaryStorage() const {
            return m_PrimaryStandardArray != nullptr || m_PrimaryIntegralArray != nullptr;
        }

        size_t GetPrimarySize() const {
            if (m_PrimaryStandardArray) {
                return m_PrimaryStandardArray->GetArraySize();
            } else if (m_PrimaryIntegralArray) {
                return MAX_ENTITIES;
            }
            return 0;
        }

        bool IsPrimaryActive(size_t index) const {
            if (m_PrimaryStandardArray) {
                return m_PrimaryStandardArray->IsComponentActive(static_cast<ComponentID>(index));
            } else if (m_PrimaryIntegralArray) {
                return m_PrimaryIntegralArray->IsComponentActive(static_cast<Entity>(index));
            }
            return false;
        }

        Entity GetEntityAt(size_t index) const {
            if (m_PrimaryStandardArray) {
                EntityID id = m_PrimaryStandardArray->ComponentIndexToEntity(static_cast<ComponentID>(index)).GetID();
                return Entity(id, m_Scene);
            } else {
                return Entity(static_cast<EntityID>(index), m_Scene);
            }
        }

        template <typename T>
        T& GetComponentFromPrimary(size_t index) const {
            if constexpr (has_annotation<Integral>(^^T)) {
                return m_PrimaryIntegralArray->GetComponent(static_cast<ComponentID>(index));
            } else {
                return m_PrimaryStandardArray->GetComponent(static_cast<ComponentID>(index));
            }
        }

        bool IsValidCandidate(size_t index) const {
            if (!IsPrimaryActive(index)) {
                return false;
            }
            Entity entity = GetEntityAt(index);
            if (!m_Scene->IsEntityActive(entity)) {
                return false;
            }
            return MatchesSecondaries(entity);
        }

        bool MatchesSecondaries(Entity entity) const {
            if constexpr (sizeof...(Secondaries) == 0) {
                return true;
            } else {
                return (CheckSecondary<Secondaries>(entity) && ...);
            }
        }

        template <typename T>
        bool CheckSecondary(Entity entity) const {
            return m_Scene->template HasComponent<T>(entity) &&
                   m_Scene->template IsComponentActive<T>(entity);
        }

        Scene* m_Scene = nullptr;
        ComponentArray<Primary>* m_PrimaryStandardArray = nullptr;
        IntegralComponentArray<Primary>* m_PrimaryIntegralArray = nullptr;
    };

    template <typename Primary, typename... Secondaries>
    using ActiveView = ActiveMultiView<Primary, Secondaries...>;

} // namespace engine::ecs

#endif // REASONABLEVULKAN_ACTIVEMULTIVIEW_HPP
