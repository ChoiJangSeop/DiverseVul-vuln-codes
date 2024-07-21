TIFFWriteDirectoryTagCheckedRationalArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, float* value)
{
	static const char module[] = "TIFFWriteDirectoryTagCheckedRationalArray";
	uint32* m;
	float* na;
	uint32* nb;
	uint32 nc;
	int o;
	assert(sizeof(uint32)==4);
	m=_TIFFmalloc(count*2*sizeof(uint32));
	if (m==NULL)
	{
		TIFFErrorExt(tif->tif_clientdata,module,"Out of memory");
		return(0);
	}
	for (na=value, nb=m, nc=0; nc<count; na++, nb+=2, nc++)
	{
		if (*na<=0.0)
		{
			nb[0]=0;
			nb[1]=1;
		}
		else if (*na==(float)(uint32)(*na))
		{
			nb[0]=(uint32)(*na);
			nb[1]=1;
		}
		else if (*na<1.0)
		{
			nb[0]=(uint32)((double)(*na)*0xFFFFFFFF);
			nb[1]=0xFFFFFFFF;
		}
		else
		{
			nb[0]=0xFFFFFFFF;
			nb[1]=(uint32)((double)0xFFFFFFFF/(*na));
		}
	}
	if (tif->tif_flags&TIFF_SWAB)
		TIFFSwabArrayOfLong(m,count*2);
	o=TIFFWriteDirectoryTagData(tif,ndir,dir,tag,TIFF_RATIONAL,count,count*8,&m[0]);
	_TIFFfree(m);
	return(o);
}