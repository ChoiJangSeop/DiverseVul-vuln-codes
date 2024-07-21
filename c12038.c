bool dwgCompressor::decompress18(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos=0; //current position in compressed buffer
    decompPos=0; //current position in resulting decompressed buffer

    DRW_DBG("dwgCompressor::decompress, last 2 bytes: ");
    DRW_DBGH(compressedBuffer[compressedSize - 2]);DRW_DBG(" ");DRW_DBGH(compressedBuffer[compressedSize - 1]);DRW_DBG("\n");

    duint32 compBytes {0};
    duint32 compOffset {0};
    duint32 litCount {litLength18()};

    //copy first literal length
    for (duint32 i = 0; i < litCount && buffersGood(); ++i) {
        decompSet( compressedByte());
    }

    while (buffersGood()) {
        duint8 oc = compressedByte(); //next opcode
        if (oc == 0x10){
            compBytes = longCompressionOffset()+ 9;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x11 && oc< 0x20){
            compBytes = (oc & 0x0F) + 2;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc == 0x20){
            compBytes = longCompressionOffset() + 0x21;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x20 && oc< 0x40){
            compBytes = oc - 0x1E;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if ( oc > 0x3F){
            compBytes = ((oc & 0xF0) >> 4) - 1;
            duint8 ll2 = compressedByte();
            compOffset =  (ll2 << 2) | ((oc & 0x0C) >> 2);
            litCount = oc & 0x03;
            if (litCount < 1){
                litCount= litLength18();}
        } else if (oc == 0x11){
            DRW_DBG("dwgCompressor::decompress, end of input stream, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG("\n");
            return true; //end of input stream
        } else { //ll < 0x10
            DRW_DBG("WARNING dwgCompressor::decompress, failed, illegal char: "); DRW_DBGH(oc);
            DRW_DBG(", Cpos: "); DRW_DBG(compressedPos);
            DRW_DBG(", Dpos: "); DRW_DBG(decompPos); DRW_DBG("\n");
            return false; //fails, not valid
        }

        //copy "compressed data", if size allows
        if (decompSize < decompPos + compBytes) {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad compBytes size, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG(", need ");DRW_DBG(compBytes);DRW_DBG(", available ");DRW_DBG(decompSize - decompPos);DRW_DBG("\n");
            // only copy what we can fit
            compBytes = decompSize - decompPos;
        }
        duint32 j {decompPos - compOffset - 1};
        for (duint32 i = 0; i < compBytes && buffersGood(); i++) {
            decompSet( decompByte( j++));
        }

        //copy "uncompressed data", if size allows
        if (decompSize < decompPos + litCount) {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad litCount size, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG(", need ");DRW_DBG(litCount);DRW_DBG(", available ");DRW_DBG(decompSize - decompPos);DRW_DBG("\n");
            // only copy what we can fit
            litCount = decompSize - decompPos;
        }
        for (duint32 i=0; i < litCount && buffersGood(); i++) {
            decompSet( compressedByte());
        }
    }

    DRW_DBG("WARNING dwgCompressor::decompress, bad out, Cpos: ");DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG("\n");
    return false;
}