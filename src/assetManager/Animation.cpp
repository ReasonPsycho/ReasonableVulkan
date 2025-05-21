//
// Created by redkc on 09/06/2024.
//

#include "Animation.h"

namespace am {
    Animation::Animation(AssetFactoryData animation_factory_context): Asset(animation_factory_context) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(animation_factory_context.path, aiProcess_Triangulate);
        assert(scene && scene->mRootNode);
        auto animation = scene->mAnimations[0];
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;
        ReadHeirarchyData(m_RootNode, scene->mRootNode);
        AssetFactoryData modelFactoryData{animation_factory_context}; // Construct new context from base
        modelFactoryData.assetType = AssetType::Model;
    std:
        shared_ptr<AssetInfo> modelInfo =
                animation_factory_context.assetManager.registerAsset(&modelFactoryData);
        ReadMissingBones(animation, dynamic_cast<Model &>(*modelInfo->getAsset()));
    }

    Bone *Animation::FindBone(const string &name) {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
                                 [&](const Bone &Bone) {
                                     return Bone.GetBoneName() == name;
                                 }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }

    float Animation::GetTicksPerSecond() { return m_TicksPerSecond; }

    float Animation::GetDuration() { return m_Duration; }

    const AssimpNodeData &Animation::GetRootNode() { return m_RootNode; }

    const std::map<std::string, BoneInfo> &Animation::GetBoneIDMap() {
        return m_BoneInfoMap;
    }

    void Animation::ReadMissingBones(const aiAnimation *animation, Model &model) {
        int size = animation->mNumChannels;

        auto &boneInfoMap = model.GetBoneInfoMap(); //getting m_BoneInfoMap from Model class
        int &boneCount = model.GetBoneCount(); //getting the m_BoneCounter from Model class

        //reading channels(bones engaged in an animation and their keyframes)
        for (int i = 0; i < size; i++) {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            m_Bones.push_back(Bone(channel->mNodeName.data,
                                   boneInfoMap[channel->mNodeName.data].id, channel));
        }

        m_BoneInfoMap = boneInfoMap;
    }

    void Animation::ReadHeirarchyData(AssimpNodeData &dest, const aiNode *src) {
        assert(src);

        dest.name = src->mName.data;
        dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++) {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }


    Animation::~Animation() {
    }

    size_t Animation::calculateContentHash() const {
        size_t hash = 0;

        // Hash duration and ticks per second
        hash = std::hash<float>{}(m_Duration);
        hash ^= std::hash<int>{}(m_TicksPerSecond) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

        // Hash bones
        for (const auto &bone: m_Bones) {
            const std::string &boneName = bone.GetBoneName();
            hash ^= std::hash<std::string>{}(boneName) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        // Hash bone info map
        for (const auto &[boneName, boneInfo]: m_BoneInfoMap) {
            hash ^= std::hash<std::string>{}(boneName) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int>{}(boneInfo.id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }

    AssetType Animation::getType() const {
        return AssetType::Animation;
    }

    glm::mat4 Animation::GetBoneTranslationMatrix(string name) {
        auto it = m_BoneInfoMap.find(name);
        std::vector<glm::mat4> boneTranslationPath;
        while (it != m_BoneInfoMap.end()) {
            BoneInfo info = it->second;
            boneTranslationPath.push_back(info.offset);
            it = m_BoneInfoMap.find(info.parentNode);
        }
        glm::mat4 translationMatrix = glm::mat4(1);
        std::reverse(boneTranslationPath.begin(), boneTranslationPath.end());
        for (int i = 0; i < boneTranslationPath.size(); ++i) {
            translationMatrix *= boneTranslationPath[i];
        }
        return translationMatrix;
    }

    int Animation::GetBoneIdFromName(string name) {
        auto it = m_BoneInfoMap.find(name);
        if (it != m_BoneInfoMap.end()) {
            return it->second.id;
        } else {
            return -1;
        }
    }
}
