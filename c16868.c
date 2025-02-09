_TIFFVSetField(TIFF* tif, uint32 tag, va_list ap)
{
	static const char module[] = "_TIFFVSetField";

	TIFFDirectory* td = &tif->tif_dir;
	int status = 1;
	uint32 v32, i, v;
    double dblval;
	char* s;
	const TIFFField *fip = TIFFFindField(tif, tag, TIFF_ANY);
	uint32 standard_tag = tag;
	if( fip == NULL ) /* cannot happen since OkToChangeTag() already checks it */
	    return 0;
	/*
	 * We want to force the custom code to be used for custom
	 * fields even if the tag happens to match a well known 
	 * one - important for reinterpreted handling of standard
	 * tag values in custom directories (i.e. EXIF) 
	 */
	if (fip->field_bit == FIELD_CUSTOM) {
		standard_tag = 0;
	}

	switch (standard_tag) {
	case TIFFTAG_SUBFILETYPE:
		td->td_subfiletype = (uint32) va_arg(ap, uint32);
		break;
	case TIFFTAG_IMAGEWIDTH:
		td->td_imagewidth = (uint32) va_arg(ap, uint32);
		break;
	case TIFFTAG_IMAGELENGTH:
		td->td_imagelength = (uint32) va_arg(ap, uint32);
		break;
	case TIFFTAG_BITSPERSAMPLE:
		td->td_bitspersample = (uint16) va_arg(ap, uint16_vap);
		/*
		 * If the data require post-decoding processing to byte-swap
		 * samples, set it up here.  Note that since tags are required
		 * to be ordered, compression code can override this behaviour
		 * in the setup method if it wants to roll the post decoding
		 * work in with its normal work.
		 */
		if (tif->tif_flags & TIFF_SWAB) {
			if (td->td_bitspersample == 8)
				tif->tif_postdecode = _TIFFNoPostDecode;
			else if (td->td_bitspersample == 16)
				tif->tif_postdecode = _TIFFSwab16BitData;
			else if (td->td_bitspersample == 24)
				tif->tif_postdecode = _TIFFSwab24BitData;
			else if (td->td_bitspersample == 32)
				tif->tif_postdecode = _TIFFSwab32BitData;
			else if (td->td_bitspersample == 64)
				tif->tif_postdecode = _TIFFSwab64BitData;
			else if (td->td_bitspersample == 128) /* two 64's */
				tif->tif_postdecode = _TIFFSwab64BitData;
		}
		break;
	case TIFFTAG_COMPRESSION:
		v = (uint16) va_arg(ap, uint16_vap);
		/*
		 * If we're changing the compression scheme, the notify the
		 * previous module so that it can cleanup any state it's
		 * setup.
		 */
		if (TIFFFieldSet(tif, FIELD_COMPRESSION)) {
			if ((uint32)td->td_compression == v)
				break;
			(*tif->tif_cleanup)(tif);
			tif->tif_flags &= ~TIFF_CODERSETUP;
		}
		/*
		 * Setup new compression routine state.
		 */
		if( (status = TIFFSetCompressionScheme(tif, v)) != 0 )
		    td->td_compression = (uint16) v;
		else
		    status = 0;
		break;
	case TIFFTAG_PHOTOMETRIC:
		td->td_photometric = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_THRESHHOLDING:
		td->td_threshholding = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_FILLORDER:
		v = (uint16) va_arg(ap, uint16_vap);
		if (v != FILLORDER_LSB2MSB && v != FILLORDER_MSB2LSB)
			goto badvalue;
		td->td_fillorder = (uint16) v;
		break;
	case TIFFTAG_ORIENTATION:
		v = (uint16) va_arg(ap, uint16_vap);
		if (v < ORIENTATION_TOPLEFT || ORIENTATION_LEFTBOT < v)
			goto badvalue;
		else
			td->td_orientation = (uint16) v;
		break;
	case TIFFTAG_SAMPLESPERPIXEL:
		v = (uint16) va_arg(ap, uint16_vap);
		if (v == 0)
			goto badvalue;
        if( v != td->td_samplesperpixel )
        {
            /* See http://bugzilla.maptools.org/show_bug.cgi?id=2500 */
            if( td->td_sminsamplevalue != NULL )
            {
                TIFFWarningExt(tif->tif_clientdata,module,
                    "SamplesPerPixel tag value is changing, "
                    "but SMinSampleValue tag was read with a different value. Cancelling it");
                TIFFClrFieldBit(tif,FIELD_SMINSAMPLEVALUE);
                _TIFFfree(td->td_sminsamplevalue);
                td->td_sminsamplevalue = NULL;
            }
            if( td->td_smaxsamplevalue != NULL )
            {
                TIFFWarningExt(tif->tif_clientdata,module,
                    "SamplesPerPixel tag value is changing, "
                    "but SMaxSampleValue tag was read with a different value. Cancelling it");
                TIFFClrFieldBit(tif,FIELD_SMAXSAMPLEVALUE);
                _TIFFfree(td->td_smaxsamplevalue);
                td->td_smaxsamplevalue = NULL;
            }
        }
		td->td_samplesperpixel = (uint16) v;
		break;
	case TIFFTAG_ROWSPERSTRIP:
		v32 = (uint32) va_arg(ap, uint32);
		if (v32 == 0)
			goto badvalue32;
		td->td_rowsperstrip = v32;
		if (!TIFFFieldSet(tif, FIELD_TILEDIMENSIONS)) {
			td->td_tilelength = v32;
			td->td_tilewidth = td->td_imagewidth;
		}
		break;
	case TIFFTAG_MINSAMPLEVALUE:
		td->td_minsamplevalue = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_MAXSAMPLEVALUE:
		td->td_maxsamplevalue = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_SMINSAMPLEVALUE:
		if (tif->tif_flags & TIFF_PERSAMPLE)
			_TIFFsetDoubleArray(&td->td_sminsamplevalue, va_arg(ap, double*), td->td_samplesperpixel);
		else
			setDoubleArrayOneValue(&td->td_sminsamplevalue, va_arg(ap, double), td->td_samplesperpixel);
		break;
	case TIFFTAG_SMAXSAMPLEVALUE:
		if (tif->tif_flags & TIFF_PERSAMPLE)
			_TIFFsetDoubleArray(&td->td_smaxsamplevalue, va_arg(ap, double*), td->td_samplesperpixel);
		else
			setDoubleArrayOneValue(&td->td_smaxsamplevalue, va_arg(ap, double), td->td_samplesperpixel);
		break;
	case TIFFTAG_XRESOLUTION:
        dblval = va_arg(ap, double);
        if( dblval < 0 )
            goto badvaluedouble;
		td->td_xresolution = (float) dblval;
		break;
	case TIFFTAG_YRESOLUTION:
        dblval = va_arg(ap, double);
        if( dblval < 0 )
            goto badvaluedouble;
		td->td_yresolution = (float) dblval;
		break;
	case TIFFTAG_PLANARCONFIG:
		v = (uint16) va_arg(ap, uint16_vap);
		if (v != PLANARCONFIG_CONTIG && v != PLANARCONFIG_SEPARATE)
			goto badvalue;
		td->td_planarconfig = (uint16) v;
		break;
	case TIFFTAG_XPOSITION:
		td->td_xposition = (float) va_arg(ap, double);
		break;
	case TIFFTAG_YPOSITION:
		td->td_yposition = (float) va_arg(ap, double);
		break;
	case TIFFTAG_RESOLUTIONUNIT:
		v = (uint16) va_arg(ap, uint16_vap);
		if (v < RESUNIT_NONE || RESUNIT_CENTIMETER < v)
			goto badvalue;
		td->td_resolutionunit = (uint16) v;
		break;
	case TIFFTAG_PAGENUMBER:
		td->td_pagenumber[0] = (uint16) va_arg(ap, uint16_vap);
		td->td_pagenumber[1] = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_HALFTONEHINTS:
		td->td_halftonehints[0] = (uint16) va_arg(ap, uint16_vap);
		td->td_halftonehints[1] = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_COLORMAP:
		v32 = (uint32)(1L<<td->td_bitspersample);
		_TIFFsetShortArray(&td->td_colormap[0], va_arg(ap, uint16*), v32);
		_TIFFsetShortArray(&td->td_colormap[1], va_arg(ap, uint16*), v32);
		_TIFFsetShortArray(&td->td_colormap[2], va_arg(ap, uint16*), v32);
		break;
	case TIFFTAG_EXTRASAMPLES:
		if (!setExtraSamples(td, ap, &v))
			goto badvalue;
		break;
	case TIFFTAG_MATTEING:
		td->td_extrasamples =  (((uint16) va_arg(ap, uint16_vap)) != 0);
		if (td->td_extrasamples) {
			uint16 sv = EXTRASAMPLE_ASSOCALPHA;
			_TIFFsetShortArray(&td->td_sampleinfo, &sv, 1);
		}
		break;
	case TIFFTAG_TILEWIDTH:
		v32 = (uint32) va_arg(ap, uint32);
		if (v32 % 16) {
			if (tif->tif_mode != O_RDONLY)
				goto badvalue32;
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
				"Nonstandard tile width %d, convert file", v32);
		}
		td->td_tilewidth = v32;
		tif->tif_flags |= TIFF_ISTILED;
		break;
	case TIFFTAG_TILELENGTH:
		v32 = (uint32) va_arg(ap, uint32);
		if (v32 % 16) {
			if (tif->tif_mode != O_RDONLY)
				goto badvalue32;
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
			    "Nonstandard tile length %d, convert file", v32);
		}
		td->td_tilelength = v32;
		tif->tif_flags |= TIFF_ISTILED;
		break;
	case TIFFTAG_TILEDEPTH:
		v32 = (uint32) va_arg(ap, uint32);
		if (v32 == 0)
			goto badvalue32;
		td->td_tiledepth = v32;
		break;
	case TIFFTAG_DATATYPE:
		v = (uint16) va_arg(ap, uint16_vap);
		switch (v) {
		case DATATYPE_VOID:	v = SAMPLEFORMAT_VOID;	break;
		case DATATYPE_INT:	v = SAMPLEFORMAT_INT;	break;
		case DATATYPE_UINT:	v = SAMPLEFORMAT_UINT;	break;
		case DATATYPE_IEEEFP:	v = SAMPLEFORMAT_IEEEFP;break;
		default:		goto badvalue;
		}
		td->td_sampleformat = (uint16) v;
		break;
	case TIFFTAG_SAMPLEFORMAT:
		v = (uint16) va_arg(ap, uint16_vap);
		if (v < SAMPLEFORMAT_UINT || SAMPLEFORMAT_COMPLEXIEEEFP < v)
			goto badvalue;
		td->td_sampleformat = (uint16) v;

		/*  Try to fix up the SWAB function for complex data. */
		if( td->td_sampleformat == SAMPLEFORMAT_COMPLEXINT
		    && td->td_bitspersample == 32
		    && tif->tif_postdecode == _TIFFSwab32BitData )
		    tif->tif_postdecode = _TIFFSwab16BitData;
		else if( (td->td_sampleformat == SAMPLEFORMAT_COMPLEXINT
			  || td->td_sampleformat == SAMPLEFORMAT_COMPLEXIEEEFP)
			 && td->td_bitspersample == 64
			 && tif->tif_postdecode == _TIFFSwab64BitData )
		    tif->tif_postdecode = _TIFFSwab32BitData;
		break;
	case TIFFTAG_IMAGEDEPTH:
		td->td_imagedepth = (uint32) va_arg(ap, uint32);
		break;
	case TIFFTAG_SUBIFD:
		if ((tif->tif_flags & TIFF_INSUBIFD) == 0) {
			td->td_nsubifd = (uint16) va_arg(ap, uint16_vap);
			_TIFFsetLong8Array(&td->td_subifd, (uint64*) va_arg(ap, uint64*),
			    (uint32) td->td_nsubifd);
		} else {
			TIFFErrorExt(tif->tif_clientdata, module,
				     "%s: Sorry, cannot nest SubIFDs",
				     tif->tif_name);
			status = 0;
		}
		break;
	case TIFFTAG_YCBCRPOSITIONING:
		td->td_ycbcrpositioning = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_YCBCRSUBSAMPLING:
		td->td_ycbcrsubsampling[0] = (uint16) va_arg(ap, uint16_vap);
		td->td_ycbcrsubsampling[1] = (uint16) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_TRANSFERFUNCTION:
		v = (td->td_samplesperpixel - td->td_extrasamples) > 1 ? 3 : 1;
		for (i = 0; i < v; i++)
			_TIFFsetShortArray(&td->td_transferfunction[i],
			    va_arg(ap, uint16*), 1U<<td->td_bitspersample);
		break;
	case TIFFTAG_REFERENCEBLACKWHITE:
		/* XXX should check for null range */
		_TIFFsetFloatArray(&td->td_refblackwhite, va_arg(ap, float*), 6);
		break;
	case TIFFTAG_INKNAMES:
		v = (uint16) va_arg(ap, uint16_vap);
		s = va_arg(ap, char*);
		v = checkInkNamesString(tif, v, s);
		status = v > 0;
		if( v > 0 ) {
			_TIFFsetNString(&td->td_inknames, s, v);
			td->td_inknameslen = v;
		}
		break;
	case TIFFTAG_PERSAMPLE:
		v = (uint16) va_arg(ap, uint16_vap);
		if( v == PERSAMPLE_MULTI )
			tif->tif_flags |= TIFF_PERSAMPLE;
		else
			tif->tif_flags &= ~TIFF_PERSAMPLE;
		break;
	default: {
		TIFFTagValue *tv;
		int tv_size, iCustom;

		/*
		 * This can happen if multiple images are open with different
		 * codecs which have private tags.  The global tag information
		 * table may then have tags that are valid for one file but not
		 * the other. If the client tries to set a tag that is not valid
		 * for the image's codec then we'll arrive here.  This
		 * happens, for example, when tiffcp is used to convert between
		 * compression schemes and codec-specific tags are blindly copied.
		 */
		if(fip->field_bit != FIELD_CUSTOM) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Invalid %stag \"%s\" (not supported by codec)",
			    tif->tif_name, isPseudoTag(tag) ? "pseudo-" : "",
			    fip->field_name);
			status = 0;
			break;
		}

		/*
		 * Find the existing entry for this custom value.
		 */
		tv = NULL;
		for (iCustom = 0; iCustom < td->td_customValueCount; iCustom++) {
			if (td->td_customValues[iCustom].info->field_tag == tag) {
				tv = td->td_customValues + iCustom;
				if (tv->value != NULL) {
					_TIFFfree(tv->value);
					tv->value = NULL;
				}
				break;
			}
		}

		/*
		 * Grow the custom list if the entry was not found.
		 */
		if(tv == NULL) {
			TIFFTagValue *new_customValues;

			td->td_customValueCount++;
			new_customValues = (TIFFTagValue *)
			    _TIFFrealloc(td->td_customValues,
			    sizeof(TIFFTagValue) * td->td_customValueCount);
			if (!new_customValues) {
				TIFFErrorExt(tif->tif_clientdata, module,
				    "%s: Failed to allocate space for list of custom values",
				    tif->tif_name);
				status = 0;
				goto end;
			}

			td->td_customValues = new_customValues;

			tv = td->td_customValues + (td->td_customValueCount - 1);
			tv->info = fip;
			tv->value = NULL;
			tv->count = 0;
		}

		/*
		 * Set custom value ... save a copy of the custom tag value.
		 */
		tv_size = _TIFFDataSize(fip->field_type);
		if (tv_size == 0) {
			status = 0;
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Bad field type %d for \"%s\"",
			    tif->tif_name, fip->field_type,
			    fip->field_name);
			goto end;
		}

		if (fip->field_type == TIFF_ASCII)
		{
			uint32 ma;
			char* mb;
			if (fip->field_passcount)
			{
				assert(fip->field_writecount==TIFF_VARIABLE2);
				ma=(uint32)va_arg(ap,uint32);
				mb=(char*)va_arg(ap,char*);
			}
			else
			{
				mb=(char*)va_arg(ap,char*);
				ma=(uint32)(strlen(mb)+1);
			}
			tv->count=ma;
			setByteArray(&tv->value,mb,ma,1);
		}
		else
		{
			if (fip->field_passcount) {
				if (fip->field_writecount == TIFF_VARIABLE2)
					tv->count = (uint32) va_arg(ap, uint32);
				else
					tv->count = (int) va_arg(ap, int);
			} else if (fip->field_writecount == TIFF_VARIABLE
			   || fip->field_writecount == TIFF_VARIABLE2)
				tv->count = 1;
			else if (fip->field_writecount == TIFF_SPP)
				tv->count = td->td_samplesperpixel;
			else
				tv->count = fip->field_writecount;

			if (tv->count == 0) {
				status = 0;
				TIFFErrorExt(tif->tif_clientdata, module,
					     "%s: Null count for \"%s\" (type "
					     "%d, writecount %d, passcount %d)",
					     tif->tif_name,
					     fip->field_name,
					     fip->field_type,
					     fip->field_writecount,
					     fip->field_passcount);
				goto end;
			}

			tv->value = _TIFFCheckMalloc(tif, tv->count, tv_size,
			    "custom tag binary object");
			if (!tv->value) {
				status = 0;
				goto end;
			}

			if (fip->field_tag == TIFFTAG_DOTRANGE 
			    && strcmp(fip->field_name,"DotRange") == 0) {
				/* TODO: This is an evil exception and should not have been
				   handled this way ... likely best if we move it into
				   the directory structure with an explicit field in 
				   libtiff 4.1 and assign it a FIELD_ value */
				uint16 v2[2];
				v2[0] = (uint16)va_arg(ap, int);
				v2[1] = (uint16)va_arg(ap, int);
				_TIFFmemcpy(tv->value, &v2, 4);
			}

			else if (fip->field_passcount
				  || fip->field_writecount == TIFF_VARIABLE
				  || fip->field_writecount == TIFF_VARIABLE2
				  || fip->field_writecount == TIFF_SPP
				  || tv->count > 1) {
				_TIFFmemcpy(tv->value, va_arg(ap, void *),
				    tv->count * tv_size);
			} else {
				char *val = (char *)tv->value;
				assert( tv->count == 1 );

				switch (fip->field_type) {
				case TIFF_BYTE:
				case TIFF_UNDEFINED:
					{
						uint8 v2 = (uint8)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SBYTE:
					{
						int8 v2 = (int8)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SHORT:
					{
						uint16 v2 = (uint16)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SSHORT:
					{
						int16 v2 = (int16)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_LONG:
				case TIFF_IFD:
					{
						uint32 v2 = va_arg(ap, uint32);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SLONG:
					{
						int32 v2 = va_arg(ap, int32);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_LONG8:
				case TIFF_IFD8:
					{
						uint64 v2 = va_arg(ap, uint64);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SLONG8:
					{
						int64 v2 = va_arg(ap, int64);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_RATIONAL:
				case TIFF_SRATIONAL:
				case TIFF_FLOAT:
					{
						float v2 = (float)va_arg(ap, double);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_DOUBLE:
					{
						double v2 = va_arg(ap, double);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				default:
					_TIFFmemset(val, 0, tv_size);
					status = 0;
					break;
				}
			}
		}
	}
	}
	if (status) {
		const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
		if (fip2)                
			TIFFSetFieldBit(tif, fip2->field_bit);
		tif->tif_flags |= TIFF_DIRTYDIRECT;
	}

end:
	va_end(ap);
	return (status);
badvalue:
        {
		const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
		TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Bad value %u for \"%s\" tag",
		     tif->tif_name, v,
		     fip2 ? fip2->field_name : "Unknown");
		va_end(ap);
        }
	return (0);
badvalue32:
        {
		const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
		TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Bad value %u for \"%s\" tag",
		     tif->tif_name, v32,
		     fip2 ? fip2->field_name : "Unknown");
		va_end(ap);
        }
	return (0);
badvaluedouble:
        {
        const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
        TIFFErrorExt(tif->tif_clientdata, module,
             "%s: Bad value %f for \"%s\" tag",
             tif->tif_name, dblval,
             fip2 ? fip2->field_name : "Unknown");
        va_end(ap);
        }
    return (0);
}