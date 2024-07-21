xmlDictComputeBigKey(const xmlChar* data, int namelen) {
    uint32_t hash;
    int i;

    if (namelen <= 0 || data == NULL) return(0);

    hash = 0;

    for (i = 0;i < namelen; i++) {
        hash += data[i];
	hash += (hash << 10);
	hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}