duint32 dwgCompressor::twoByteOffset(duint32 *ll){
    duint32 cont = 0;
    duint8 fb = bufC[pos++];
    cont = (fb >> 2) | (bufC[pos++] << 6);
    *ll = (fb & 0x03);
    return cont;
}