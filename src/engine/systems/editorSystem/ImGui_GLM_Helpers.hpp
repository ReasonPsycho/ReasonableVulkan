
#ifndef IMGUI_GLM_HELPERS_HPP
#define IMGUI_GLM_HELPERS_HPP

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace ImGui {
    // Helper for vec3 with label
    static bool DragVec3(const char* label, glm::vec3& v, float v_speed = 0.1f,
                        float v_min = 0.0f, float v_max = 0.0f,
                        const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        return DragFloat3(label, glm::value_ptr(v), v_speed, v_min, v_max, format, flags);
    }

    // Helper for vec4 with label
    static bool DragVec4(const char* label, glm::vec4& v, float v_speed = 0.1f,
                        float v_min = 0.0f, float v_max = 0.0f,
                        const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        return DragFloat4(label, glm::value_ptr(v), v_speed, v_min, v_max, format, flags);
    }

    // Helper for displaying mat4 with label
    static bool DisplayMat4(const char* label, const glm::mat4& matrix, bool detailed = false) {
        bool modified = false;
        if (TreeNode(label)) {
            if (detailed) {
                for (int i = 0; i < 4; i++) {
                    Text("Row %d: %.3f %.3f %.3f %.3f", i,
                         matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3]);
                }
            } else {
                // Simplified view - just show translation and scale
                glm::vec3 translation(matrix[3]);
                glm::vec3 scale(
                    glm::length(glm::vec3(matrix[0])),
                    glm::length(glm::vec3(matrix[1])),
                    glm::length(glm::vec3(matrix[2]))
                );

                Text("Translation: %.3f, %.3f, %.3f", translation.x, translation.y, translation.z);
                Text("Scale: %.3f, %.3f, %.3f", scale.x, scale.y, scale.z);
            }
            TreePop();
        }
        return modified;
    }

    // Helper for editing projection parameters
    static bool EditProjectionMatrix(glm::mat4& projection, float& fov, float& nearPlane,
                                   float& farPlane, float& aspect) {
        bool modified = false;

        if (TreeNode("Projection Parameters")) {
            modified |= DragFloat("FOV", &fov, 0.1f, 1.0f, 179.0f, "%.1f°");
            modified |= DragFloat("Near Plane", &nearPlane, 0.01f, 0.001f, farPlane);
            modified |= DragFloat("Far Plane", &farPlane, 0.1f, nearPlane, 1000.0f);
            modified |= DragFloat("Aspect Ratio", &aspect, 0.01f, 0.1f, 10.0f);

            if (modified) {
                projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
            }
            TreePop();
        }

        return modified;
    }
}

#endif // IMGUI_GLM_HELPERS_HPP