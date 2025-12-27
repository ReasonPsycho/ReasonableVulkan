//
// Created by redkc on 27/12/2025.
//

#ifndef SHADERINCLUDER_HPP
#define SHADERINCLUDER_HPP

#include <glslang/Public/ShaderLang.h>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

class ShaderIncluder : public glslang::TShader::Includer {
private:
    std::filesystem::path basePath;

public:
    explicit ShaderIncluder(const std::filesystem::path& shaderBaseDir);

    IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override;
    IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override;
    void releaseInclude(IncludeResult* result) override;
};

#endif //SHADERINCLUDER_HPP