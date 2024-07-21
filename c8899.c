void freeListData(char *** data, unsigned int length) {
     for(int i=0; i<length; i++) {
        free((*data)[i]);
     }
     free(*data);
}