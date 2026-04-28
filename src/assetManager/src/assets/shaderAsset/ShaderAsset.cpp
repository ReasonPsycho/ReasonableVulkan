#include "ShaderAsset.h"
#include "../../JsonHelpers.hpp"

#include <spdlog/spdlog.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <filesystem>
#include <regex>
#include <set>

#include "ShaderIncluder.hpp"

namespace am {
    ShaderAsset::ShaderAsset(const boost::uuids::uuid& id, ImportContext assetFactoryData) : Asset(id, assetFactoryData) {
        loadFromFile(assetFactoryData.importPath);
    }

    ShaderAsset::ShaderAsset(const boost::uuids::uuid& id, const std::string& path, AssetFormat format) : Asset(id, path, format) {
        if (format == AssetFormat::Json) {
            throw std::runtime_error("ShaderAsset does not support JSON format");
        } else if (format == AssetFormat::Binary) {
            std::ifstream ifs(path, std::ios::binary | std::ios::in);
            if (!ifs.is_open()) {
                spdlog::error("Failed to open binary shader asset: {}", path);
                return;
            }

            char magic[6];
            ifs.read(magic, sizeof(magic));
            if (std::string(magic) != "RSHDR") {
                spdlog::error("Invalid magic number in binary shader asset: {}", path);
                return;
            }

            ifs.read(reinterpret_cast<char*>(&data.stage), sizeof(data.stage));

            size_t sourcePathSize;
            ifs.read(reinterpret_cast<char*>(&sourcePathSize), sizeof(sourcePathSize));
            data.originalSource.resize(sourcePathSize);
            ifs.read(&data.originalSource[0], sourcePathSize);

            size_t definesCount;
            ifs.read(reinterpret_cast<char*>(&definesCount), sizeof(definesCount));
            for (size_t i = 0; i < definesCount; ++i) {
                size_t keySize;
                ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
                std::string key(keySize, '\0');
                ifs.read(&key[0], keySize);

                size_t valueSize;
                ifs.read(reinterpret_cast<char*>(&valueSize), sizeof(valueSize));
                std::string value(valueSize, '\0');
                ifs.read(&value[0], valueSize);

                data.defines[key] = value;
            }

            size_t bytecodeSize;
            ifs.read(reinterpret_cast<char*>(&bytecodeSize), sizeof(bytecodeSize));
            data.bytecode.resize(bytecodeSize);
            if (bytecodeSize > 0) {
                ifs.read(reinterpret_cast<char*>(data.bytecode.data()), bytecodeSize * sizeof(std::uint32_t));
            }

            ifs.close();
        }
    }

    // Helper function to extract defines from shader source
    void extractDefinesRecursive(const std::string& source, const std::filesystem::path& currentFilePath,
                                 const std::filesystem::path& shaderBaseDir,
                                 std::map<std::string, std::string>& defines,
                                 std::set<std::filesystem::path>& processedFiles) {
        // Regex to match: #define NAME VALUE (with optional value)
        std::regex defineRegex(R"(#\s*define\s+(\w+)(?:\s+([^\n\r]+?))?\s*(?:\n|\r|$))");
        // Regex to match: #include "PATH" or #include <PATH>
        std::regex includeRegex(R"(#\s*include\s+["<]([^">]+)[">])");

        // Mark this file as processed to avoid infinite inclusion loops
        try {
            processedFiles.insert(std::filesystem::canonical(currentFilePath));
        } catch (...) {
            processedFiles.insert(currentFilePath);
        }

        // 1. Extract defines from the current source
        std::sregex_iterator d_begin(source.begin(), source.end(), defineRegex);
        std::sregex_iterator d_end;
        for (auto it = d_begin; it != d_end; ++it) {
            std::string name = (*it)[1].str();
            std::string value = (*it)[2].matched ? (*it)[2].str() : "";

            if (!value.empty()) {
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
            }

            if (defines.find(name) == defines.end()) {
                defines[name] = value;
        spdlog::info("Found shader define: {} = '{}'", name, value);
            }
        }

        // 2. Process includes recursively
        std::sregex_iterator i_begin(source.begin(), source.end(), includeRegex);
        std::sregex_iterator i_end;
        for (auto it = i_begin; it != i_end; ++it) {
            std::string includePathStr = (*it)[1];
            std::filesystem::path includePath;

            // Resolve path (logic mirrored from ShaderIncluder)
            std::filesystem::path currentDir = currentFilePath.parent_path();
            includePath = currentDir / includePathStr;

            if (!std::filesystem::exists(includePath)) {
                includePath = shaderBaseDir / includePathStr;
            }

            if (std::filesystem::exists(includePath)) {
                try {
                    std::filesystem::path canonicalPath = std::filesystem::canonical(includePath);
                    if (processedFiles.find(canonicalPath) == processedFiles.end()) {
                        std::ifstream file(canonicalPath, std::ios::binary | std::ios::in | std::ios::ate);
                        if (file.is_open()) {
                            size_t fileSize = static_cast<size_t>(file.tellg());
                            file.seekg(0);
                            std::string includeSource(fileSize, '\0');
                            file.read(includeSource.data(), fileSize);
                            file.close();

                            extractDefinesRecursive(includeSource, canonicalPath, shaderBaseDir, defines,
                                                     processedFiles);
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("Failed to process include for defines: {}", e.what());
                }
            } else {
                spdlog::warn("Could not find include file: {}", includePathStr);
            }
        }
    }

    void ShaderAsset::SaveAssetToBin(std::string& path)
    {
        std::ofstream ofs(path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) {
            spdlog::error("Failed to open file for writing binary shader asset: {}", path);
            return;
        }

        // Write magic number
        const char magic[] = "RSHDR";
        ofs.write(magic, sizeof(magic));

        // Write stage
        ofs.write(reinterpret_cast<const char*>(&data.stage), sizeof(data.stage));

        // Write originalSource
        size_t sourcePathSize = data.originalSource.size();
        ofs.write(reinterpret_cast<const char*>(&sourcePathSize), sizeof(sourcePathSize));
        ofs.write(data.originalSource.c_str(), sourcePathSize);

        // Write defines map
        size_t definesCount = data.defines.size();
        ofs.write(reinterpret_cast<const char*>(&definesCount), sizeof(definesCount));
        for (const auto& [key, value] : data.defines) {
            size_t keySize = key.size();
            ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
            ofs.write(key.c_str(), keySize);

            size_t valueSize = value.size();
            ofs.write(reinterpret_cast<const char*>(&valueSize), sizeof(valueSize));
            ofs.write(value.c_str(), valueSize);
        }

        // Write bytecode
        size_t bytecodeSize = data.bytecode.size();
        ofs.write(reinterpret_cast<const char*>(&bytecodeSize), sizeof(bytecodeSize));
        if (bytecodeSize > 0) {
            ofs.write(reinterpret_cast<const char*>(data.bytecode.data()), bytecodeSize * sizeof(std::uint32_t));
        }

        ofs.close();
        spdlog::info("Saved binary shader asset: {}", path);
    }

    void ShaderAsset::loadFromFile(const std::string& path) {
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

        // Extract defines from shader source (recursively including files)
        std::map<std::string, std::string> defines;
        std::set<std::filesystem::path> processedFiles;
        std::filesystem::path shaderBaseDir("res/shaders/glsl/entry");
        extractDefinesRecursive(source, std::filesystem::absolute(path), shaderBaseDir, defines, processedFiles);

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
            .originalSource = path
        };

        spdlog::info("Compiled shader: {} (size: {} bytes)",
                     path, (int)(data.bytecode.size() * 4));
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
            spdlog::error("Cannot recompile: original source path not stored");
            return false;
        }

        std::ifstream file(data.originalSource, std::ios::binary | std::ios::in | std::ios::ate);
        if (!file.is_open()) {
            spdlog::error("Failed to open shader file for recompilation: {}", data.originalSource);
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0);
        std::string source(fileSize, '\0');
        file.read(source.data(), fileSize);
        file.close();

        auto bytecode = compileGLSLToSPIRV(source, data.stage, newDefines);

        if (bytecode.empty()) {
            spdlog::error("Failed to recompile shader with new defines");
            return false;
        }

        // Update shader data with new bytecode and defines
        data.bytecode = std::move(bytecode);
        data.defines = newDefines;

        spdlog::info("Shader recompiled");
        return true;
    }

    bool ShaderAsset::setDefineAndRecompile(const std::string& name, const std::string& value) {
        auto newDefines = data.defines;
        newDefines[name] = value;
        return recompileWithDefines(newDefines);
    }

}
