void dwgCompressor::decompress18(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize){
    bufC = cbuf;
    sizeC = csize -2;
    DRW_DBG("dwgCompressor::decompress, last 2 bytes: ");
    DRW_DBGH(bufC[sizeC]);DRW_DBGH(bufC[sizeC+1]);DRW_DBG("\n");
    sizeC = csize;

    duint32 compBytes;
    duint32 compOffset;
    duint32 litCount;

    pos=0; //current position in compressed buffer
    duint32 rpos=0; //current position in resulting decompressed buffer
    litCount = litLength18();
    //copy first literal length
    for (duint32 i=0; i < litCount; ++i) {
        dbuf[rpos++] = bufC[pos++];
    }

    while (pos < csize && (rpos < dsize+1)){//rpos < dsize to prevent crash more robust are needed
        duint8 oc = bufC[pos++]; //next opcode
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
            duint8 ll2 = bufC[pos++];
            compOffset =  (ll2 << 2) | ((oc & 0x0C) >> 2);
            litCount = oc & 0x03;
            if (litCount < 1){
                litCount= litLength18();}
        } else if (oc == 0x11){
            DRW_DBG("dwgCompressor::decompress, end of input stream, Cpos: ");
            DRW_DBG(pos);DRW_DBG(", Dpos: ");DRW_DBG(rpos);DRW_DBG("\n");
            return; //end of input stream
        } else { //ll < 0x10
            DRW_DBG("WARNING dwgCompressor::decompress, failed, illegal char, Cpos: ");
            DRW_DBG(pos);DRW_DBG(", Dpos: ");DRW_DBG(rpos);DRW_DBG("\n");
            return; //fails, not valid
        }
        //copy "compressed data", if size allows
        if (dsize<rpos+compBytes)
        {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad compBytes size, Cpos: ");
            DRW_DBG(pos);DRW_DBG(", Dpos: ");DRW_DBG(rpos);DRW_DBG(", need ");DRW_DBG(compBytes);DRW_DBG(", available ");DRW_DBG(dsize-rpos);DRW_DBG("\n");
            // only copy what we can fit
            compBytes=dsize-rpos;
        }
        for (duint32 i=0, j= rpos - compOffset -1; i < compBytes; i++) {
            dbuf[rpos++] = dbuf[j++];
        }

        //copy "uncompressed data", if size allows
        if ( dsize < rpos + litCount )
        {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad litCount size, Cpos: ");
            DRW_DBG(pos);DRW_DBG(", Dpos: ");DRW_DBG(rpos);DRW_DBG(", need ");DRW_DBG(litCount);DRW_DBG(", available ");DRW_DBG(dsize-rpos);DRW_DBG("\n");
            // only copy what we can fit
            litCount = dsize - rpos;
        }
        for (duint32 i=0; i < litCount; i++) {
            dbuf[rpos++] = bufC[pos++];
        }
    }
    DRW_DBG("WARNING dwgCompressor::decompress, bad out, Cpos: ");DRW_DBG(pos);DRW_DBG(", Dpos: ");DRW_DBG(rpos);DRW_DBG("\n");
}