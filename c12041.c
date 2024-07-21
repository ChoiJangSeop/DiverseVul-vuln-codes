duint32 dwgCompressor::compressedByte(void)
{
    duint32 result {0};

    compressedGood = (compressedPos < compressedSize);
    if (compressedGood) {
        result = compressedBuffer[compressedPos];
        ++compressedPos;
    }

    return result;
}