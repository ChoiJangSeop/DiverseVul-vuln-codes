static uint32_t color_string_to_rgba(const char *p, int len)
{
    uint32_t ret = 0xFF000000;
    const ColorEntry *entry;
    char color_name[100];

    if (*p == '#') {
        p++;
        len--;
        if (len == 3) {
            ret |= (hex_char_to_number(p[2]) <<  4) |
                   (hex_char_to_number(p[1]) << 12) |
                   (hex_char_to_number(p[0]) << 20);
        } else if (len == 4) {
            ret  = (hex_char_to_number(p[3]) <<  4) |
                   (hex_char_to_number(p[2]) << 12) |
                   (hex_char_to_number(p[1]) << 20) |
                   (hex_char_to_number(p[0]) << 28);
        } else if (len == 6) {
            ret |=  hex_char_to_number(p[5])        |
                   (hex_char_to_number(p[4]) <<  4) |
                   (hex_char_to_number(p[3]) <<  8) |
                   (hex_char_to_number(p[2]) << 12) |
                   (hex_char_to_number(p[1]) << 16) |
                   (hex_char_to_number(p[0]) << 20);
        } else if (len == 8) {
            ret  =  hex_char_to_number(p[7])        |
                   (hex_char_to_number(p[6]) <<  4) |
                   (hex_char_to_number(p[5]) <<  8) |
                   (hex_char_to_number(p[4]) << 12) |
                   (hex_char_to_number(p[3]) << 16) |
                   (hex_char_to_number(p[2]) << 20) |
                   (hex_char_to_number(p[1]) << 24) |
                   (hex_char_to_number(p[0]) << 28);
        }
    } else {
        strncpy(color_name, p, len);
        color_name[len] = '\0';

        entry = bsearch(color_name,
                        color_table,
                        FF_ARRAY_ELEMS(color_table),
                        sizeof(ColorEntry),
                        color_table_compare);

        if (!entry)
            return ret;

        ret = entry->rgb_color;
    }
    return ret;
}