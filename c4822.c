static void write_palette(int idx, uint32_t color, void *opaque)
{
    struct palette_cb_priv *priv = opaque;
    VncState *vs = priv->vs;
    uint32_t bytes = vs->clientds.pf.bytes_per_pixel;

    if (bytes == 4) {
        ((uint32_t*)priv->header)[idx] = color;
    } else {
        ((uint16_t*)priv->header)[idx] = color;
    }
}