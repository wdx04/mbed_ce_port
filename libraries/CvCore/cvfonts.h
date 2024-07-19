#pragma once

#include <mbed.h>
#include <cmath>
#include <vector>
#include <string_view>
#include <stdint.h>
#include "cvcore.h"

// Text Rendering using Nextion/TJC Fonts

extern "C" const uint8_t _default_ascii_font[];
extern "C" const uint8_t _default_gb2312_font[];

namespace cv
{
    #pragma pack(push, 1)
    typedef struct _font_header_t
    {
        uint8_t signature;
        uint8_t low_byte_skip;
        uint8_t char_skip;
        uint8_t orientation;
        uint8_t encoding;
        uint8_t multi_byte_mode;
        uint8_t char_width;
        uint8_t char_height;
        uint8_t first_byte_start;
        uint8_t first_byte_end;
        uint8_t second_byte_start;
        uint8_t second_byte_end;
        uint32_t number_of_chars;
        uint8_t version;
        uint8_t length_of_description;
        uint16_t reserved1;
        uint32_t data_total_length;
        uint32_t data_address;
        uint8_t high_byte_to_skip;
        uint8_t number_of_chars_to_skip;
        uint8_t anti_alias;
        uint8_t variable_width_flag;
        uint8_t length_of_font_name;
        uint8_t alignment_flag;
        uint16_t reserved2;
        uint32_t actual_number_of_chars;
        uint32_t reserved3;
    } font_header_t;

    typedef struct _font_char_entry_t
    {
        uint16_t char_code;
        uint8_t char_width;
        uint8_t kerning_left;
        uint8_t kerning_right;
        uint8_t char_data_addr_info[3];
        uint16_t char_data_len;
    } font_char_entry_t;

    typedef struct _char_data_info_t
    {
        uint16_t char_code;
        uint32_t address;
        uint16_t length;
        uint8_t width;
        uint8_t height;
        bool cached;
    } char_data_info_t;

    typedef struct _get_text_bitmap_result_t
    {
        Size text_size;
        size_t consumed_chars;
    } get_text_bitmap_result_t;
    #pragma pack(pop)

    inline bool operator==(const char_data_info_t& c1, const char_data_info_t& c2)
    {
        return c1.char_code == c2.char_code;
    }

    inline bool operator<(const char_data_info_t& c1, const char_data_info_t& c2)
    {
        return c1.char_code < c2.char_code;
    }

    class FontBase
    {
    public:
    #if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
        // Construct GB2312Font from file system
        FontBase(FileSystem *fs, const char *path);
    #endif

        // Construct GB2312Font from memory
        FontBase(const uint8_t *_font_data);

        // Get the bitmap of a given character and store the bitmap into the given Mat object
        Size get_char_bitmap(uint16_t char_code, Mat result, uint16_t text_color, uint16_t bg_color);

        // Get the bitmap of a given character
        // Actual bitmap data are stored in the given buffer
        // Size of the buffer can be adjusted automatically
        // 'type' param can be either MONO8 or RGB565
        Mat get_char_bitmap(uint16_t char_code, uint16_t text_color, uint16_t bg_color, int type, std::vector<uint8_t>& buffer);

        // Get the bitmap of a given text string and store the bitmap into the given Mat object
        get_text_bitmap_result_t get_text_bitmap(std::string_view text, Mat result, uint16_t text_color, uint16_t bg_color, uint16_t wrap_width = 0);

        // Get the bitmap of a given text string
        // Actual bitmap data are stored in the given buffer
        // Size of the buffer can be adjusted automatically
        // 'type' param can be either MONO8 or RGB565
        Mat get_text_bitmap(std::string_view text, int type, uint16_t text_color, uint16_t bg_color, std::vector<uint8_t>& buffer, uint16_t wrap_width = 0);

        // Get required bitmap size of a given text string
        Size get_text_size(std::string_view text, uint16_t wrap_width = 0);

        // Cache commonly used character data in memory
        void cache_chars(std::string_view text);

    protected:
        // get next character from text
        virtual uint16_t get_next_character(const std::string_view& text, size_t& index) const;

        // get character information
        virtual char_data_info_t get_char_data_address(uint16_t char_code) = 0;

        // get the width of character code(1 or 2 bytes)
        virtual uint8_t get_character_code_width(uint16_t character) const;

        void get_text_chars_info(const std::string_view& text, std::vector<char_data_info_t>& chars_info);

        Size get_text_size(const std::vector<char_data_info_t>& addrs, uint16_t wrap_width = 0);

        void decode_char(char_data_info_t char_addr, Mat result, uint16_t text_color);

    protected:
        uint32_t data_address = 0;
        uint32_t font_map_address = 0;
        uint8_t font_height = 0;
    #if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
        File font_file;
    #endif
        const uint8_t *font_data = nullptr;
        std::vector<char_data_info_t> cached_chars;
        std::vector<uint8_t> cached_char_data;
        std::vector<char_data_info_t> working_chars;
        std::vector<uint8_t> working_char_data;
    };

    // ASCII font
    // Code Point 0x20~0x7F
    class ASCIIFont : public FontBase
    {
    public:
        using FontBase::FontBase;
        virtual char_data_info_t get_char_data_address(uint16_t char_code) override;
    };

    // Simplified Chinese font in GB2312 charset
    // Code Point 0x20~0x7F
    // Code Point 0xA1A1~0xFEF7
    class GB2312Font : public FontBase
    {
    public:
        using FontBase::FontBase;
        virtual char_data_info_t get_char_data_address(uint16_t char_code) override;
    };

    // Unicode font in UTF-16 LE charset
    // Code Point 0x0000~0xD7AF
    class UnicodeFont: public FontBase
    {
    public:
        using FontBase::FontBase;
        virtual uint16_t get_next_character(const std::string_view& text, size_t& index) const override;
        virtual char_data_info_t get_char_data_address(uint16_t char_code) override;
        virtual uint8_t get_character_code_width(uint16_t character) const override;

        Size get_text_size(std::wstring_view text, uint16_t wrap_width = 0);
    };
}
