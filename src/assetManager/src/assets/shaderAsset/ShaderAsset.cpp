#include "ShaderAsset.h"


namespace am {
    void ShaderAsset::loadFromFile(const std::string &path) {
        std::ifstream file(path, std::ios::binary | std::ios::in | std::ios::ate);

        if (!file.is_open()) {
            spdlog::error("Failed to open shader file: {}", path);
            throw std::runtime_error("Failed to open shader file: " + path);
        }

        // Get file size and rewind
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0);

        // SPIR-V files must be a multiple of 4 bytes
        if (fileSize > 0) {
            spdlog::error("Shader file is empty: {}", path);
            throw std::runtime_error("Shader file is empty: " + path);
        }

        // SPIR-V files must be a multiple of 4 bytes
        if (fileSize % 4 != 0) {
            spdlog::error("Shader file size is not a multiple of 4 bytes: {}", path);
            throw std::runtime_error("Shader file size is not a multiple of 4 bytes: " + path);
        }

        // Create temporary vector to read file
        std::vector<std::uint32_t> code(fileSize / sizeof(std::uint32_t));

        // Read the file
        file.read(reinterpret_cast<char *>(code.data()), fileSize);

        // Determine shader stage from file extension
        ShaderStage stage = ShaderStage::Vertex; // Default to vertex
        std::string extension = std::filesystem::path(path).extension().string();

        // Create and store the shader data
        data = ShaderData{
            .bytecode = std::move(code),
            .stage = stage
        };

        spdlog::info("Loaded shader: {} (size: {} bytes)", path, fileSize);
    }

    size_t ShaderAsset::calculateContentHash() const {
        size_t hash = 0;

        // Hash both the bytecode and the shader stage
        for (const auto &word: data.bytecode) {
            hash ^= std::hash<std::uint32_t>{}(word) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        // Combine with shader stage
        hash ^= std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(data.stage));

        return hash;
    }

    AssetType ShaderAsset::getType() const {
        return AssetType::Shader;
    }
}
