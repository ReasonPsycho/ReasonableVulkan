//
// Created by redkc on 04.10.2026.
//

#include "../AssetManager.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <algorithm>

#include "IconsFontAwesome6.h"
#include "assets/shaderAsset/ShaderAsset.h"
#include "assets/configAsset/ConfigAsset.h"

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

        if (currentPath != resourceFolder)
        {
            if (ImGui::Button(".."))
            {
                currentPath = currentPath.parent_path();
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
            float cellSize = iconSize + padding;

            float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            try {
                int i = 0;
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

                    ImGui::PushID(i++);
                    ImGui::BeginGroup();

                    // Dummy "icon"
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

                    ImGui::SetWindowFontScale(fileBrowserScale);

                    void* thumbTex = nullptr;
                    if (!entry.is_directory()) {
                        thumbTex = getThumbnailTexture(path);
                    }

                    if (thumbTex != nullptr) {
                        bool clicked = ImGui::ImageButton("##thumb", (ImTextureID)thumbTex, ImVec2(iconSize, iconSize));
                        bool doubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);
                        if (clicked || doubleClicked) {
                            openAssetFile(path);
                        }
                    } else {
                        // Use large icon font if available (it's the second font we loaded)
                        bool pushedFont = false;
                        if (ImGui::GetIO().Fonts->Fonts.Size > 1) {
                            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
                            pushedFont = true;
                        }

                        if (entry.is_directory()) {
                            ImGui::Button(ICON_FA_FOLDER, ImVec2(iconSize, iconSize));
                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                currentPath = path;
                            }
                        } else {
                            bool clicked = ImGui::Button(GetAssetIcon(path), ImVec2(iconSize, iconSize));
                            bool doubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);
                            if (clicked || doubleClicked) {
                                openAssetFile(path);
                            }
                        }

                        if (pushedFont)
                        {
                            ImGui::PopFont();
                        }
                    }

                    ImGui::PopStyleColor(3);

                    ImGui::SetWindowFontScale(fileBrowserLetterScale);

                    // Centered text below icon
                    float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
                    if (textWidth > iconSize) {
                        // Truncate text if too long
                        std::string truncated = filename;
                        while (!truncated.empty() && ImGui::CalcTextSize((truncated + "...").c_str()).x > iconSize) {
                            truncated.pop_back();
                        }
                        truncated += "...";
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - ImGui::CalcTextSize(truncated.c_str()).x) * 0.5f);
                        ImGui::Text("%s", truncated.c_str());
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", filename.c_str());
                    } else {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - textWidth) * 0.5f);
                        ImGui::Text("%s", filename.c_str());
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        if (entry.is_directory()) {
                            currentPath = path;
                        } else {
                            openAssetFile(path);
                        }
                    }

                    ImGui::EndGroup();

                    float lastButtonX2 = ImGui::GetItemRectMax().x;

                    if (entry.is_directory()) {
                        if (ImGui::BeginPopupContextItem("##FolderContext")) {
                            if (ImGui::MenuItem("Open in System File Explorer")) {
                                openFileWithDefaultApp(path);
                            }
                            ImGui::EndPopup();
                        }
                    } else {
                        auto fileUuid = getAssetUuidByPath(path);
                        bool isRegistered = fileUuid.has_value();
                        bool canImport = (StringToAssetOwnership(path.extension().string()) == AssetOwnership::Import);
                        AssetType type = GetAssetTypeFromExtension(path.extension().string());

                        if (type == AssetType::Scene && engine) {
                            if (ImGui::BeginPopupContextItem("##SceneContext")) {
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
                                ImGui::EndPopup();
                            }
                        } else if ((type == AssetType::Model || type == AssetType::Mesh) && engine) {
                            if (ImGui::BeginPopupContextItem("##ModelContext")) {
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
                                ImGui::EndPopup();
                            }
                        } else {
                            if (ImGui::BeginPopupContextItem("##FileContext")) {
                                if (isRegistered) {
                                    if (type == AssetType::Texture || path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg" || path.extension() == ".bmp") {
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
                                ImGui::EndPopup();
                            }
                        }
                    }

                    float nextButtonX2 = lastButtonX2 + padding + cellSize;
                    if (nextButtonX2 < windowVisibleX2)
                        ImGui::SameLine(0, padding);

                    ImGui::PopID();
                }
            } catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
                if (ImGui::Button("Reset to Resource Folder")) {
                    currentPath = resourceFolder;
                }
            }

            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
        }

        ImGui::End();
    }

} // namespace am
