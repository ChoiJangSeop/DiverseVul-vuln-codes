static DexString *dex_string_new(RzBuffer *buf, ut64 offset, st64 *pread) {
	ut64 size = 0;
	char *data = NULL;
	st64 read;
	DexString *string = NULL;

	read = rz_buf_uleb128(buf, &size);
	data = malloc(size + 1);
	if (!data || rz_buf_read(buf, (ut8 *)data, size) != size) {
		free(data);
		return NULL;
	}
	data[size] = 0;

	string = RZ_NEW0(DexString);
	if (!string) {
		free(data);
		return NULL;
	}

	*pread = read;
	string->size = size;
	string->offset = offset;
	string->data = data;
	return string;
}