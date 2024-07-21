void DRW_TableEntry::parseCode(int code, dxfReader *reader){
    switch (code) {
    case 5:
        handle = reader->getHandleString();
        break;
    case 330:
        parentHandle = reader->getHandleString();
        break;
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
        extData.push_back(new DRW_Variant(code, reader->getString()));
        break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
        curr = new DRW_Variant(code, DRW_Coord(reader->getDouble(), 0.0, 0.0));
        extData.push_back(curr);
        break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
        if (curr)
            curr->setCoordY(reader->getDouble());
        break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
        if (curr)
            curr->setCoordZ(reader->getDouble());
        curr=NULL;
        break;
    case 1040:
    case 1041:
    case 1042:
        extData.push_back(new DRW_Variant(code, reader->getDouble()));
        break;
    case 1070:
    case 1071:
        extData.push_back(new DRW_Variant(code, reader->getInt32() ));
        break;
    default:
        break;
    }
}