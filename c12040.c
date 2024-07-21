bool dwgReader21::parseSysPage(duint64 sizeCompressed, duint64 sizeUncompressed, duint64 correctionFactor, duint64 offset, duint8 *decompData){
    //round to 8
    duint64 alsize = (sizeCompressed + 7) &(-8);
    //minimum RS chunk:
    duint32 chunks = (((alsize * correctionFactor)+238)/239);
    duint64 fpsize = chunks * 255;

    if (! fileBuf->setPosition(offset))
        return false;
    std::vector<duint8> tmpDataRaw(fpsize);
    fileBuf->getBytes(&tmpDataRaw.front(), fpsize);
    std::vector<duint8> tmpDataRS(fpsize);
    dwgRSCodec::decode239I(&tmpDataRaw.front(), &tmpDataRS.front(), fpsize/255);
    dwgCompressor::decompress21(&tmpDataRS.front(), decompData, sizeCompressed, sizeUncompressed);
    return true;
}