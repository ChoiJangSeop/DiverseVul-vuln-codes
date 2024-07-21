xmlDictComputeFastKey(const xmlChar *name, int namelen) {
    unsigned long value = 0L;

    if (name == NULL) return(0);
    value = *name;
    value <<= 5;
    if (namelen > 10) {
        value += name[namelen - 1];
        namelen = 10;
    }
    switch (namelen) {
        case 10: value += name[9];
        case 9: value += name[8];
        case 8: value += name[7];
        case 7: value += name[6];
        case 6: value += name[5];
        case 5: value += name[4];
        case 4: value += name[3];
        case 3: value += name[2];
        case 2: value += name[1];
        default: break;
    }
    return(value);
}