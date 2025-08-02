#ifndef TRANSFORM_H
#define TRANSFORM_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace engine::ecs
{
    struct Transform
    {
        glm::vec3 position;
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::mat4 matrix = glm::mat4(1.0f);
        bool isDirty = true;
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

    // Core functions
    inline glm::mat4 getLocalModelMatrix(const Transform& t)
    {
        return glm::translate(glm::mat4(1.0f), t.position) *
               glm::toMat4(t.rotation) *
               glm::scale(glm::mat4(1.0f), t.scale);
    }

    inline glm::mat4 getLocalTranslationMatrix(const Transform& t)
    {
        return glm::translate(glm::mat4(1.0f), t.position) *
               glm::toMat4(t.rotation) *
               glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
    }

    inline void computeModelMatrix(Transform& t)
    {
        t.matrix = getLocalModelMatrix(t);
        t.isDirty = false;
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

    // Getters
    inline glm::vec3 getGlobalPosition(const Transform& t)
    {
        return glm::vec3(t.matrix[3]);
    }

    inline glm::quat getGlobalRotation(const Transform& t)
    {
        return glm::quat_cast(t.matrix);
    }

    inline glm::vec3 getGlobalScale(const Transform& t)
    {
        return {
            glm::length(glm::vec3(t.matrix[0])),
            glm::length(glm::vec3(t.matrix[1])),
            glm::length(glm::vec3(t.matrix[2]))
        };
    }

    inline glm::vec3 getRight(const Transform& t)
    {
        return glm::vec3(t.matrix[0]);
    }

    inline glm::vec3 getUp(const Transform& t)
    {
        return glm::vec3(t.matrix[1]);
    }

    inline glm::vec3 getBackward(const Transform& t)
    {
        return glm::vec3(t.matrix[2]);
    }

    inline glm::vec3 getForward(const Transform& t)
    {
        return -glm::vec3(t.matrix[2]);
    }

    inline bool isDirty(const Transform& t)
    {
        return t.isDirty;
    }

    inline glm::mat4 getLocalToChildMatrix(const Transform& parent, const Transform& child)
    {
        return glm::inverse(getLocalModelMatrix(parent)) * getLocalModelMatrix(child);
    }
}

#endif // TRANSFORH
