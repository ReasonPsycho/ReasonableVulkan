//
// Created by redkc on 02/12/2023.
//

#ifndef OPENGLGP_MODEL_H
#define OPENGLGP_MODEL_H


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Asset.hpp"
#include "AssetFactoryRegistry.hpp"

using namespace std;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "map"
#include <string>
#include <vector>
#include "AssimpGLMHelpers.h"
#include "Texture.h"
#include "Mesh.h"
namespace ae {
    
    struct BoneInfo
    {
        /*id is index in finalBoneMatrices*/
        int id;
        string parentNode;
        /*offset matrix transforms vertex from model space to bone space*/
        glm::mat4 offset;

    };

    class Model :  public Asset {
        static inline AssetFactoryRegistry::Registrar<Model> registrar{AssetType::Model};
        public:
        // model data 
        vector<std::shared_ptr<Texture>> textureCatalogue;    // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
        vector<Mesh> meshes;
        string directory;

        std::map<string, BoneInfo> m_BoneInfoMap; //
        int m_BoneCounter = 0;

        auto& GetBoneInfoMap() { return m_BoneInfoMap; }
        int& GetBoneCount() { return m_BoneCounter; }
        void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
        void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
        void Normalize(Vertex& vertex);

        void SetVertexBoneDataToDefault(Vertex& vertex);


        Model(){};
    
        void loadFromFile(const std::string &path) override;
    
        string const *path;
        private:

        void processNode(aiNode *node, const aiScene *scene);
        Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    };

}

#endif //OPENGLGP_MODEL_H
