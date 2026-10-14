/*
    FileSelector.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once
#include <memory>
#include <string>
#include <vector>

namespace GuiSupport {
    class FileSelector {
    public:
        enum class Mode {
            OpenExistingFile,
            SaveNewFile,
        };
        enum class Result {
            /** User has not yet selected a file or cancelled. */
            InProgress,
            /** User has cancelled selection. */
            Cancelled,
            /** User has finished selecting a file. */
            Complete,
        };

    private:
        const Mode _mode;
        Result _result = Result::InProgress;
        std::vector<std::string> _pathParts;
        std::string _resultPath;
        struct FileInfo {
            std::string name;
            bool isDirectory = false;
            FileInfo(const std::string& name_, bool isDirectory_) : name(name_), isDirectory(isDirectory_) {}
        };
        std::unique_ptr<std::vector<FileInfo>> _fileList;
        char _saveFilenameBuf[1024];

    public:
        /** Start file selector in the given mode. If initialPath is the empty
            string, start in the process's current directory.

            In Mode::SaveNewFile, initialFilename prepopulates the filename field. Ignored in other modes. */
        FileSelector(Mode mode, std::string initialPath = "", std::string initialFilename = "");
        ~FileSelector();
        FileSelector(const FileSelector&) = delete;
        FileSelector(FileSelector&&) = delete;
        FileSelector& operator=(const FileSelector&) = delete;
        FileSelector& operator=(FileSelector&&) = delete;

        /** Render the contents of the file selection window. Caller is
            responsible for creating and positioning the window. */
        void renderInterior();
        /** Return the result of file selection. */
        Result result();
        /** Return selected file path if result() is Complete, otherwise the
            empty string. */
        std::string resultPath();
    };
}
