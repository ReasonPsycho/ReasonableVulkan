//
// Created by redkc on 18/02/2024.
//
#include "Types.h"
#include "../systems/transformSystem/componets/TransformComponent.hpp"
#include "Scene.h"
#include <unordered_set>
#include "../assetManager/src/assets/engineAssets/PrefabAsset.h"
#include "assetDatas/ModelData.h"
#include "assetDatas/MeshData.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "tracy/Tracy.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"


using namespace engine::ecs;

am::AssetManagerInterface* engine::ecs::GetAssetManagerFromScene(Scene* scene) {
    if (!scene) return nullptr;
    return scene->engine.assetManagerInterface;
}

#include "systems/transformSystem/TransformSystem.h"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "NameComponent.hpp"
#include "TagComponent.hpp"

void Scene::AddComponent(Entity entity, std::type_index typeIdx)
{
    AddComponent(typeIdx);
    auto componentID = componentArrays[typeIdx]->AddComponentUntyped(entity);

    Signature& signature = entitySignatures[entity];
    signature.set(engine.GetComponentTypeID(typeIdx), true);

    // Check each system
    for (auto& [_, system] : systems)
    {
        for (auto type : system->registeredComponentTypes)
        {
            if (type == typeIdx)
            {
                system->AddComponent(componentID, typeIdx);
            }
        }
    }
}

size_t Scene::RegisteredComponentsSize() const
{
    return componentArrays.size();
}


std::type_index Scene::GetTypeFromIndex(std::size_t index) const
{
    return engine.GetComponentTypeFromID(index);
}

Scene::Scene(Engine& engine): engine(engine)
{
    ForEachType<EngineSystems>([this]<typename T>() {
        RegisterSystem<T>();
    });

    sceneId = boost::uuids::nil_uuid();
}

void Scene::SetActive(bool active) {
    this->active = active;
}

bool Scene::IsActive() const {
    return active;
}

void Scene::Update(float deltaTime) {
    for (auto& [_, system] : systems) {
        ZoneTransientN(zoneName,(system->name).c_str(),true);
        system->Update(deltaTime);
    }
}

Entity Scene::CreateEntity(Entity parentEntity)
{
    return CreateEntity(TransformComponent(),parentEntity);
}

const std::unordered_map<std::type_index, std::shared_ptr<SystemBase>> Scene::GetSystems()
{
    return systems;
}


bool Scene::IsAncestor(Entity potentialAncestor, Entity entity) const {
    Entity current = entity;
    while (HasParent(current)) {
        current = GetParent(current);
        if (current == potentialAncestor) {
            return true;
        }
    }
    return false;
}

void Scene::SetParent(Entity child, Entity parent) {
    assert(child < maxEntityIndex && parent < maxEntityIndex);

    // Prevent setting parent to itself
    if (child == parent) {
        return;
    }

    // Check for circular reference
    if (IsAncestor(child, parent)) {
        return;
    }

    // Remove child from previous parent and from rootEntities if needed
    RemoveParent(child);

    // Set new parent
    sceneGraph[child].parent = parent;
    sceneGraph[parent].children.push_back(child);

    // Remove child from rootEntities because it now has a parent
    rootEntities.erase(std::remove(rootEntities.begin(), rootEntities.end(), child), rootEntities.end());
}


void Scene::RemoveParent(Entity child) {
    assert(child < maxEntityIndex);

    auto& node = sceneGraph[child];
    if (node.parent != MAX_ENTITIES) {
        auto& siblings = sceneGraph[node.parent].children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        node.parent = MAX_ENTITIES;

        // Add child to rootEntities since it lost its parent
        rootEntities.push_back(child);
    }
}

Entity Scene::GetParent(Entity entity) const {
    auto it = sceneGraph.find(entity);
    if (it != sceneGraph.end()) {
        return it->second.parent;
    }
    return MAX_ENTITIES;
}

const std::vector<Entity>& Scene::GetChildren(Entity entity) const {
    static const std::vector<Entity> empty{};
    auto it = sceneGraph.find(entity);
    return it != sceneGraph.end() ? it->second.children : empty;
}

bool Scene::HasParent(Entity entity) const {
    auto it = sceneGraph.find(entity);
    return it != sceneGraph.end() && it->second.parent != MAX_ENTITIES;
}


Entity Scene::CreateEntity(TransformComponent transform ,Entity parentEntity ) {
    Entity entity;
    if (!freeEntities.empty()) {
        entity = freeEntities.front();
        freeEntities.pop();
    } else {
        entity = maxEntityIndex++;
    }

    AddComponent<TransformComponent>(entity, transform);
    AddComponent<NameComponent>(entity, NameComponent{"Entity"});

    if (parentEntity == -1)
    {
        rootEntities.push_back(entity);
    }else
    {
        auto parentNode = sceneGraph.find(parentEntity);
        parentNode->second.children.push_back(entity);
    }

    activeEntities.set(entity, true);
    return entity;
}

Entity Scene::CreateEntity(std::string entityName, Entity parentEntity)
{
    return CreateEntity(entityName,TransformComponent(),parentEntity);
}

Entity Scene::CreateEntity(std::string entityName, TransformComponent transform,  Entity parentEntity)
{
    auto entity = CreateEntity(transform,parentEntity);
    SetEntityName(entity, entityName);
    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    RemoveParent(entity);
    auto it = sceneGraph.find(entity);
    if (it != sceneGraph.end()) {
        for (Entity child : it->second.children) {
            sceneGraph[child].parent = MAX_ENTITIES;
            rootEntities.push_back(child);dcsa
        }
        sceneGraph.erase(it);
    }
    rootEntities.erase(std::remove(rootEntities.begin(), rootEntities.end(), entity), rootEntities.end());

    Signature signature = entitySignatures[entity]; // Get entity signature
    std::type_index componentIndex(typeid(void)); // Initialize with a dummy type
    for (size_t i = 0; i < signature.size(); ++i) {
        if (signature.test(i)) {
            componentIndex = GetTypeFromIndex(i);
            auto& array = componentArrays[componentIndex];
            array->RemoveComponentUntyped(entity);
        }
    }
    entitySignatures.erase(entity);
    activeEntities.reset(entity);

    for (auto& [_, system] : systems) {
        system->RemoveComponent(entity,componentIndex);
    }

    freeEntities.push(entity);  // add ID back for reuse
}


void Scene::SetEntityActive(Entity entity, bool active)
{
    activeEntities[entity] = active;
}

bool Scene::IsEntityActive(Entity entity) const
{
    return activeEntities[entity];
}

std::string Scene::GetEntityName(Entity entity) const
{
    auto it = componentArrays.find(std::type_index(typeid(NameComponent)));
    if (it != componentArrays.end()) {
        auto nameArray = static_cast<IntegralComponentArray<NameComponent>*>(it->second.get());
        if (nameArray->HasComponent(entity)) {
            const auto& comp = nameArray->GetComponentFromEntity(entity);
            if (!comp.name.empty()) {
                return comp.name;
            }
        }
    }
    return "Entity";
}

void Scene::SetEntityName(Entity entity, const std::string& name)
{
    if (HasComponent<NameComponent>(entity)) {
        GetComponent<NameComponent>(entity).name = name;
    } else {
        AddComponent<NameComponent>(entity, NameComponent{name});
    }

    auto editorSystem = engine.GetEditorSystem();
    if (editorSystem) {
        editorSystem->SetEntityName(entity, name, this);
    }
}

std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> Scene::GetComponentArrays()
{
    return componentArrays;
}


void Scene::SerializeToJson(rapidjson::Document& doc) const {
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("isEditable", isEditable, allocator);

    // Entities
    rapidjson::Value entitiesObj(rapidjson::kObjectType);
    SerializeEntities(entitiesObj, allocator);
    doc.AddMember("entities", entitiesObj, allocator);

    // Components
    rapidjson::Value componentsObj(rapidjson::kObjectType);
    SerializeComponents(componentsObj, allocator);
    doc.AddMember("components", componentsObj, allocator);

    // Systems
    rapidjson::Value systemsObj(rapidjson::kObjectType);
    SerializeSystems(systemsObj, allocator);
    doc.AddMember("systems", systemsObj, allocator);

    // Scene Graph
    rapidjson::Value sceneGraphObj(rapidjson::kObjectType);
    SerializeSceneGraph(sceneGraphObj, allocator);
    doc.AddMember("sceneGraph", sceneGraphObj, allocator);
}

void Scene::SerializeEntities(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    // Store maxEntityIndex
    obj.AddMember("maxEntityIndex", maxEntityIndex, allocator);

    // Store active entities
    rapidjson::Value activeEntitiesStr;
    std::string activeEntitiesString = activeEntities.to_string();
    activeEntitiesStr.SetString(activeEntitiesString.c_str(), allocator);
    obj.AddMember("activeEntities", activeEntitiesStr, allocator);

    // Store entity signatures
    rapidjson::Value signaturesObj(rapidjson::kObjectType);
    for (const auto& [entity, signature] : entitySignatures) {
        rapidjson::Value entityStr;
        std::string entityKey = std::to_string(entity);
        entityStr.SetString(entityKey.c_str(), allocator);

        rapidjson::Value signatureStr;
        std::string signatureString = signature.to_string();
        signatureStr.SetString(signatureString.c_str(), allocator);

        signaturesObj.AddMember(entityStr, signatureStr, allocator);
    }
    obj.AddMember("signatures", signaturesObj, allocator);
}

void Scene::SerializeComponents(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    for (const auto& [typeIndex, componentArray] : componentArrays) {
        rapidjson::Value typeStr;
        std::string_view typeName = componentArray->GetName();
        typeStr.SetString(typeName.data(), static_cast<rapidjson::SizeType>(typeName.size()), allocator);

        rapidjson::Value componentObj(rapidjson::kObjectType);
        componentArray->SerializeToJson(componentObj, allocator);

        obj.AddMember(typeStr, componentObj, allocator);
    }
}

void Scene::SerializeSystems(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    for (const auto& [typeIndex, system] : systems) {
        rapidjson::Value typeStr;
        std::string typeName = system->name;
        typeStr.SetString(typeName.c_str(), allocator);

        rapidjson::Value systemObj(rapidjson::kObjectType);
        system->SerializeToJson(systemObj, allocator);

        obj.AddMember(typeStr, systemObj, allocator);
    }
}

void Scene::SerializeSceneGraph(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    // Store root entities
    rapidjson::Value rootEntitiesArray(rapidjson::kArrayType);
    for (Entity entity : rootEntities) {
        rootEntitiesArray.PushBack(static_cast<uint64_t>(entity), allocator);
    }
    obj.AddMember("rootEntities", rootEntitiesArray, allocator);

    // Store scene graph nodes
    rapidjson::Value nodesObj(rapidjson::kObjectType);
    for (const auto& [entity, node] : sceneGraph) {
        rapidjson::Value nodeObj(rapidjson::kObjectType);

        // Store parent
        nodeObj.AddMember("parent", static_cast<uint64_t>(node.parent), allocator);

        // Store children
        rapidjson::Value childrenArray(rapidjson::kArrayType);
        for (Entity child : node.children) {
            childrenArray.PushBack(static_cast<uint64_t>(child), allocator);
        }
        nodeObj.AddMember("children", childrenArray, allocator);

        // Add node to nodes object
        rapidjson::Value entityStr;
        std::string entityKey = std::to_string(entity);
        entityStr.SetString(entityKey.c_str(), allocator);
        nodesObj.AddMember(entityStr, nodeObj, allocator);
    }
    obj.AddMember("nodes", nodesObj, allocator);
}

void Scene::CollectSubtreeEntities(Entity root, std::vector<Entity>& outSubtree) const {
    outSubtree.push_back(root);
    auto it = sceneGraph.find(root);
    if (it != sceneGraph.end()) {
        for (Entity child : it->second.children) {
            CollectSubtreeEntities(child, outSubtree);
        }
    }
}

void Scene::SerializeObjectToJson(Entity entity, rapidjson::Document& doc) const {
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    std::vector<Entity> subtree;
    CollectSubtreeEntities(entity, subtree);

    // Entities
    rapidjson::Value entitiesObj(rapidjson::kObjectType);
    SerializeObjectEntities(entity, subtree, entitiesObj, allocator);
    doc.AddMember("entities", entitiesObj, allocator);

    // Components
    rapidjson::Value componentsObj(rapidjson::kObjectType);
    SerializeObjectComponents(subtree, componentsObj, allocator);
    doc.AddMember("components", componentsObj, allocator);

    // Scene Graph
    rapidjson::Value sceneGraphObj(rapidjson::kObjectType);
    SerializeObjectSceneGraph(entity, subtree, sceneGraphObj, allocator);
    doc.AddMember("sceneGraph", sceneGraphObj, allocator);
}

void Scene::SerializePrefabToJson(Entity entity, rapidjson::Document& doc) const {
    SerializeObjectToJson(entity, doc);
}

void Scene::SerializeObjectEntities(Entity rootEntity, const std::vector<Entity>& subtree, rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    uint32_t maxIdx = 0;
    for (Entity e : subtree) {
        if (e >= maxIdx) maxIdx = e + 1;
    }
    obj.AddMember("maxEntityIndex", maxIdx, allocator);

    // Store active entities bitset
    rapidjson::Value activeEntitiesStr;
    std::string activeEntitiesString = activeEntities.to_string();
    activeEntitiesStr.SetString(activeEntitiesString.c_str(), allocator);
    obj.AddMember("activeEntities", activeEntitiesStr, allocator);

    // Store entity signatures for subtree
    rapidjson::Value signaturesObj(rapidjson::kObjectType);
    for (Entity e : subtree) {
        auto it = entitySignatures.find(e);
        if (it != entitySignatures.end()) {
            rapidjson::Value entityStr;
            std::string entityKey = std::to_string(e);
            entityStr.SetString(entityKey.c_str(), allocator);

            rapidjson::Value signatureStr;
            std::string signatureString = it->second.to_string();
            signatureStr.SetString(signatureString.c_str(), allocator);

            signaturesObj.AddMember(entityStr, signatureStr, allocator);
        }
    }
    obj.AddMember("signatures", signaturesObj, allocator);
}

void Scene::SerializeObjectComponents(const std::vector<Entity>& subtree, rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    for (const auto& [typeIndex, componentArray] : componentArrays) {
        rapidjson::Value componentObj(rapidjson::kObjectType);
        componentArray->SerializeEntitiesToJson(subtree, componentObj, allocator);

        if (componentObj.HasMember("components") && componentObj["components"].IsArray() && componentObj["components"].Size() > 0) {
            rapidjson::Value typeStr;
            std::string_view typeName = componentArray->GetName();
            typeStr.SetString(typeName.data(), static_cast<rapidjson::SizeType>(typeName.size()), allocator);
            obj.AddMember(typeStr, componentObj, allocator);
        }
    }
}

void Scene::SerializeObjectSceneGraph(Entity rootEntity, const std::vector<Entity>& subtree, rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    // Store root entities (in prefab, rootEntities is just the rootEntity)
    rapidjson::Value rootEntitiesArray(rapidjson::kArrayType);
    rootEntitiesArray.PushBack(static_cast<uint64_t>(rootEntity), allocator);
    obj.AddMember("rootEntities", rootEntitiesArray, allocator);

    std::unordered_set<Entity> subtreeSet(subtree.begin(), subtree.end());

    // Store scene graph nodes for subtree entities
    rapidjson::Value nodesObj(rapidjson::kObjectType);
    for (Entity entity : subtree) {
        auto it = sceneGraph.find(entity);
        rapidjson::Value nodeObj(rapidjson::kObjectType);

        if (it != sceneGraph.end()) {
            Entity parent = it->second.parent;
            if (entity == rootEntity || subtreeSet.find(parent) == subtreeSet.end()) {
                nodeObj.AddMember("parent", static_cast<uint64_t>(MAX_ENTITIES), allocator);
            } else {
                nodeObj.AddMember("parent", static_cast<uint64_t>(parent), allocator);
            }

            rapidjson::Value childrenArray(rapidjson::kArrayType);
            for (Entity child : it->second.children) {
                if (subtreeSet.find(child) != subtreeSet.end()) {
                    childrenArray.PushBack(static_cast<uint64_t>(child), allocator);
                }
            }
            nodeObj.AddMember("children", childrenArray, allocator);
        } else {
            nodeObj.AddMember("parent", static_cast<uint64_t>(MAX_ENTITIES), allocator);
            rapidjson::Value childrenArray(rapidjson::kArrayType);
            nodeObj.AddMember("children", childrenArray, allocator);
        }

        rapidjson::Value entityStr;
        std::string entityKey = std::to_string(entity);
        entityStr.SetString(entityKey.c_str(), allocator);
        nodesObj.AddMember(entityStr, nodeObj, allocator);
    }
    obj.AddMember("nodes", nodesObj, allocator);
}

Entity Scene::DeserializeObjectFromJson(const rapidjson::Document& doc, Entity parentEntity) {
    return InstantiatePrefab(doc, parentEntity);
}

Entity Scene::InstantiatePrefab(const rapidjson::Document& doc, Entity parentEntity) {
    if (doc.HasMember("components") && doc["components"].IsObject()) {
        const auto& registeredComponents = engine.GetRegisteredComponentTypes();

        for (auto it = doc["components"].MemberBegin(); it != doc["components"].MemberEnd(); ++it) {
            std::string typeName = it->name.GetString();
            bool found = false;

            auto typeOpt = engine.GetComponentTypeByName(typeName);
            if (typeOpt.has_value()) {
                if (componentArrays.find(*typeOpt) == componentArrays.end()) {
                    AddComponent(*typeOpt);
                }
                found = true;
            } else {
                for (const auto& type : registeredComponents) {
                    if (type.name() == typeName) {
                        if (componentArrays.find(type) == componentArrays.end()) {
                            AddComponent(type);
                        }
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                throw std::runtime_error("Unknown component type in prefab file: " + typeName);
            }
        }
    }

    Entity prefabRootEntity = MAX_ENTITIES;
    if (doc.HasMember("sceneGraph") && doc["sceneGraph"].IsObject() &&
        doc["sceneGraph"].HasMember("rootEntities") && doc["sceneGraph"]["rootEntities"].IsArray() &&
        doc["sceneGraph"]["rootEntities"].Size() > 0) {
        prefabRootEntity = static_cast<Entity>(doc["sceneGraph"]["rootEntities"][0].GetUint64());
    }

    std::unordered_map<Entity, Entity> oldToNewEntityMap;

    if (doc.HasMember("sceneGraph") && doc["sceneGraph"].IsObject() &&
        doc["sceneGraph"].HasMember("nodes") && doc["sceneGraph"]["nodes"].IsObject()) {
        const auto& nodes = doc["sceneGraph"]["nodes"];
        for (auto it = nodes.MemberBegin(); it != nodes.MemberEnd(); ++it) {
            Entity oldEntity = std::stoul(it->name.GetString());
            Entity newEntity;
            if (!freeEntities.empty()) {
                newEntity = freeEntities.front();
                freeEntities.pop();
            } else {
                newEntity = maxEntityIndex++;
            }
            activeEntities.set(newEntity, true);
            oldToNewEntityMap[oldEntity] = newEntity;
        }
    } else if (doc.HasMember("entities") && doc["entities"].IsObject() &&
               doc["entities"].HasMember("signatures") && doc["entities"]["signatures"].IsObject()) {
        const auto& signatures = doc["entities"]["signatures"];
        for (auto it = signatures.MemberBegin(); it != signatures.MemberEnd(); ++it) {
            Entity oldEntity = std::stoul(it->name.GetString());
            Entity newEntity;
            if (!freeEntities.empty()) {
                newEntity = freeEntities.front();
                freeEntities.pop();
            } else {
                newEntity = maxEntityIndex++;
            }
            activeEntities.set(newEntity, true);
            oldToNewEntityMap[oldEntity] = newEntity;
        }
    }

    if (oldToNewEntityMap.empty()) {
        return MAX_ENTITIES;
    }

    if (prefabRootEntity == MAX_ENTITIES || oldToNewEntityMap.find(prefabRootEntity) == oldToNewEntityMap.end()) {
        prefabRootEntity = oldToNewEntityMap.begin()->first;
    }
    Entity createdRootEntity = oldToNewEntityMap[prefabRootEntity];

    // Hierarchy setup
    if (doc.HasMember("sceneGraph") && doc["sceneGraph"].IsObject() &&
        doc["sceneGraph"].HasMember("nodes") && doc["sceneGraph"]["nodes"].IsObject()) {
        const auto& nodes = doc["sceneGraph"]["nodes"];
        for (auto it = nodes.MemberBegin(); it != nodes.MemberEnd(); ++it) {
            Entity oldEntity = std::stoul(it->name.GetString());
            Entity newEntity = oldToNewEntityMap[oldEntity];
            const auto& nodeObj = it->value;

            Entity oldParent = MAX_ENTITIES;
            if (nodeObj.HasMember("parent") && nodeObj["parent"].IsUint64()) {
                oldParent = static_cast<Entity>(nodeObj["parent"].GetUint64());
            }

            if (oldParent != MAX_ENTITIES && oldToNewEntityMap.find(oldParent) != oldToNewEntityMap.end()) {
                SetParent(newEntity, oldToNewEntityMap[oldParent]);
            } else if (oldEntity == prefabRootEntity) {
                if (parentEntity != -1 && parentEntity != MAX_ENTITIES) {
                    SetParent(newEntity, parentEntity);
                } else {
                    rootEntities.push_back(newEntity);
                }
            } else {
                rootEntities.push_back(newEntity);
            }
        }
    } else {
        if (parentEntity != -1 && parentEntity != MAX_ENTITIES) {
            SetParent(createdRootEntity, parentEntity);
        } else {
            rootEntities.push_back(createdRootEntity);
        }
    }

    // Deserializing components
    if (doc.HasMember("components") && doc["components"].IsObject()) {
        const auto& registeredComponents = engine.GetRegisteredComponentTypes();

        for (auto it = doc["components"].MemberBegin(); it != doc["components"].MemberEnd(); ++it) {
            std::string typeName = it->name.GetString();
            std::optional<std::type_index> targetTypeIndex;

            auto typeOpt = engine.GetComponentTypeByName(typeName);
            if (typeOpt.has_value()) {
                targetTypeIndex = *typeOpt;
            } else {
                for (const auto& type : registeredComponents) {
                    if (type.name() == typeName) {
                        targetTypeIndex = type;
                        break;
                    }
                }
            }

            if (!targetTypeIndex.has_value()) continue;

            auto arrayIt = componentArrays.find(*targetTypeIndex);
            if (arrayIt == componentArrays.end()) continue;

            auto& array = arrayIt->second;
            const auto& compTypeObj = it->value;

            if (compTypeObj.HasMember("components") && compTypeObj["components"].IsArray()) {
                const auto& compsArray = compTypeObj["components"];
                for (rapidjson::SizeType c = 0; c < compsArray.Size(); ++c) {
                    const auto& compObj = compsArray[c];
                    if (compObj.HasMember("entity") && compObj.HasMember("data")) {
                        Entity oldEntity = compObj["entity"].GetUint64();
                        auto mapIt = oldToNewEntityMap.find(oldEntity);
                        if (mapIt == oldToNewEntityMap.end()) continue;
                        Entity newEntity = mapIt->second;

                        bool active = true;
                        if (compObj.HasMember("active") && compObj["active"].IsBool()) {
                            active = compObj["active"].GetBool();
                        }

                        // Ensure component is added to entity in scene
                        if (!array->HasComponentUntyped(newEntity)) {
                            AddComponent(newEntity, *targetTypeIndex);
                        }
                        array->DeserializeEntityComponent(newEntity, compObj["data"], active);
                    }
                }
            }
        }
    }

    return createdRootEntity;
}

Entity Scene::InstantiatePrefab(boost::uuids::uuid prefabUuid, Entity parentEntity) {
    if (prefabUuid.is_nil()) return MAX_ENTITIES;
    auto am = engine.assetManagerInterface;
    if (!am) return MAX_ENTITIES;
    auto assetInfo = am->getAssetInfo(prefabUuid);
    if (!assetInfo) return MAX_ENTITIES;
    auto prefabAsset = dynamic_cast<am::PrefabAsset*>(assetInfo->get()->getAsset());
    if (!prefabAsset) return MAX_ENTITIES;
    rapidjson::Document* doc = prefabAsset->getAssetDataAs<rapidjson::Document>();
    if (!doc) return MAX_ENTITIES;
    return InstantiatePrefab(*doc, parentEntity);
}

Entity Scene::InstantiateModel(boost::uuids::uuid modelOrMeshUuid, Entity parentEntity) {
    if (!engine.assetManagerInterface || modelOrMeshUuid.is_nil()) {
        return MAX_ENTITIES;
    }

    auto infoOpt = engine.assetManagerInterface->getAssetInfo(modelOrMeshUuid);
    if (!infoOpt.has_value()) {
        return MAX_ENTITIES;
    }

    auto* assetInfo = infoOpt.value().get();
    auto assetType = assetInfo->type;

    boost::uuids::uuid defaultShaderUuid = boost::uuids::nil_uuid();
    auto shaderOpt = engine.assetManagerInterface->getAssetUuid("pbrShader");
    if (shaderOpt) {
        defaultShaderUuid = shaderOpt.value();
    }

    if (assetType == am::AssetType::Mesh) {
        std::string entityName = assetInfo->lookUpName.empty() ? "Mesh" : assetInfo->lookUpName;
        Entity entity = (parentEntity != MAX_ENTITIES && parentEntity != -1)
                        ? CreateEntity(entityName, parentEntity)
                        : CreateEntity(entityName);

        AddComponent<MeshComponent>(entity, MeshComponent(modelOrMeshUuid));

        boost::uuids::uuid materialUuid = boost::uuids::nil_uuid();
        auto meshData = engine.assetManagerInterface->getAssetData<am::MeshData>(modelOrMeshUuid);
        if (meshData && meshData->material) {
            materialUuid = meshData->material->id;
        }

        RendererComponent rendererComp(defaultShaderUuid, materialUuid);
        AddComponent<RendererComponent>(entity, rendererComp);

        return entity;
    }

    if (assetType == am::AssetType::Model) {
        auto modelData = engine.assetManagerInterface->getAssetData<am::ModelData>(modelOrMeshUuid);
        if (!modelData) {
            return MAX_ENTITIES;
        }

        std::string rootName = assetInfo->lookUpName.empty() ? "Model" : assetInfo->lookUpName;

        std::function<Entity(const am::Node&, Entity, bool)> buildNodeEntity =
            [&](const am::Node& node, Entity parent, bool isRoot) -> Entity {

            std::string nodeName = isRoot ? rootName : (node.mName.empty() ? "Node" : node.mName);

            TransformComponent nodeTransform;
            glm::vec3 pos(0.0f);
            glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 sc(1.0f);
            decomposeMtx(node.mTransformation, pos, rot, sc);
            nodeTransform.position = pos;
            nodeTransform.rotation = rot;
            nodeTransform.scale = sc;

            Entity nodeEntity;
            if (parent != MAX_ENTITIES && parent != -1) {
                nodeEntity = CreateEntity(nodeName, nodeTransform, parent);
            } else {
                nodeEntity = CreateEntity(nodeName, nodeTransform);
            }

            if (node.meshes.size() == 1) {
                auto meshInfo = node.meshes[0];
                if (meshInfo) {
                    boost::uuids::uuid meshUuid = meshInfo->id;
                    boost::uuids::uuid materialUuid = boost::uuids::nil_uuid();
                    auto meshData = engine.assetManagerInterface->getAssetData<am::MeshData>(meshUuid);
                    if (meshData && meshData->material) {
                        materialUuid = meshData->material->id;
                    }

                    AddComponent<MeshComponent>(nodeEntity, MeshComponent(meshUuid));
                    AddComponent<RendererComponent>(nodeEntity, RendererComponent(defaultShaderUuid, materialUuid));
                }
            } else if (node.meshes.size() > 1) {
                for (size_t i = 0; i < node.meshes.size(); ++i) {
                    auto meshInfo = node.meshes[i];
                    if (!meshInfo) continue;

                    boost::uuids::uuid meshUuid = meshInfo->id;
                    boost::uuids::uuid materialUuid = boost::uuids::nil_uuid();
                    auto meshData = engine.assetManagerInterface->getAssetData<am::MeshData>(meshUuid);
                    if (meshData && meshData->material) {
                        materialUuid = meshData->material->id;
                    }

                    std::string meshEntityName = meshInfo->lookUpName.empty()
                                                ? (nodeName + "_Mesh_" + std::to_string(i))
                                                : meshInfo->lookUpName;
                    Entity meshChildEntity = CreateEntity(meshEntityName, nodeEntity);
                    AddComponent<MeshComponent>(meshChildEntity, MeshComponent(meshUuid));
                    AddComponent<RendererComponent>(meshChildEntity, RendererComponent(defaultShaderUuid, materialUuid));
                }
            }

            for (const auto& childNode : node.mChildren) {
                buildNodeEntity(childNode, nodeEntity, false);
            }

            return nodeEntity;
        };

        return buildNodeEntity(modelData->rootNode, parentEntity, true);
    }

    return MAX_ENTITIES;
}

void Scene::DeserializeFromJson(const rapidjson::Document& doc) {

    if (doc.HasMember("isEditable") && doc["isEditable"].IsBool()) {
        isEditable = doc["isEditable"].GetBool();
    } else {
        isEditable = true;
    }

    //TODO here are going to be problems if there are different systems
    sceneGraph.clear();
    componentArrays.clear();
    rootEntities.clear();
    maxEntityIndex = 0;
    freeEntities = std::queue<Entity>{};
    entitySignatures.clear();
    activeEntities.reset();

    // First, ensure all required components are registered
    if (doc.HasMember("components") && doc["components"].IsObject()) {
        const auto& registeredComponents = engine.GetRegisteredComponentTypes();

        for (auto it = doc["components"].MemberBegin(); it != doc["components"].MemberEnd(); ++it) {
            std::string typeName = it->name.GetString();
            bool found = false;

            // Check if there is a match by clean name in engine
            auto typeOpt = engine.GetComponentTypeByName(typeName);
            if (typeOpt.has_value()) {
                if (componentArrays.find(*typeOpt) == componentArrays.end()) {
                    AddComponent(*typeOpt);
                }
                found = true;
            } else {
                // Fallback for legacy mangled names
                for (const auto& type : registeredComponents) {
                    if (type.name() == typeName) {
                        if (componentArrays.find(type) == componentArrays.end()) {
                            AddComponent(type);
                        }
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                // Log warning or throw exception for unknown component type
                throw std::runtime_error("Unknown component type in scene file: " + typeName);
            }
        }
    }

    // Then, ensure all required systems are registered
    if (doc.HasMember("systems") && doc["systems"].IsObject()) {
        const auto& registeredSystems = engine.GetRegisteredSystemTypes();

        for (auto it = doc["systems"].MemberBegin(); it != doc["systems"].MemberEnd(); ++it) {
            std::string typeName = it->name.GetString();
            bool found = false;

            auto typeOpt = engine.GetSystemTypeByName(typeName);
            if (typeOpt.has_value()) {
                if (systems.find(*typeOpt) == systems.end()) {
                    RegisterSystem(*typeOpt);
                }
                found = true;
            } else {
                // Fallback for legacy mangled names
                for (const auto& type : registeredSystems) {
                    if (type.name() == typeName) {
                        if (systems.find(type) == systems.end()) {
                            RegisterSystem(type);
                        }
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                // Log warning or throw exception for unknown system type
                throw std::runtime_error("Unknown system type in scene file: " + typeName);
            }
        }
    }

    // Now proceed with the actual deserialization
    DeserializeEntities(doc["entities"]);
    DeserializeComponents(doc["components"]);
    DeserializeSystems(doc["systems"]);
    DeserializeSceneGraph(doc["sceneGraph"]);
}


void Scene::AddComponent(const std::type_index& type) {
    if (componentArrays.find(type) == componentArrays.end()) {
        componentArrays[type] = engine.CreateComponentArray(type);
    }
}

CameraObject Scene::GetActiveCamera()
    {
        static TransformComponent defaultTransform;
        static CameraComponent defaultCamera;

        auto editorSystem = engine.GetEditorSystem();
        if (editorSystem && editorSystem->inEditMode && editorSystem->GetTargetScene() == this)
        {
            return {&editorSystem->camera, &editorSystem->cameraTransform};
        }
        else
        {
            auto& cameras = GetComponentArray<CameraComponent>().get()->GetComponents();
            auto& transforms = GetIntegralComponentArray<TransformComponent>().get()->GetComponents();

            for (int i = 0; i < cameras.size(); i++)
            {
                if (GetComponentArray<CameraComponent>().get()->IsComponentActive(i))
                {
                    auto cameraEntity = GetComponentArray<CameraComponent>().get()->ComponentIndexToEntity(i);
                    return {&cameras[cameraEntity], &transforms[cameraEntity]};
                }
            }
        }

        // If no active camera found, return default camera
        return {&defaultCamera, &defaultTransform};
    }

void Scene::RegisterSystem(const std::type_index& type) {
    if (systems.find(type) == systems.end()) {
        systems[type] = engine.CreateSystem(type, this);
    }
}
void Scene::DeserializeEntities(const rapidjson::Value& obj) {
    // Restore maxEntityIndex
    if (obj.HasMember("maxEntityIndex") && obj["maxEntityIndex"].IsUint()) {
        maxEntityIndex = obj["maxEntityIndex"].GetUint();
    }

    // Restore active entities
    if (obj.HasMember("activeEntities") && obj["activeEntities"].IsString()) {
        std::string activeEntitiesStr = obj["activeEntities"].GetString();
        activeEntities = std::bitset<MAX_ENTITIES>(activeEntitiesStr);
    }

    // Restore entity signatures
    if (obj.HasMember("signatures") && obj["signatures"].IsObject()) {
        const auto& signatures = obj["signatures"];
        for (auto it = signatures.MemberBegin(); it != signatures.MemberEnd(); ++it) {
            Entity entity = std::stoul(it->name.GetString());
            std::string signatureStr = it->value.GetString();
            entitySignatures[entity] = std::bitset<MAX_COMPONENTS>(signatureStr);
        }
    }
}

void Scene::DeserializeComponents(const rapidjson::Value& obj) {
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
        std::string typeName = it->name.GetString();
        for (const auto& [typeIndex, componentArray] : componentArrays) {
            if (componentArray->GetName() == typeName || typeIndex.name() == typeName) {
                componentArray->DeserializeFromJson(it->value);
                break;
            }
        }
    }
}

void Scene::DeserializeSystems(const rapidjson::Value& obj) {
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
        std::string typeName = it->name.GetString();
        for (const auto& [typeIndex, system] : systems) {
            if (system->name == typeName || typeIndex.name() == typeName) {
                system->DeserializeFromJson(it->value);
                break;
            }
        }
    }
}

void Scene::DeserializeSceneGraph(const rapidjson::Value& obj) {
    // Restore root entities
    if (obj.HasMember("rootEntities") && obj["rootEntities"].IsArray()) {
        const auto& rootArray = obj["rootEntities"];
        for (rapidjson::SizeType i = 0; i < rootArray.Size(); i++) {
            rootEntities.push_back(static_cast<Entity>(rootArray[i].GetUint64()));
        }
    }

    // Restore scene graph nodes
    if (obj.HasMember("nodes") && obj["nodes"].IsObject()) {
        const auto& nodes = obj["nodes"];
        for (auto it = nodes.MemberBegin(); it != nodes.MemberEnd(); ++it) {
            Entity entity = std::stoul(it->name.GetString());
            const auto& nodeObj = it->value;

            TransformNode node;

            // Restore parent
            if (nodeObj.HasMember("parent") && nodeObj["parent"].IsUint64()) {
                node.parent = static_cast<Entity>(nodeObj["parent"].GetUint64());
            }

            // Restore children
            if (nodeObj.HasMember("children") && nodeObj["children"].IsArray()) {
                const auto& childrenArray = nodeObj["children"];
                for (rapidjson::SizeType i = 0; i < childrenArray.Size(); i++) {
                    node.children.push_back(static_cast<Entity>(childrenArray[i].GetUint64()));
                }
            }

            sceneGraph[entity] = node;
        }
    }
}