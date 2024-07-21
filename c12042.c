void dwgCompressor::readInstructions21(duint8 *cbuf, duint32 *si, duint8 *oc, duint32 *so, duint32 *l){
    duint32 length;
    duint32 srcIndex = *si;
    duint32 sourceOffset;
    unsigned char opCode = *oc;
    switch ((opCode >> 4)) {
    case 0:
        length = (opCode & 0xf) + 0x13;
        sourceOffset = cbuf[srcIndex++];
        opCode = cbuf[srcIndex++];
        length = ((opCode >> 3) & 0x10) + length;
        sourceOffset = ((opCode & 0x78) << 5) + 1 + sourceOffset;
        break;
    case 1:
        length = (opCode & 0xf) + 3;
        sourceOffset = cbuf[srcIndex++];
        opCode = cbuf[srcIndex++];
        sourceOffset = ((opCode & 0xf8) << 5) + 1 + sourceOffset;
        break;
    case 2:
        sourceOffset = cbuf[srcIndex++];
        sourceOffset = ((cbuf[srcIndex++] << 8) & 0xff00) | sourceOffset;
        length = opCode & 7;
        if ((opCode & 8) == 0) {
            opCode = cbuf[srcIndex++];
            length = (opCode & 0xf8) + length;
        } else {
            sourceOffset++;
            length = (cbuf[srcIndex++] << 3) + length;
            opCode = cbuf[srcIndex++];
            length = (((opCode & 0xf8) << 8) + length) + 0x100;
        }
        break;
    default:
        length = opCode >> 4;
        sourceOffset = opCode & 15;
        opCode = cbuf[srcIndex++];
        sourceOffset = (((opCode & 0xf8) << 1) + sourceOffset) + 1;
        break;
    }
    *oc = opCode;
    *si = srcIndex;
    *so = sourceOffset;
    *l = length;
}