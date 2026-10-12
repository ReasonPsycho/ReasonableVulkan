//
// Created by redkc on 12/10/2026.
//

#ifndef REASONABLEVULKAN_THUMBNAILCOMPONENT_HPP
#define REASONABLEVULKAN_THUMBNAILCOMPONENT_HPP

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct ThumbnailComponent : public Component
    {
        [[=Tooltip{"Asset ID for thumbnail"}]]
        boost::uuids::uuid assetId{boost::uuids::nil_uuid()};

        [[=Tooltip{"Output thumbnail path"}]]
        std::string outputPath{""};

        [[=Tooltip{"Target width"}]]
        int targetWidth{128};

        [[=Tooltip{"Target height"}]]
        int targetHeight{128};

        [[=NonSerialized{}]]
        bool captured{false};

        [[=NonSerialized{}]]
        int framesRendered{0};

        ThumbnailComponent() = default;
        ThumbnailComponent(const boost::uuids::uuid& assetId, const std::string& outputPath, int targetWidth = 128, int targetHeight = 128)
            : assetId(assetId), outputPath(outputPath), targetWidth(targetWidth), targetHeight(targetHeight) {}
    };
}

#endif //REASONABLEVULKAN_THUMBNAILCOMPONENT_HPP
