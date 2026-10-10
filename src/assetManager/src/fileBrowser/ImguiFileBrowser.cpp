//
// Created by redkc on 04.10.2026.
//

#include "ImguiFileBrowser.hpp"
#include "../AssetManager.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>

#include "IconsFontAwesome6.h"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/configAsset/ConfigAsset.h"
#include "assetDatas/TextureData.h"
#include "assetDatas/ModelData.h"
#include "assetDatas/MeshData.h"
#include "assetDatas/ShaderData.h"
#include "assetDatas/MaterialData.h"
#include "EngineInterface.hpp"
#include "PlatformInterface.hpp"

namespace am {

    static std::vector<std::filesystem::path> ParseFilePathPayload(const ImGuiPayload* payload) {
        std::vector<std::filesystem::path> paths;
        if (!payload || !payload->Data || payload->DataSize == 0) return paths;
        const char* str = (const char*)payload->Data;
        std::stringstream ss(str);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) {
                paths.push_back(std::filesystem::path(line));
            }
        }
        return paths;
    }

    FileBrowser::FileBrowser(AssetManager* assetManager)
        : assetManager(assetManager)
    {
    }

    void FileBrowser::loadFileBrowserConfig()
    {
        if (!assetManager) return;
        auto uuid = assetManager->getAssetUuid(fileBrowserConfigLookupName);
        if (!uuid) return;

        auto configData = assetManager->getAssetData<rapidjson::Document>(uuid.value());
        if (!configData) return;

        if (configData->HasMember("scale") && (*configData)["scale"].IsNumber()) {
            fileBrowserScale = (*configData)["scale"].GetFloat();
            if (fileBrowserScale <= 0.0f) {
                fileBrowserScale = 1.0f;
            }
        }
        if (configData->HasMember("letterScale") && (*configData)["letterScale"].IsNumber()) {
            fileBrowserLetterScale = (*configData)["letterScale"].GetFloat();
            if (fileBrowserLetterScale <= 0.0f) {
                fileBrowserLetterScale = 1.0f;
            }
        }
    }

    void FileBrowser::saveFileBrowserConfig()
    {
        if (!assetManager) return;
        auto uuid = assetManager->getAssetUuid(fileBrowserConfigLookupName);
        if (!uuid) {
            std::filesystem::path configPath = std::filesystem::path(assetManager->resourceFolder) / ".cache" / "config" / "fileBrowser.config";
            try {
                uuid = assetManager->createAsset(AssetType::Config, configPath.string(), fileBrowserConfigLookupName);
            } catch (...) {
                return;
            }
        }
        if (!uuid) return;

        auto configData = assetManager->getAssetData<rapidjson::Document>(uuid.value());
        if (!configData) return;

        configData->SetObject();
        auto& allocator = configData->GetAllocator();
        configData->AddMember("scale", fileBrowserScale, allocator);
        configData->AddMember("letterScale", fileBrowserLetterScale, allocator);

        assetManager->saveAsset(uuid.value());
    }

    void FileBrowser::ImguiFileBrowser(std::string windowName)
    {
        ImGui::Begin(windowName.c_str());

        bool isBrowserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        bool isCtrlOrCmd = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        if (isBrowserFocused && !ImGui::GetIO().WantTextInput)
        {
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_A))
            {
                std::vector<std::filesystem::path> allPaths;
                try {
                    for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                        std::string fn = entry.path().filename().string();
                        if (!fn.empty() && fn[0] != '.' && entry.path().extension() != ".meta") {
                            allPaths.push_back(entry.path());
                        }
                    }
                } catch (...) {}
                setSelectedFiles(allPaths);
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_C))
            {
                if (!selectedFiles.empty()) {
                    copyFilesToClipboard(selectedFiles);
                } else if (!selectedFile.empty()) {
                    copyFileToClipboard(selectedFile);
                }
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_X))
            {
                if (!selectedFiles.empty()) {
                    cutFilesToClipboard(selectedFiles);
                } else if (!selectedFile.empty()) {
                    cutFileToClipboard(selectedFile);
                }
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_V))
            {
                pasteFileFromClipboard(currentPath);
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_D))
            {
                if (!selectedFiles.empty()) {
                    duplicateFiles(selectedFiles);
                } else if (!selectedFile.empty()) {
                    duplicateFile(selectedFile);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
            {
                if (!selectedFiles.empty()) {
                    deleteFiles(selectedFiles);
                    clearSelectedFiles();
                } else if (!selectedFile.empty()) {
                    deleteFile(selectedFile);
                    clearSelectedFiles();
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                clearSelectedFiles();
            }
        }

        if (assetManager && currentPath != assetManager->resourceFolder)
        {
            ImVec2 btnMin = ImGui::GetCursorScreenPos();
            if (ImGui::Button(".."))
            {
                currentPath = currentPath.parent_path();
            }
            ImVec2 btnMax = ImGui::GetItemRectMax();
            bool isParentDropTarget = false;
            if (ImGui::BeginDragDropTarget())
            {
                isParentDropTarget = true;
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                {
                    auto droppedPaths = ParseFilePathPayload(payload);
                    std::error_code ec;
                    auto targetDir = currentPath.parent_path();
                    for (const auto& droppedPath : droppedPaths) {
                        if (std::filesystem::exists(droppedPath, ec) && !std::filesystem::equivalent(droppedPath, targetDir, ec))
                        {
                            moveFileOrDirectory(droppedPath, targetDir, false);
                        }
                    }
                }
                else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                {
                    if (assetManager && assetManager->engine)
                    {
                        std::vector<engine::ecs::Entity> ents;
                        engine::ecs::Scene* scn = nullptr;
                        if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                            const auto* pl = (const engine::ecs::SceneEntityPayload*)payload->Data;
                            scn = pl->scene;
                            if (pl->count > 0) {
                                for (uint32_t k = 0; k < pl->count; ++k) ents.push_back(pl->entities[k]);
                            }
                        }
                        if (ents.empty()) {
                            ents.push_back(*(const engine::ecs::Entity*)payload->Data);
                        }
                        for (auto entity : ents) {
                            assetManager->engine->SaveEntityAsPrefab(entity, currentPath.parent_path(), scn);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (isParentDropTarget)
            {
                ImGui::GetWindowDrawList()->AddRect(btnMin, btnMax, IM_COL32(70, 240, 120, 255), 3.0f, 0, 2.0f);
                ImGui::GetWindowDrawList()->AddRectFilled(btnMin, btnMax, IM_COL32(50, 205, 50, 80), 3.0f);
            }
            ImGui::SameLine();
        }
        ImGui::Text("Current Path: %s", currentPath.string().c_str());

        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderFloat("##ScaleSlider", &fileBrowserScale, 0.5f, 2.0f, "Icons: %.2f")) {
            if (fileBrowserScale < 0.2f) fileBrowserScale = 0.2f;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            saveFileBrowserConfig();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Scale icons");
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderFloat("##LetterScaleSlider", &fileBrowserLetterScale, 0.5f, 2.0f, "Letters: %.2f")) {
            if (fileBrowserLetterScale < 0.2f) fileBrowserLetterScale = 0.2f;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            saveFileBrowserConfig();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Scale letters");
        }

        ImGui::Separator();

        if (fileBrowserScale <= 0.0f) {
            fileBrowserScale = 1.0f;
        }
        if (fileBrowserLetterScale <= 0.0f) {
            fileBrowserLetterScale = 1.0f;
        }

        auto renderFolderBackgroundContextMenu = [this]() {
            if (!selectedFiles.empty() || !selectedFile.empty()) {
                auto targets = !selectedFiles.empty() ? selectedFiles : std::vector<std::filesystem::path>{selectedFile};
                std::string copyLabel = targets.size() > 1 ? ("Copy (" + std::to_string(targets.size()) + " items)") : "Copy";
                std::string cutLabel = targets.size() > 1 ? ("Cut (" + std::to_string(targets.size()) + " items)") : "Cut";
                std::string dupLabel = targets.size() > 1 ? ("Duplicate (" + std::to_string(targets.size()) + " items)") : "Duplicate";
                std::string delLabel = targets.size() > 1 ? ("Delete (" + std::to_string(targets.size()) + " items)") : "Delete";

                if (ImGui::MenuItem(copyLabel.c_str(), "Ctrl+C")) {
                    copyFilesToClipboard(targets);
                }
                if (ImGui::MenuItem(cutLabel.c_str(), "Ctrl+X")) {
                    cutFilesToClipboard(targets);
                }
                if (ImGui::MenuItem(dupLabel.c_str(), "Ctrl+D")) {
                    duplicateFiles(targets);
                }
                if (ImGui::MenuItem(delLabel.c_str(), "Del")) {
                    deleteFiles(targets);
                    clearSelectedFiles();
                }
                ImGui::Separator();
            }

            std::error_code ecClip;
            bool hasClipboard = (!clipboardPaths.empty() || (!clipboardPath.empty() && std::filesystem::exists(clipboardPath, ecClip)));
            if (!hasClipboard) {
                const char* sysClip = ImGui::GetClipboardText();
                if (sysClip && sysClip[0] != '\0') {
                    std::filesystem::path sysP(sysClip);
                    if (std::filesystem::exists(sysP, ecClip)) {
                        hasClipboard = true;
                    }
                }
            }

            if (ImGui::MenuItem("Paste", "Ctrl+V", false, hasClipboard)) {
                pasteFileFromClipboard(currentPath);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("New Folder")) {
                std::filesystem::path newFolderPath = currentPath / "New Folder";
                newFolderPath = getUniqueCopyPath(newFolderPath);
                std::error_code ec;
                std::filesystem::create_directory(newFolderPath, ec);
                setSelectedFile(newFolderPath);
            }
            if (ImGui::MenuItem("New Material")) {
                std::filesystem::path newMatPath = currentPath / "New Material.material";
                newMatPath = getUniqueCopyPath(newMatPath);
                auto matUuid = assetManager ? assetManager->createAsset(AssetType::Material, newMatPath.string()) : std::nullopt;
                if (matUuid) {
                    setSelectedFile(newMatPath);
                }
            }
            if (ImGui::MenuItem("Open in System File Explorer")) {
                if (assetManager) assetManager->openFileWithDefaultApp(currentPath);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Current Path")) {
                ImGui::SetClipboardText(currentPath.string().c_str());
            }
        };

        if (ImGui::BeginChild("FileBrowserScroll"))
        {
            float baseIconSize = 64.0f;
            float basePadding = 16.0f;
            float iconSize = baseIconSize * fileBrowserScale;
            float padding = basePadding * fileBrowserScale;

            float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            std::vector<std::filesystem::directory_entry> entries;
            try {
                for (const auto& entry : std::filesystem::directory_iterator(currentPath))
                {
                    const auto& path = entry.path();
                    std::string filename = path.filename().string();
                    if (filename.empty() || filename[0] == '.') {
                        continue;
                    }
                    if (path.extension() == ".meta") {
                        continue;
                    }
                    entries.push_back(entry);
                }
            } catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
                if (ImGui::Button("Reset to Resource Folder")) {
                    if (assetManager) currentPath = assetManager->resourceFolder;
                }
            }

            // Sort: directories first, then alphabetical
            std::sort(entries.begin(), entries.end(), [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                if (a.is_directory() != b.is_directory()) {
                    return a.is_directory() > b.is_directory();
                }
                return a.path().filename().string() < b.path().filename().string();
            });

            ImFont* textFont = ImGui::GetFont();
            float textFontSize = ImGui::GetFontSize() * fileBrowserLetterScale;
            float textLineHeight = textFontSize + 4.0f;
            float totalTileHeight = iconSize + textLineHeight + 4.0f;
            ImVec2 tileSize = ImVec2(iconSize, totalTileHeight);

            for (size_t i = 0; i < entries.size(); ++i)
            {
                const auto& entry = entries[i];
                const auto& path = entry.path();
                std::string filename = path.filename().string();

                ImGui::PushID((int)i);

                std::error_code ecEquiv;
                bool isSelected = isFileSelected(path);

                ImVec2 cursorPos = ImGui::GetCursorScreenPos();

                // Drag detection
                bool isBeingDragged = false;
                const ImGuiPayload* curPayload = ImGui::GetDragDropPayload();
                if (curPayload && curPayload->IsDataType("AM_FILE_PATH") && curPayload->Data) {
                    auto droppedPaths = ParseFilePathPayload(curPayload);
                    std::error_code ecDrag;
                    for (const auto& dp : droppedPaths) {
                        if (std::filesystem::equivalent(path, dp, ecDrag)) {
                            isBeingDragged = true;
                            break;
                        }
                    }
                }

                // Invisible button covering the entire tile
                ImGui::InvisibleButton("##tile", tileSize);
                bool isHovered = ImGui::IsItemHovered();
                bool isActive = ImGui::IsItemActive();

                // Drag Source on tile
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    std::vector<std::filesystem::path> dragList;
                    if (isFileSelected(path) && selectedFiles.size() > 1) {
                        dragList = selectedFiles;
                    } else {
                        dragList.push_back(path);
                    }

                    std::string payloadStr;
                    for (size_t k = 0; k < dragList.size(); ++k) {
                        if (k > 0) payloadStr += "\n";
                        payloadStr += dragList[k].string();
                    }
                    ImGui::SetDragDropPayload("AM_FILE_PATH", payloadStr.c_str(), payloadStr.size() + 1);

                    // Rich visual indicator tooltip
                    if (dragList.size() > 1) {
                        ImGui::BeginGroup();
                        ImFont* iconFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
                        ImGui::PushFont(iconFont);
                        ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", ICON_FA_BOXES_STACKED);
                        ImGui::PopFont();
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Text("%zu Items", dragList.size());
                        ImGui::TextDisabled("Drag to move or instantiate");
                        ImGui::EndGroup();
                        ImGui::EndGroup();
                    } else {
                        ImGui::BeginGroup();
                        void* dragThumb = !entry.is_directory() && assetManager ? assetManager->getThumbnailTexture(path) : nullptr;
                        if (dragThumb != nullptr) {
                            ImGui::Image((ImTextureID)dragThumb, ImVec2(32, 32));
                            ImGui::SameLine();
                        } else {
                            ImFont* iconFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
                            ImGui::PushFont(iconFont);
                            ImGui::TextColored(entry.is_directory() ? ImVec4(1.0f, 0.85f, 0.4f, 1.0f) : ImVec4(0.40f, 0.75f, 1.0f, 1.0f),
                                "%s", entry.is_directory() ? ICON_FA_FOLDER : GetAssetIcon(path));
                            ImGui::PopFont();
                            ImGui::SameLine();
                        }
                        ImGui::BeginGroup();
                        ImGui::Text("%s", filename.c_str());
                        std::error_code ecDragSz;
                        if (!entry.is_directory()) {
                            auto fsz = std::filesystem::file_size(path, ecDragSz);
                            if (!ecDragSz) {
                                if (fsz < 1024) ImGui::TextDisabled("%llu B", static_cast<unsigned long long>(fsz));
                                else if (fsz < 1024 * 1024) ImGui::TextDisabled("%.1f KB", fsz / 1024.0);
                                else ImGui::TextDisabled("%.1f MB", fsz / (1024.0 * 1024.0));
                            }
                        } else {
                            ImGui::TextDisabled("Folder");
                        }
                        ImGui::EndGroup();
                        ImGui::EndGroup();
                    }

                    ImGui::EndDragDropSource();
                }

                // Drop Target for directories
                bool isFolderDropTarget = false;
                if (entry.is_directory() && ImGui::BeginDragDropTarget())
                {
                    isFolderDropTarget = true;
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                    {
                        auto droppedPaths = ParseFilePathPayload(payload);
                        std::error_code ec;
                        for (const auto& droppedPath : droppedPaths) {
                            if (std::filesystem::exists(droppedPath, ec) && !std::filesystem::equivalent(droppedPath, path, ec))
                            {
                                moveFileOrDirectory(droppedPath, path, false);
                            }
                        }
                    }
                    else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                    {
                        if (assetManager && assetManager->engine)
                        {
                            std::vector<engine::ecs::Entity> ents;
                            engine::ecs::Scene* scn = nullptr;
                            if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                                const auto* pl = (const engine::ecs::SceneEntityPayload*)payload->Data;
                                scn = pl->scene;
                                if (pl->count > 0) {
                                    for (uint32_t k = 0; k < pl->count; ++k) ents.push_back(pl->entities[k]);
                                }
                            }
                            if (ents.empty()) {
                                ents.push_back(*(const engine::ecs::Entity*)payload->Data);
                            }
                            for (auto entity : ents) {
                                assetManager->engine->SaveEntityAsPrefab(entity, path, scn);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Click interactions
                if (isHovered) {
                    bool isCtrl = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
                    bool isShift = ImGui::GetIO().KeyShift;

                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        if (entry.is_directory()) {
                            currentPath = path;
                            clearSelectedFiles();
                        } else {
                            if (assetManager) assetManager->openAssetFile(path);
                        }
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        if (isCtrl) {
                            if (isSelected) {
                                removeSelectedFile(path);
                            } else {
                                addSelectedFile(path);
                            }
                        } else if (isShift && !selectedFiles.empty()) {
                            int lastIdx = -1;
                            for (int k = 0; k < (int)entries.size(); ++k) {
                                if (entries[k].path() == selectedFile || isFileSelected(entries[k].path())) {
                                    lastIdx = k;
                                }
                            }
                            if (lastIdx != -1) {
                                int start = std::min(lastIdx, (int)i);
                                int end = std::max(lastIdx, (int)i);
                                selectedFiles.clear();
                                for (int k = start; k <= end; ++k) {
                                    selectedFiles.push_back(entries[k].path());
                                }
                                selectedFile = path;
                            } else {
                                setSelectedFile(path);
                            }
                        } else {
                            if (!isSelected) {
                                setSelectedFile(path);
                            }
                        }
                    } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !isCtrl && !isShift && !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        if (isSelected && selectedFiles.size() > 1) {
                            setSelectedFile(path);
                        }
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        if (!isSelected) {
                            setSelectedFile(path);
                        }
                    }
                }

                auto renderCommonFileContextMenu = [this, &path, &entry]() {
                    std::error_code ecClip;
                    bool hasClipboard = (!clipboardPaths.empty() || (!clipboardPath.empty() && std::filesystem::exists(clipboardPath, ecClip)));
                    if (!hasClipboard) {
                        const char* sysClip = ImGui::GetClipboardText();
                        if (sysClip && sysClip[0] != '\0') {
                            hasClipboard = true;
                        }
                    }

                    if (selectedFiles.size() > 1 && isFileSelected(path)) {
                        std::string copyLabel = "Copy (" + std::to_string(selectedFiles.size()) + " items)";
                        std::string cutLabel = "Cut (" + std::to_string(selectedFiles.size()) + " items)";
                        std::string dupLabel = "Duplicate (" + std::to_string(selectedFiles.size()) + " items)";
                        std::string delLabel = "Delete (" + std::to_string(selectedFiles.size()) + " items)";

                        if (ImGui::MenuItem(copyLabel.c_str(), "Ctrl+C")) {
                            copyFilesToClipboard(selectedFiles);
                        }
                        if (ImGui::MenuItem(cutLabel.c_str(), "Ctrl+X")) {
                            cutFilesToClipboard(selectedFiles);
                        }
                        if (ImGui::MenuItem(dupLabel.c_str(), "Ctrl+D")) {
                            duplicateFiles(selectedFiles);
                        }
                        if (ImGui::MenuItem(delLabel.c_str(), "Del")) {
                            deleteFiles(selectedFiles);
                            clearSelectedFiles();
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Reimport Selected Assets")) {
                            for (const auto& p : selectedFiles) {
                                if (assetManager) assetManager->reimportAsset(p);
                            }
                        }
                        if (ImGui::MenuItem("Open in IDE")) {
                            for (const auto& p : selectedFiles) {
                                if (assetManager) assetManager->openFileInIDE(p);
                            }
                        }
                        return;
                    }

                    if (entry.is_directory()) {
                        if (ImGui::MenuItem("Paste into Folder", "Ctrl+V", false, hasClipboard)) {
                            pasteFileFromClipboard(path);
                        }
                    } else {
                        if (ImGui::MenuItem("Paste", "Ctrl+V", false, hasClipboard)) {
                            pasteFileFromClipboard(currentPath);
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                        copyFileToClipboard(path);
                    }
                    if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                        cutFileToClipboard(path);
                    }
                    if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                        duplicateFile(path);
                    }
                    if (ImGui::MenuItem("Delete", "Del")) {
                        deleteFile(path);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Copy Full Path")) {
                        ImGui::SetClipboardText(path.string().c_str());
                    }
                    std::error_code ecRel;
                    std::string resFolder = assetManager ? assetManager->resourceFolder : "";
                    std::string relPath = !resFolder.empty() ? std::filesystem::relative(path, resFolder, ecRel).string() : "";
                    if (!ecRel && !relPath.empty()) {
                        if (ImGui::MenuItem("Copy Relative Path")) {
                            ImGui::SetClipboardText(relPath.c_str());
                        }
                    }
                };

                if (entry.is_directory()) {
                    if (ImGui::BeginPopupContextItem("##FolderContext")) {
                        if (selectedFiles.size() <= 1 || !isSelected) {
                            setSelectedFile(path);
                            if (ImGui::MenuItem("Open Folder")) {
                                currentPath = path;
                                clearSelectedFiles();
                            }
                            if (ImGui::MenuItem("Open in System File Explorer")) {
                                if (assetManager) assetManager->openFileWithDefaultApp(path);
                            }
                        }
                        renderCommonFileContextMenu();
                        ImGui::EndPopup();
                    }
                } else {
                    auto fileUuid = assetManager ? assetManager->getAssetUuidByPath(path) : std::nullopt;
                    bool isRegistered = fileUuid.has_value();
                    bool canImport = (StringToAssetOwnership(path.extension().string()) == AssetOwnership::Import);
                    AssetType type = GetAssetTypeFromExtension(path.extension().string());

                    if (type == AssetType::Scene && assetManager && assetManager->engine) {
                        if (ImGui::BeginPopupContextItem("##SceneContext")) {
                            if (selectedFiles.size() <= 1 || !isSelected) {
                                setSelectedFile(path);
                                if (isRegistered) {
                                    if (ImGui::MenuItem("Open Scene")) {
                                        assetManager->engine->LoadScene(fileUuid.value());
                                    }
                                    if (ImGui::MenuItem("Close Scene")) {
                                        assetManager->engine->CloseScene(fileUuid.value());
                                    }
                                    if (ImGui::MenuItem("Reimport")) {
                                        assetManager->reimportAsset(path);
                                    }
                                } else if (canImport) {
                                    if (ImGui::MenuItem("Import")) {
                                        assetManager->registerAsset(path.string());
                                    }
                                }
                                if (ImGui::MenuItem("Open in IDE")) {
                                    assetManager->openFileInIDE(path);
                                }
                                if (ImGui::MenuItem("Open in System Default")) {
                                    assetManager->openFileWithDefaultApp(path);
                                }
                            }
                            renderCommonFileContextMenu();
                            ImGui::EndPopup();
                        }
                    } else if ((type == AssetType::Model || type == AssetType::Mesh) && assetManager && assetManager->engine) {
                        if (ImGui::BeginPopupContextItem("##ModelContext")) {
                            if (selectedFiles.size() <= 1 || !isSelected) {
                                setSelectedFile(path);
                                if (isRegistered) {
                                    if (ImGui::MenuItem("Open in Preview Scene")) {
                                        assetManager->engine->OpenModelPreviewScene(fileUuid.value());
                                    }
                                    if (ImGui::MenuItem("Regenerate Thumbnail")) {
                                        assetManager->generateThumbnail(fileUuid.value());
                                    }
                                    if (ImGui::MenuItem("Reimport")) {
                                        assetManager->reimportAsset(path);
                                    }
                                } else if (canImport) {
                                    if (ImGui::MenuItem("Import")) {
                                        assetManager->registerAsset(path.string());
                                    }
                                }
                                if (ImGui::MenuItem("Open in IDE")) {
                                    assetManager->openFileInIDE(path);
                                }
                                if (ImGui::MenuItem("Open in System Default")) {
                                    assetManager->openFileWithDefaultApp(path);
                                }
                            }
                            renderCommonFileContextMenu();
                            ImGui::EndPopup();
                        }
                    } else {
                        if (ImGui::BeginPopupContextItem("##FileContext")) {
                            if (selectedFiles.size() <= 1 || !isSelected) {
                                setSelectedFile(path);
                                if (isRegistered) {
                                    if (type == AssetType::Texture || type == AssetType::Material || path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg" || path.extension() == ".bmp" || path.extension() == ".mat") {
                                        if (ImGui::MenuItem("Regenerate Thumbnail")) {
                                            if (assetManager) assetManager->generateThumbnail(fileUuid.value());
                                        }
                                    }
                                    if (ImGui::MenuItem("Reimport")) {
                                        if (assetManager) assetManager->reimportAsset(path);
                                    }
                                } else if (canImport) {
                                    if (ImGui::MenuItem("Import")) {
                                        if (assetManager) assetManager->registerAsset(path.string());
                                    }
                                }
                                if (ImGui::MenuItem("Open in IDE")) {
                                    if (assetManager) assetManager->openFileInIDE(path);
                                }
                                if (ImGui::MenuItem("Open in System Default")) {
                                    if (assetManager) assetManager->openFileWithDefaultApp(path);
                                }
                            }
                            renderCommonFileContextMenu();
                            ImGui::EndPopup();
                        }
                    }
                }

                // Render Visuals with DrawList
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                if (isFolderDropTarget) {
                    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(50, 205, 50, 75), 4.0f);
                    drawList->AddRect(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(70, 240, 120, 255), 4.0f, 0, 2.5f);
                } else if (isBeingDragged) {
                    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(66, 150, 250, 45), 4.0f);
                    drawList->AddRect(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(100, 200, 255, 200), 4.0f, 0, 2.0f);
                } else if (isSelected) {
                    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(66, 150, 250, 90), 4.0f);
                    drawList->AddRect(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(66, 150, 250, 220), 4.0f, 0, 1.5f);
                } else if (isActive) {
                    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(255, 255, 255, 45), 4.0f);
                } else if (isHovered) {
                    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + iconSize, cursorPos.y + totalTileHeight), IM_COL32(255, 255, 255, 25), 4.0f);
                }

                void* thumbTex = nullptr;
                if (!entry.is_directory() && assetManager) {
                    thumbTex = assetManager->getThumbnailTexture(path);
                }

                if (thumbTex != nullptr) {
                    float imgPad = 2.0f;
                    ImU32 imgTint = isBeingDragged ? IM_COL32(255, 255, 255, 120) : IM_COL32_WHITE;
                    drawList->AddImage((ImTextureID)thumbTex,
                        ImVec2(cursorPos.x + imgPad, cursorPos.y + imgPad),
                        ImVec2(cursorPos.x + iconSize - imgPad, cursorPos.y + iconSize - imgPad),
                        ImVec2(0, 0), ImVec2(1, 1), imgTint);
                } else {
                    ImFont* iconFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
                    float iconFontSize = (ImGui::GetIO().Fonts->Fonts.Size > 1 ? 48.0f : ImGui::GetFontSize() * 2.5f) * fileBrowserScale;
                    const char* iconStr = entry.is_directory() ? ICON_FA_FOLDER : GetAssetIcon(path);
                    ImVec2 iconTextSize = iconFont->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, iconStr);
                    ImVec2 iconTextPos = ImVec2(
                        cursorPos.x + (iconSize - iconTextSize.x) * 0.5f,
                        cursorPos.y + (iconSize - iconTextSize.y) * 0.5f
                    );
                    ImU32 iconColor = isBeingDragged
                        ? IM_COL32(180, 180, 180, 130)
                        : (entry.is_directory() ? IM_COL32(245, 205, 110, 255) : IM_COL32(210, 210, 210, 255));
                    drawList->AddText(iconFont, iconFontSize, iconTextPos, iconColor, iconStr);
                }

                std::string truncated = filename;
                float textWidth = textFont->CalcTextSizeA(textFontSize, FLT_MAX, 0.0f, truncated.c_str()).x;
                bool isTruncated = false;
                if (textWidth > iconSize) {
                    isTruncated = true;
                    while (!truncated.empty() && textFont->CalcTextSizeA(textFontSize, FLT_MAX, 0.0f, (truncated + "...").c_str()).x > iconSize) {
                        truncated.pop_back();
                    }
                    truncated += "...";
                    textWidth = textFont->CalcTextSizeA(textFontSize, FLT_MAX, 0.0f, truncated.c_str()).x;
                }

                ImVec2 textPos = ImVec2(
                    cursorPos.x + (iconSize - textWidth) * 0.5f,
                    cursorPos.y + iconSize + 2.0f
                );
                drawList->AddText(textFont, textFontSize, textPos, isBeingDragged ? IM_COL32(180, 180, 180, 130) : IM_COL32_WHITE, truncated.c_str());

                if (isHovered && isTruncated) {
                    ImGui::SetTooltip("%s", filename.c_str());
                }

                float lastButtonX2 = ImGui::GetItemRectMax().x;
                float nextButtonX2 = lastButtonX2 + padding + iconSize;
                if (i + 1 < entries.size() && nextButtonX2 < windowVisibleX2)
                {
                    ImGui::SameLine(0.0f, padding);
                }

                ImGui::PopID();
            }

            // Drag and drop onto empty folder area
            ImVec2 remainingSpace = ImGui::GetContentRegionAvail();
            if (remainingSpace.x > 0 && remainingSpace.y > 0)
            {
                ImGui::Dummy(remainingSpace);
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                    {
                        auto droppedPaths = ParseFilePathPayload(payload);
                        std::error_code ec;
                        for (const auto& droppedPath : droppedPaths) {
                            if (std::filesystem::exists(droppedPath, ec) && !std::filesystem::equivalent(droppedPath, currentPath, ec))
                            {
                                moveFileOrDirectory(droppedPath, currentPath, false);
                            }
                        }
                    }
                    else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                    {
                        if (assetManager && assetManager->engine)
                        {
                            std::vector<engine::ecs::Entity> ents;
                            engine::ecs::Scene* scn = nullptr;
                            if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                                const auto* pl = (const engine::ecs::SceneEntityPayload*)payload->Data;
                                scn = pl->scene;
                                if (pl->count > 0) {
                                    for (uint32_t k = 0; k < pl->count; ++k) ents.push_back(pl->entities[k]);
                                }
                            }
                            if (ents.empty()) {
                                ents.push_back(*(const engine::ecs::Entity*)payload->Data);
                            }
                            for (auto entity : ents) {
                                assetManager->engine->SaveEntityAsPrefab(entity, currentPath, scn);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::BeginPopupContextItem("##BackgroundContext"))
                {
                    renderFolderBackgroundContextMenu();
                    ImGui::EndPopup();
                }
            }

            ImGui::EndChild();
        }

        // Drop target on whole window
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
            {
                auto droppedPaths = ParseFilePathPayload(payload);
                std::error_code ec;
                for (const auto& droppedPath : droppedPaths) {
                    if (std::filesystem::exists(droppedPath, ec) && !std::filesystem::equivalent(droppedPath, currentPath, ec))
                    {
                        moveFileOrDirectory(droppedPath, currentPath, false);
                    }
                }
            }
            else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
            {
                if (assetManager && assetManager->engine)
                {
                    std::vector<engine::ecs::Entity> ents;
                    engine::ecs::Scene* scn = nullptr;
                    if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                        const auto* pl = (const engine::ecs::SceneEntityPayload*)payload->Data;
                        scn = pl->scene;
                        if (pl->count > 0) {
                            for (uint32_t k = 0; k < pl->count; ++k) ents.push_back(pl->entities[k]);
                        }
                    }
                    if (ents.empty()) {
                        ents.push_back(*(const engine::ecs::Entity*)payload->Data);
                    }
                    for (auto entity : ents) {
                        assetManager->engine->SaveEntityAsPrefab(entity, currentPath, scn);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Context menu on outer window space
        if (ImGui::BeginPopupContextWindow("##FileBrowserWindowContext", ImGuiPopupFlags_MouseButtonRight))
        {
            renderFolderBackgroundContextMenu();
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void FileBrowser::ImguiFileInspector(std::string windowName)
    {
        ImGui::Begin(windowName.c_str());

        if (selectedFiles.empty() && selectedFile.empty()) {
            ImGui::TextDisabled("No file selected in File Browser");
            ImGui::End();
            return;
        }

        if (selectedFiles.size() > 1) {
            ImGui::Text("%s %zu Items Selected", ICON_FA_FOLDER_OPEN, selectedFiles.size());
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Selection Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Button("Copy All (Ctrl+C)")) {
                    copyFilesToClipboard(selectedFiles);
                }
                ImGui::SameLine();
                if (ImGui::Button("Cut All (Ctrl+X)")) {
                    cutFilesToClipboard(selectedFiles);
                }
                ImGui::SameLine();
                if (ImGui::Button("Duplicate All (Ctrl+D)")) {
                    duplicateFiles(selectedFiles);
                }

                if (ImGui::Button("Reimport Registered")) {
                    for (const auto& p : selectedFiles) {
                        if (assetManager) assetManager->reimportAsset(p);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Open in IDE")) {
                    for (const auto& p : selectedFiles) {
                        if (assetManager) assetManager->openFileInIDE(p);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Selection")) {
                    clearSelectedFiles();
                    ImGui::End();
                    return;
                }

                ImGui::Separator();
                if (ImGui::Button("Delete All Selected (Del)")) {
                    deleteFiles(selectedFiles);
                    clearSelectedFiles();
                    ImGui::End();
                    return;
                }
            }

            if (ImGui::CollapsingHeader("Selected Items", ImGuiTreeNodeFlags_DefaultOpen)) {
                std::filesystem::path itemToRemove;
                for (size_t i = 0; i < selectedFiles.size(); ++i) {
                    const auto& p = selectedFiles[i];
                    std::error_code ec;
                    bool isDir = std::filesystem::is_directory(p, ec);
                    ImGui::PushID((int)i);
                    ImGui::Text("%s %s", isDir ? ICON_FA_FOLDER : GetAssetIcon(p), p.filename().string().c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x")) {
                        itemToRemove = p;
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove from selection");
                    ImGui::PopID();
                }
                if (!itemToRemove.empty()) {
                    removeSelectedFile(itemToRemove);
                }
            }

            ImGui::End();
            return;
        }

        std::filesystem::path currentSelected = !selectedFiles.empty() ? selectedFiles[0] : selectedFile;

        std::error_code ec;
        if (!std::filesystem::exists(currentSelected, ec)) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Selected file does not exist:");
            ImGui::TextWrapped("%s", currentSelected.string().c_str());
            ImGui::End();
            return;
        }

        std::string filename = currentSelected.filename().string();
        std::string ext = currentSelected.extension().string();
        bool isDirectory = std::filesystem::is_directory(currentSelected, ec);
        AssetType assetType = GetAssetTypeFromExtension(ext);
        AssetOwnership ownership = StringToAssetOwnership(ext);

        auto fileUuid = assetManager ? assetManager->getAssetUuidByPath(currentSelected) : std::nullopt;
        bool isRegistered = fileUuid.has_value();

        // 1. Preview / Header
        void* thumbTex = nullptr;
        if (!isDirectory && assetManager) {
            thumbTex = assetManager->getThumbnailTexture(currentSelected);
        }

        if (thumbTex != nullptr) {
            float previewSize = 128.0f;
            float availWidth = ImGui::GetContentRegionAvail().x;
            if (availWidth > previewSize) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - previewSize) * 0.5f);
            }
            ImGui::Image((ImTextureID)thumbTex, ImVec2(previewSize, previewSize));
            ImGui::Spacing();
        }

        ImGui::Text("%s %s", isDirectory ? ICON_FA_FOLDER : GetAssetIcon(currentSelected), filename.c_str());
        ImGui::Separator();

        // 2. File Information Header
        if (ImGui::CollapsingHeader("File Information", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Name: %s", filename.c_str());
            if (!isDirectory) {
                ImGui::Text("Extension: %s", ext.empty() ? "(none)" : ext.c_str());
            }

            std::string resFolder = assetManager ? assetManager->resourceFolder : "";
            std::string relPath = !resFolder.empty() ? std::filesystem::relative(currentSelected, resFolder, ec).string() : "";
            if (!ec && !relPath.empty()) {
                ImGui::Text("Relative Path: %s", relPath.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy##RelPath")) {
                    ImGui::SetClipboardText(relPath.c_str());
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy relative path to clipboard");
            }
            ImGui::TextWrapped("Full Path: %s", currentSelected.string().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Copy##FullPath")) {
                ImGui::SetClipboardText(currentSelected.string().c_str());
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy full path to clipboard");

            if (!isDirectory) {
                auto fSize = std::filesystem::file_size(currentSelected, ec);
                if (!ec) {
                    if (fSize < 1024) {
                        ImGui::Text("File Size: %llu B", static_cast<unsigned long long>(fSize));
                    } else if (fSize < 1024 * 1024) {
                        ImGui::Text("File Size: %.2f KB (%llu bytes)", fSize / 1024.0, static_cast<unsigned long long>(fSize));
                    } else {
                        ImGui::Text("File Size: %.2f MB (%llu bytes)", fSize / (1024.0 * 1024.0), static_cast<unsigned long long>(fSize));
                    }
                }

                auto lastWrite = std::filesystem::last_write_time(currentSelected, ec);
                if (!ec) {
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        lastWrite - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
                    );
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                    std::tm* timeInfo = std::localtime(&cftime);
                    if (timeInfo) {
                        char timeBuf[64];
                        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", timeInfo);
                        ImGui::Text("Last Modified: %s", timeBuf);
                    }
                }

                ImGui::Text("Inferred Asset Type: %s", AssetTypeToString(assetType).c_str());
            } else {
                ImGui::Text("Type: Directory / Folder");
            }
        }

        // 3. Asset Metadata Header
        if (!isDirectory && ImGui::CollapsingHeader("Asset Metadata", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (isRegistered) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: Registered");
                std::string uuidStr = boost::uuids::to_string(fileUuid.value());
                ImGui::Text("UUID: %s", uuidStr.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy##UUID")) {
                    ImGui::SetClipboardText(uuidStr.c_str());
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy UUID to clipboard");

                auto infoOpt = assetManager ? assetManager->getAssetInfo(fileUuid.value()) : std::nullopt;
                if (infoOpt && infoOpt.value()) {
                    auto info = infoOpt.value();
                    if (!info->lookUpName.empty()) {
                        ImGui::Text("Lookup Name: %s", info->lookUpName.c_str());
                    }
                    ImGui::Text("Content Hash: %zu", info->contentHash);
                    ImGui::Text("Loaded in Memory: %s", info->isLoaded ? "Yes" : "No");
                    if (!info->importContext.importPath.empty()) {
                        ImGui::Text("Import Path: %s", info->importContext.importPath.c_str());
                    }
                }

                // Type specific info
                if (assetType == AssetType::Texture) {
                    auto texData = assetManager ? assetManager->getAssetData<TextureData>(fileUuid.value()) : nullptr;
                    if (texData) {
                        ImGui::Separator();
                        ImGui::Text("Dimensions: %u x %u", texData->width, texData->height);
                        ImGui::Text("Channels: %u", texData->channels);
                        ImGui::Text("Has Alpha: %s", texData->hasAlpha ? "Yes" : "No");
                        ImGui::Text("Texture Type: %s", texData->type == TextureType::Texture2D ? "2D" : "Cube Map");
                    }
                } else if (assetType == AssetType::Model) {
                    auto modelData = assetManager ? assetManager->getAssetData<ModelData>(fileUuid.value()) : nullptr;
                    if (modelData) {
                        ImGui::Separator();
                        ImGui::Text("Bounding Box Min: (%.2f, %.2f, %.2f)", modelData->boundingBoxMin.x, modelData->boundingBoxMin.y, modelData->boundingBoxMin.z);
                        ImGui::Text("Bounding Box Max: (%.2f, %.2f, %.2f)", modelData->boundingBoxMax.x, modelData->boundingBoxMax.y, modelData->boundingBoxMax.z);
                    }
                } else if (assetType == AssetType::Shader) {
                    auto shaderData = assetManager ? assetManager->getAssetData<ShaderData>(fileUuid.value()) : nullptr;
                    if (shaderData) {
                        ImGui::Separator();
                        ImGui::Text("SPIR-V Bytecode Size: %zu words", shaderData->bytecode.size());
                        ImGui::Text("Stage: %s", EnumToString(shaderData->stage).data());
                    }
                } else if (assetType == AssetType::Material) {
                    auto matData = assetManager ? assetManager->getAssetData<MaterialData>(fileUuid.value()) : nullptr;
                    if (matData) {
                        bool matModified = false;
                        ImGui::Separator();
                        ImGui::Text("Material Properties");

                        auto renderTextureSlot = [this, &matModified](const char* label, const char* imguiId, std::shared_ptr<am::AssetInfo>& texInfo) {
                            ImGui::PushID(imguiId);
                            ImGui::Text("%s", label);

                            std::string currentName = "(None)";
                            if (texInfo) {
                                if (!texInfo->lookUpName.empty()) {
                                    currentName = texInfo->lookUpName;
                                } else {
                                    currentName = std::filesystem::path(texInfo->path).filename().string();
                                }
                            }

                            void* thumb = (texInfo && assetManager) ? assetManager->getThumbnailTexture(texInfo->path) : nullptr;
                            if (thumb != nullptr) {
                                ImGui::Image((ImTextureID)thumb, ImVec2(24, 24));
                                ImGui::SameLine();
                            }

                            float availW = ImGui::GetContentRegionAvail().x - 30.0f;
                            if (availW < 80.0f) availW = 80.0f;
                            ImGui::SetNextItemWidth(availW);

                            auto registeredTextures = assetManager ? assetManager->getRegisteredAssetsNames(AssetType::Texture) : std::vector<std::string>();
                            if (ImGui::BeginCombo("##TexCombo", currentName.c_str())) {
                                bool isNoneSelected = (texInfo == nullptr);
                                if (ImGui::Selectable("(None)", isNoneSelected)) {
                                    texInfo = nullptr;
                                    matModified = true;
                                }
                                if (isNoneSelected) {
                                    ImGui::SetItemDefaultFocus();
                                }

                                for (const auto& texName : registeredTextures) {
                                    bool isSelected = (texInfo && (texInfo->lookUpName == texName || std::filesystem::path(texInfo->path).filename().string() == texName));
                                    if (ImGui::Selectable(texName.c_str(), isSelected)) {
                                        auto tUuid = assetManager ? assetManager->getAssetUuid(texName) : std::nullopt;
                                        if (tUuid && assetManager) {
                                            texInfo = assetManager->getAssetInfo(tUuid.value()).value_or(nullptr);
                                            matModified = true;
                                        }
                                    }
                                    if (isSelected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            // Drag and Drop target for texture
                            if (ImGui::BeginDragDropTarget()) {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH")) {
                                    auto droppedPaths = ParseFilePathPayload(payload);
                                    if (!droppedPaths.empty()) {
                                        const auto& droppedPath = droppedPaths[0];
                                        auto droppedExt = droppedPath.extension().string();
                                        if (GetAssetTypeFromExtension(droppedExt) == AssetType::Texture ||
                                            droppedExt == ".png" || droppedExt == ".jpg" || droppedExt == ".jpeg" ||
                                            droppedExt == ".bmp" || droppedExt == ".tga" || droppedExt == ".dds" || droppedExt == ".hdr") {
                                            auto tUuid = assetManager ? assetManager->getAssetUuidByPath(droppedPath) : std::nullopt;
                                            if (!tUuid && assetManager) {
                                                tUuid = assetManager->registerAsset(droppedPath.string());
                                            }
                                            if (tUuid && assetManager) {
                                                texInfo = assetManager->getAssetInfo(tUuid.value()).value_or(nullptr);
                                                matModified = true;
                                            }
                                        }
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }

                            ImGui::SameLine();
                            if (ImGui::SmallButton("x")) {
                                if (texInfo != nullptr) {
                                    texInfo = nullptr;
                                    matModified = true;
                                }
                            }
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear texture");

                            ImGui::PopID();
                        };

                        if (ImGui::Checkbox("Use Specular-Glossiness Workflow", &matData->useSpecularGlossiness)) {
                            matModified = true;
                        }

                        ImGui::Spacing();

                        // Base Color / Albedo
                        if (ImGui::ColorEdit4("Base Color Factor", &matData->baseColorFactor.x, ImGuiColorEditFlags_Float)) {
                            matModified = true;
                        }
                        renderTextureSlot("Base Color Texture", "##BaseColorTex", matData->baseColorTexture);

                        ImGui::Spacing();

                        if (!matData->useSpecularGlossiness) {
                            // Metallic - Roughness Workflow
                            if (ImGui::SliderFloat("Metallic Factor", &matData->metallicFactor, 0.0f, 1.0f, "%.3f")) {
                                matModified = true;
                            }
                            if (ImGui::SliderFloat("Roughness Factor", &matData->roughnessFactor, 0.0f, 1.0f, "%.3f")) {
                                matModified = true;
                            }
                            renderTextureSlot("Metallic-Roughness Texture", "##MetRoughTex", matData->metallicRoughnessTexture);
                        } else {
                            // Specular - Glossiness Workflow
                            if (ImGui::ColorEdit3("Diffuse Factor", &matData->diffuseFactor.x, ImGuiColorEditFlags_Float)) {
                                matModified = true;
                            }
                            if (ImGui::ColorEdit3("Specular Factor", &matData->specularFactor.x, ImGuiColorEditFlags_Float)) {
                                matModified = true;
                            }
                            if (ImGui::SliderFloat("Glossiness Factor", &matData->glossinessFactor, 0.0f, 1.0f, "%.3f")) {
                                matModified = true;
                            }
                            renderTextureSlot("Specular-Glossiness Texture", "##SpecGlossTex", matData->specularGlossinessTexture);
                        }

                        ImGui::Spacing();

                        // Normal Map
                        renderTextureSlot("Normal Map Texture", "##NormalTex", matData->normalTexture);

                        ImGui::Spacing();

                        // Ambient Occlusion
                        if (ImGui::SliderFloat("Occlusion Strength", &matData->occlusionStrength, 0.0f, 1.0f, "%.3f")) {
                            matModified = true;
                        }
                        renderTextureSlot("Occlusion Texture", "##OcclusionTex", matData->occlusionTexture);

                        ImGui::Spacing();

                        // Emissive
                        if (ImGui::ColorEdit3("Emissive Factor", &matData->emissiveFactor.x, ImGuiColorEditFlags_Float)) {
                            matModified = true;
                        }
                        renderTextureSlot("Emissive Texture", "##EmissiveTex", matData->emissiveTexture);

                        ImGui::Spacing();

                        // Alpha & Transparency
                        if (ImGui::Checkbox("Is Opaque", &matData->isOpaque)) {
                            matModified = true;
                        }
                        if (ImGui::SliderFloat("Alpha Cutoff", &matData->alphaCutoff, 0.0f, 1.0f, "%.3f")) {
                            matModified = true;
                        }

                        ImGui::Spacing();

                        // Legacy Diffuse
                        if (ImGui::TreeNode("Legacy & Additional Maps")) {
                            renderTextureSlot("Legacy Diffuse Texture", "##LegacyDiffTex", matData->diffuseTexture);
                            ImGui::TreePop();
                        }

                        ImGui::Spacing();
                        if (ImGui::Button("Save Material") || matModified) {
                            if (assetManager) assetManager->saveAsset(fileUuid.value());
                        }
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status: Not Registered in Registry");
                if (ownership == AssetOwnership::Import) {
                    if (ImGui::Button("Import into Asset Manager")) {
                        if (assetManager) assetManager->registerAsset(currentSelected.string());
                    }
                }
            }
        }

        // 4. Actions
        if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!isDirectory) {
                if (ImGui::Button("Open / Preview Asset")) {
                    if (assetManager) assetManager->openAssetFile(currentSelected);
                }
                ImGui::SameLine();
            } else {
                if (ImGui::Button("Open Directory")) {
                    currentPath = currentSelected;
                    clearSelectedFiles();
                    ImGui::End();
                    return;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Open in IDE")) {
                if (assetManager) assetManager->openFileInIDE(currentSelected);
            }
            ImGui::SameLine();

            if (ImGui::Button("Open in System Explorer")) {
                if (assetManager) assetManager->openFileWithDefaultApp(currentSelected);
            }

            if (isRegistered) {
                if (ImGui::Button("Reimport Asset")) {
                    if (assetManager) assetManager->reimportAsset(currentSelected);
                }
                if (assetType == AssetType::Texture || assetType == AssetType::Model || assetType == AssetType::Mesh || assetType == AssetType::Material || assetType == AssetType::Scene ||
                    ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".fbx" || ext == ".obj" || ext == ".mat") {
                    ImGui::SameLine();
                    if (ImGui::Button("Regenerate Thumbnail")) {
                        if (assetManager) assetManager->generateThumbnail(fileUuid.value());
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Copy File")) {
                copyFileToClipboard(currentSelected);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cut File")) {
                cutFileToClipboard(currentSelected);
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate File")) {
                duplicateFile(currentSelected);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete File")) {
                deleteFile(currentSelected);
                clearSelectedFiles();
                ImGui::End();
                return;
            }
        }

        ImGui::End();
    }

    void FileBrowser::setSelectedFile(const std::filesystem::path& path)
    {
        selectedFiles.clear();
        if (!path.empty()) {
            selectedFiles.push_back(path);
        }
        selectedFile = path;
    }

    std::filesystem::path FileBrowser::getSelectedFile() const
    {
        if (!selectedFiles.empty()) {
            return selectedFiles.back();
        }
        return selectedFile;
    }

    const std::vector<std::filesystem::path>& FileBrowser::getSelectedFiles() const
    {
        return selectedFiles;
    }

    void FileBrowser::setSelectedFiles(const std::vector<std::filesystem::path>& paths)
    {
        selectedFiles = paths;
        selectedFile = selectedFiles.empty() ? std::filesystem::path() : selectedFiles.back();
    }

    void FileBrowser::addSelectedFile(const std::filesystem::path& path)
    {
        if (path.empty()) return;
        std::error_code ec;
        for (const auto& p : selectedFiles) {
            if (p == path || std::filesystem::equivalent(p, path, ec)) return;
        }
        selectedFiles.push_back(path);
        selectedFile = path;
    }

    void FileBrowser::removeSelectedFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        selectedFiles.erase(std::remove_if(selectedFiles.begin(), selectedFiles.end(),
            [&path, &ec](const std::filesystem::path& p) {
                return p == path || std::filesystem::equivalent(p, path, ec);
            }), selectedFiles.end());
        selectedFile = selectedFiles.empty() ? std::filesystem::path() : selectedFiles.back();
    }

    void FileBrowser::clearSelectedFiles()
    {
        selectedFiles.clear();
        selectedFile.clear();
    }

    bool FileBrowser::isFileSelected(const std::filesystem::path& path) const
    {
        std::error_code ec;
        for (const auto& p : selectedFiles) {
            if (p == path || std::filesystem::equivalent(p, path, ec)) {
                return true;
            }
        }
        return false;
    }

    std::filesystem::path FileBrowser::getUniqueCopyPath(const std::filesystem::path& targetPath) const
    {
        std::error_code ec;
        if (!std::filesystem::exists(targetPath, ec)) {
            return targetPath;
        }

        std::filesystem::path parentDir = targetPath.parent_path();
        std::string stem = targetPath.stem().string();
        std::string ext = targetPath.extension().string();

        int counter = 1;
        while (true) {
            std::string newFilename = stem + " (" + std::to_string(counter) + ")" + ext;
            std::filesystem::path candidate = parentDir / newFilename;
            if (!std::filesystem::exists(candidate, ec)) {
                return candidate;
            }
            counter++;
        }
    }

    bool FileBrowser::copyFileOrDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, bool overwrite)
    {
        std::error_code ec;
        if (!std::filesystem::exists(sourcePath, ec)) {
            spdlog::error("copyFileOrDirectory failed: source '{}' does not exist", sourcePath.string());
            return false;
        }
        if (!std::filesystem::is_directory(destDir, ec)) {
            spdlog::error("copyFileOrDirectory failed: destination '{}' is not a directory", destDir.string());
            return false;
        }

        std::filesystem::path target = destDir / sourcePath.filename();
        if (std::filesystem::equivalent(sourcePath, target, ec)) {
            target = getUniqueCopyPath(target);
        } else if (!overwrite && std::filesystem::exists(target, ec)) {
            target = getUniqueCopyPath(target);
        }

        try {
            if (std::filesystem::is_directory(sourcePath, ec)) {
                std::filesystem::copy(sourcePath, target, std::filesystem::copy_options::recursive | (overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none));
            } else {
                std::filesystem::copy_file(sourcePath, target, overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none);
                auto ext = target.extension().string();
                if (StringToAssetOwnership(ext) == AssetOwnership::Import || GetAssetTypeFromExtension(ext) != AssetType::Other) {
                    if (assetManager) {
                        assetManager->registerAsset(target.string());
                    }
                }
            }
            setSelectedFile(target);
            return true;
        } catch (const std::exception& e) {
            spdlog::error("copyFileOrDirectory exception: {}", e.what());
            return false;
        }
    }

    bool FileBrowser::moveFileOrDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, bool overwrite)
    {
        std::error_code ec;
        if (!std::filesystem::exists(sourcePath, ec)) {
            spdlog::error("moveFileOrDirectory failed: source '{}' does not exist", sourcePath.string());
            return false;
        }
        if (!std::filesystem::is_directory(destDir, ec)) {
            spdlog::error("moveFileOrDirectory failed: destination '{}' is not a directory", destDir.string());
            return false;
        }

        std::filesystem::path target = destDir / sourcePath.filename();
        if (std::filesystem::equivalent(sourcePath, target, ec)) {
            return true;
        }
        if (!overwrite && std::filesystem::exists(target, ec)) {
            target = getUniqueCopyPath(target);
        }

        try {
            std::filesystem::path sourceMeta = sourcePath.string() + ".meta";
            std::filesystem::path targetMeta = target.string() + ".meta";

            std::filesystem::rename(sourcePath, target);
            if (std::filesystem::exists(sourceMeta, ec)) {
                std::filesystem::rename(sourceMeta, targetMeta, ec);
            }

            if (assetManager) {
                auto uuidOpt = assetManager->getAssetUuidByPath(sourcePath);
                if (uuidOpt.has_value()) {
                    auto it = assetManager->metadata.find(uuidOpt.value());
                    if (it != assetManager->metadata.end()) {
                        it->second->path = target.string();
                        assetManager->saveAssetMetadata(uuidOpt.value());
                    }
                }
            }

            for (auto& p : selectedFiles) {
                if (p == sourcePath || std::filesystem::equivalent(p, sourcePath, ec)) {
                    p = target;
                }
            }
            if (selectedFile == sourcePath) {
                selectedFile = target;
            }
            return true;
        } catch (const std::exception& e) {
            spdlog::error("moveFileOrDirectory exception: {}", e.what());
            return false;
        }
    }

    bool FileBrowser::duplicateFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            return false;
        }
        return copyFileOrDirectory(path, path.parent_path(), false);
    }

    bool FileBrowser::duplicateFiles(const std::vector<std::filesystem::path>& paths)
    {
        bool any = false;
        std::vector<std::filesystem::path> newSelection;
        auto pathsCopy = paths;
        for (const auto& p : pathsCopy) {
            if (copyFileOrDirectory(p, p.parent_path(), false)) {
                any = true;
                if (!selectedFile.empty()) {
                    newSelection.push_back(selectedFile);
                }
            }
        }
        if (!newSelection.empty()) {
            setSelectedFiles(newSelection);
        }
        return any;
    }

    bool FileBrowser::deleteFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            return false;
        }

        try {
            if (assetManager) {
                auto uuidOpt = assetManager->getAssetUuidByPath(path);
                if (uuidOpt.has_value()) {
                    assetManager->metadata.erase(uuidOpt.value());
                    assetManager->assets.erase(uuidOpt.value());
                }
            }

            std::filesystem::path metaPath = path.string() + ".meta";
            if (std::filesystem::exists(metaPath, ec)) {
                std::filesystem::remove(metaPath, ec);
            }

            std::filesystem::remove_all(path, ec);

            removeSelectedFile(path);
            return true;
        } catch (const std::exception& e) {
            spdlog::error("deleteFile exception: {}", e.what());
            return false;
        }
    }

    bool FileBrowser::deleteFiles(const std::vector<std::filesystem::path>& paths)
    {
        auto pathsCopy = paths;
        bool any = false;
        for (const auto& p : pathsCopy) {
            if (deleteFile(p)) {
                any = true;
            }
        }
        return any;
    }

    void FileBrowser::copyFilesToClipboard(const std::vector<std::filesystem::path>& paths)
    {
        auto pathsCopy = paths;
        clipboardPaths = pathsCopy;
        clipboardPath = clipboardPaths.empty() ? std::filesystem::path() : clipboardPaths.back();
        clipboardIsCut = false;
        if (!clipboardPaths.empty()) {
            std::string allText;
            for (size_t i = 0; i < clipboardPaths.size(); ++i) {
                if (i > 0) allText += "\n";
                allText += clipboardPaths[i].string();
            }
            ImGui::SetClipboardText(allText.c_str());
        }
    }

    void FileBrowser::cutFilesToClipboard(const std::vector<std::filesystem::path>& paths)
    {
        auto pathsCopy = paths;
        clipboardPaths = pathsCopy;
        clipboardPath = clipboardPaths.empty() ? std::filesystem::path() : clipboardPaths.back();
        clipboardIsCut = true;
        if (!clipboardPaths.empty()) {
            std::string allText;
            for (size_t i = 0; i < clipboardPaths.size(); ++i) {
                if (i > 0) allText += "\n";
                allText += clipboardPaths[i].string();
            }
            ImGui::SetClipboardText(allText.c_str());
        }
    }

    void FileBrowser::copyFileToClipboard(const std::filesystem::path& path)
    {
        copyFilesToClipboard({path});
    }

    void FileBrowser::cutFileToClipboard(const std::filesystem::path& path)
    {
        cutFilesToClipboard({path});
    }

    const std::vector<std::filesystem::path>& FileBrowser::getClipboardPaths() const
    {
        return clipboardPaths;
    }

    bool FileBrowser::pasteFileFromClipboard(const std::filesystem::path& targetDir)
    {
        std::vector<std::filesystem::path> sources = clipboardPaths;
        std::error_code ec;
        if (sources.empty()) {
            if (!clipboardPath.empty() && std::filesystem::exists(clipboardPath, ec)) {
                sources.push_back(clipboardPath);
            } else {
                const char* sysClip = ImGui::GetClipboardText();
                if (sysClip && sysClip[0] != '\0') {
                    std::stringstream ss(sysClip);
                    std::string line;
                    while (std::getline(ss, line)) {
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        if (!line.empty()) {
                            std::filesystem::path sysP(line);
                            if (std::filesystem::exists(sysP, ec)) {
                                sources.push_back(sysP);
                            }
                        }
                    }
                }
            }
        }
        if (sources.empty()) {
            return false;
        }

        bool res = false;
        std::vector<std::filesystem::path> newSelection;
        if (clipboardIsCut) {
            for (const auto& src : sources) {
                if (moveFileOrDirectory(src, targetDir, false)) {
                    res = true;
                }
            }
            clipboardPaths.clear();
            clipboardPath.clear();
            clipboardIsCut = false;
        } else {
            for (const auto& src : sources) {
                if (copyFileOrDirectory(src, targetDir, false)) {
                    res = true;
                    if (!selectedFile.empty()) {
                        newSelection.push_back(selectedFile);
                    }
                }
            }
            if (!newSelection.empty()) {
                setSelectedFiles(newSelection);
            }
        }
        return res;
    }

    std::filesystem::path FileBrowser::getClipboardPath() const
    {
        return clipboardPath;
    }

    bool FileBrowser::isClipboardCut() const
    {
        return clipboardIsCut;
    }

} // namespace am
