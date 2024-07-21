static status ParseFormat (AFfilehandle filehandle, AFvirtualfile *fp,
	uint32_t id, size_t size)
{
	_Track		*track;
	uint16_t	formatTag, channelCount;
	uint32_t	sampleRate, averageBytesPerSecond;
	uint16_t	blockAlign;
	_WAVEInfo	*wave;

	assert(filehandle != NULL);
	assert(fp != NULL);
	assert(!memcmp(&id, "fmt ", 4));

	track = _af_filehandle_get_track(filehandle, AF_DEFAULT_TRACK);

	assert(filehandle->formatSpecific != NULL);
	wave = (_WAVEInfo *) filehandle->formatSpecific;

	af_read_uint16_le(&formatTag, fp);
	af_read_uint16_le(&channelCount, fp);
	af_read_uint32_le(&sampleRate, fp);
	af_read_uint32_le(&averageBytesPerSecond, fp);
	af_read_uint16_le(&blockAlign, fp);

	track->f.channelCount = channelCount;
	track->f.sampleRate = sampleRate;
	track->f.byteOrder = AF_BYTEORDER_LITTLEENDIAN;

	/* Default to uncompressed audio data. */
	track->f.compressionType = AF_COMPRESSION_NONE;

	switch (formatTag)
	{
		case WAVE_FORMAT_PCM:
		{
			uint16_t	bitsPerSample;

			af_read_uint16_le(&bitsPerSample, fp);

			track->f.sampleWidth = bitsPerSample;

			if (bitsPerSample == 0 || bitsPerSample > 32)
			{
				_af_error(AF_BAD_WIDTH,
					"bad sample width of %d bits",
					bitsPerSample);
				return AF_FAIL;
			}

			if (bitsPerSample <= 8)
				track->f.sampleFormat = AF_SAMPFMT_UNSIGNED;
			else
				track->f.sampleFormat = AF_SAMPFMT_TWOSCOMP;
		}
		break;

		case WAVE_FORMAT_MULAW:
		case IBM_FORMAT_MULAW:
			track->f.sampleWidth = 16;
			track->f.sampleFormat = AF_SAMPFMT_TWOSCOMP;
			track->f.compressionType = AF_COMPRESSION_G711_ULAW;
			break;

		case WAVE_FORMAT_ALAW:
		case IBM_FORMAT_ALAW:
			track->f.sampleWidth = 16;
			track->f.sampleFormat = AF_SAMPFMT_TWOSCOMP;
			track->f.compressionType = AF_COMPRESSION_G711_ALAW;
			break;

		case WAVE_FORMAT_IEEE_FLOAT:
		{
			uint16_t	bitsPerSample;

			af_read_uint16_le(&bitsPerSample, fp);

			if (bitsPerSample == 64)
			{
				track->f.sampleWidth = 64;
				track->f.sampleFormat = AF_SAMPFMT_DOUBLE;
			}
			else
			{
				track->f.sampleWidth = 32;
				track->f.sampleFormat = AF_SAMPFMT_FLOAT;
			}
		}
		break;

		case WAVE_FORMAT_ADPCM:
		{
			uint16_t	bitsPerSample, extraByteCount,
					samplesPerBlock, numCoefficients;
			int		i;
			AUpvlist	pv;
			long		l;
			void		*v;

			if (track->f.channelCount != 1 &&
				track->f.channelCount != 2)
			{
				_af_error(AF_BAD_CHANNELS,
					"WAVE file with MS ADPCM compression "
					"must have 1 or 2 channels");
			}

			af_read_uint16_le(&bitsPerSample, fp);
			af_read_uint16_le(&extraByteCount, fp);
			af_read_uint16_le(&samplesPerBlock, fp);
			af_read_uint16_le(&numCoefficients, fp);

			/* numCoefficients should be at least 7. */
			assert(numCoefficients >= 7 && numCoefficients <= 255);

			for (i=0; i<numCoefficients; i++)
			{
				int16_t	a0, a1;

				af_fread(&a0, 1, 2, fp);
				af_fread(&a1, 1, 2, fp);

				a0 = LENDIAN_TO_HOST_INT16(a0);
				a1 = LENDIAN_TO_HOST_INT16(a1);

				wave->msadpcmCoefficients[i][0] = a0;
				wave->msadpcmCoefficients[i][1] = a1;
			}

			track->f.sampleWidth = 16;
			track->f.sampleFormat = AF_SAMPFMT_TWOSCOMP;
			track->f.compressionType = AF_COMPRESSION_MS_ADPCM;
			track->f.byteOrder = _AF_BYTEORDER_NATIVE;

			/* Create the parameter list. */
			pv = AUpvnew(4);
			AUpvsetparam(pv, 0, _AF_MS_ADPCM_NUM_COEFFICIENTS);
			AUpvsetvaltype(pv, 0, AU_PVTYPE_LONG);
			l = numCoefficients;
			AUpvsetval(pv, 0, &l);

			AUpvsetparam(pv, 1, _AF_MS_ADPCM_COEFFICIENTS);
			AUpvsetvaltype(pv, 1, AU_PVTYPE_PTR);
			v = wave->msadpcmCoefficients;
			AUpvsetval(pv, 1, &v);

			AUpvsetparam(pv, 2, _AF_SAMPLES_PER_BLOCK);
			AUpvsetvaltype(pv, 2, AU_PVTYPE_LONG);
			l = samplesPerBlock;
			AUpvsetval(pv, 2, &l);

			AUpvsetparam(pv, 3, _AF_BLOCK_SIZE);
			AUpvsetvaltype(pv, 3, AU_PVTYPE_LONG);
			l = blockAlign;
			AUpvsetval(pv, 3, &l);

			track->f.compressionParams = pv;
		}
		break;

		case WAVE_FORMAT_DVI_ADPCM:
		{
			AUpvlist	pv;
			long		l;

			uint16_t	bitsPerSample, extraByteCount,
					samplesPerBlock;

			af_read_uint16_le(&bitsPerSample, fp);
			af_read_uint16_le(&extraByteCount, fp);
			af_read_uint16_le(&samplesPerBlock, fp);

			track->f.sampleWidth = 16;
			track->f.sampleFormat = AF_SAMPFMT_TWOSCOMP;
			track->f.compressionType = AF_COMPRESSION_IMA;
			track->f.byteOrder = _AF_BYTEORDER_NATIVE;

			/* Create the parameter list. */
			pv = AUpvnew(2);
			AUpvsetparam(pv, 0, _AF_SAMPLES_PER_BLOCK);
			AUpvsetvaltype(pv, 0, AU_PVTYPE_LONG);
			l = samplesPerBlock;
			AUpvsetval(pv, 0, &l);

			AUpvsetparam(pv, 1, _AF_BLOCK_SIZE);
			AUpvsetvaltype(pv, 1, AU_PVTYPE_LONG);
			l = blockAlign;
			AUpvsetval(pv, 1, &l);

			track->f.compressionParams = pv;
		}
		break;

		case WAVE_FORMAT_YAMAHA_ADPCM:
		case WAVE_FORMAT_OKI_ADPCM:
		case WAVE_FORMAT_CREATIVE_ADPCM:
		case IBM_FORMAT_ADPCM:
			_af_error(AF_BAD_NOT_IMPLEMENTED, "WAVE ADPCM data format 0x%x is not currently supported", formatTag);
			return AF_FAIL;
			break;

		case WAVE_FORMAT_MPEG:
			_af_error(AF_BAD_NOT_IMPLEMENTED, "WAVE MPEG data format is not supported");
			return AF_FAIL;
			break;

		case WAVE_FORMAT_MPEGLAYER3:
			_af_error(AF_BAD_NOT_IMPLEMENTED, "WAVE MPEG layer 3 data format is not supported");
			return AF_FAIL;
			break;

		default:
			_af_error(AF_BAD_NOT_IMPLEMENTED, "WAVE file data format 0x%x not currently supported", formatTag);
			return AF_FAIL;
			break;
	}

	_af_set_sample_format(&track->f, track->f.sampleFormat, track->f.sampleWidth);

	return AF_SUCCEED;
}