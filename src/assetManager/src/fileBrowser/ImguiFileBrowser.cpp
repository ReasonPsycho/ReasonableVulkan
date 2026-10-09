//
// Created by redkc on 04.10.2026.
//

#include "../AssetManager.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <ctime>

#include "IconsFontAwesome6.h"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/configAsset/ConfigAsset.h"
#include "assetDatas/TextureData.h"
#include "assetDatas/ModelData.h"
#include "assetDatas/MeshData.h"
#include "assetDatas/ShaderData.h"
#include "assetDatas/MaterialData.h"

namespace am {

    void AssetManager::loadFileBrowserConfig()
    {
        auto uuid = getAssetUuid(fileBrowserConfigLookupName);
        if (!uuid) return;

        auto configData = getAssetData<rapidjson::Document>(uuid.value());
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

    void AssetManager::saveFileBrowserConfig()
    {
        auto uuid = getAssetUuid(fileBrowserConfigLookupName);
        if (!uuid) {
            std::filesystem::path configPath = std::filesystem::path(resourceFolder) / ".cache" / "config" / "fileBrowser.config";
            try {
                uuid = createAsset(AssetType::Config, configPath.string(), fileBrowserConfigLookupName);
            } catch (...) {
                return;
            }
        }
        if (!uuid) return;

        auto configData = getAssetData<rapidjson::Document>(uuid.value());
        if (!configData) return;

        configData->SetObject();
        auto& allocator = configData->GetAllocator();
        configData->AddMember("scale", fileBrowserScale, allocator);
        configData->AddMember("letterScale", fileBrowserLetterScale, allocator);

        saveAsset(uuid.value());
    }

    void AssetManager::ImguiFileBrowser(std::string windowName)
    {
        ImGui::Begin(windowName.c_str());

        bool isBrowserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        bool isCtrlOrCmd = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        if (isBrowserFocused && !ImGui::GetIO().WantTextInput)
        {
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_C))
            {
                if (!selectedFile.empty()) {
                    copyFileToClipboard(selectedFile);
                }
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_X))
            {
                if (!selectedFile.empty()) {
                    cutFileToClipboard(selectedFile);
                }
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_V))
            {
                pasteFileFromClipboard(currentPath);
            }
            if (isCtrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_D))
            {
                if (!selectedFile.empty()) {
                    duplicateFile(selectedFile);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
            {
                if (!selectedFile.empty()) {
                    deleteFile(selectedFile);
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                selectedFile.clear();
            }
        }

        if (currentPath != resourceFolder)
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
                    const char* droppedPathStr = (const char*)payload->Data;
                    std::filesystem::path droppedPath(droppedPathStr);
                    std::error_code ec;
                    if (std::filesystem::exists(droppedPath, ec) && !std::filesystem::equivalent(droppedPath, currentPath.parent_path(), ec))
                    {
                        moveFileOrDirectory(droppedPath, currentPath.parent_path(), false);
                    }
                }
                else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                {
                    if (engine)
                    {
                        engine::ecs::Entity entity = *(const engine::ecs::Entity*)payload->Data;
                        engine::ecs::Scene* scn = nullptr;
                        if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                            scn = ((const engine::ecs::SceneEntityPayload*)payload->Data)->scene;
                        }
                        engine->SaveEntityAsPrefab(entity, currentPath.parent_path(), scn);
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
                    currentPath = resourceFolder;
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
                bool isSelected = (!selectedFile.empty() && std::filesystem::equivalent(selectedFile, path, ecEquiv));

                ImVec2 cursorPos = ImGui::GetCursorScreenPos();

                // Drag detection
                bool isBeingDragged = false;
                const ImGuiPayload* curPayload = ImGui::GetDragDropPayload();
                if (curPayload && curPayload->IsDataType("AM_FILE_PATH") && curPayload->Data) {
                    const char* curPayloadStr = (const char*)curPayload->Data;
                    std::error_code ecDrag;
                    if (curPayloadStr && curPayloadStr[0] != '\0' && std::filesystem::equivalent(path, curPayloadStr, ecDrag)) {
                        isBeingDragged = true;
                    }
                }

                // Invisible button covering the entire tile
                ImGui::InvisibleButton("##tile", tileSize);
                bool isHovered = ImGui::IsItemHovered();
                bool isActive = ImGui::IsItemActive();

                // Drag Source on tile
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    std::string pathStr = path.string();
                    ImGui::SetDragDropPayload("AM_FILE_PATH", pathStr.c_str(), pathStr.size() + 1);

                    // Rich visual indicator tooltip
                    ImGui::BeginGroup();
                    void* dragThumb = !entry.is_directory() ? getThumbnailTexture(path) : nullptr;
                    if (dragThumb != nullptr) {
                        ImGui::Image((ImTextureID)dragThumb, ImVec2(36, 36));
                        ImGui::SameLine();
                    } else {
                        ImFont* iconFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
                        float dragIconSize = (ImGui::GetIO().Fonts->Fonts.Size > 1 ? 28.0f : ImGui::GetFontSize() * 2.0f);
                        const char* iconStr = entry.is_directory() ? ICON_FA_FOLDER : GetAssetIcon(path);
                        ImVec4 iconCol = entry.is_directory() ? ImVec4(0.96f, 0.80f, 0.43f, 1.0f) : ImVec4(0.40f, 0.75f, 1.0f, 1.0f);
                        ImGui::PushFont(iconFont);
                        ImGui::TextColored(iconCol, "%s", iconStr);
                        ImGui::PopFont();
                        ImGui::SameLine();
                    }
                    ImGui::BeginGroup();
                    ImGui::TextUnformatted(filename.c_str());
                    if (entry.is_directory()) {
                        ImGui::TextDisabled("Folder (drop to move)");
                    } else {
                        std::error_code ecSize;
                        auto fsize = std::filesystem::file_size(path, ecSize);
                        if (!ecSize) {
                            if (fsize < 1024) ImGui::TextDisabled("%llu B", (unsigned long long)fsize);
                            else if (fsize < 1024 * 1024) ImGui::TextDisabled("%.1f KB", fsize / 1024.0f);
                            else ImGui::TextDisabled("%.2f MB", fsize / (1024.0f * 1024.0f));
                        } else {
                            ImGui::TextDisabled("%s", path.extension().string().c_str());
                        }
                    }
                    ImGui::EndGroup();
                    ImGui::EndGroup();

                    ImGui::EndDragDropSource();
                }

                // Drop Target for directories
                bool isFolderDropTarget = false;
                if (entry.is_directory() && ImGui::BeginDragDropTarget())
                {
                    isFolderDropTarget = true;
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                    {
                        const char* droppedPathStr = (const char*)payload->Data;
                        std::filesystem::path droppedPath(droppedPathStr);
                        std::error_code ec;
                        if (std::filesystem::exists(droppedPath, ec) && !std::filesystem::equivalent(droppedPath, path, ec))
                        {
                            moveFileOrDirectory(droppedPath, path, false);
                        }
                    }
                    else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                    {
                        if (engine)
                        {
                            engine::ecs::Entity entity = *(const engine::ecs::Entity*)payload->Data;
                            engine::ecs::Scene* scn = nullptr;
                            if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                                scn = ((const engine::ecs::SceneEntityPayload*)payload->Data)->scene;
                            }
                            engine->SaveEntityAsPrefab(entity, path, scn);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Click interactions
                if (isHovered) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        if (entry.is_directory()) {
                            currentPath = path;
                            selectedFile.clear();
                        } else {
                            openAssetFile(path);
                        }
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        selectedFile = path;
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        selectedFile = path;
                    }
                }

                auto renderCommonFileContextMenu = [this, &path, &entry]() {
                    std::error_code ecClip;
                    bool hasClipboard = (!clipboardPath.empty() && std::filesystem::exists(clipboardPath, ecClip));
                    if (!hasClipboard) {
                        const char* sysClip = ImGui::GetClipboardText();
                        if (sysClip && sysClip[0] != '\0') {
                            std::filesystem::path sysP(sysClip);
                            if (std::filesystem::exists(sysP, ecClip)) {
                                hasClipboard = true;
                            }
                        }
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
                    std::string relPath = std::filesystem::relative(path, resourceFolder, ecRel).string();
                    if (!ecRel && !relPath.empty()) {
                        if (ImGui::MenuItem("Copy Relative Path")) {
                            ImGui::SetClipboardText(relPath.c_str());
                        }
                    }
                };

                if (entry.is_directory()) {
                    if (ImGui::BeginPopupContextItem("##FolderContext")) {
                        selectedFile = path;
                        if (ImGui::MenuItem("Open Folder")) {
                            currentPath = path;
                            selectedFile.clear();
                        }
                        if (ImGui::MenuItem("Open in System File Explorer")) {
                            openFileWithDefaultApp(path);
                        }
                        renderCommonFileContextMenu();
                        ImGui::EndPopup();
                    }
                } else {
                    auto fileUuid = getAssetUuidByPath(path);
                    bool isRegistered = fileUuid.has_value();
                    bool canImport = (StringToAssetOwnership(path.extension().string()) == AssetOwnership::Import);
                    AssetType type = GetAssetTypeFromExtension(path.extension().string());

                    if (type == AssetType::Scene && engine) {
                        if (ImGui::BeginPopupContextItem("##SceneContext")) {
                            selectedFile = path;
                            if (isRegistered) {
                                if (ImGui::MenuItem("Open Scene")) {
                                    engine->LoadScene(fileUuid.value());
                                }
                                if (ImGui::MenuItem("Close Scene")) {
                                    engine->CloseScene(fileUuid.value());
                                }
                                if (ImGui::MenuItem("Reimport")) {
                                    reimportAsset(path);
                                }
                            } else if (canImport) {
                                if (ImGui::MenuItem("Import")) {
                                    registerAsset(path.string());
                                }
                            }
                            if (ImGui::MenuItem("Open in IDE")) {
                                openFileInIDE(path);
                            }
                            if (ImGui::MenuItem("Open in System Default")) {
                                openFileWithDefaultApp(path);
                            }
                            renderCommonFileContextMenu();
                            ImGui::EndPopup();
                        }
                    } else if ((type == AssetType::Model || type == AssetType::Mesh) && engine) {
                        if (ImGui::BeginPopupContextItem("##ModelContext")) {
                            selectedFile = path;
                            if (isRegistered) {
                                if (ImGui::MenuItem("Open in Preview Scene")) {
                                    engine->OpenModelPreviewScene(fileUuid.value());
                                }
                                if (ImGui::MenuItem("Regenerate Thumbnail")) {
                                    generateThumbnail(fileUuid.value());
                                }
                                if (ImGui::MenuItem("Reimport")) {
                                    reimportAsset(path);
                                }
                            } else if (canImport) {
                                if (ImGui::MenuItem("Import")) {
                                    registerAsset(path.string());
                                }
                            }
                            if (ImGui::MenuItem("Open in IDE")) {
                                openFileInIDE(path);
                            }
                            if (ImGui::MenuItem("Open in System Default")) {
                                openFileWithDefaultApp(path);
                            }
                            renderCommonFileContextMenu();
                            ImGui::EndPopup();
                        }
                    } else {
                        if (ImGui::BeginPopupContextItem("##FileContext")) {
                            selectedFile = path;
                            if (isRegistered) {
                                if (type == AssetType::Texture || type == AssetType::Material || path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg" || path.extension() == ".bmp" || path.extension() == ".mat") {
                                    if (ImGui::MenuItem("Regenerate Thumbnail")) {
                                        generateThumbnail(fileUuid.value());
                                    }
                                }
                                if (ImGui::MenuItem("Reimport")) {
                                    reimportAsset(path);
                                }
                            } else if (canImport) {
                                if (ImGui::MenuItem("Import")) {
                                    registerAsset(path.string());
                                }
                            }
                            if (ImGui::MenuItem("Open in IDE")) {
                                openFileInIDE(path);
                            }
                            if (ImGui::MenuItem("Open in System Default")) {
                                openFileWithDefaultApp(path);
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
                if (!entry.is_directory()) {
                    thumbTex = getThumbnailTexture(path);
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
                ImU32 textColor = isBeingDragged ? IM_COL32(160, 180, 220, 160) : IM_COL32(230, 230, 230, 255);
                drawList->AddText(textFont, textFontSize, textPos, textColor, truncated.c_str());

                if (isFolderDropTarget) {
                    const char* dropLabel = "Drop Here";
                    float dropTextW = textFont->CalcTextSizeA(textFontSize * 0.85f, FLT_MAX, 0.0f, dropLabel).x;
                    ImVec2 dropBadgePos = ImVec2(cursorPos.x + (iconSize - dropTextW) * 0.5f, cursorPos.y + 4.0f);
                    drawList->AddRectFilled(ImVec2(dropBadgePos.x - 4, dropBadgePos.y - 2), ImVec2(dropBadgePos.x + dropTextW + 4, dropBadgePos.y + textFontSize + 2), IM_COL32(20, 70, 30, 220), 3.0f);
                    drawList->AddText(textFont, textFontSize * 0.85f, dropBadgePos, IM_COL32(90, 255, 130, 255), dropLabel);
                } else if (isBeingDragged) {
                    const char* dragLabel = "Dragging";
                    float dragTextW = textFont->CalcTextSizeA(textFontSize * 0.85f, FLT_MAX, 0.0f, dragLabel).x;
                    ImVec2 dragBadgePos = ImVec2(cursorPos.x + (iconSize - dragTextW) * 0.5f, cursorPos.y + 4.0f);
                    drawList->AddRectFilled(ImVec2(dragBadgePos.x - 4, dragBadgePos.y - 2), ImVec2(dragBadgePos.x + dragTextW + 4, dragBadgePos.y + textFontSize + 2), IM_COL32(20, 35, 60, 220), 3.0f);
                    drawList->AddText(textFont, textFontSize * 0.85f, dragBadgePos, IM_COL32(100, 200, 255, 255), dragLabel);
                }

                if (isHovered && isTruncated) {
                    ImGui::SetTooltip("%s", filename.c_str());
                }

                float lastButtonX2 = cursorPos.x + iconSize;
                float nextButtonX2 = lastButtonX2 + padding + iconSize;
                if (i + 1 < entries.size() && nextButtonX2 < windowVisibleX2) {
                    ImGui::SameLine(0, padding);
                }

                ImGui::PopID();
            }

            ImVec2 avail = ImGui::GetContentRegionAvail();
            if (avail.y > 10.0f) {
                ImVec2 emptyMin = ImGui::GetCursorScreenPos();
                ImGui::Dummy(avail);
                ImVec2 emptyMax = ImGui::GetItemRectMax();
                bool isEmptyDropTarget = false;
                if (ImGui::BeginDragDropTarget())
                {
                    isEmptyDropTarget = true;
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
                    {
                        const char* droppedPathStr = (const char*)payload->Data;
                        std::filesystem::path droppedPath(droppedPathStr);
                        std::error_code ec;
                        if (std::filesystem::exists(droppedPath, ec))
                        {
                            if (!std::filesystem::equivalent(droppedPath.parent_path(), currentPath, ec))
                            {
                                moveFileOrDirectory(droppedPath, currentPath, false);
                            }
                        }
                    }
                    else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
                    {
                        if (engine)
                        {
                            engine::ecs::Entity entity = *(const engine::ecs::Entity*)payload->Data;
                            engine::ecs::Scene* scn = nullptr;
                            if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                                scn = ((const engine::ecs::SceneEntityPayload*)payload->Data)->scene;
                            }
                            engine->SaveEntityAsPrefab(entity, currentPath, scn);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                if (isEmptyDropTarget)
                {
                    ImGui::GetWindowDrawList()->AddRect(emptyMin, emptyMax, IM_COL32(66, 180, 255, 180), 4.0f, 0, 2.0f);
                    ImGui::GetWindowDrawList()->AddRectFilled(emptyMin, emptyMax, IM_COL32(66, 180, 255, 30), 4.0f);
                }
            }

            if (ImGui::BeginPopupContextWindow("##FileBrowserScrollContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                std::error_code ecClip;
                bool hasClipboard = (!clipboardPath.empty() && std::filesystem::exists(clipboardPath, ecClip));
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
                    selectedFile = newFolderPath;
                }
                if (ImGui::MenuItem("New Material")) {
                    std::filesystem::path newMatPath = currentPath / "New Material.material";
                    newMatPath = getUniqueCopyPath(newMatPath);
                    auto matUuid = createAsset(AssetType::Material, newMatPath.string());
                    if (matUuid) {
                        selectedFile = newMatPath;
                    }
                }
                if (ImGui::MenuItem("Open in System File Explorer")) {
                    openFileWithDefaultApp(currentPath);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Copy Current Path")) {
                    ImGui::SetClipboardText(currentPath.string().c_str());
                }
                ImGui::EndPopup();
            }

            ImGui::EndChild();
        }

        // Drop Target on the outer window space
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AM_FILE_PATH"))
            {
                const char* droppedPathStr = (const char*)payload->Data;
                std::filesystem::path droppedPath(droppedPathStr);
                std::error_code ec;
                if (std::filesystem::exists(droppedPath, ec))
                {
                    if (!std::filesystem::equivalent(droppedPath.parent_path(), currentPath, ec))
                    {
                        moveFileOrDirectory(droppedPath, currentPath, false);
                    }
                }
            }
            else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
            {
                if (engine)
                {
                    engine::ecs::Entity entity = *(const engine::ecs::Entity*)payload->Data;
                    engine::ecs::Scene* scn = nullptr;
                    if (payload->DataSize >= sizeof(engine::ecs::SceneEntityPayload)) {
                        scn = ((const engine::ecs::SceneEntityPayload*)payload->Data)->scene;
                    }
                    engine->SaveEntityAsPrefab(entity, currentPath, scn);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Context menu on outer window space
        if (ImGui::BeginPopupContextWindow("##FileBrowserWindowContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            std::error_code ecClip;
            bool hasClipboard = (!clipboardPath.empty() && std::filesystem::exists(clipboardPath, ecClip));
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
                selectedFile = newFolderPath;
            }
            if (ImGui::MenuItem("New Material")) {
                std::filesystem::path newMatPath = currentPath / "New Material.material";
                newMatPath = getUniqueCopyPath(newMatPath);
                auto matUuid = createAsset(AssetType::Material, newMatPath.string());
                if (matUuid) {
                    selectedFile = newMatPath;
                }
            }
            if (ImGui::MenuItem("Open in System File Explorer")) {
                openFileWithDefaultApp(currentPath);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Current Path")) {
                ImGui::SetClipboardText(currentPath.string().c_str());
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void AssetManager::ImguiFileInspector(std::string windowName)
    {
        if (focusFileInspectorRequested) {
            ImGui::SetNextWindowFocus();
            focusFileInspectorRequested = false;
        }

        ImGui::Begin(windowName.c_str());

        if (selectedFile.empty()) {
            ImGui::TextDisabled("No file selected in File Browser");
            ImGui::End();
            return;
        }

        std::error_code ec;
        if (!std::filesystem::exists(selectedFile, ec)) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Selected file does not exist:");
            ImGui::TextWrapped("%s", selectedFile.string().c_str());
            ImGui::End();
            return;
        }

        std::string filename = selectedFile.filename().string();
        std::string ext = selectedFile.extension().string();
        bool isDirectory = std::filesystem::is_directory(selectedFile, ec);
        AssetType assetType = GetAssetTypeFromExtension(ext);
        AssetOwnership ownership = StringToAssetOwnership(ext);

        auto fileUuid = getAssetUuidByPath(selectedFile);
        bool isRegistered = fileUuid.has_value();

        // 1. Preview / Header
        void* thumbTex = nullptr;
        if (!isDirectory) {
            thumbTex = getThumbnailTexture(selectedFile);
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

        ImGui::Text("%s %s", isDirectory ? ICON_FA_FOLDER : GetAssetIcon(selectedFile), filename.c_str());
        ImGui::Separator();

        // 2. File Information Header
        if (ImGui::CollapsingHeader("File Information", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Name: %s", filename.c_str());
            if (!isDirectory) {
                ImGui::Text("Extension: %s", ext.empty() ? "(none)" : ext.c_str());
            }

            std::string relPath = std::filesystem::relative(selectedFile, resourceFolder, ec).string();
            if (!ec && !relPath.empty()) {
                ImGui::Text("Relative Path: %s", relPath.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy##RelPath")) {
                    ImGui::SetClipboardText(relPath.c_str());
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy relative path to clipboard");
            }
            ImGui::TextWrapped("Full Path: %s", selectedFile.string().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Copy##FullPath")) {
                ImGui::SetClipboardText(selectedFile.string().c_str());
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy full path to clipboard");

            if (!isDirectory) {
                auto fSize = std::filesystem::file_size(selectedFile, ec);
                if (!ec) {
                    if (fSize < 1024) {
                        ImGui::Text("File Size: %llu B", static_cast<unsigned long long>(fSize));
                    } else if (fSize < 1024 * 1024) {
                        ImGui::Text("File Size: %.2f KB (%llu bytes)", fSize / 1024.0, static_cast<unsigned long long>(fSize));
                    } else {
                        ImGui::Text("File Size: %.2f MB (%llu bytes)", fSize / (1024.0 * 1024.0), static_cast<unsigned long long>(fSize));
                    }
                }

                auto lastWrite = std::filesystem::last_write_time(selectedFile, ec);
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

                auto infoOpt = getAssetInfo(fileUuid.value());
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
                    auto texData = getAssetData<TextureData>(fileUuid.value());
                    if (texData) {
                        ImGui::Separator();
                        ImGui::Text("Dimensions: %u x %u", texData->width, texData->height);
                        ImGui::Text("Channels: %u", texData->channels);
                        ImGui::Text("Has Alpha: %s", texData->hasAlpha ? "Yes" : "No");
                        ImGui::Text("Texture Type: %s", texData->type == TextureType::Texture2D ? "2D" : "Cube Map");
                    }
                } else if (assetType == AssetType::Model) {
                    auto modelData = getAssetData<ModelData>(fileUuid.value());
                    if (modelData) {
                        ImGui::Separator();
                        ImGui::Text("Bounding Box Min: (%.2f, %.2f, %.2f)", modelData->boundingBoxMin.x, modelData->boundingBoxMin.y, modelData->boundingBoxMin.z);
                        ImGui::Text("Bounding Box Max: (%.2f, %.2f, %.2f)", modelData->boundingBoxMax.x, modelData->boundingBoxMax.y, modelData->boundingBoxMax.z);
                    }
                } else if (assetType == AssetType::Shader) {
                    auto shaderData = getAssetData<ShaderData>(fileUuid.value());
                    if (shaderData) {
                        ImGui::Separator();
                        ImGui::Text("SPIR-V Bytecode Size: %zu words", shaderData->bytecode.size());
                        ImGui::Text("Stage: %s", EnumToString(shaderData->stage).data());
                    }
                } else if (assetType == AssetType::Material) {
                    auto matData = getAssetData<MaterialData>(fileUuid.value());
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

                            void* thumb = texInfo ? getThumbnailTexture(texInfo->path) : nullptr;
                            if (thumb != nullptr) {
                                ImGui::Image((ImTextureID)thumb, ImVec2(24, 24));
                                ImGui::SameLine();
                            }

                            float availW = ImGui::GetContentRegionAvail().x - 30.0f;
                            if (availW < 80.0f) availW = 80.0f;
                            ImGui::SetNextItemWidth(availW);

                            auto registeredTextures = getRegisteredAssetsNames(AssetType::Texture);
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
                                        auto tUuid = getAssetUuid(texName);
                                        if (tUuid) {
                                            texInfo = getAssetInfo(tUuid.value()).value_or(nullptr);
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
                                    std::string droppedPathStr = (const char*)payload->Data;
                                    std::filesystem::path droppedPath(droppedPathStr);
                                    auto droppedExt = droppedPath.extension().string();
                                    if (GetAssetTypeFromExtension(droppedExt) == AssetType::Texture ||
                                        droppedExt == ".png" || droppedExt == ".jpg" || droppedExt == ".jpeg" ||
                                        droppedExt == ".bmp" || droppedExt == ".tga" || droppedExt == ".dds" || droppedExt == ".hdr") {
                                        auto tUuid = getAssetUuidByPath(droppedPath);
                                        if (!tUuid) {
                                            tUuid = registerAsset(droppedPath.string());
                                        }
                                        if (tUuid) {
                                            texInfo = getAssetInfo(tUuid.value()).value_or(nullptr);
                                            matModified = true;
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
                            saveAsset(fileUuid.value());
                        }
                    }
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status: Not Registered in Registry");
                if (ownership == AssetOwnership::Import) {
                    if (ImGui::Button("Import into Asset Manager")) {
                        registerAsset(selectedFile.string());
                    }
                }
            }
        }

        // 4. Actions
        if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!isDirectory) {
                if (ImGui::Button("Open / Preview Asset")) {
                    openAssetFile(selectedFile);
                }
                ImGui::SameLine();
            } else {
                if (ImGui::Button("Open Directory")) {
                    currentPath = selectedFile;
                    selectedFile.clear();
                    ImGui::End();
                    return;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Open in IDE")) {
                openFileInIDE(selectedFile);
            }
            ImGui::SameLine();

            if (ImGui::Button("Open in System Explorer")) {
                openFileWithDefaultApp(selectedFile);
            }

            if (isRegistered) {
                if (ImGui::Button("Reimport Asset")) {
                    reimportAsset(selectedFile);
                }
                if (assetType == AssetType::Texture || assetType == AssetType::Model || assetType == AssetType::Mesh || assetType == AssetType::Material || assetType == AssetType::Scene ||
                    ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".fbx" || ext == ".obj" || ext == ".mat") {
                    ImGui::SameLine();
                    if (ImGui::Button("Regenerate Thumbnail")) {
                        generateThumbnail(fileUuid.value());
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Copy File")) {
                copyFileToClipboard(selectedFile);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cut File")) {
                cutFileToClipboard(selectedFile);
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate File")) {
                duplicateFile(selectedFile);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete File")) {
                deleteFile(selectedFile);
                ImGui::End();
                return;
            }
        }

        ImGui::End();
    }

} // namespace am
