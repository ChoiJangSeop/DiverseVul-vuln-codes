TIFFWriteDirectoryTagSampleformatArray(TIFF* tif, uint32* ndir, TIFFDirEntry* dir, uint16 tag, uint32 count, double* value)
{
	static const char module[] = "TIFFWriteDirectoryTagSampleformatArray";
	void* conv;
	uint32 i;
	int ok;
	conv = _TIFFmalloc(count*sizeof(double));
	if (conv == NULL)
	{
		TIFFErrorExt(tif->tif_clientdata, module, "Out of memory");
		return (0);
	}

	switch (tif->tif_dir.td_sampleformat)
	{
		case SAMPLEFORMAT_IEEEFP:
			if (tif->tif_dir.td_bitspersample<=32)
			{
				for (i = 0; i < count; ++i)
					((float*)conv)[i] = (float)value[i];
				ok = TIFFWriteDirectoryTagFloatArray(tif,ndir,dir,tag,count,(float*)conv);
			}
			else
			{
				ok = TIFFWriteDirectoryTagDoubleArray(tif,ndir,dir,tag,count,value);
			}
			break;
		case SAMPLEFORMAT_INT:
			if (tif->tif_dir.td_bitspersample<=8)
			{
				for (i = 0; i < count; ++i)
					((int8*)conv)[i] = (int8)value[i];
				ok = TIFFWriteDirectoryTagSbyteArray(tif,ndir,dir,tag,count,(int8*)conv);
			}
			else if (tif->tif_dir.td_bitspersample<=16)
			{
				for (i = 0; i < count; ++i)
					((int16*)conv)[i] = (int16)value[i];
				ok = TIFFWriteDirectoryTagSshortArray(tif,ndir,dir,tag,count,(int16*)conv);
			}
			else
			{
				for (i = 0; i < count; ++i)
					((int32*)conv)[i] = (int32)value[i];
				ok = TIFFWriteDirectoryTagSlongArray(tif,ndir,dir,tag,count,(int32*)conv);
			}
			break;
		case SAMPLEFORMAT_UINT:
			if (tif->tif_dir.td_bitspersample<=8)
			{
				for (i = 0; i < count; ++i)
					((uint8*)conv)[i] = (uint8)value[i];
				ok = TIFFWriteDirectoryTagByteArray(tif,ndir,dir,tag,count,(uint8*)conv);
			}
			else if (tif->tif_dir.td_bitspersample<=16)
			{
				for (i = 0; i < count; ++i)
					((uint16*)conv)[i] = (uint16)value[i];
				ok = TIFFWriteDirectoryTagShortArray(tif,ndir,dir,tag,count,(uint16*)conv);
			}
			else
			{
				for (i = 0; i < count; ++i)
					((uint32*)conv)[i] = (uint32)value[i];
				ok = TIFFWriteDirectoryTagLongArray(tif,ndir,dir,tag,count,(uint32*)conv);
			}
			break;
		default:
			ok = 0;
	}

	_TIFFfree(conv);
	return (ok);
}