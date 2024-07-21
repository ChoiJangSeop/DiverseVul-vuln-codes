void dwgCompressor::decompress21(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize){
    duint32 srcIndex=0;
    duint32 dstIndex=0;
    duint32 length=0;
    duint32 sourceOffset;
    duint8 opCode;

    opCode = cbuf[srcIndex++];
    if ((opCode >> 4) == 2){
        srcIndex = srcIndex +2;
        length = cbuf[srcIndex++] & 0x07;
    }

    while (srcIndex < csize && (dstIndex < dsize+1)){//dstIndex < dsize to prevent crash more robust are needed
        if (length == 0)
            length = litLength21(cbuf, opCode, &srcIndex);
        copyCompBytes21(cbuf, dbuf, length, srcIndex, dstIndex);
        srcIndex += length;
        dstIndex += length;
        if (dstIndex >=dsize) break; //check if last chunk are compressed & terminate

        length = 0;
        opCode = cbuf[srcIndex++];
        readInstructions21(cbuf, &srcIndex, &opCode, &sourceOffset, &length);
        while (true) {
            //prevent crash with corrupted data
            if (sourceOffset > dstIndex){
                DRW_DBG("\nWARNING dwgCompressor::decompress21 => sourceOffset> dstIndex.\n");
                DRW_DBG("csize = "); DRW_DBG(csize); DRW_DBG("  srcIndex = "); DRW_DBG(srcIndex);
                DRW_DBG("\ndsize = "); DRW_DBG(dsize); DRW_DBG("  dstIndex = "); DRW_DBG(dstIndex);
                sourceOffset = dstIndex;
            }
            //prevent crash with corrupted data
            if (length > dsize - dstIndex){
                DRW_DBG("\nWARNING dwgCompressor::decompress21 => length > dsize - dstIndex.\n");
                DRW_DBG("csize = "); DRW_DBG(csize); DRW_DBG("  srcIndex = "); DRW_DBG(srcIndex);
                DRW_DBG("\ndsize = "); DRW_DBG(dsize); DRW_DBG("  dstIndex = "); DRW_DBG(dstIndex);
                length = dsize - dstIndex;
                srcIndex = csize;//force exit
            }
            sourceOffset = dstIndex-sourceOffset;
            for (duint32 i=0; i< length; i++)
                dbuf[dstIndex++] = dbuf[sourceOffset+i];

            length = opCode & 7;
            if ((length != 0) || (srcIndex >= csize)) {
                break;
            }
            opCode = cbuf[srcIndex++];
            if ((opCode >> 4) == 0) {
                break;
            }
            if ((opCode >> 4) == 15) {
                opCode &= 15;
            }
            readInstructions21(cbuf, &srcIndex, &opCode, &sourceOffset, &length);
        }
    }
    DRW_DBG("\ncsize = "); DRW_DBG(csize); DRW_DBG("  srcIndex = "); DRW_DBG(srcIndex);
    DRW_DBG("\ndsize = "); DRW_DBG(dsize); DRW_DBG("  dstIndex = "); DRW_DBG(dstIndex);DRW_DBG("\n");
}