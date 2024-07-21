void freeListData(char *** data, unsigned int length) {
	int i;
	for(i=0; i<length; i++) {
		free((*data)[i]);
	}
	free(*data);
}