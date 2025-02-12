static void tight_pack24(VncState *vs, uint8_t *buf, size_t count, size_t *ret)
{
    uint32_t *buf32;
    uint32_t pix;
    int rshift, gshift, bshift;

    buf32 = (uint32_t *)buf;

    if ((vs->clientds.flags & QEMU_BIG_ENDIAN_FLAG) ==
        (vs->ds->surface->flags & QEMU_BIG_ENDIAN_FLAG)) {
        rshift = vs->clientds.pf.rshift;
        gshift = vs->clientds.pf.gshift;
        bshift = vs->clientds.pf.bshift;
    } else {
        rshift = 24 - vs->clientds.pf.rshift;
        gshift = 24 - vs->clientds.pf.gshift;
        bshift = 24 - vs->clientds.pf.bshift;
    }

    if (ret) {
        *ret = count * 3;
    }

    while (count--) {
        pix = *buf32++;
        *buf++ = (char)(pix >> rshift);
        *buf++ = (char)(pix >> gshift);
        *buf++ = (char)(pix >> bshift);
    }
}