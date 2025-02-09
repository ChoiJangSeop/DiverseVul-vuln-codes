void vnc_hextile_set_pixel_conversion(VncState *vs, int generic)
{
    if (!generic) {
        switch (vs->ds->surface->pf.bits_per_pixel) {
            case 8:
                vs->hextile.send_tile = send_hextile_tile_8;
                break;
            case 16:
                vs->hextile.send_tile = send_hextile_tile_16;
                break;
            case 32:
                vs->hextile.send_tile = send_hextile_tile_32;
                break;
        }
    } else {
        switch (vs->ds->surface->pf.bits_per_pixel) {
            case 8:
                vs->hextile.send_tile = send_hextile_tile_generic_8;
                break;
            case 16:
                vs->hextile.send_tile = send_hextile_tile_generic_16;
                break;
            case 32:
                vs->hextile.send_tile = send_hextile_tile_generic_32;
                break;
        }
    }
}