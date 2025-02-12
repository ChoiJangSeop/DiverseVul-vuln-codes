static void write_png_palette(int idx, uint32_t pix, void *opaque)
{
    struct palette_cb_priv *priv = opaque;
    VncState *vs = priv->vs;
    png_colorp color = &priv->png_palette[idx];

    if (vs->tight.pixel24)
    {
        color->red = (pix >> vs->clientds.pf.rshift) & vs->clientds.pf.rmax;
        color->green = (pix >> vs->clientds.pf.gshift) & vs->clientds.pf.gmax;
        color->blue = (pix >> vs->clientds.pf.bshift) & vs->clientds.pf.bmax;
    }
    else
    {
        int red, green, blue;

        red = (pix >> vs->clientds.pf.rshift) & vs->clientds.pf.rmax;
        green = (pix >> vs->clientds.pf.gshift) & vs->clientds.pf.gmax;
        blue = (pix >> vs->clientds.pf.bshift) & vs->clientds.pf.bmax;
        color->red = ((red * 255 + vs->clientds.pf.rmax / 2) /
                      vs->clientds.pf.rmax);
        color->green = ((green * 255 + vs->clientds.pf.gmax / 2) /
                        vs->clientds.pf.gmax);
        color->blue = ((blue * 255 + vs->clientds.pf.bmax / 2) /
                       vs->clientds.pf.bmax);
    }
}