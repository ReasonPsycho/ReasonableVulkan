//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_MODEL_HPP
#define REASONABLEVULKAN_MODEL_HPP
#include <boost/uuid/uuid.hpp>

namespace engine::ecs
{
    struct Model
    {
        boost::uuids::uuid modelUuid;
    };
}

#endif //REASONABLEVULKAN_MODEL_HPP