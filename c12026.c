bool dwgReader18::readFileHeader() {

    if (! fileBuf->setPosition(0x80))
        return false;

//    genMagicNumber(); DBG("\n"); DBG("\n");
    DRW_DBG("Encrypted Header Data=\n");
    duint8 byteStr[0x6C];
    int size =0x6C;
    for (int i=0, j=0; i< 0x6C;i++) {
        duint8 ch = fileBuf->getRawChar8();
        DRW_DBGH(ch);
        if (j == 15) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
        byteStr[i] = DRW_magicNum18[i] ^ ch;
    }
    DRW_DBG("\n");

//    size =0x6C;
    DRW_DBG("Decrypted Header Data=\n");
    for (int i=0, j = 0; i< size;i++) {
        DRW_DBGH( static_cast<unsigned char>(byteStr[i]));
        if (j == 15) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
    }
    dwgBuffer buff(byteStr, 0x6C, &decoder);
    std::string name = reinterpret_cast<char*>(byteStr);
    DRW_DBG("\nFile ID string (AcFssFcAJMB)= "); DRW_DBG(name.c_str());
    //ID string + NULL = 12
    buff.setPosition(12);
    DRW_DBG("\n0x00 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x6c long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x04 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nRoot tree node gap= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLowermost left tree node gap= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLowermost right tree node gap= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nUnknown long (1)= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLast section page Id= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLast section page end address 64b= "); DRW_DBGH(buff.getRawLong64());
    DRW_DBG("\nStart of second header data address 64b= "); DRW_DBGH(buff.getRawLong64());
    DRW_DBG("\nGap amount= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nSection page amount= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x20 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x80 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x40 long= "); DRW_DBGH(buff.getRawLong32());
    dint32 secPageMapId = buff.getRawLong32();
    DRW_DBG("\nSection Page Map Id= "); DRW_DBGH(secPageMapId);
    duint64 secPageMapAddr = buff.getRawLong64()+0x100;
    DRW_DBG("\nSection Page Map address 64b= "); DRW_DBGH(secPageMapAddr);
    DRW_DBG("\nSection Page Map address 64b dec= "); DRW_DBG(secPageMapAddr);
    duint32 secMapId = buff.getRawLong32();
    DRW_DBG("\nSection Map Id= "); DRW_DBGH(secMapId);
    DRW_DBG("\nSection page array size= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nGap array size= "); DRW_DBGH(buff.getRawLong32());
    //TODO: verify CRC
    DRW_DBG("\nCRC32= "); DRW_DBGH(buff.getRawLong32());
    for (duint8 i = 0x68; i < 0x6c; ++i)
        byteStr[i] = '\0';
//    byteStr[i] = '\0';
    duint32 crcCalc = buff.crc32(0x00,0,0x6C);
    DRW_DBG("\nCRC32 calculated= "); DRW_DBGH(crcCalc);

    DRW_DBG("\nEnd Encrypted Data. Reads 0x14 bytes, equal to magic number:\n");
    for (int i=0, j=0; i< 0x14;i++) {
        DRW_DBG("magic num: "); DRW_DBGH( static_cast<unsigned char>(DRW_magicNumEnd18[i]));
        DRW_DBG(",read "); DRW_DBGH( static_cast<unsigned char>(fileBuf->getRawChar8()));
        if (j == 3) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
    }
// At this point are parsed the first 256 bytes
    DRW_DBG("\nJump to Section Page Map address: "); DRW_DBGH(secPageMapAddr);

    if (! fileBuf->setPosition(secPageMapAddr))
        return false;
    duint32 pageType = fileBuf->getRawLong32();
    DRW_DBG("\nSection page type= "); DRW_DBGH(pageType);
    duint32 decompSize = fileBuf->getRawLong32();
    DRW_DBG("\nDecompressed size= "); DRW_DBG(decompSize); DRW_DBG(", "); DRW_DBGH(decompSize);
    if (pageType != 0x41630e3b){
        //bad page type, ends
        DRW_DBG("Warning, bad page type, was expected 0x41630e3b instead of");  DRW_DBGH(pageType); DRW_DBG("\n");
        return false;
    }
    std::vector<duint8> tmpDecompSec(decompSize);
    parseSysPage(tmpDecompSec.data(), decompSize);

//parses "Section page map" decompressed data
    dwgBuffer buff2(tmpDecompSec.data(), decompSize, &decoder);
    duint32 address = 0x100;
    //stores temporarily info of all pages:
    std::unordered_map<duint64, dwgPageInfo >sectionPageMapTmp;

    for (unsigned int i = 0; i < decompSize;) {
        dint32 id = buff2.getRawLong32();//RLZ bad can be +/-
        duint32 size = buff2.getRawLong32();
        i += 8;
        DRW_DBG("Page num= "); DRW_DBG(id); DRW_DBG(" size= "); DRW_DBGH(size);
        DRW_DBG(" address= "); DRW_DBGH(address);  DRW_DBG("\n");
        //TODO num can be negative indicating gap
//        duint64 ind = id > 0 ? id : -id;
        if (id < 0){
            DRW_DBG("Parent= "); DRW_DBG(buff2.getRawLong32());
            DRW_DBG("\nLeft= "); DRW_DBG(buff2.getRawLong32());
            DRW_DBG(", Right= "); DRW_DBG(buff2.getRawLong32());
            DRW_DBG(", 0x00= ");DRW_DBGH(buff2.getRawLong32()); DRW_DBG("\n");
            i += 16;
        }

        sectionPageMapTmp[id] = dwgPageInfo(id, address, size);
        address += size;
    }

    DRW_DBG("\n*** dwgReader18: Processing Data Section Map ***\n");
    dwgPageInfo sectionMap = sectionPageMapTmp[secMapId];
    if (!fileBuf->setPosition(sectionMap.address))
        return false;
    pageType = fileBuf->getRawLong32();
    DRW_DBG("\nSection page type= "); DRW_DBGH(pageType);
    decompSize = fileBuf->getRawLong32();
    DRW_DBG("\nDecompressed size= "); DRW_DBG(decompSize); DRW_DBG(", "); DRW_DBGH(decompSize);
    if (pageType != 0x4163003b){
        //bad page type, ends
        DRW_DBG("Warning, bad page type, was expected 0x4163003b instead of");  DRW_DBGH(pageType); DRW_DBG("\n");
        return false;
    }
    tmpDecompSec.resize(decompSize);
    parseSysPage(tmpDecompSec.data(), decompSize);

//reads sections:
    DRW_DBG("\n*** dwgReader18: reads sections:");
    dwgBuffer buff3(tmpDecompSec.data(), decompSize, &decoder);
    duint32 numDescriptions = buff3.getRawLong32();
    DRW_DBG("\nnumDescriptions (sections)= "); DRW_DBG(numDescriptions);
    DRW_DBG("\n0x02 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\n0x00007400 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\n0x00 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\nunknown long (numDescriptions?)= "); DRW_DBG(buff3.getRawLong32()); DRW_DBG("\n");

    for (unsigned int i = 0; i < numDescriptions; i++) {
        dwgSectionInfo secInfo;
        secInfo.size = buff3.getRawLong64();
        DRW_DBG("\nSize of section= "); DRW_DBGH(secInfo.size);
        secInfo.pageCount = buff3.getRawLong32();
        DRW_DBG("\nPage count= "); DRW_DBGH(secInfo.pageCount);
        secInfo.maxSize = buff3.getRawLong32();
        DRW_DBG("\nMax Decompressed Size= "); DRW_DBGH(secInfo.maxSize);
        DRW_DBG("\nunknown long= "); DRW_DBGH(buff3.getRawLong32());
        secInfo.compressed = buff3.getRawLong32();
        DRW_DBG("\nis Compressed? 1:no, 2:yes= "); DRW_DBGH(secInfo.compressed);
        secInfo.Id = buff3.getRawLong32();
        DRW_DBG("\nSection Id= "); DRW_DBGH(secInfo.Id);
        secInfo.encrypted = buff3.getRawLong32();
        //encrypted (doc: 0 no, 1 yes, 2 unkn) on read: objects 0 and encrypted yes
        DRW_DBG("\nEncrypted= "); DRW_DBGH(secInfo.encrypted);
        duint8 nameCStr[64];
        buff3.getBytes(nameCStr, 64);
        secInfo.name = reinterpret_cast<char*>(nameCStr);
        DRW_DBG("\nSection std::Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
        for (unsigned int i = 0; i < secInfo.pageCount; i++){
            duint32 pn = buff3.getRawLong32();
            dwgPageInfo pi = sectionPageMapTmp[pn]; //get a copy
            DRW_DBG(" reading pag num = "); DRW_DBGH(pn);
            pi.dataSize = buff3.getRawLong32();
            pi.startOffset = buff3.getRawLong64();
            secInfo.pages[pn]= pi;//complete copy in secInfo
            DRW_DBG("\n    Page number= "); DRW_DBGH(secInfo.pages[pn].Id);
            DRW_DBG("\n    size in file= "); DRW_DBGH(secInfo.pages[pn].size);
            DRW_DBG("\n    address in file= "); DRW_DBGH(secInfo.pages[pn].address);
            DRW_DBG("\n    Data size= "); DRW_DBGH(secInfo.pages[pn].dataSize);
            DRW_DBG("\n    Start offset= "); DRW_DBGH(secInfo.pages[pn].startOffset); DRW_DBG("\n");
        }
        //do not save empty section
        if (!secInfo.name.empty()) {
            DRW_DBG("Saved section Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
            sections[secEnum::getEnum(secInfo.name)] = secInfo;
        }
    }

    if (! fileBuf->isGood())
        return false;
    DRW_DBG("\ndwgReader18::readFileHeader END\n\n");
    return true;
}