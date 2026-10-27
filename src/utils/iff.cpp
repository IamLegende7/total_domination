#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <cstddef>
#include <filesystem>
#include <string>
#include <cstring>
#include <vector>
#include <memory>

#include "utils/logger.hpp"
#include "utils/iff.hpp"

#include "settings/locations.hpp"

Sint32 IFF_Chunk::set_size(const Sint32& size) {
    this->size = size;
    return this->size;
};

void IFF_Chunk::set_parent(IFF_Chunk* parent) {
    this->parent = parent;
};

const std::vector<std::byte> IFF_Chunk::get_bytes() const {
    static std::vector<std::byte> empty;
    return empty;
};





const std::vector<std::byte> IFF_RawChunk::get_bytes() const {
    return data;
};

Sint32 IFF_RawChunk::set_size(const Sint32& size) {
    this->size = size;
    data.resize(static_cast<std::size_t>(size));
    return this->size;
};



std::string IFF_RawChunk::load_nstr(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    std::string result = "";
    for (Sint32 i = offset; i < size; i++) {
        this->offset++;
        if (std::to_integer<char>(data[i]) == '\0') {
            LOG(LogLevel::Debug, "Loaded nstr at %d: \"%s\"", offset, result.c_str());
            return result;
        } else
            result.push_back(std::to_integer<char>(data[i]));
    }

    LOG(LogLevel::Warning, "Could not load null terminated string: reached max size before null. Returning \"%s\"", result.c_str());
    return std::string(result);
};

Sint32 IFF_RawChunk::load_sint32(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    if (offset+3 >= size) {
        LOG(LogLevel::Warning, "Could not load Sin32: offset+3 >= size");
        return -1;
    }

    this->offset += 4;
    return (
        (Sint32(data[offset])   << 24) |
        (Sint32(data[offset+1]) << 16) |
        (Sint32(data[offset+2]) << 8)  |
        (Sint32(data[offset+3]))
    );
};

Sint16 IFF_RawChunk::load_sint16(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    if (offset+1 >= size) {
        LOG(LogLevel::Warning, "Could not load Sin16: offset+1 >= size");
        return -1;
    }

    this->offset += 2;
    return (
        (Sint16(data[offset]) << 8)  |
        (Sint16(data[offset+1]))
    );
};

Sint8 IFF_RawChunk::load_sint8(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    if (offset >= size) {
        LOG(LogLevel::Warning, "Could not load Sin8: offset >= size");
        return -1;
    }

    this->offset += 1;
    return (Sint8(data[offset]));
};

Uint32 IFF_RawChunk::load_uint32(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    if (offset+3 >= size) {
        LOG(LogLevel::Warning, "Could not load Sin32: offset+3 >= size");
        return -1;
    }

    this->offset += 4;
    return (
        (Uint32(data[offset])   << 24) |
        (Uint32(data[offset+1]) << 16) |
        (Uint32(data[offset+2]) << 8)  |
        (Uint32(data[offset+3]))
    );
};

Uint16 IFF_RawChunk::load_uint16(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    if (offset+1 >= size) {
        LOG(LogLevel::Warning, "Could not load Sin16: offset+1 >= size");
        return -1;
    }

    this->offset += 2;
    return (
        (Uint16(data[offset]) << 8)  |
        (Uint16(data[offset+1]))
    );
};

Uint8 IFF_RawChunk::load_uint8(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;

    if (offset >= size) {
        LOG(LogLevel::Warning, "Could not load Sin8: offset >= size");
        return -1;
    }

    this->offset += 1;
    return (Sint8(data[offset]));
};



bool IFF_RawChunk::save_nstr(const std::string& string, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    const Sint32 required = offset+static_cast<Sint32>(string.length())+1;
    if (required > size)
        set_size(required);
    for (Sint32 i = offset; i < size; ++i) {
        if (i >= required-1) {
            data[static_cast<std::size_t>(i)] = std::byte(0);
            break;
        }
        data[static_cast<std::size_t>(i)] = static_cast<std::byte>(static_cast<unsigned char>(string[static_cast<std::size_t>(i-offset)]));
    }
    this->offset += string.length()+1;
    return true;
};

bool IFF_RawChunk::save_sint32(const Sint32& integer, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset+4 > size)
        set_size(offset+4);
    data[offset] = std::byte(integer >> 24);
    data[offset+1] = std::byte(integer >> 16);
    data[offset+2] = std::byte(integer >> 8);
    data[offset+3] = std::byte(integer);
    this->offset += 4;
    return true;
};

bool IFF_RawChunk::save_sint16(const Sint16& integer, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset+2 > size)
        set_size(offset+2);
    data[offset] = std::byte(integer >> 8);
    data[offset+1] = std::byte(integer);
    this->offset += 2;
    return true;
};

bool IFF_RawChunk::save_sint8(const Sint8& integer, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset+1 > size)
        set_size(offset+1);
    data[offset] = std::byte(integer);
    this->offset += 1;
    return true;
};

bool IFF_RawChunk::save_uint32(const Uint32& integer, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset+4 > size)
        set_size(offset+4);
    data[offset] = std::byte(integer >> 24);
    data[offset+1] = std::byte(integer >> 16);
    data[offset+2] = std::byte(integer >> 8);
    data[offset+3] = std::byte(integer);
    this->offset += 4;
    return true;
};

bool IFF_RawChunk::save_uint16(const Uint16& integer, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset+2 > size)
        set_size(offset+2);
    data[offset] = std::byte(integer >> 8);
    data[offset+1] = std::byte(integer);
    this->offset += 2;
    return true;
};

bool IFF_RawChunk::save_uint8(const Uint8& integer, Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset+1 > size)
        set_size(offset+1);
    data[offset] = std::byte(integer);
    this->offset += 1;
    return true;
};





const std::vector<std::byte> IFF_FormChunk::get_bytes() const {
    std::vector<std::byte> bytes;
    bytes.reserve(4);
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(form_type[0])));
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(form_type[1])));
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(form_type[2])));
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(form_type[3])));
    for (const auto& chunk : contents) {
        const std::vector<std::byte>& child_bytes = chunk->get_bytes();
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->type[0])));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->type[1])));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->type[2])));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->type[3])));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->size >> 24)));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->size >> 16)));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->size >> 8)));
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(chunk->size)));
        bytes.insert(bytes.end(), child_bytes.begin(), child_bytes.end());
    }
    return bytes;
};

void IFF_FormChunk::add_child(std::unique_ptr<IFF_Chunk> chunk) {
    if (!chunk)
        return;
    chunk->set_parent(this);
    this->size += chunk->size+8;
    this->offset += chunk->size+8;
    contents.push_back(std::move(chunk));
};

std::unique_ptr<IFF_Chunk> IFF_FormChunk::get_child(Sint32 offset) {
    if (offset < 0)
        offset = this->offset;
    if (offset >= size) {
        LOG(LogLevel::Warning, "Could not load child chunk:offset (%d) >= size (%d).", offset, size);
        return nullptr;
    }
    file->move(file_pos+12+offset);
    std::unique_ptr<IFF_Chunk> chunk = file->get_chunk();
    if (!chunk) {
        LOG(LogLevel::Warning, "Could not load child chunk at offset %d: could not get chunk.", file_pos+12+offset);
        return nullptr;
    }
    chunk->set_parent(this);
    this->offset += chunk->size+8;
    //LOG(LogLevel::Debug, "Found chunk of type \"%s\" at %d, size %d.", chunk->type.c_str(), chunk->file_pos, chunk->size);
    return chunk;
};





IFF_File::IFF_File(const std::filesystem::path& path) {
    this->path = replace_locations(path);
    const std::string path_str = this->path.u8string();
    bool new_file = false;
    if (!std::filesystem::exists(this->path)) {
        new_file = true;
        LOG(LogLevel::Warning, "IFF file \"%s\" does not exists. Creating it..", path_str.c_str());
        stream = SDL_IOFromFile(path_str.c_str(), "w+xb");
    } else {
        stream = SDL_IOFromFile(path_str.c_str(), "r+b");
    }
    if (!stream) {
        LOG(LogLevel::Error, "Could not load IIF File \"%s\": Could not read file: %s", path_str.c_str(), SDL_GetError());
        return;
    }

    move(0);
    if (new_file) {
        write_char4("FORM");
        write_sint32(4);
        write_char4("TDMP");
        move(0);
    }

    char root_type[4];
    if (SDL_ReadIO(stream, root_type, sizeof root_type) != sizeof root_type) {
        LOG(LogLevel::Error, "Could not load IIF File \"%s\": Could not load root chunk type: %s", path_str.c_str(), SDL_GetError());
        return;
    }
    if (std::memcmp(root_type, "FORM", sizeof root_type) != 0) {
        LOG(LogLevel::Error, "Could not load IIF File \"%s\": Root chunk type is not \"FORM\". Is: \"%s\"", path_str.c_str(), std::string(root_type, sizeof root_type).c_str());
        return;
    }

    if (!SDL_ReadS32BE(stream, &size)) {
        if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
            LOG(LogLevel::Error, "Could not load IIF File \"%s\": could not load chunk size: EOF", path_str.c_str());
        } else {
            LOG(LogLevel::Error, "Could not load IIF File \"%s\": could not load chunk size: %s", path_str.c_str(), SDL_GetError());
        }
        return;
    }

    char type[4];
    if (SDL_ReadIO(stream, type, sizeof type) != sizeof type) {
        LOG(LogLevel::Error, "Could not load IIF File \"%s\": Could not load type: %s", path_str.c_str(), SDL_GetError());
        return;
    }
    if (std::memcmp(type, "TDMP", sizeof type) != 0) {
        LOG(LogLevel::Error, "Could not load IIF File \"%s\": Type is not \"TDMP\". Is: \"%s\"", path_str.c_str(), std::string(type, sizeof type).c_str());
        return;
    }
};

IFF_File::~IFF_File() {
    if (!SDL_CloseIO(stream)) {
        LOG(LogLevel::Warning, "Could not close File IO stream: %s", SDL_GetError());
    }
};



void IFF_File::move(Sint32 offset) {
    SDL_SeekIO(stream, offset, SDL_IO_SEEK_SET);
};

void IFF_File::skip(Sint32 offset) {
    SDL_SeekIO(stream, offset, SDL_IO_SEEK_CUR);
};

Sint32 IFF_File::get_offset() {
    return Sint32(SDL_TellIO(stream));
};

Sint32 IFF_File::get_size() {
    return size;
};


std::string IFF_File::get_chunk_type() {
    char bytes[4];
    if (SDL_ReadIO(stream, bytes, sizeof bytes) != sizeof bytes) {
        Sint32 offset = SDL_TellIO(stream);
        LOG(LogLevel::Error, "Could not load IIF Chunk at offset %d: Could not load type: %s", offset, SDL_GetError());
        return {};
    }
    return std::string(bytes, sizeof bytes);
};

Sint32 IFF_File::get_chunk_size() {
    return read_sint32();
};

std::vector<std::byte> IFF_File::get_chunk_data(Sint32 size) {
    if (size < 0) {
        Sint32 offset = SDL_TellIO(stream);
        LOG(LogLevel::Error, "Could not load IIF Chunk at offset %d: Could not load data: size is negative", offset);
        return std::vector<std::byte>(0);
    }

    std::vector<std::byte> data(static_cast<std::size_t>(size));
    if (data.empty()) {
        return data;
    }

    const std::size_t bytes_read = SDL_ReadIO(stream, data.data(), data.size());
    if (bytes_read != data.size()) {
        LOG(LogLevel::Error, "Could not read IFF chunk: requested %d bytes, got %zu: %s", size, bytes_read, SDL_GetError());
        return std::vector<std::byte>(0);
    }

    return data;
};

std::unique_ptr<IFF_Chunk> IFF_File::get_chunk(Sint8 depth) {
    if (depth < 0)
        return nullptr;
    const Sint32 chunk_start = SDL_TellIO(stream);
    const std::string type = get_chunk_type();
    if (type == "FORM") {
        auto chunk = std::make_unique<IFF_FormChunk>("none");
        chunk->size = get_chunk_size();
        chunk->file_pos = chunk_start;
        chunk->form_type = get_chunk_type();
        chunk->file = this;
        return chunk;
    } else {
        auto chunk = std::make_unique<IFF_RawChunk>(type);
        chunk->size = get_chunk_size();
        chunk->file_pos = chunk_start;
        chunk->data = get_chunk_data(chunk->size);
        chunk->file = this;
        return chunk;
    }
    return nullptr;
};

std::unique_ptr<IFF_Chunk> IFF_File::find_chunk(const char* wanted_type, Sint8 depth) {
    if (depth < 0)
        return nullptr;
    Sint32 offset = SDL_TellIO(stream);
    char type[4];
    Sint32 size;
    while (true) {
        const Sint32 chunk_start = SDL_TellIO(stream);
        if (SDL_ReadIO(stream, type, sizeof type) != sizeof type) {
            if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
                LOG(LogLevel::Error, "Could not find chunk of type \"%s\": could not load chunk type at %d: EOF", wanted_type, offset);
            } else {
                LOG(LogLevel::Error, "Could not find chunk of type \"%s\": could not load chunk type at %d: %s", wanted_type, offset, SDL_GetError());
            }
            return nullptr;
        }
        offset += 4;
        if (!SDL_ReadS32BE(stream, &size)) {
            if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
                LOG(LogLevel::Error, "Could not find chunk of type \"%s\": could not load chunk size at %d: EOF", wanted_type, offset);
            } else {
                LOG(LogLevel::Error, "Could not find chunk of type \"%s\": could not load chunk size at %d: %s", wanted_type, offset, SDL_GetError());
            }
            return nullptr;
        }
        offset += 4;
        LOG(LogLevel::Debug, "Found chunk of type \"%s\"; size %d at %d", std::string(type, sizeof type).c_str(), size, offset-8);
        if (std::memcmp(type, wanted_type, sizeof type) == 0) {
            auto chunk = std::make_unique<IFF_RawChunk>(std::string(type, sizeof type));
            chunk->size = size;
            chunk->offset = 0;
            chunk->file_pos = chunk_start;
            chunk->data = get_chunk_data(chunk->size);
            chunk->file = this;
            return chunk;
        } else {
            skip(size);
            offset += size;
        }
    }
    return nullptr;
};

std::unique_ptr<IFF_FormChunk> IFF_File::find_form_chunk(const char* wanted_type, Sint8 depth) {
    if (depth < 0)
        return nullptr;
    Sint32 offset = SDL_TellIO(stream);
    char type[4];
    char form_type[4];
    Sint32 size;
    while (true) {
        const Sint32 chunk_start = SDL_TellIO(stream);
        if (SDL_ReadIO(stream, type, sizeof type) != sizeof type) {
            if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
                LOG(LogLevel::Error, "Could not find form chunk of type \"%s\": could not load chunk type at %d: EOF", wanted_type, offset);
            } else {
                LOG(LogLevel::Error, "Could not find form chunk of type \"%s\": could not load chunk type at %d: %s", wanted_type, offset, SDL_GetError());
            }
            return nullptr;
        }
        offset += 4;
        if (!SDL_ReadS32BE(stream, &size)) {
            if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
                LOG(LogLevel::Error, "Could not find form chunk of type \"%s\": could not load chunk size at %d: EOF", wanted_type, offset);
            } else {
                LOG(LogLevel::Error, "Could not find form chunk of type \"%s\": could not load chunk size at %d: %s", wanted_type, offset, SDL_GetError());
            }
            return nullptr;
        }
        offset += 4;
        LOG(LogLevel::Debug, "Found chunk of type \"%s\"; size %d at %d", std::string(type, sizeof type).c_str(), size, offset-8);
        if (std::memcmp(type, "FORM", sizeof type) == 0) {
            if (SDL_ReadIO(stream, form_type, sizeof form_type) != sizeof form_type) {
                if (SDL_GetIOStatus(stream) == SDL_IO_STATUS_EOF) {
                    LOG(LogLevel::Error, "Could not find form chunk of type \"%s\": could not load chunk type at %d: EOF", wanted_type, offset);
                } else {
                    LOG(LogLevel::Error, "Could not find form chunk of type \"%s\": could not load chunk type at %d: %s", wanted_type, offset, SDL_GetError());
                }
                return nullptr;
            }
            if (std::memcmp(form_type, wanted_type, sizeof form_type) == 0) {
                auto chunk = std::make_unique<IFF_FormChunk>(std::string(form_type, sizeof form_type));
                chunk->size = size;
                chunk->offset = 0;
                chunk->file_pos = chunk_start;
                chunk->file = this;
                return chunk;
            } else {
                skip(size-4);
                offset += size;
            }
        } else {
            skip(size);
            offset += size;
        }
    }
    return nullptr;
};

bool IFF_File::write_chunk(std::unique_ptr<IFF_Chunk> chunk, Sint32 offset) {
    if (offset < 0)
        offset = get_offset();
    if (offset+chunk->size > size)
        size = offset+chunk->size;
    move(offset);
    if (!write_char4(chunk->type)) {
        LOG(LogLevel::Warning, "Could not write chunk at offset %d+%d: failed to write chunk type", offset, chunk->size);
        return false;
    }
    if (!write_sint32(chunk->size)) {
        LOG(LogLevel::Warning, "Could not write chunk at offset %d+%d: failed to write chunk size", offset, chunk->size);
        return false;
    }
    if (!write_bytes(chunk->get_bytes())) {
        LOG(LogLevel::Warning, "Could not write chunk at offset %d+%d: failed to write chunk data", offset, chunk->size);
        return false;
    }
    return true;
};



Sint32 IFF_File::read_sint32() {
    Sint32 value = -1;
    if (!SDL_ReadS32BE(stream, &value)) {
        LOG(LogLevel::Warning, "Could not read Sint32: %s", SDL_GetError());
        return 0;
    }
    return value;
};

bool IFF_File::write_char4(const std::string& string) {
    char bytes[4];
    if (string.size() != sizeof bytes) {
        LOG(LogLevel::Warning, "Could not write char[4]: string \"%s\" is too long (%d)", string.c_str(), string.size());
        return false;
    } else {
        std::memcpy(bytes, string.data(), sizeof bytes);
    }
    if (SDL_WriteIO(stream, bytes, sizeof bytes) != sizeof bytes) {
        LOG(LogLevel::Warning, "Could not write char[4]: %s", SDL_GetError());
        return false;
    }
    return true;
};

bool IFF_File::write_nstr(const std::string& string) {
    const char* bytes = std::string(string+"\0").c_str();
    const size_t length = string.length();
    if (SDL_WriteIO(stream, bytes, length) != length) {
        LOG(LogLevel::Warning, "Could not write null terminated string: %s", SDL_GetError());
        return false;
    }
    return true;
};

bool IFF_File::write_sint32(const Sint32& integer) {
    return SDL_WriteS32BE(stream, integer);
};

bool IFF_File::write_bytes(const std::vector<std::byte>& bytes) {
    const std::size_t written = SDL_WriteIO(stream, bytes.data(), bytes.size());
    if (written != bytes.size()) {
        LOG(LogLevel::Warning, "Could not write bytes: %s", SDL_GetError());
        return false;
    }
    return true;
};