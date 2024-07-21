void dwgReader18::parseSysPage(duint8 *decompSec, duint32 decompSize){
    DRW_DBG("\nparseSysPage:\n ");
    duint32 compSize = fileBuf->getRawLong32();
    DRW_DBG("Compressed size= "); DRW_DBG(compSize); DRW_DBG(", "); DRW_DBGH(compSize);
    DRW_DBG("\nCompression type= "); DRW_DBGH(fileBuf->getRawLong32());
    DRW_DBG("\nSection page checksum= "); DRW_DBGH(fileBuf->getRawLong32()); DRW_DBG("\n");

    duint8 hdrData[20];
    fileBuf->moveBitPos(-160);
    fileBuf->getBytes(hdrData, 20);
    for (duint8 i= 16; i<20; ++i)
        hdrData[i]=0;
    duint32 calcsH = checksum(0, hdrData, 20);
    DRW_DBG("Calc hdr checksum= "); DRW_DBGH(calcsH);
    std::vector<duint8> tmpCompSec(compSize);
    fileBuf->getBytes(tmpCompSec.data(), compSize);
    duint32 calcsD = checksum(calcsH, tmpCompSec.data(), compSize);
    DRW_DBG("\nCalc data checksum= "); DRW_DBGH(calcsD); DRW_DBG("\n");

#ifdef DRW_DBG_DUMP
    for (unsigned int i=0, j=0; i< compSize;i++) {
        DRW_DBGH( (unsigned char)compSec[i]);
        if (j == 7) { DRW_DBG("\n"); j = 0;
        } else { DRW_DBG(", "); j++; }
    } DRW_DBG("\n");
#endif
    DRW_DBG("decompressing "); DRW_DBG(compSize); DRW_DBG(" bytes in "); DRW_DBG(decompSize); DRW_DBG(" bytes\n");
    dwgCompressor comp;
    comp.decompress18(tmpCompSec.data(), decompSec, compSize, decompSize);
#ifdef DRW_DBG_DUMP
    for (unsigned int i=0, j=0; i< decompSize;i++) {
        DRW_DBGH( decompSec[i]);
        if (j == 7) { DRW_DBG("\n"); j = 0;
        } else { DRW_DBG(", "); j++; }
    } DRW_DBG("\n");
#endif
}