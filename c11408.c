
static SDL_Surface* Create_Surface_Shaded(int width, int height, SDL_Color fg, SDL_Color bg, Uint32 *color)
{
    const int alignment = Get_Alignement() - 1;
    SDL_Surface *textbuf;
    Sint64 size;
    Uint8 bg_alpha = bg.a;

    /* Create a surface with memory:
     * - pitch is rounded to alignment
     * - adress is aligned
     */
    void *pixels, *ptr;
    /* Worse case at the end of line pulling 'alignment' extra blank pixels */
    Sint64 pitch = width + alignment;
    pitch += alignment;
    pitch &= ~alignment;
    size = height * pitch + sizeof (void *) + alignment;
    if (size < 0 || size > SDL_MAX_SINT32) {
        /* Overflow... */
        return NULL;
    }

    ptr = SDL_malloc((size_t)size);
    if (ptr == NULL) {
        return NULL;
    }

    /* address is aligned */
    pixels = (void *)(((uintptr_t)ptr + sizeof(void *) + alignment) & ~alignment);
    ((void **)pixels)[-1] = ptr;

    textbuf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height, 0, pitch, SDL_PIXELFORMAT_INDEX8);
    if (textbuf == NULL) {
        SDL_free(ptr);
        return NULL;
    }

    /* Let SDL handle the memory allocation */
    textbuf->flags &= ~SDL_PREALLOC;
    textbuf->flags |= SDL_SIMD_ALIGNED;

    /* Initialize with background to 0 */
    SDL_memset(pixels, 0, height * pitch);

    /* Underline/Strikethrough color style */
    *color = NUM_GRAYS - 1;

    /* Support alpha blending */
    if (fg.a != SDL_ALPHA_OPAQUE || bg.a != SDL_ALPHA_OPAQUE) {
        SDL_SetSurfaceBlendMode(textbuf, SDL_BLENDMODE_BLEND);

        /* Would disturb alpha palette */
        if (bg.a == SDL_ALPHA_OPAQUE) {
            bg.a = 0;
        }
    }

    /* Fill the palette with NUM_GRAYS levels of shading from bg to fg */
    {
        SDL_Palette *palette = textbuf->format->palette;
        int rdiff  = fg.r - bg.r;
        int gdiff  = fg.g - bg.g;
        int bdiff  = fg.b - bg.b;
        int adiff  = fg.a - bg.a;
        int sign_r = (rdiff >= 0) ? 1 : 255;
        int sign_g = (gdiff >= 0) ? 1 : 255;
        int sign_b = (bdiff >= 0) ? 1 : 255;
        int sign_a = (adiff >= 0) ? 1 : 255;
        int i;

        for (i = 0; i < NUM_GRAYS; ++i) {
            /* Compute color[i] = (i * color_diff / 255) */
            int tmp_r = i * rdiff;
            int tmp_g = i * gdiff;
            int tmp_b = i * bdiff;
            int tmp_a = i * adiff;
            palette->colors[i].r = (Uint8)(bg.r + DIVIDE_BY_255_SIGNED(tmp_r, sign_r));
            palette->colors[i].g = (Uint8)(bg.g + DIVIDE_BY_255_SIGNED(tmp_g, sign_g));
            palette->colors[i].b = (Uint8)(bg.b + DIVIDE_BY_255_SIGNED(tmp_b, sign_b));
            palette->colors[i].a = (Uint8)(bg.a + DIVIDE_BY_255_SIGNED(tmp_a, sign_a));
        }

        /* Make sure background has the correct alpha value */
        palette->colors[0].a = bg_alpha;
    }

    return textbuf;