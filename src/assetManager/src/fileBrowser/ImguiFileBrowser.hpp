//
// Created by redkc on 10/10/2026.
//

#ifndef IMGUIFILEBROWSER_HPP
#define IMGUIFILEBROWSER_HPP

#include "ImguiFileBrowserInterface.h"
#include <string>
#include <vector>
#include <filesystem>

namespace am {

    class AssetManager;

    class FileBrowser : public ImguiFileBrowserInterface {
    public:
        explicit FileBrowser(AssetManager* assetManager);
        ~FileBrowser() override = default;

        void ImguiFileBrowser(std::string windowName) override;
        void ImguiFileInspector(std::string windowName = "File Inspector") override;

        void setSelectedFile(const std::filesystem::path& path) override;
        std::filesystem::path getSelectedFile() const override;
        const std::vector<std::filesystem::path>& getSelectedFiles() const override;
        void setSelectedFiles(const std::vector<std::filesystem::path>& paths) override;
        void addSelectedFile(const std::filesystem::path& path) override;
        void removeSelectedFile(const std::filesystem::path& path) override;
        void clearSelectedFiles() override;
        bool isFileSelected(const std::filesystem::path& path) const override;

        void copyFileToClipboard(const std::filesystem::path& path) override;
        void cutFileToClipboard(const std::filesystem::path& path) override;
        void copyFilesToClipboard(const std::vector<std::filesystem::path>& paths) override;
        void cutFilesToClipboard(const std::vector<std::filesystem::path>& paths) override;
        const std::vector<std::filesystem::path>& getClipboardPaths() const override;
        bool pasteFileFromClipboard(const std::filesystem::path& targetDir) override;
        bool duplicateFile(const std::filesystem::path& path) override;
        bool duplicateFiles(const std::vector<std::filesystem::path>& paths) override;
        bool deleteFile(const std::filesystem::path& path) override;
        bool deleteFiles(const std::vector<std::filesystem::path>& paths) override;
        bool copyFileOrDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, bool overwrite = false) override;
        bool moveFileOrDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, bool overwrite = false) override;
        std::filesystem::path getUniqueCopyPath(const std::filesystem::path& targetPath) const override;
        std::filesystem::path getClipboardPath() const override;
        bool isClipboardCut() const override;

        void saveFileBrowserConfig();
        void loadFileBrowserConfig();

        float fileBrowserScale = 1.0f;
        float fileBrowserLetterScale = 1.0f;
        std::string fileBrowserConfigLookupName = "fileBrowserConfig";
        std::filesystem::path currentPath;
        std::filesystem::path selectedFile;
        std::vector<std::filesystem::path> selectedFiles;
        std::filesystem::path clipboardPath;
        std::vector<std::filesystem::path> clipboardPaths;
        bool clipboardIsCut = false;

    private:
        AssetManager* assetManager = nullptr;
    };

} // namespace am

#endif // IMGUIFILEBROWSER_HPP
