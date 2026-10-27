#ifndef IFF_HPP
#define IFF_HPP

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>

class IFF_File;

struct IFF_Chunk {
    std::string type = "errr";
    Sint32 size = 0;
    Sint32 offset = 0;
    Sint32 file_pos = 0;
    IFF_Chunk* parent;
    IFF_File* file;

    explicit IFF_Chunk(std::string type) : type(std::move(type)) {};
    virtual ~IFF_Chunk() = default;

    virtual Sint32 set_size(const Sint32& size);
    void set_parent(IFF_Chunk* parent);
    virtual const std::vector<std::byte> get_bytes() const;
};

struct IFF_RawChunk final : IFF_Chunk {
    std::vector<std::byte> data;

    explicit IFF_RawChunk(std::string type): IFF_Chunk(std::move(type)) {};

    const std::vector<std::byte> get_bytes() const override;
    Sint32 set_size(const Sint32& size) override;

    std::string load_nstr(Sint32 offset=-1);
    Sint32 load_sint32(Sint32 offset=-1);
    Sint16 load_sint16(Sint32 offset=-1);
    Sint8 load_sint8(Sint32 offset=-1);
    Uint32 load_uint32(Sint32 offset=-1);
    Uint16 load_uint16(Sint32 offset=-1);
    Uint8 load_uint8(Sint32 offset=-1);

    bool save_nstr(const std::string& string, Sint32 offset=-1);
    bool save_sint32(const Sint32& integer, Sint32 offset=-1);
    bool save_sint16(const Sint16& integer, Sint32 offset=-1);
    bool save_sint8(const Sint8& integer, Sint32 offset=-1);
    bool save_uint32(const Uint32& integer, Sint32 offset=-1);
    bool save_uint16(const Uint16& integer, Sint32 offset=-1);
    bool save_uint8(const Uint8& integer, Sint32 offset=-1);
};

struct IFF_FormChunk final : IFF_Chunk {
    std::string form_type;
    std::vector<std::unique_ptr<IFF_Chunk>> contents;

    explicit IFF_FormChunk(std::string form_type): IFF_Chunk("FORM") {
        this->form_type = std::move(form_type);
        if (this->form_type.length() != 4)
            this->form_type = "errr";
        size = 4;
    };

    const std::vector<std::byte> get_bytes() const override;
    void add_child(std::unique_ptr<IFF_Chunk> chunk);
    std::unique_ptr<IFF_Chunk> get_child(Sint32 offset=-1);
};

class IFF_File {
    private:
        SDL_IOStream* stream;
        std::filesystem::path path;
        Sint32 size;
    public:
        IFF_File(const std::filesystem::path& path);
        ~IFF_File();

        void move(Sint32 offset);
        void skip(Sint32 offset);
        Sint32 get_offset();
        Sint32 get_size();

        std::string get_chunk_type();
        Sint32 get_chunk_size();
        std::vector<std::byte> get_chunk_data(Sint32 size);
        std::unique_ptr<IFF_Chunk> get_chunk(Sint8 depth=8);
        std::unique_ptr<IFF_Chunk> find_chunk(const char* type, Sint8 depth=8);
        std::unique_ptr<IFF_FormChunk> find_form_chunk(const char* type, Sint8 depth=8);
        bool write_chunk(std::unique_ptr<IFF_Chunk> chunk, Sint32 offset=-1);

        Sint32 read_sint32();
        bool write_char4(const std::string& string);
        bool write_nstr(const std::string& string);
        bool write_sint32(const Sint32& integer);
        bool write_bytes(const std::vector<std::byte>& bytes);
};

#endif