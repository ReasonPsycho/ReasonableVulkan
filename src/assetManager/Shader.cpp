#include "Shader.h"


namespace am {
    void Shader::loadFromFile(const std::string &path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            spdlog::error("Failed to open shader file: {}", path);
            throw std::runtime_error("Failed to open shader file: " + path);       
        }

        // Get file size and rewind
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0);

        // SPIR-V files must be a multiple of 4 bytes
        if (fileSize % 4 != 0) {
            spdlog::error("Shader file size is not a multiple of 4 bytes: {}", path);
            throw std::runtime_error("Shader file size is not a multiple of 4 bytes: " + path);
        }

        // Create temporary vector to read file
        std::vector<std::uint32_t> code(fileSize / sizeof(std::uint32_t));

        // Read the file
        file.read(reinterpret_cast<char *>(code.data()), fileSize);

        if (!file) {
            spdlog::error("Failed to read shader file: {}", path);
            throw std::runtime_error("Failed to read shader file: " + path);
        }

        // Determine shader stage from file extension
        ShaderStage stage = ShaderStage::Vertex; // Default to vertex
        std::string extension = std::filesystem::path(path).extension().string();

        if (extension == ".vert" || extension == ".vsh") {
            stage = ShaderStage::Vertex;
        } else if (extension == ".frag" || extension == ".fsh") {
            stage = ShaderStage::Fragment;
        } else if (extension == ".comp") {
            stage = ShaderStage::Compute;
        } else if (extension == ".geom") {
            stage = ShaderStage::Geometry;
        } else if (extension == ".tesc") {
            stage = ShaderStage::TessellationControl;
        } else if (extension == ".tese") {
            stage = ShaderStage::TessellationEvaluation;
        }

        // Create and store the shader data
        shaderData = ShaderData{
            .bytecode = std::move(code),
            .stage = stage
        };

        spdlog::info("Loaded shader: {} (size: {} bytes)", path, fileSize);
    }

    size_t Shader::calculateContentHash() const {
        if (!shaderData) {
            return 0;
        }

        size_t hash = 0;

        // Hash both the bytecode and the shader stage
        for (const auto &word: shaderData->bytecode) {
            hash ^= std::hash<std::uint32_t>{}(word) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        // Combine with shader stage
        hash ^= std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(shaderData->stage));

        return hash;
    }

    AssetType Shader::getType() const {
        return AssetType::Shader;
    }
}
