//
// Created by redkc on 01/10/2025.
//

#ifndef REASONABLEVULKAN_RENDERERCOMPONENT_HPP
#define REASONABLEVULKAN_RENDERERCOMPONENT_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Handle.hpp"
#include "ecs/Component.hpp"

namespace engine::ecs
{
    class Scene;

    struct RendererComponent : public Component
    {
        [[=Tooltip{"Shader program asset UUID"}]]
        boost::uuids::uuid shaderUuid{boost::uuids::nil_uuid()};

        [[=NonSerialized{}]]
        gfx::ShaderProgramHandle runtimeShaderHandle = gfx::ShaderProgramHandle::invalid();

        [[=Tooltip{"Material asset UUID"}]]
        boost::uuids::uuid materialUuid{boost::uuids::nil_uuid()};

        [[=NonSerialized{}]]
        gfx::MaterialHandle runtimeMaterialHandle = gfx::MaterialHandle::invalid();

        RendererComponent() = default;
        explicit RendererComponent(boost::uuids::uuid shaderId)
            : shaderUuid(shaderId) {}
        explicit RendererComponent(boost::uuids::uuid shaderId, gfx::ShaderProgramHandle handle)
            : shaderUuid(shaderId), runtimeShaderHandle(handle) {}
        explicit RendererComponent(boost::uuids::uuid shaderId, boost::uuids::uuid materialId)
            : shaderUuid(shaderId), materialUuid(materialId) {}
        explicit RendererComponent(boost::uuids::uuid shaderId, gfx::ShaderProgramHandle handle, boost::uuids::uuid materialId, gfx::MaterialHandle matHandle)
            : shaderUuid(shaderId), runtimeShaderHandle(handle), materialUuid(materialId), runtimeMaterialHandle(matHandle) {}

        void CustomDrawImGui(Scene* scene);
    };
}

#endif //REASONABLEVULKAN_RENDERERCOMPONENT_HPP