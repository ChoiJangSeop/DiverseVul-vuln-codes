bool dwgReader18::parseDataPage(const dwgSectionInfo &si/*, duint8 *dData*/){
    DRW_DBG("\nparseDataPage\n ");
    objData.reset( new duint8 [si.pageCount * si.maxSize] );

    for (auto it=si.pages.begin(); it!=si.pages.end(); ++it){
        dwgPageInfo pi = it->second;
        if (!fileBuf->setPosition(pi.address))
            return false;
        //decript section header
        duint8 hdrData[32];
        fileBuf->getBytes(hdrData, 32);
        dwgCompressor::decrypt18Hdr(hdrData, 32, pi.address);
        DRW_DBG("Section  "); DRW_DBG(si.name); DRW_DBG(" page header=\n");
        for (unsigned int i=0, j=0; i< 32;i++) {
            DRW_DBGH( static_cast<unsigned char>(hdrData[i]));
            if (j == 7) {
                DRW_DBG("\n");
                j = 0;
            } else {
                DRW_DBG(", ");
                j++;
            }
        } DRW_DBG("\n");

        DRW_DBG("\n    Page number= "); DRW_DBGH(pi.Id);
        DRW_DBG("\n    size in file= "); DRW_DBGH(pi.size);
        DRW_DBG("\n    address in file= "); DRW_DBGH(pi.address);
        DRW_DBG("\n    Data size= "); DRW_DBGH(pi.dataSize);
        DRW_DBG("\n    Start offset= "); DRW_DBGH(pi.startOffset); DRW_DBG("\n");
        dwgBuffer bufHdr(hdrData, 32, &decoder);
        DRW_DBG("      section page type= "); DRW_DBGH(bufHdr.getRawLong32());
        DRW_DBG("\n      section number= "); DRW_DBGH(bufHdr.getRawLong32());
        pi.cSize = bufHdr.getRawLong32();
        DRW_DBG("\n      data size (compressed)= "); DRW_DBGH(pi.cSize); DRW_DBG(" dec "); DRW_DBG(pi.cSize);
        pi.uSize = bufHdr.getRawLong32();
        DRW_DBG("\n      page size (decompressed)= "); DRW_DBGH(pi.uSize); DRW_DBG(" dec "); DRW_DBG(pi.uSize);
        DRW_DBG("\n      start offset (in decompressed buffer)= "); DRW_DBGH(bufHdr.getRawLong32());
        DRW_DBG("\n      unknown= "); DRW_DBGH(bufHdr.getRawLong32());
        DRW_DBG("\n      header checksum= "); DRW_DBGH(bufHdr.getRawLong32());
        DRW_DBG("\n      data checksum= "); DRW_DBGH(bufHdr.getRawLong32()); DRW_DBG("\n");

        //get compressed data
        std::vector<duint8> cData(pi.cSize);
        if (!fileBuf->setPosition(pi.address + 32)) {
            return false;
        }
        fileBuf->getBytes(cData.data(), pi.cSize);

        //calculate checksum
        duint32 calcsD = checksum(0, cData.data(), pi.cSize);
        for (duint8 i= 24; i<28; ++i)
            hdrData[i]=0;
        duint32 calcsH = checksum(calcsD, hdrData, 32);
        DRW_DBG("Calc header checksum= "); DRW_DBGH(calcsH);
        DRW_DBG("\nCalc data checksum= "); DRW_DBGH(calcsD); DRW_DBG("\n");

        duint8* oData = objData.get() + pi.startOffset;
        pi.uSize = si.maxSize;
        DRW_DBG("decompressing "); DRW_DBG(pi.cSize); DRW_DBG(" bytes in "); DRW_DBG(pi.uSize); DRW_DBG(" bytes\n");
        dwgCompressor comp;
        comp.decompress18(cData.data(), oData, pi.cSize, pi.uSize);
    }
    return true;
}