static void write_png_palette(int idx, uint32_t pix, void *opaque)
{
    struct palette_cb_priv *priv = opaque;
    VncState *vs = priv->vs;
    png_colorp color = &priv->png_palette[idx];

    if (vs->tight.pixel24)
    {
        color->red = (pix >> vs->client_pf.rshift) & vs->client_pf.rmax;
        color->green = (pix >> vs->client_pf.gshift) & vs->client_pf.gmax;
        color->blue = (pix >> vs->client_pf.bshift) & vs->client_pf.bmax;
    }
    else
    {
        int red, green, blue;

        red = (pix >> vs->client_pf.rshift) & vs->client_pf.rmax;
        green = (pix >> vs->client_pf.gshift) & vs->client_pf.gmax;
        blue = (pix >> vs->client_pf.bshift) & vs->client_pf.bmax;
        color->red = ((red * 255 + vs->client_pf.rmax / 2) /
                      vs->client_pf.rmax);
        color->green = ((green * 255 + vs->client_pf.gmax / 2) /
                        vs->client_pf.gmax);
        color->blue = ((blue * 255 + vs->client_pf.bmax / 2) /
                       vs->client_pf.bmax);
    }
}