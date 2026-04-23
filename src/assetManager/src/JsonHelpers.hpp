//
// Created by redkc on 14/04/2026.
//

#ifndef REASONABLEVULKAN_JSONHELPERS_HPP
#define REASONABLEVULKAN_JSONHELPERS_HPP

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
#include <string>
#include <spdlog/spdlog.h>

namespace am {

    inline bool saveJsonToFile(const std::string& filename, const rapidjson::Document& document) {
        try {
            // Convert to string with pretty printing
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            writer.SetIndent(' ', 2);
            document.Accept(writer);

            // Save to file
            std::ofstream ofs(filename, std::ios::out | std::ios::binary);
            if (!ofs.is_open()) {
                spdlog::error("Failed to open file for writing: {}", filename);
                return false;
            }

            // Write UTF-8 BOM
            const char bom[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
            ofs.write(bom, 3);

            // Write the JSON content
            ofs.write(buffer.GetString(), buffer.GetSize());
            ofs.close();

            return true;
        } catch (const std::exception& e) {
            spdlog::error("Error saving JSON to file {}: {}", filename, e.what());
            return false;
        }
    }

    inline bool loadJsonFromFile(const std::string& filename, rapidjson::Document& document) {
        try {
            std::ifstream ifs(filename, std::ios::binary);
            if (!ifs.is_open()) {
                // Not logging as error because sometimes files don't exist yet
                return false;
            }

            // Skip UTF-8 BOM if present
            char bom[3];
            ifs.read(bom, 3);
            if (!(bom[0] == static_cast<char>(0xEF) &&
                  bom[1] == static_cast<char>(0xBB) &&
                  bom[2] == static_cast<char>(0xBF))) {
                ifs.seekg(0);
            }

            std::string json_content((std::istreambuf_iterator<char>(ifs)),
                                   std::istreambuf_iterator<char>());

            document.Parse(json_content.c_str());

            if (document.HasParseError()) {
                spdlog::error("JSON parse error in file {}: {} (offset {})", 
                             filename, (int)document.GetParseError(), document.GetErrorOffset());
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            spdlog::error("Error loading JSON from file {}: {}", filename, e.what());
            return false;
        }
    }

} // namespace am

#endif //REASONABLEVULKAN_JSONHELPERS_HPP
