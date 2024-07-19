#include "cvfonts.h"
#include "cvspan.h"
#include <algorithm>

namespace cv
{

    inline uint8_t alpha_blending_channel(uint8_t bg, uint8_t fg, uint8_t alpha)
    {
        return (uint8_t) ((uint16_t(fg) * alpha + bg * (7 - alpha)) / 7);
    }

    static uint8_t alpha_blending(uint8_t bg, uint8_t fg, uint8_t alpha)
    {
        // RGB332
        uint8_t bg_r = uint8_t(bg >> 5);
        uint8_t bg_g = uint8_t((bg >> 2) & 0x07);
        uint8_t bg_b = uint8_t(bg & 0x03);
        uint8_t fg_r = uint8_t(fg >> 5);
        uint8_t fg_g = uint8_t((fg >> 2) & 0x07);
        uint8_t fg_b = uint8_t(fg & 0x03);
        uint8_t result_r = alpha_blending_channel(bg_r, fg_r, alpha);
        uint8_t result_g = alpha_blending_channel(bg_g, fg_g, alpha);
        uint8_t result_b = alpha_blending_channel(bg_b, fg_b, alpha);
        uint8_t result = (result_r << 5) + (result_g << 2) + result_b;
        return result;
    }

    static uint16_t alpha_blending(uint16_t bg, uint16_t fg, uint8_t alpha)
    {
        // RGB565
        uint8_t bg_r = uint8_t(bg >> 11);
        uint8_t bg_g = uint8_t((bg >> 5) & 0x3F);
        uint8_t bg_b = uint8_t(bg & 0x1F);
        uint8_t fg_r = uint8_t(fg >> 11);
        uint8_t fg_g = uint8_t((fg >> 5) & 0x3F);
        uint8_t fg_b = uint8_t(fg & 0x1F);
        uint8_t result_r = alpha_blending_channel(bg_r, fg_r, alpha);
        uint8_t result_g = alpha_blending_channel(bg_g, fg_g, alpha);
        uint8_t result_b = alpha_blending_channel(bg_b, fg_b, alpha);
        uint16_t result = (uint16_t(result_r) << 11) + (uint16_t(result_g) << 5) + result_b;
        return result;
    }

    template<typename value_type>
    static void decode_char_direct(span<const uint8_t> char_data, uint8_t width, uint8_t height, cv::Mat result, value_type text_color)
    {
        int x = 0, y = 0;
        value_type* p_row = result.ptr<value_type>(y);
        if (char_data[0] == 1)
        {
            for (size_t index = 1; index < char_data.size(); index++)
            {
                uint8_t char_byte = char_data[index];
                uint8_t op_type = char_byte >> 5;
                uint8_t op_param = char_byte & 0x1F;
                switch (op_type)
                {
                case 0:
                    // op_param个背景像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    break;
                case 1:
                    // op_param个前景像素
                    for (uint8_t px_index = 0; px_index < op_param; px_index++)
                    {
                        p_row[x] = text_color;
                        if (++x >= width) {
                            p_row = result.ptr<value_type>(++y);
                            x = 0;
                        }
                    }
                    break;
                case 2:
                    // op_param个背景像素 + 1个前景像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                case 3:
                    // op_param个背景像素 + 2个前景像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                case 4:
                    // op_param个背景像素 + 3个前景像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                case 5:
                    // op_param个背景像素 + 4个前景像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                case 6:
                case 7:
                {
                    // op_param1个背景像素后跟op_param2个前景像素
                    uint8_t op_param1 = (char_byte >> 3) & 7;
                    uint8_t op_param2 = char_byte & 7;
                    x += op_param1;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    for (uint8_t px_index = 0; px_index < op_param2; px_index++)
                    {
                        p_row[x] = text_color;
                        if (++x >= width) {
                            p_row = result.ptr<value_type>(++y);
                            x = 0;
                        }
                    }
                }
                }
            }
        }
        else if (char_data[0] == 3)
        {
            for (size_t index = 1; index < char_data.size(); index++)
            {
                uint8_t char_byte = char_data[index];
                uint8_t op_type = char_byte >> 5;
                uint8_t op_param = char_byte & 0x1F;
                switch (op_type)
                {
                case 0:
                    // op_param个透明像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    break;
                case 1:
                    // op_param个不透明像素
                    for (uint8_t px_index = 0; px_index < op_param; px_index++)
                    {
                        p_row[x] = text_color;
                        if (++x >= width) {
                            p_row = result.ptr<value_type>(++y);
                            x = 0;
                        }
                    }
                    break;
                case 2:
                    // op_param个透明像素 + 1个不透明像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                case 3:
                    // op_param个透明像素 + 2个不透明像素
                    x += op_param;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = text_color;
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                case 4:
                case 5:
                {
                    // op_param个透明像素 + 1个半像素
                    uint8_t op_param1 = (char_byte >> 3) & 7;
                    uint8_t op_param2 = char_byte & 7;
                    x += op_param1;
                    while (x >= width)
                    {
                        x -= width;
                        y++;
                    }
                    p_row = result.ptr<value_type>(y);
                    p_row[x] = alpha_blending(p_row[x], text_color, op_param2);
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    break;
                }
                case 6:
                case 7:
                {
                    // 2个半透明像素
                    uint8_t op_param1 = (char_byte >> 3) & 7;
                    uint8_t op_param2 = char_byte & 7;
                    p_row[x] = alpha_blending(p_row[x], text_color, op_param1);
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                    p_row[x] = alpha_blending(p_row[x], text_color, op_param2);
                    if (++x >= width) {
                        p_row = result.ptr<value_type>(++y);
                        x = 0;
                    }
                }
                }
            }
        }
    }

#if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
    FontBase::FontBase(FileSystem *fs, const char *path)
        : font_file(fs, path)
    {
        font_header_t font_header;
        font_file.read(reinterpret_cast<char*>(&font_header), sizeof(font_header_t));
        data_address = font_header.data_address;
        font_height = font_header.char_height;
        font_map_address = data_address + font_header.length_of_description;
    }
#endif

    FontBase::FontBase(const uint8_t *_font_data)
        : font_data(_font_data)
    {
        const font_header_t *font_header = reinterpret_cast<const font_header_t*>(_font_data);
        data_address = font_header->data_address;
        font_height = font_header->char_height;
        font_map_address = data_address + font_header->length_of_description;
    }

    cv::Size FontBase::get_char_bitmap(uint16_t char_code, cv::Mat result, uint16_t text_color, uint16_t bg_color)
    {
        char_data_info_t addr = get_char_data_address(char_code);
        if(text_color != bg_color)
        {
            result(cv::Rect(0, 0, addr.width, addr.height)) = bg_color;
        }
        decode_char(addr, result, text_color);
        return cv::Size(addr.width, addr.height);
    }

    cv::Mat FontBase::get_char_bitmap(uint16_t char_code, uint16_t text_color, uint16_t bg_color, int type, std::vector<uint8_t>& buffer)
    {
        char_data_info_t addr = get_char_data_address(char_code);
        switch (type)
        {
        case cv::MONO8:
            buffer.resize(addr.height * addr.width);
            break;
        case cv::RGB565:
            buffer.resize(addr.height * addr.width * 2);
            break;
        }
        cv::Mat result(addr.height, addr.width, type, &buffer[0]);
        if(text_color != bg_color)
        {
            result = bg_color;
        }
        decode_char(addr, result, text_color);
        return result;
    }

    get_text_bitmap_result_t FontBase::get_text_bitmap(std::string_view text, cv::Mat result, uint16_t text_color, uint16_t bg_color, uint16_t wrap_width)
    {
        get_text_chars_info(text, working_chars);
        uint8_t max_char_height = 0;
        for(const auto& ch: working_chars)
        {
            max_char_height = std::max(max_char_height, ch.height);
        }
        cv::Size text_size = get_text_size(working_chars, wrap_width);
        cv::Rect text_rc(0, 0, text_size.width, text_size.height);
        if(text_rc.height > result.rows) text_rc.height = result.rows;
        if(text_color != bg_color)
        {
            result(text_rc) = bg_color;
        }
        int x = 0, y = 0;
        size_t decoded_chars = 0;
        for (const auto& addr : working_chars)
        {
            if(addr.char_code == '\r')
            {
                // ignore \r
                decoded_chars += get_character_code_width(addr.char_code);
                continue;
            }
            else if(addr.char_code == '\n')
            {
                y += max_char_height;
                x = 0;
                decoded_chars += get_character_code_width(addr.char_code);
                continue;
            }
            else if (wrap_width != 0 && x + addr.width > wrap_width)
            {
                y += max_char_height;
                x = 0;
            }
            if(x + addr.width > result.cols || y + addr.height > result.rows)
            {
                break;
            }
            decode_char(addr, result(cv::Rect(x, y, addr.width, addr.height)), text_color);
            decoded_chars += get_character_code_width(addr.char_code);
            x += addr.width;
        }
        if(decoded_chars > text.length())
        {
            decoded_chars = text.length();
        }
        return get_text_bitmap_result_t{ text_size, decoded_chars };
    }

    cv::Mat FontBase::get_text_bitmap(std::string_view text, int type, uint16_t text_color, uint16_t bg_color, std::vector<uint8_t>& buffer, uint16_t wrap_width)
    {
        get_text_chars_info(text, working_chars);
        uint8_t max_char_height = 0;
        for(const auto& ch: working_chars)
        {
            max_char_height = std::max(max_char_height, ch.height);
        }
        cv::Size text_size = get_text_size(working_chars, wrap_width);
        switch (type)
        {
        case cv::MONO8:
            buffer.assign(text_size.area(), uint8_t(bg_color));
            break;
        case cv::RGB565:
            buffer.resize(text_size.area() * 2);
            std::fill_n(reinterpret_cast<uint16_t*>(&buffer[0]), text_size.area(), bg_color);
            break;
        }
        cv::Mat result(text_size.height, text_size.width, type, &buffer[0]);
        if(text_color != bg_color)
        {
            result = bg_color;
        }
        int x = 0, y = 0;
        for (const auto& addr : working_chars)
        {
            if (wrap_width != 0 && x + addr.width > wrap_width)
            {
                y += max_char_height;
                x = 0;
            }
            decode_char(addr, result(cv::Rect(x, y, addr.width, addr.height)), text_color);
            x += addr.width;
        }
        return result;
    }

    cv::Size FontBase::get_text_size(std::string_view text, uint16_t wrap_width)
    {
        get_text_chars_info(text, working_chars);
        return get_text_size(working_chars, wrap_width);
    }

    cv::Size FontBase::get_text_size(const std::vector<char_data_info_t>& addrs, uint16_t wrap_width)
    {
        int row_height = 0;
        int row_width = 0;
        int row_count = 1;
        for (const auto& addr : addrs)
        {
            if (addr.width > 0)
            {
                if (row_height == 0)
                {
                    row_height = addr.height;
                }
                if(addr.char_code == '\r')
                {
                    continue;
                }
                if(addr.char_code == '\n')
                {
                    row_count++;
                    continue;
                }
                row_width += addr.width;
                if (wrap_width != 0 && wrap_width > addr.width && row_width > wrap_width)
                {
                    row_width = addr.width;
                    row_count++;
                }
            }
        }
        if (row_width == 0)
        {
            return cv::Size();
        }
        else if (row_count == 1)
        {
            return cv::Size(row_width, row_height);
        }
        else
        {
            return cv::Size(wrap_width, row_height * row_count);
        }
    }

    void FontBase::decode_char(char_data_info_t char_addr, cv::Mat result, uint16_t text_color)
    {
        span<const uint8_t> char_data;
        if(char_addr.cached)
        {
            char_data = span<const uint8_t>(&cached_char_data[char_addr.address], char_addr.length);
        }
        else
        {
            if(font_data != nullptr)
            {
                char_data = span<const uint8_t>(font_data + char_addr.address, char_addr.length);
            }
#if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
            else
            {
                font_file.seek(char_addr.address);
                font_file.read(reinterpret_cast<char*>(&working_char_data[0]), char_addr.length);
                char_data = working_char_data;
            }
#endif
        }
        switch (result.type)
        {
        case cv::MONO8:
            decode_char_direct<uint8_t>(char_data, char_addr.width, char_addr.height, result, uint8_t(text_color));
            break;
        case cv::RGB565:
            decode_char_direct<uint16_t>(char_data, char_addr.width, char_addr.height, result, text_color);
            break;
        }
    }

    void FontBase::cache_chars(std::string_view text)
    {
        cached_chars.clear();
        cached_char_data.clear();
        get_text_chars_info(text, cached_chars);
        std::sort(cached_chars.begin(), cached_chars.end());
        cached_chars.erase(std::unique(cached_chars.begin(), cached_chars.end()), cached_chars.end());
        size_t total_char_data_length = 0;
        for(const auto& addr: cached_chars)
        {
            total_char_data_length += addr.length;
        }
        cached_char_data.resize(total_char_data_length);
        uint32_t current_cache_pos = 0;
        for(auto& addr: cached_chars)
        {
            if(font_data != nullptr)
            {
                const uint8_t *p_char_data = font_data + addr.address;
                std::copy_n(p_char_data, addr.length, &cached_char_data[current_cache_pos]);
            }
#if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
            else
            {
                font_file.seek(addr.address);
                font_file.read(reinterpret_cast<char*>(&cached_char_data[current_cache_pos]), addr.length);
            }
#endif
            addr.address = current_cache_pos;
            addr.cached = true;
            current_cache_pos += addr.length;
        }
    }

    uint16_t FontBase::get_next_character(const std::string_view& text, size_t& index) const
    {
        uint16_t character;
        if (int8_t(text[index]) >= 0)
        {
            character = static_cast<uint16_t>(text[index]);
            index++;
        }
        else if (index + 1 < text.size())
        {
            character = static_cast<uint16_t>(static_cast<uint8_t>(text[index])) +
                (static_cast<uint16_t>(static_cast<uint8_t>(text[index + 1])) << 8);
            index += 2;
        }
        else
        {
            character = static_cast<uint16_t>('?');
            index++;
        }
        return character;
    }

    uint8_t FontBase::get_character_code_width(uint16_t character) const
    {
        return character < 128 ? 1: 2;
    }

    void FontBase::get_text_chars_info(const std::string_view& text, std::vector<char_data_info_t>& chars_info)
    {
        size_t index = 0;
        chars_info.clear();
        while(index < text.size())
        {
            uint16_t character = get_next_character(text, index);
            chars_info.push_back(get_char_data_address(character));
        }
    }

    char_data_info_t ASCIIFont::get_char_data_address(uint16_t char_code)
    {
        char_data_info_t addr{ .char_code = char_code, .address = 0, .length = 0, .width = 0, .height = 0, .cached = false };
        auto cached_it = std::lower_bound(cached_chars.cbegin(), cached_chars.cend(), addr);
        if(cached_it != cached_chars.cend() && cached_it->char_code == char_code)
        {
            addr = *cached_it;
        }
        else
        {
            int32_t char_index = -1;
            if (char_code < 0x80)
            {
                if (char_code >= 0x20 && char_code < 0x80)
                {
                    char_index = char_code - 0x20;
                }
            }
            else
            {
                return get_char_data_address('?');
            }
            if (char_index >= 0)
            {
                font_char_entry_t entry;
                if(font_data != nullptr)
                {
                    const uint8_t *p_font_entry = font_data + font_map_address + char_index * sizeof(font_char_entry_t);
                    entry = *reinterpret_cast<const font_char_entry_t*>(p_font_entry);
                }
#if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
                else
                {
                    font_file.seek(font_map_address + char_index * sizeof(font_char_entry_t));
                    font_file.read(reinterpret_cast<char*>(&entry), sizeof(font_char_entry_t));
                }
#endif
                addr.address = entry.char_data_addr_info[0] + (uint32_t(entry.char_data_addr_info[1]) << 8) + (uint32_t(entry.char_data_addr_info[2]) << 16) + font_map_address;
                addr.length = entry.char_data_len;
                addr.height = font_height;
                addr.width = entry.char_width + entry.kerning_left + entry.kerning_right;
            }
        }
        return addr;
    }

    char_data_info_t GB2312Font::get_char_data_address(uint16_t char_code)
    {
        char_data_info_t addr{ .char_code = char_code, .address = 0, .length = 0, .width = 0, .height = 0, .cached = false };
        auto cached_it = std::lower_bound(cached_chars.cbegin(), cached_chars.cend(), addr);
        if(cached_it != cached_chars.cend() && cached_it->char_code == char_code)
        {
            addr = *cached_it;
        }
        else
        {
            int32_t char_index = -1;
            if (char_code < 0x80)
            {
                if (char_code >= 0x20 && char_code < 0x80)
                {
                    char_index = 8178 + char_code - 0x20;
                }
            }
            else
            {
                uint8_t low_byte = char_code & 0xFF;
                uint8_t hi_byte = char_code >> 8;
                if (low_byte >= 0xA1 && low_byte <= 0xF7 && hi_byte >= 0xA1 && hi_byte <= 0xFE)
                {
                    char_index = (low_byte - 0xA1) * 94 + hi_byte - 0xA1;
                }
            }
            if (char_index >= 0)
            {
                font_char_entry_t entry;
                if(font_data != nullptr)
                {
                    const uint8_t *p_font_entry = font_data + font_map_address + char_index * sizeof(font_char_entry_t);
                    entry = *reinterpret_cast<const font_char_entry_t*>(p_font_entry);
                }
#if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
                else
                {
                    font_file.seek(font_map_address + char_index * sizeof(font_char_entry_t));
                    font_file.read(reinterpret_cast<char*>(&entry), sizeof(font_char_entry_t));
                }
#endif
                addr.address = entry.char_data_addr_info[0] + (uint32_t(entry.char_data_addr_info[1]) << 8) + (uint32_t(entry.char_data_addr_info[2]) << 16) + font_map_address;
                addr.length = entry.char_data_len;
                addr.height = font_height;
                addr.width = entry.char_width + entry.kerning_left + entry.kerning_right;
            }
        }
        return addr;
    }

    uint16_t UnicodeFont::get_next_character(const std::string_view& text, size_t& index) const
    {
        uint16_t character;
        if (index + 1 < text.size())
        {
            character = static_cast<uint16_t>(static_cast<uint8_t>(text[index])) +
                (static_cast<uint16_t>(static_cast<uint8_t>(text[index + 1])) << 8);
            index += 2;
        }
        else
        {
            character = static_cast<uint16_t>('?');
            index++;
        }
        return character;
    }

    char_data_info_t UnicodeFont::get_char_data_address(uint16_t char_code)
    {
        char_data_info_t addr{ .char_code = char_code, .address = 0, .length = 0, .width = 0, .height = 0, .cached = false };
        auto cached_it = std::lower_bound(cached_chars.cbegin(), cached_chars.cend(), addr);
        if(cached_it != cached_chars.cend() && cached_it->char_code == char_code)
        {
            addr = *cached_it;
        }
        else
        {
            int32_t char_index = -1;
            if(char_code <= 0xD7AF)
            {
                char_index = char_code;
            }
            else
            {
                char_index = 0x003F;
            }
            if (char_index >= 0)
            {
                font_char_entry_t entry;
                if(font_data != nullptr)
                {
                    const uint8_t *p_font_entry = font_data + font_map_address + char_index * sizeof(font_char_entry_t);
                    entry = *reinterpret_cast<const font_char_entry_t*>(p_font_entry);
                }
#if defined(MBED_CONF_FILESYSTEM_PRESENT) && (MBED_CONF_FILESYSTEM_PRESENT == 1)
                else
                {
                    font_file.seek(font_map_address + char_index * sizeof(font_char_entry_t));
                    font_file.read(reinterpret_cast<char*>(&entry), sizeof(font_char_entry_t));
                }
#endif
                addr.address = (entry.char_data_addr_info[0] + (uint32_t(entry.char_data_addr_info[1]) << 8) + (uint32_t(entry.char_data_addr_info[2]) << 16)) * 8 + font_map_address;
                addr.length = entry.char_data_len;
                addr.height = font_height;
                addr.width = entry.char_width + entry.kerning_left + entry.kerning_right;
            }
        }
        return addr;
    }

    uint8_t UnicodeFont::get_character_code_width(uint16_t character) const
    {
        return 2;
    }

    Size UnicodeFont::get_text_size(std::wstring_view text, uint16_t wrap_width)
    {
        return FontBase::get_text_size(std::string_view(reinterpret_cast<const char*>(text.data()), text.length() * 2), wrap_width);
    }

}
