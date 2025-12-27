//
// Created by redkc on 27/12/2025.
//

#include "ShaderIncluder.hpp"

ShaderIncluder::ShaderIncluder(const std::filesystem::path& shaderBaseDir)
    : basePath(shaderBaseDir)
{
    // Ensure the base path exists and is absolute
    if (!basePath.is_absolute()) {
        basePath = std::filesystem::absolute(basePath);
    }
    spdlog::debug("ShaderIncluder base path: {}", basePath.string());
}

glslang::TShader::Includer::IncludeResult* ShaderIncluder::includeSystem(const char* headerName,
    const char* includerName, size_t inclusionDepth)
{
    return nullptr; // Not used for local includes
}

glslang::TShader::Includer::IncludeResult* ShaderIncluder::includeLocal(const char* headerName,
                                                                        const char* includerName, size_t inclusionDepth)
{
    try {
        std::filesystem::path headerPath;

        if (includerName) {
            // Resolve relative to the including file's directory
            std::filesystem::path includerPath(includerName);
            if (includerPath.is_relative()) {
                includerPath = basePath / includerPath;
            }
            std::filesystem::path includerDir = includerPath.parent_path();
            headerPath = includerDir / headerName;
        } else {
            // First include - resolve relative to base shader directory
            headerPath = basePath / headerName;
        }

        // Normalize and resolve the path
        headerPath = std::filesystem::absolute(headerPath);
        headerPath = std::filesystem::canonical(headerPath);

        spdlog::debug("Resolving include: {} -> {}", headerName, headerPath.string());

        if (!std::filesystem::exists(headerPath)) {
            spdlog::error("Include file not found: {}", headerPath.string());
            return nullptr;
        }

        // Read the file
        std::ifstream file(headerPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            spdlog::error("Failed to open include file: {}", headerPath.string());
            return nullptr;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0);

        auto* content = new std::string(fileSize, '\0');
        file.read(content->data(), fileSize);
        file.close();

        auto* result = new IncludeResult(headerPath.string(), content->c_str(), content->length(), content);
        return result;
    } catch (const std::exception& e) {
        spdlog::error("Exception while including file: {}", std::string(e.what()));
        return nullptr;
    }
}

void ShaderIncluder::releaseInclude(IncludeResult* result)
{
    if (result) {
        delete static_cast<std::string*>(result->userData);
        delete result;
    }
}