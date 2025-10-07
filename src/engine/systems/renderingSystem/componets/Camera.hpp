//
// Created by redkc on 05/08/2025.
//

#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm.hpp>

namespace engine::ecs
{
    struct Camera
    {
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec4 lightpos;
    };
}
#endif //CAMERA_H
