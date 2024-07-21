pk11_numbits(CK_BYTE_PTR data, unsigned int bytecnt) {
	unsigned int bitcnt, i;
	CK_BYTE top;

	if (bytecnt == 0)
		return (0);
	bitcnt = bytecnt * 8;
	for (i = 0; i < bytecnt; i++) {
		top = data[i];
		if (top == 0) {
			bitcnt -= 8;
			continue;
		}
		if (top & 0x80)
			return (bitcnt);
		if (top & 0x40)
			return (bitcnt - 1);
		if (top & 0x20)
			return (bitcnt - 2);
		if (top & 0x10)
			return (bitcnt - 3);
		if (top & 0x08)
			return (bitcnt - 4);
		if (top & 0x04)
			return (bitcnt - 5);
		if (top & 0x02)
			return (bitcnt - 6);
		if (top & 0x01)
			return (bitcnt - 7);
		break;
	}
	INSIST(0);
	ISC_UNREACHABLE();
}