#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "ecs/Component.hpp"
#include "systems/editorSystem/ImGui_GLM_Helpers.hpp"

namespace engine::ecs
{
    struct Transform;
    inline void setLocalRotationFromEulerDegrees(Transform& t, const glm::vec3& eulerDegrees);

    struct Transform: public Component
    {
        glm::vec3 position;
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale{1.0f, 1.0f, 1.0f};

        glm::mat4 localMatrix = glm::mat4(1.0f);   // local to parent
        glm::mat4 globalMatrix = glm::mat4(1.0f);  // local to world

        bool isDirty = true;

#ifdef EDITOR_ENABLED
        void ImGuiComponent() override
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                if (ImGui::DragVec3("Position", position, 0.1f))
                {
                    isDirty = true;
                }

                // For rotation, we'll show it as Euler angles in degrees for easier editing
                glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(rotation));
                if (ImGui::DragVec3("Rotation", eulerDegrees, 1.0f))
                {
                    setLocalRotationFromEulerDegrees(*this, eulerDegrees);
                }

                if (ImGui::DragVec3("Scale", scale, 0.1f))
                {
                    isDirty = true;
                }

                if (ImGui::TreeNode("Matrices"))
                {
                    ImGui::DisplayMat4("Local Matrix", localMatrix);
                    ImGui::DisplayMat4("Global Matrix", globalMatrix);
                    ImGui::TreePop();
                }
            }
        }
#endif

    };

    // Utility
    inline void decomposeMtx(const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale)
    {
        pos = glm::vec3(m[3]);
        for (int i = 0; i < 3; ++i)
            scale[i] = glm::length(glm::vec3(m[i]));
        glm::mat3 rotMat(
            glm::vec3(m[0]) / scale[0],
            glm::vec3(m[1]) / scale[1],
            glm::vec3(m[2]) / scale[2]
        );
        rot = glm::quat_cast(rotMat);
    }

    // Local matrix construction
    inline glm::mat4 getLocalModelMatrix(const Transform& t)
    {
        return glm::translate(glm::mat4(1.0f), t.position) *
               glm::toMat4(t.rotation) *
               glm::scale(glm::mat4(1.0f), t.scale);
    }

    // Local matrix without scale (for direction vectors)
    inline glm::mat4 getLocalTranslationMatrix(const Transform& t)
    {
        return glm::translate(glm::mat4(1.0f), t.position) *
               glm::toMat4(t.rotation);
    }

    // Matrix computations
    inline void computeLocalMatrix(Transform& t)
    {
        t.localMatrix = getLocalModelMatrix(t);
        t.isDirty = false;
    }

    inline void computeGlobalMatrix(Transform& t, const glm::mat4& parentGlobal)
    {
        if (t.isDirty)
            computeLocalMatrix(t);
        t.globalMatrix = parentGlobal * t.localMatrix;
    }

    inline void computeGlobalMatrixRoot(Transform& t)
    {
        if (t.isDirty)
            computeLocalMatrix(t);
        t.globalMatrix = t.localMatrix;
    }

    // Setters
    inline void setLocalPosition(Transform& t, const glm::vec3& pos)
    {
        t.position = pos;
        t.isDirty = true;
    }

    inline void setLocalRotation(Transform& t, const glm::quat& quat)
    {
        t.rotation = glm::normalize(quat);
        t.isDirty = true;
    }

    inline void setLocalRotationFromEulerDegrees(Transform& t, const glm::vec3& eulerDegrees)
    {
        t.rotation = glm::quat(glm::radians(eulerDegrees));
        t.isDirty = true;
    }

    inline void setLocalScale(Transform& t, const glm::vec3& scl)
    {
        t.scale = scl;
        t.isDirty = true;
    }

    inline void setLocalMatrix(Transform& t, const glm::mat4& mat)
    {
        glm::vec3 pos, scale;
        glm::quat rot;
        decomposeMtx(mat, pos, rot, scale);
        t.position = pos;
        t.rotation = glm::normalize(rot);
        t.scale = scale;
        t.isDirty = true;
    }

    // Set local transform from global one using parent global
    inline void setLocalMatrixFromGlobal(Transform& t, const glm::mat4& global, const glm::mat4& parentGlobal)
    {
        setLocalMatrix(t, glm::inverse(parentGlobal) * global);
    }

    // Getters (global)
    inline glm::vec3 getGlobalPosition(const Transform& t)
    {
        return glm::vec3(t.globalMatrix[3]);
    }

    inline glm::quat getGlobalRotation(const Transform& t)
    {
        return glm::quat_cast(t.globalMatrix);
    }

    inline glm::vec3 getGlobalScale(const Transform& t)
    {
        return {
            glm::length(glm::vec3(t.globalMatrix[0])),
            glm::length(glm::vec3(t.globalMatrix[1])),
            glm::length(glm::vec3(t.globalMatrix[2]))
        };
    }

    inline glm::vec3 getRight(const Transform& t)
    {
        return glm::vec3(t.globalMatrix[0]);
    }

    inline glm::vec3 getUp(const Transform& t)
    {
        return glm::vec3(t.globalMatrix[1]);
    }

    inline glm::vec3 getBackward(const Transform& t)
    {
        return glm::vec3(t.globalMatrix[2]);
    }

    inline glm::vec3 getForward(const Transform& t)
    {
        return -glm::vec3(t.globalMatrix[2]);
    }

    inline bool isDirty(const Transform& t)
    {
        return t.isDirty;
    }

    // Useful for relative transformations (e.g., from parent to child)
    inline glm::mat4 getLocalToChildMatrix(const Transform& parent, const Transform& child)
    {
        return glm::inverse(parent.localMatrix) * child.localMatrix;
    }
}

#endif // TRANSFORM_H
