Map1toN(SDL_PixelFormat * src, Uint8 Rmod, Uint8 Gmod, Uint8 Bmod, Uint8 Amod,
        SDL_PixelFormat * dst)
{
    Uint8 *map;
    int i;
    int bpp;
    SDL_Palette *pal = src->palette;

    bpp = ((dst->BytesPerPixel == 3) ? 4 : dst->BytesPerPixel);
    map = (Uint8 *) SDL_malloc(pal->ncolors * bpp);
    if (map == NULL) {
        SDL_OutOfMemory();
        return (NULL);
    }

    /* We memory copy to the pixel map so the endianness is preserved */
    for (i = 0; i < pal->ncolors; ++i) {
        Uint8 R = (Uint8) ((pal->colors[i].r * Rmod) / 255);
        Uint8 G = (Uint8) ((pal->colors[i].g * Gmod) / 255);
        Uint8 B = (Uint8) ((pal->colors[i].b * Bmod) / 255);
        Uint8 A = (Uint8) ((pal->colors[i].a * Amod) / 255);
        ASSEMBLE_RGBA(&map[i * bpp], dst->BytesPerPixel, dst, (Uint32)R, (Uint32)G, (Uint32)B, (Uint32)A);
    }
    return (map);
}