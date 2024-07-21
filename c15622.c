GF_Err urn_Read(GF_Box *s, GF_BitStream *bs)
{
	u32 i, to_read;
	char *tmpName;
	GF_DataEntryURNBox *ptr = (GF_DataEntryURNBox *)s;
	if (! ptr->size ) return GF_OK;

	//here we have to handle that in a clever way
	to_read = (u32) ptr->size;
	tmpName = (char*)gf_malloc(sizeof(char) * to_read);
	if (!tmpName) return GF_OUT_OF_MEM;
	//get the data
	gf_bs_read_data(bs, tmpName, to_read);

	//then get the break
	i = 0;
	while ( (tmpName[i] != 0) && (i < to_read) ) {
		i++;
	}
	//check the data is consistent
	if (i == to_read) {
		gf_free(tmpName);
		return GF_ISOM_INVALID_FILE;
	}
	//no NULL char, URL is not specified
	if (i == to_read - 1) {
		ptr->nameURN = tmpName;
		ptr->location = NULL;
		return GF_OK;
	}
	//OK, this has both URN and URL
	ptr->nameURN = (char*)gf_malloc(sizeof(char) * (i+1));
	if (!ptr->nameURN) {
		gf_free(tmpName);
		return GF_OUT_OF_MEM;
	}
	ptr->location = (char*)gf_malloc(sizeof(char) * (to_read - i - 1));
	if (!ptr->location) {
		gf_free(tmpName);
		gf_free(ptr->nameURN);
		ptr->nameURN = NULL;
		return GF_OUT_OF_MEM;
	}
	memcpy(ptr->nameURN, tmpName, i + 1);
	memcpy(ptr->location, tmpName + i + 1, (to_read - i - 1));
	gf_free(tmpName);
	return GF_OK;
}