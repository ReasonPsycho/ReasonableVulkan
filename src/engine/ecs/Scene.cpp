//
// Created by redkc on 18/02/2024.
//
#include "Types.h"
#include "../systems/transformSystem/componets/Transform.hpp"
#include "Scene.h"
#include "tracy/Tracy.hpp"
#include "systems/editorSystem/EditorSystem.hpp"


using namespace engine::ecs;

#include "systems/transformSystem/TransformSystem.h"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/renderingSystem/componets/Camera.hpp"
#include "systems/renderingSystem/componets/Model.hpp"

void Scene::AddComponent(Entity entity, std::type_index typeIdx)
{
    componentArrays[typeIdx]->AddComponentUntyped(entity);

    Signature& signature = entitySignatures[entity];
    signature.set(GetUniqueComponentTypeID(), true);

    // Check each system
    for (auto& [_, system] : systems) {
        if ((signature & system->signature) == system->signature) {
            system->AddEntity(entity);
        }
    }
}

size_t Scene::RegisteredComponentsSize() const
{
    return componentArrays.size();
}


std::type_index Scene::GetTypeFromIndex(std::size_t index) const
{
    auto it = indexToType.find(index);
    assert(it != indexToType.end() && "Index not registered.");
    return it->second;
}

Scene::Scene(Engine& engine): engine(engine)
{
    RegisterSystem<RenderSystem>();

#ifdef EDITOR_ENABLED
    RegisterSystem<EditorSystem>();
    GetSystem<EditorSystem>().get()->RegisterComponentType<Transform>();
    GetSystem<EditorSystem>().get()->RegisterComponentType<Model>();
    GetSystem<EditorSystem>().get()->RegisterComponentType<Camera>();
#endif
    RegisterComponent<Model>(); //For some reason I have to register them in reverse
    RegisterComponent<Camera>();

    RegisterIntegralComponent<Transform>();
    RegisterSystem<TransformSystem>();
}

void Scene::Update(float deltaTime) {
    engine.graphicsEngine->beginFrame();
    for (auto& [_, system] : systems) {
    ZoneTransientN(zoneName,(system->name).c_str(),true);
        system->Update(deltaTime);
    }
    engine.graphicsEngine->endFrame();
}

Entity Scene::CreateEntity(Entity parentEntity)
{
    return CreateEntity(Transform(),parentEntity);
}

const std::unordered_map<std::type_index, std::shared_ptr<SystemBase>> Scene::GetSystems()
{
    return systems;
}

void Scene::SetParent(Entity child, Entity parent) {
    assert(child < maxEntityIndex && parent < maxEntityIndex);

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


Entity Scene::CreateEntity(Transform transform ,Entity parentEntity ) {
    Entity entity;
    if (!freeEntities.empty()) {
        entity = freeEntities.front();
        freeEntities.pop();
    } else {
        entity = maxEntityIndex++;
    }

    AddComponent<Transform>(entity,transform);


    if (parentEntity == -1)
    {
        rootEntities.push_back(entity);
    }else
    {
        auto parentNode = sceneGraph.find(parentEntity);
        parentNode->second.children.push_back(entity);
    }

    auto signature = Signature{};
    signature.set(GetComponentTypeID<Transform>());
    entitySignatures[entity] = signature;

    activeEntities.set(entity, true);
    return entity;
}

Entity Scene::CreateEntity(std::string entityName, Entity parentEntity)
{
    return CreateEntity(entityName,Transform(),parentEntity);
}

Entity Scene::CreateEntity(std::string entityName, Transform transform,  Entity parentEntity)
{
    auto entity = CreateEntity(transform,parentEntity);
    GetSystem<engine::ecs::EditorSystem>().get()->SetEntityName(entity,entityName);
    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    Signature signature = entitySignatures[entity]; // Get entity signature
    for (size_t i = 0; i < signature.size(); ++i) {
        if (signature.test(i)) {
            auto componentIndex = GetTypeFromIndex(i);
            auto& array = componentArrays[componentIndex];
            array->RemoveComponentUntyped(entity);
        }
    }
    entitySignatures.erase(entity);
    activeEntities.reset(entity);

    for (auto& [_, system] : systems) {
        system->RemoveEntity(entity);
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

std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> Scene::GetComponentArrays()
{
    return componentArrays;
}


void Scene::SerializeToJson(rapidjson::Document& doc) const {
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

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
        std::string typeName = typeIndex.name();
        typeStr.SetString(typeName.c_str(), allocator);

        rapidjson::Value componentObj(rapidjson::kObjectType);
        componentArray->SerializeToJson(componentObj, allocator);

        obj.AddMember(typeStr, componentObj, allocator);
    }
}

void Scene::SerializeSystems(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const {
    for (const auto& [typeIndex, system] : systems) {
        rapidjson::Value typeStr;
        std::string typeName = typeIndex.name();
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

void Scene::DeserializeFromJson(const rapidjson::Document& doc) {

    //TODO here are going to be problems if there are different systems
    sceneGraph.clear();
    componentArrays.clear();
    rootEntities.clear();
    maxEntityIndex = 0;
    freeEntities = std::queue<Entity>{};
    entitySignatures.clear();
    activeEntities.reset();
    indexToType.clear();

    // First, ensure all required components are registered
    if (doc.HasMember("components") && doc["components"].IsObject()) {
        const auto& registeredComponents = engine.GetRegisteredComponentTypes();

        for (auto it = doc["components"].MemberBegin(); it != doc["components"].MemberEnd(); ++it) {
            std::string typeName = it->name.GetString();
            bool found = false;

            // Find and register the component if needed
            for (const auto& type : registeredComponents) {
                if (type.name() == typeName) {
                    // Check if component is already registered in scene
                    if (componentArrays.find(type) == componentArrays.end()) {
                        // Register the component using type information
                        AddComponent(type);
                    }
                    found = true;
                    break;
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

            // Find and register the system if needed
            for (const auto& type : registeredSystems) {
                if (type.name() == typeName) {
                    // Check if system is already registered in scene
                    if (systems.find(type) == systems.end()) {
                        // Register the system using type information
                        RegisterSystem(type);
                    }
                    found = true;
                    break;
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
            if (typeIndex.name() == typeName) {
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
            if (typeIndex.name() == typeName) {
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