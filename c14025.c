Map1to1(SDL_Palette * src, SDL_Palette * dst, int *identical)
{
    Uint8 *map;
    int i;

    if (identical) {
        if (src->ncolors <= dst->ncolors) {
            /* If an identical palette, no need to map */
            if (src == dst
                ||
                (SDL_memcmp
                 (src->colors, dst->colors,
                  src->ncolors * sizeof(SDL_Color)) == 0)) {
                *identical = 1;
                return (NULL);
            }
        }
        *identical = 0;
    }
    map = (Uint8 *) SDL_malloc(src->ncolors);
    if (map == NULL) {
        SDL_OutOfMemory();
        return (NULL);
    }
    for (i = 0; i < src->ncolors; ++i) {
        map[i] = SDL_FindColor(dst,
                               src->colors[i].r, src->colors[i].g,
                               src->colors[i].b, src->colors[i].a);
    }
    return (map);
}