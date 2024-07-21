duint32 dwgCompressor::litLength21(duint8 *cbuf, duint8 oc, duint32 *si){

    duint32 srcIndex=*si;

    duint32 length = oc + 8;
    if (length == 0x17) {
        duint32 n = cbuf[srcIndex++];
        length += n;
        if (n == 0xff) {
            do {
                n = cbuf[srcIndex++];
                n |= static_cast<duint32>(cbuf[srcIndex++] << 8);
                length += n;
            } while (n == 0xffff);
        }
    }

    *si = srcIndex;
    return length;
}