#include <boost/test/unit_test.hpp>
#include "../src/AssetManager.hpp"
#include "../src/fileBrowser/ImguiFileBrowser.hpp"
#include <filesystem>
#include <fstream>

using namespace am;

BOOST_AUTO_TEST_SUITE(FileBrowserTestSuite)

BOOST_AUTO_TEST_CASE(FileBrowserSelectionTest) {
    auto& manager = AssetManager::getInstance();
    FileBrowser browser(&manager);

    std::filesystem::path p1 = "res/textures/Missing_Texture.png";
    std::filesystem::path p2 = "res/.internal/tests/Box.fbx";

    browser.clearSelectedFiles();
    BOOST_CHECK(!browser.isFileSelected(p1));
    BOOST_CHECK(browser.getSelectedFiles().empty());

    browser.setSelectedFile(p1);
    BOOST_CHECK(browser.isFileSelected(p1));
    BOOST_CHECK_EQUAL(browser.getSelectedFile(), p1);
    BOOST_CHECK_EQUAL(browser.getSelectedFiles().size(), 1);

    browser.addSelectedFile(p2);
    BOOST_CHECK(browser.isFileSelected(p1));
    BOOST_CHECK(browser.isFileSelected(p2));
    BOOST_CHECK_EQUAL(browser.getSelectedFiles().size(), 2);

    browser.removeSelectedFile(p1);
    BOOST_CHECK(!browser.isFileSelected(p1));
    BOOST_CHECK(browser.isFileSelected(p2));
    BOOST_CHECK_EQUAL(browser.getSelectedFiles().size(), 1);

    browser.clearSelectedFiles();
    BOOST_CHECK(browser.getSelectedFiles().empty());
}

BOOST_AUTO_TEST_CASE(FileBrowserClipboardAndHistoryTest) {
    auto& manager = AssetManager::getInstance();
    FileBrowser browser(&manager);

    std::filesystem::path testDir = "res/.internal/tests/temp/temp_fb_test_dir";
    std::filesystem::create_directories(testDir);

    std::filesystem::path srcFile = testDir / "source.txt";
    {
        std::ofstream out(srcFile);
        out << "hello world";
    }

    browser.copyFileToClipboard(srcFile);
    BOOST_CHECK(!browser.isClipboardCut());
    BOOST_CHECK_EQUAL(browser.getClipboardPath(), srcFile);

    std::filesystem::path uniquePath = browser.getUniqueCopyPath(srcFile);
    BOOST_CHECK(uniquePath != srcFile);

    bool pasted = browser.pasteFileFromClipboard(testDir);
    BOOST_CHECK(pasted);

    bool dup = browser.duplicateFile(srcFile);
    BOOST_CHECK(dup);

    FileBrowser::FileActionRecord action;
    action.type = FileBrowser::FileActionRecord::Type::Copy;
    action.sourcePath = srcFile;
    action.targetPath = uniquePath;
    action.description = "Copy Test File";

    browser.clearFileActionHistory();
    BOOST_CHECK(!browser.canUndoFileAction());
    BOOST_CHECK(!browser.canRedoFileAction());

    browser.pushFileAction(action);
    BOOST_CHECK(browser.canUndoFileAction());
    BOOST_CHECK_EQUAL(browser.getUndoFileActionName(), "Copy Test File");

    browser.undoFileAction();
    BOOST_CHECK(!browser.canUndoFileAction());
    BOOST_CHECK(browser.canRedoFileAction());

    browser.redoFileAction();
    BOOST_CHECK(browser.canUndoFileAction());
    BOOST_CHECK(!browser.canRedoFileAction());

    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);
}

BOOST_AUTO_TEST_SUITE_END()
