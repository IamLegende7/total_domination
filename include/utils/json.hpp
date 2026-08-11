#ifndef JSON_HPP
#define JSON_HPP

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/filereadstream.h"

// Printing out
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

#include "utils/logger.hpp"
#include "settings/debug.hpp"

inline std::string open_file(const std::filesystem::path& filename) {
    const std::string filename_str = filename.u8string();
    size_t size = 0;
    char *file = (char *)SDL_LoadFile(filename_str.c_str(), &size);

    if (!file) {
        LOG(LogLevel::Warning, "Could not open file: %s", filename_str.c_str());
        return "{}";
    }

    return std::string(file);
}

inline rapidjson::Document open_json(const std::filesystem::path& filename) {
    rapidjson::Document document;
    const std::string filename_str = filename.u8string();
    try {
        std::string json_contents = open_file(filename);

        // Parse
        document.Parse<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(json_contents.c_str(), strlen(json_contents.c_str()));

        // Check errors
        if (document.HasParseError()) {
            int error_offset = document.GetErrorOffset();
            const char* error_message = rapidjson::GetParseError_En(document.GetParseError());
            LOG(LogLevel::Warning, "Error parsing JSON %s: %s (offset: %d)", filename_str.c_str(), error_message, error_offset);
            document = rapidjson::Document();
            document.SetObject();
        }

    } catch (const std::exception& e) {
        LOG(LogLevel::Warning, "Could not load json %s: %s", filename_str.c_str(), e.what());
        document = rapidjson::Document();
        document.SetObject();
    }

    if (DEBUG["all_debug_logs"].get<bool>()) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        if (DEBUG["all_debug_logs"].get<bool>()) LOG(LogLevel::Debug, "Loaded Json %s:", filename_str.c_str());
        std::cout << buffer.GetString() << std::endl;
    }

    return document;
}

#endif