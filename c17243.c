static QEMUCursor *cursor_parse_xpm(const char *xpm[])
{
    QEMUCursor *c;
    uint32_t ctab[128];
    unsigned int width, height, colors, chars;
    unsigned int line = 0, i, r, g, b, x, y, pixel;
    char name[16];
    uint8_t idx;

    /* parse header line: width, height, #colors, #chars */
    if (sscanf(xpm[line], "%u %u %u %u",
               &width, &height, &colors, &chars) != 4) {
        fprintf(stderr, "%s: header parse error: \"%s\"\n",
                __func__, xpm[line]);
        return NULL;
    }
    if (chars != 1) {
        fprintf(stderr, "%s: chars != 1 not supported\n", __func__);
        return NULL;
    }
    line++;

    /* parse color table */
    for (i = 0; i < colors; i++, line++) {
        if (sscanf(xpm[line], "%c c %15s", &idx, name) == 2) {
            if (sscanf(name, "#%02x%02x%02x", &r, &g, &b) == 3) {
                ctab[idx] = (0xff << 24) | (b << 16) | (g << 8) | r;
                continue;
            }
            if (strcmp(name, "None") == 0) {
                ctab[idx] = 0x00000000;
                continue;
            }
        }
        fprintf(stderr, "%s: color parse error: \"%s\"\n",
                __func__, xpm[line]);
        return NULL;
    }

    /* parse pixel data */
    c = cursor_alloc(width, height);
    for (pixel = 0, y = 0; y < height; y++, line++) {
        for (x = 0; x < height; x++, pixel++) {
            idx = xpm[line][x];
            c->data[pixel] = ctab[idx];
        }
    }
    return c;
}