gx_image_enum_alloc(const gs_image_common_t * pic,
                    const gs_int_rect * prect, gs_memory_t * mem,
                    gx_image_enum **ppenum)
{
    const gs_pixel_image_t *pim = (const gs_pixel_image_t *)pic;
    int width = pim->Width, height = pim->Height;
    int bpc = pim->BitsPerComponent;
    gx_image_enum *penum;

    if (width < 0 || height < 0)
        return_error(gs_error_rangecheck);
    switch (pim->format) {
    case gs_image_format_chunky:
    case gs_image_format_component_planar:
        switch (bpc) {
        case 1: case 2: case 4: case 8: case 12: case 16: break;
        default: return_error(gs_error_rangecheck);
        }
        break;
    case gs_image_format_bit_planar:
        if (bpc < 1 || bpc > 8)
            return_error(gs_error_rangecheck);
    }
    if (prect) {
        if (prect->p.x < 0 || prect->p.y < 0 ||
            prect->q.x < prect->p.x || prect->q.y < prect->p.y ||
            prect->q.x > width || prect->q.y > height
            )
            return_error(gs_error_rangecheck);
    }
    penum = gs_alloc_struct(mem, gx_image_enum, &st_gx_image_enum,
                            "gx_default_begin_image");
    if (penum == 0)
        return_error(gs_error_VMerror);
    if (prect) {
        penum->rect.x = prect->p.x;
        penum->rect.y = prect->p.y;
        penum->rect.w = prect->q.x - prect->p.x;
        penum->rect.h = prect->q.y - prect->p.y;
    } else {
        penum->rect.x = 0, penum->rect.y = 0;
        penum->rect.w = width, penum->rect.h = height;
    }
    penum->rrect.x = penum->rect.x;
    penum->rrect.y = penum->rect.y;
    penum->rrect.w = penum->rect.w;
    penum->rrect.h = penum->rect.h;
#ifdef DEBUG
    if (gs_debug_c('b')) {
        dmlprintf2(mem, "[b]Image: w=%d h=%d", width, height);
        if (prect)
            dmprintf4(mem, " ((%d,%d),(%d,%d))",
                     prect->p.x, prect->p.y, prect->q.x, prect->q.y);
    }
#endif
    *ppenum = penum;
    return 0;
}