//
// Created by redkc on 10/10/2026.
//

#ifndef IMGUIFILEBROWSERINTERFACE_H
#define IMGUIFILEBROWSERINTERFACE_H

#include <string>
#include <vector>
#include <filesystem>

namespace am {

    class ImguiFileBrowserInterface {
    public:
        virtual ~ImguiFileBrowserInterface() = default;

        virtual void ImguiFileBrowser(std::string windowName) = 0;
        virtual void ImguiFileInspector(std::string windowName = "File Inspector") = 0;
        virtual void setSelectedFile(const std::filesystem::path& path) = 0;
        virtual std::filesystem::path getSelectedFile() const = 0;
        virtual const std::vector<std::filesystem::path>& getSelectedFiles() const = 0;
        virtual void setSelectedFiles(const std::vector<std::filesystem::path>& paths) = 0;
        virtual void addSelectedFile(const std::filesystem::path& path) = 0;
        virtual void removeSelectedFile(const std::filesystem::path& path) = 0;
        virtual void clearSelectedFiles() = 0;
        virtual bool isFileSelected(const std::filesystem::path& path) const = 0;

        virtual void copyFileToClipboard(const std::filesystem::path& path) = 0;
        virtual void cutFileToClipboard(const std::filesystem::path& path) = 0;
        virtual void copyFilesToClipboard(const std::vector<std::filesystem::path>& paths) = 0;
        virtual void cutFilesToClipboard(const std::vector<std::filesystem::path>& paths) = 0;
        virtual const std::vector<std::filesystem::path>& getClipboardPaths() const = 0;
        virtual bool pasteFileFromClipboard(const std::filesystem::path& targetDir) = 0;
        virtual bool duplicateFile(const std::filesystem::path& path) = 0;
        virtual bool duplicateFiles(const std::vector<std::filesystem::path>& paths) = 0;
        virtual bool deleteFile(const std::filesystem::path& path) = 0;
        virtual bool deleteFiles(const std::vector<std::filesystem::path>& paths) = 0;
        virtual bool copyFileOrDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, bool overwrite = false) = 0;
        virtual bool moveFileOrDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, bool overwrite = false) = 0;
        virtual std::filesystem::path getUniqueCopyPath(const std::filesystem::path& targetPath) const = 0;
        virtual std::filesystem::path getClipboardPath() const = 0;
        virtual bool isClipboardCut() const = 0;
    };

} // namespace am

#endif // IMGUIFILEBROWSERINTERFACE_H
