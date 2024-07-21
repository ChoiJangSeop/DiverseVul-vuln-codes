bool dwgReader21::readFileHeader() {

    DRW_DBG("\n\ndwgReader21::parsing file header\n");
    if (! fileBuf->setPosition(0x80))
        return false;
    duint8 fileHdrRaw[0x2FD];//0x3D8
    fileBuf->getBytes(fileHdrRaw, 0x2FD);
    duint8 fileHdrdRS[0x2CD];
    dwgRSCodec::decode239I(fileHdrRaw, fileHdrdRS, 3);

#ifdef DRW_DBG_DUMP
    DRW_DBG("\ndwgReader21::parsed Reed Solomon decode:\n");
    int j = 0;
    for (int i=0, j=0; i<0x2CD; i++){
        DRW_DBGH( (unsigned char)fileHdrdRS[i]);
        if (j== 15){ j=0; DRW_DBG("\n");
        } else{ j++; DRW_DBG(", "); }
    } DRW_DBG("\n");
#endif

    dwgBuffer fileHdrBuf(fileHdrdRS, 0x2CD, &decoder);
    DRW_DBG("\nCRC 64b= "); DRW_DBGH(fileHdrBuf.getRawLong64());
    DRW_DBG("\nunknown key 64b= "); DRW_DBGH(fileHdrBuf.getRawLong64());
    DRW_DBG("\ncomp data CRC 64b= "); DRW_DBGH(fileHdrBuf.getRawLong64());
    dint32 fileHdrCompLength = fileHdrBuf.getRawLong32();
    DRW_DBG("\ncompr len 4bytes= "); DRW_DBG(fileHdrCompLength);
    dint32 fileHdrCompLength2 = fileHdrBuf.getRawLong32();
    DRW_DBG("\nlength2 4bytes= "); DRW_DBG(fileHdrCompLength2);

    int fileHdrDataLength = 0x110;
    std::vector<duint8> fileHdrData;
    if (fileHdrCompLength < 0) {
        fileHdrDataLength = fileHdrCompLength * -1;
        fileHdrData.resize(fileHdrDataLength);
        fileHdrBuf.getBytes(&fileHdrData.front(), fileHdrDataLength);
    }
    else {
        DRW_DBG("\ndwgReader21:: file header are compressed:\n");
        std::vector<duint8> compByteStr(fileHdrCompLength);
        fileHdrBuf.getBytes(compByteStr.data(), fileHdrCompLength);
        fileHdrData.resize(fileHdrDataLength);
        dwgCompressor::decompress21(compByteStr.data(), &fileHdrData.front(),
                                    fileHdrCompLength, fileHdrDataLength);
    }

#ifdef DRW_DBG_DUMP
    DRW_DBG("\ndwgReader21::parsed file header:\n");
    for (int i=0, j=0; i<fileHdrDataLength; i++){
        DRW_DBGH( (unsigned char)fileHdrData[i]);
        if (j== 15){ j=0; DRW_DBG("\n");
        } else{ j++; DRW_DBG(", "); }
    } DRW_DBG("\n");
#endif

    dwgBuffer fileHdrDataBuf(&fileHdrData.front(), fileHdrDataLength, &decoder);
    DRW_DBG("\nHeader size = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nFile size = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nPagesMapCrcCompressed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    duint64 PagesMapCorrectionFactor = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapCorrectionFactor = "); DRW_DBG(PagesMapCorrectionFactor);
    DRW_DBG("\nPagesMapCrcSeed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nPages map2offset = "); DRW_DBGH(fileHdrDataBuf.getRawLong64()); //relative to data page map 1, add 0x480 to get stream position
    DRW_DBG("\nPages map2Id = "); DRW_DBG(fileHdrDataBuf.getRawLong64());
    duint64 PagesMapOffset = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapOffset = "); DRW_DBGH(PagesMapOffset); //relative to data page map 1, add 0x480 to get stream position
    DRW_DBG("\nPagesMapId = "); DRW_DBG(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nHeader2offset = "); DRW_DBGH(fileHdrDataBuf.getRawLong64()); //relative to data page map 1, add 0x480 to get stream position
    duint64 PagesMapSizeCompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapSizeCompressed = "); DRW_DBG(PagesMapSizeCompressed);
    duint64 PagesMapSizeUncompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapSizeUncompressed = "); DRW_DBG(PagesMapSizeUncompressed);
    DRW_DBG("\nPagesAmount = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    duint64 PagesMaxId = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMaxId = "); DRW_DBG(PagesMaxId);
    DRW_DBG("\nUnknown (normally 0x20) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 0x40) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nPagesMapCrcUncompressed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 0xf800) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 4) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 1) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nSectionsAmount (number of sections + 1) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nSectionsMapCrcUncompressed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    duint64 SectionsMapSizeCompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapSizeCompressed = "); DRW_DBGH(SectionsMapSizeCompressed);
    DRW_DBG("\nSectionsMap2Id = "); DRW_DBG(fileHdrDataBuf.getRawLong64());
    duint64 SectionsMapId = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapId = "); DRW_DBG(SectionsMapId);
    duint64 SectionsMapSizeUncompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapSizeUncompressed = "); DRW_DBGH(SectionsMapSizeUncompressed);
    DRW_DBG("\nSectionsMapCrcCompressed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    duint64 SectionsMapCorrectionFactor = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapCorrectionFactor = "); DRW_DBG(SectionsMapCorrectionFactor);
    DRW_DBG("\nSectionsMapCrcSeed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nStreamVersion (normally 0x60100) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nCrcSeed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nCrcSeedEncoded = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nRandomSeed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nHeader CRC64 = "); DRW_DBGH(fileHdrDataBuf.getRawLong64()); DRW_DBG("\n");

    DRW_DBG("\ndwgReader21::parse page map:\n");
    std::vector<duint8> PagesMapData(PagesMapSizeUncompressed);

    bool ret = parseSysPage(PagesMapSizeCompressed, PagesMapSizeUncompressed,
                            PagesMapCorrectionFactor, 0x480+PagesMapOffset,
                            &PagesMapData.front());
    if (!ret) {
        return false;
    }

    duint64 address = 0x480;
    duint64 i = 0;
    dwgBuffer PagesMapBuf(&PagesMapData.front(), PagesMapSizeUncompressed, &decoder);
    //stores temporarily info of all pages:
    std::unordered_map<duint64, dwgPageInfo >sectionPageMapTmp;

//    dwgPageInfo *m_pages= new dwgPageInfo[PagesMaxId+1];
    while (PagesMapSizeUncompressed > i ) {
        duint64 size = PagesMapBuf.getRawLong64();
        dint64 id = PagesMapBuf.getRawLong64();
        duint64 ind = id > 0 ? id : -id;
        i += 16;

        DRW_DBG("Page gap= "); DRW_DBG(id); DRW_DBG(" Page num= "); DRW_DBG(ind); DRW_DBG(" size= "); DRW_DBGH(size);
        DRW_DBG(" address= "); DRW_DBGH(address);  DRW_DBG("\n");
        sectionPageMapTmp[ind] = dwgPageInfo(ind, address,size);
        address += size;
        //TODO num can be negative indicating gap
//        seek += offset;
    }

    DRW_DBG("\n*** dwgReader21: Processing Section Map ***\n");
    std::vector<duint8> SectionsMapData( SectionsMapSizeUncompressed);
    dwgPageInfo sectionMap = sectionPageMapTmp[SectionsMapId];
    ret = parseSysPage(SectionsMapSizeCompressed, SectionsMapSizeUncompressed,
                       SectionsMapCorrectionFactor, sectionMap.address, &SectionsMapData.front());
    if (!ret)
        return false;

//reads sections:
    //Note: compressed value are not stored in file then, commpresed field are use to store
    // encoding value
    dwgBuffer SectionsMapBuf( &SectionsMapData.front(), SectionsMapSizeUncompressed, &decoder);
    duint8 nextId = 1;
    while(SectionsMapBuf.getPosition() < SectionsMapBuf.size()){
        dwgSectionInfo secInfo;
        secInfo.size = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nSize of section (data size)= "); DRW_DBGH(secInfo.size);
        secInfo.maxSize = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nMax Decompressed Size= "); DRW_DBGH(secInfo.maxSize);
        secInfo.encrypted = SectionsMapBuf.getRawLong64();
        //encrypted (doc: 0 no, 1 yes, 2 unkn) on read: objects 0 and encrypted yes
        DRW_DBG("\nencription= "); DRW_DBGH(secInfo.encrypted);
        DRW_DBG("\nHashCode = "); DRW_DBGH(SectionsMapBuf.getRawLong64());
        duint64 SectionNameLength = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nSectionNameLength = "); DRW_DBG(SectionNameLength);
        DRW_DBG("\nUnknown = "); DRW_DBGH(SectionsMapBuf.getRawLong64());
        secInfo.compressed = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nEncoding (compressed) = "); DRW_DBGH(secInfo.compressed);
        secInfo.pageCount = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nPage count= "); DRW_DBGH(secInfo.pageCount);
        secInfo.name = SectionsMapBuf.getUCSStr(SectionNameLength);
        DRW_DBG("\nSection name = "); DRW_DBG(secInfo.name); DRW_DBG("\n");

        for (unsigned int i=0; i< secInfo.pageCount; i++){
            duint64 po = SectionsMapBuf.getRawLong64();
            duint32 ds = SectionsMapBuf.getRawLong64();
            duint32 pn = SectionsMapBuf.getRawLong64();
            DRW_DBG("  pag Id = "); DRW_DBGH(pn); DRW_DBG(" data size = "); DRW_DBGH(ds);
            dwgPageInfo pi = sectionPageMapTmp[pn]; //get a copy
            pi.dataSize = ds;
            pi.startOffset = po;
            pi.uSize = SectionsMapBuf.getRawLong64();
            pi.cSize = SectionsMapBuf.getRawLong64();
            secInfo.pages[pn]= pi;//complete copy in secInfo
            DRW_DBG("\n    Page number= "); DRW_DBGH(secInfo.pages[pn].Id);
            DRW_DBG("\n    address in file= "); DRW_DBGH(secInfo.pages[pn].address);
            DRW_DBG("\n    size in file= "); DRW_DBGH(secInfo.pages[pn].size);
            DRW_DBG("\n    Data size= "); DRW_DBGH(secInfo.pages[pn].dataSize);
            DRW_DBG("\n    Start offset= "); DRW_DBGH(secInfo.pages[pn].startOffset);
            DRW_DBG("\n    Page uncompressed size = "); DRW_DBGH(secInfo.pages[pn].uSize);
            DRW_DBG("\n    Page compressed size = "); DRW_DBGH(secInfo.pages[pn].cSize);

            DRW_DBG("\n    Page checksum = "); DRW_DBGH(SectionsMapBuf.getRawLong64());
            DRW_DBG("\n    Page CRC = "); DRW_DBGH(SectionsMapBuf.getRawLong64()); DRW_DBG("\n");
        }

        if (!secInfo.name.empty()) {
            secInfo.Id = nextId++;
            DRW_DBG("Saved section Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
            sections[secEnum::getEnum(secInfo.name)] = secInfo;
        }
    }

    if (! fileBuf->isGood())
        return false;

    DRW_DBG("\ndwgReader21::readFileHeader END\n");
    return true;
}