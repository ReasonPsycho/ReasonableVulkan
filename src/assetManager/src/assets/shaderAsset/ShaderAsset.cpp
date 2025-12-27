#include "ShaderAsset.h"

#include <spdlog/spdlog.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <filesystem>
#include <regex>

#include "ShaderIncluder.hpp"

namespace am {


    // Helper function to extract defines from shader source
    std::map<std::string, std::string> extractDefines(const std::string& source) {
        std::map<std::string, std::string> defines;

        // Regex to match: #define NAME VALUE (with optional value)
        std::regex defineRegex(R"(#\s*define\s+(\w+)(?:\s+([^\n\r]+?))?\s*(?:\n|\r|$))");

        std::sregex_iterator begin(source.begin(), source.end(), defineRegex);
        std::sregex_iterator end;

        for (auto it = begin; it != end; ++it) {
            std::string name = std::string((*it)[1]);
            std::string value = (*it)[2].matched ? std::string((*it)[2]) : "";

            // Trim whitespace from value
            if (!value.empty()) {
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
            }

            defines[name] = value;
            spdlog::info("Found shader define: {} = '{}'", name, value);
        }

        return defines;
    }

 void ShaderAsset::loadFromFile(const std::string &path) {
        // Read the GLSL source file
        std::ifstream file(path, std::ios::binary | std::ios::in | std::ios::ate);

        if (!file.is_open()) {
            spdlog::error("Failed to open shader file: {}", path);
            throw std::runtime_error("Failed to open shader file: " + path);
        }

        // Get file size
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0);

        if (fileSize <= 0) {
            spdlog::error("Shader file is empty: {}", path);
            throw std::runtime_error("Shader file is empty: " + path);
        }

        // Read the GLSL source code
        std::string source(fileSize, '\0');
        file.read(source.data(), fileSize);
        file.close();

        // Determine shader stage from file extension
        ShaderStage stage = ShaderStage::Vertex; // Default to vertex
        std::string filepath = std::filesystem::path(path).filename().string();

        if (filepath.ends_with(".frag")) {
            stage = ShaderStage::Fragment;
        } else if (filepath.ends_with(".vert")) {
            stage = ShaderStage::Vertex;
        } else if (filepath.ends_with(".comp")) {
            stage = ShaderStage::Compute;
        } else if (filepath.ends_with(".geom")) {
            stage = ShaderStage::Geometry;
        } else if (filepath.ends_with(".tesc")) {
            stage = ShaderStage::TessellationControl;
        } else if (filepath.ends_with(".tese")) {
            stage = ShaderStage::TessellationEvaluation;
        }

        // Extract defines from shader source
        auto defines = extractDefines(source);

        // Compile GLSL to SPIR-V with extracted defines
        auto bytecode = compileGLSLToSPIRV(source, stage, defines);

        if (bytecode.empty()) {
            spdlog::error("Failed to compile shader: {}", path);
            throw std::runtime_error("Failed to compile shader: " + path);
        }

        // Create and store the shader data
        data = ShaderData{
            .bytecode = std::move(bytecode),
            .stage = stage,
            .defines = std::move(defines),
            .originalSource = source
        };

        spdlog::info("Compiled shader: {} (size: {} bytes, defines: {})",
                     path, data.bytecode.size() * 4, data.defines.size());
    }

    std::vector<std::uint32_t> ShaderAsset::compileGLSLToSPIRV(
        const std::string &source,
        ShaderStage stage,
        const std::map<std::string, std::string>& defines
    ) const {
        // Initialize glslang library
        glslang::InitializeProcess();

        // Determine EShLanguage from ShaderStage
        EShLanguage language;
        switch (stage) {
            case ShaderStage::Vertex:
                language = EShLangVertex;
                break;
            case ShaderStage::Fragment:
                language = EShLangFragment;
                break;
            case ShaderStage::Compute:
                language = EShLangCompute;
                break;
            case ShaderStage::Geometry:
                language = EShLangGeometry;
                break;
            case ShaderStage::TessellationControl:
                language = EShLangTessControl;
                break;
            case ShaderStage::TessellationEvaluation:
                language = EShLangTessEvaluation;
                break;
            default:
                spdlog::error("Unknown shader stage");
                return {};
        }

        // Create shader object
        glslang::TShader shader(language);
        const char* sourcePtr = source.c_str();

        // Build preamble with defines
        std::string preamble;
        for (const auto& [name, value] : defines) {
            if (value.empty()) {
                preamble += "#define " + name + "\n";
            } else {
                preamble += "#define " + name + " " + value + "\n";
            }
        }

        // Combine preamble and source into a single string
        std::string combinedSource = preamble + source;

        shader.setStrings(&sourcePtr, 1);

        shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

        // Create custom includer
        ShaderIncluder includer(std::filesystem::path("res/shaders/glsl/entry"));

        // Compile the shader with include support
        EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules);
        if (!shader.parse(GetDefaultResources(), 100, false, messages, includer)) {
            spdlog::error("Failed to parse shader:");
            spdlog::error("Info: {}", shader.getInfoLog());
            spdlog::error("Debug: {}", shader.getInfoDebugLog());
            glslang::FinalizeProcess();
            return {};
        }

        // Create program and link
        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(EShMessages::EShMsgDefault)) {
            spdlog::error("Failed to link shader program:");
            spdlog::error("Info: {}", program.getInfoLog());
            spdlog::error("Debug: {}", program.getInfoDebugLog());
            glslang::FinalizeProcess();
            return {};
        }

        // Generate SPIR-V from the program
        std::vector<std::uint32_t> spirv;
        glslang::GlslangToSpv(*program.getIntermediate(language), spirv);

        glslang::FinalizeProcess();

        return spirv;
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

    bool ShaderAsset::recompileWithDefines(const std::map<std::string, std::string>& newDefines) {
        if (data.originalSource.empty()) {
            spdlog::error("Cannot recompile: original source not stored");
            return false;
        }

        auto bytecode = compileGLSLToSPIRV(data.originalSource, data.stage, newDefines);

        if (bytecode.empty()) {
            spdlog::error("Failed to recompile shader with new defines");
            return false;
        }

        // Update shader data with new bytecode and defines
        data.bytecode = std::move(bytecode);
        data.defines = newDefines;

        spdlog::info("Shader recompiled with {} defines", data.defines.size());
        return true;
    }

    bool ShaderAsset::setDefineAndRecompile(const std::string& name, const std::string& value) {
        auto newDefines = data.defines;
        newDefines[name] = value;
        return recompileWithDefines(newDefines);
    }

}
