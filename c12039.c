duint32 dwgCompressor::decompByte(const duint32 index)
{
    if (index < decompSize) {
        return decompBuffer[index];
    }

    return 0;
}