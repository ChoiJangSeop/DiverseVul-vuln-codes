static void vnc_zrle_start(VncState *vs)
{
    buffer_reset(&vs->zrle.zrle);

    /* make the output buffer be the zlib buffer, so we can compress it later */
    vs->zrle.tmp = vs->output;
    vs->output = vs->zrle.zrle;
}