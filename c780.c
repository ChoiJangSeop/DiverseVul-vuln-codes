xmlHashComputeQKey(xmlHashTablePtr table,
		   const xmlChar *prefix, const xmlChar *name,
		   const xmlChar *prefix2, const xmlChar *name2,
		   const xmlChar *prefix3, const xmlChar *name3) {
    unsigned long value = 0L;
    char ch;
    
    if (prefix != NULL)
	value += 30 * (*prefix);
    else
	value += 30 * (*name);

    if (prefix != NULL) {
	while ((ch = *prefix++) != 0) {
	    value = value ^ ((value << 5) + (value >> 3) + (unsigned long)ch);
	}
	value = value ^ ((value << 5) + (value >> 3) + (unsigned long)':');
    }
    if (name != NULL) {
	while ((ch = *name++) != 0) {
	    value = value ^ ((value << 5) + (value >> 3) + (unsigned long)ch);
	}
    }
    if (prefix2 != NULL) {
	while ((ch = *prefix2++) != 0) {
	    value = value ^ ((value << 5) + (value >> 3) + (unsigned long)ch);
	}
	value = value ^ ((value << 5) + (value >> 3) + (unsigned long)':');
    }
    if (name2 != NULL) {
	while ((ch = *name2++) != 0) {
	    value = value ^ ((value << 5) + (value >> 3) + (unsigned long)ch);
	}
    }
    if (prefix3 != NULL) {
	while ((ch = *prefix3++) != 0) {
	    value = value ^ ((value << 5) + (value >> 3) + (unsigned long)ch);
	}
	value = value ^ ((value << 5) + (value >> 3) + (unsigned long)':');
    }
    if (name3 != NULL) {
	while ((ch = *name3++) != 0) {
	    value = value ^ ((value << 5) + (value >> 3) + (unsigned long)ch);
	}
    }
    return (value % table->size);
}