#include "Engine.h"
#include <meta>

namespace engine {

template <typename Tuple>
inline std::shared_ptr<IComponentArray> CreateComponentArrayFromType(const std::type_index& type) {
    static constexpr auto types = get_template_args_array<Tuple>();
    template for (constexpr auto t : types) {
        using Comp = typename [:t:];
        if (type == std::type_index(typeid(Comp))) {
            if constexpr (has_annotation<Integral>(t)) {
                return std::make_shared<IntegralComponentArray<Comp>>();
            } else {
                return std::make_shared<ComponentArray<Comp>>();
            }
        }
    }
    throw std::runtime_error("No factory registered for component type: " + std::string(type.name()));
}

template <typename Tuple>
inline std::shared_ptr<SystemBase> CreateSystemFromType(const std::type_index& type, Scene* scene) {
    static constexpr auto types = get_template_args_array<Tuple>();
    template for (constexpr auto t : types) {
        using Sys = typename [:t:];
        if (type == std::type_index(typeid(Sys))) {
            return std::make_shared<Sys>(scene);
        }
    }
    throw std::runtime_error("No factory registered for system type: " + std::string(type.name()));
}

} // namespace engine