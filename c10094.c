void t2p_read_tiff_size(T2P* t2p, TIFF* input){

	uint64* sbc=NULL;
#if defined(JPEG_SUPPORT) || defined (OJPEG_SUPPORT)
	unsigned char* jpt=NULL;
	tstrip_t i=0;
	tstrip_t stripcount=0;
#endif
        uint64 k = 0;

	if(t2p->pdf_transcode == T2P_TRANSCODE_RAW){
#ifdef CCITT_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_G4 ){
			TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc);
            if (sbc[0] != (uint64)(tmsize_t)sbc[0]) {
                TIFFError(TIFF2PDF_MODULE, "Integer overflow");
                t2p->t2p_error = T2P_ERR_ERROR;
            }
			t2p->tiff_datasize=(tmsize_t)sbc[0];
			return;
		}
#endif
#ifdef ZIP_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_ZIP){
			TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc);
            if (sbc[0] != (uint64)(tmsize_t)sbc[0]) {
                TIFFError(TIFF2PDF_MODULE, "Integer overflow");
                t2p->t2p_error = T2P_ERR_ERROR;
            }
			t2p->tiff_datasize=(tmsize_t)sbc[0];
			return;
		}
#endif
#ifdef OJPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_OJPEG){
			if(!TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc)){
				TIFFError(TIFF2PDF_MODULE, 
					"Input file %s missing field: TIFFTAG_STRIPBYTECOUNTS",
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			stripcount=TIFFNumberOfStrips(input);
			for(i=0;i<stripcount;i++){
				k = checkAdd64(k, sbc[i], t2p);
			}
			if(TIFFGetField(input, TIFFTAG_JPEGIFOFFSET, &(t2p->tiff_dataoffset))){
				if(t2p->tiff_dataoffset != 0){
					if(TIFFGetField(input, TIFFTAG_JPEGIFBYTECOUNT, &(t2p->tiff_datasize))!=0){
						if((uint64)t2p->tiff_datasize < k) {
							TIFFWarning(TIFF2PDF_MODULE, 
								"Input file %s has short JPEG interchange file byte count", 
								TIFFFileName(input));
							t2p->pdf_ojpegiflength=t2p->tiff_datasize;
							k = checkAdd64(k, t2p->tiff_datasize, t2p);
							k = checkAdd64(k, 6, t2p);
							k = checkAdd64(k, stripcount, t2p);
							k = checkAdd64(k, stripcount, t2p);
							t2p->tiff_datasize = (tsize_t) k;
							if ((uint64) t2p->tiff_datasize != k) {
								TIFFError(TIFF2PDF_MODULE, "Integer overflow");
								t2p->t2p_error = T2P_ERR_ERROR;
							}
							return;
						}
						return;
					}else {
						TIFFError(TIFF2PDF_MODULE, 
							"Input file %s missing field: TIFFTAG_JPEGIFBYTECOUNT",
							TIFFFileName(input));
							t2p->t2p_error = T2P_ERR_ERROR;
							return;
					}
				}
			}
			k = checkAdd64(k, stripcount, t2p);
			k = checkAdd64(k, stripcount, t2p);
			k = checkAdd64(k, 2048, t2p);
			t2p->tiff_datasize = (tsize_t) k;
			if ((uint64) t2p->tiff_datasize != k) {
				TIFFError(TIFF2PDF_MODULE, "Integer overflow");
				t2p->t2p_error = T2P_ERR_ERROR;
			}
			return;
		}
#endif
#ifdef JPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_JPEG) {
			uint32 count = 0;
			if(TIFFGetField(input, TIFFTAG_JPEGTABLES, &count, &jpt) != 0 ){
				if(count > 4){
					k += count;
					k -= 2; /* don't use EOI of header */
				}
			} else {
				k = 2; /* SOI for first strip */
			}
			stripcount=TIFFNumberOfStrips(input);
			if(!TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc)){
				TIFFError(TIFF2PDF_MODULE, 
					"Input file %s missing field: TIFFTAG_STRIPBYTECOUNTS",
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			for(i=0;i<stripcount;i++){
				k = checkAdd64(k, sbc[i], t2p);
				k -=2; /* don't use EOI of strip */
				k +=2; /* add space for restart marker */
			}
			k = checkAdd64(k, 2, t2p); /* use EOI of last strip */
			k = checkAdd64(k, 6, t2p); /* for DRI marker of first strip */
			t2p->tiff_datasize = (tsize_t) k;
			if ((uint64) t2p->tiff_datasize != k) {
				TIFFError(TIFF2PDF_MODULE, "Integer overflow");
				t2p->t2p_error = T2P_ERR_ERROR;
			}
			return;
		}
#endif
		(void) 0;
	}
	k = checkMultiply64(TIFFScanlineSize(input), t2p->tiff_length, t2p);
	if(t2p->tiff_planar==PLANARCONFIG_SEPARATE){
		k = checkMultiply64(k, t2p->tiff_samplesperpixel, t2p);
	}
	if (k == 0) {
		/* Assume we had overflow inside TIFFScanlineSize */
		t2p->t2p_error = T2P_ERR_ERROR;
	}

	t2p->tiff_datasize = (tsize_t) k;
	if ((uint64) t2p->tiff_datasize != k) {
		TIFFError(TIFF2PDF_MODULE, "Integer overflow");
		t2p->t2p_error = T2P_ERR_ERROR;
	}

	return;
}