PickContigCase(TIFFRGBAImage* img)
{
	img->get = TIFFIsTiled(img->tif) ? gtTileContig : gtStripContig;
	img->put.contig = NULL;
	switch (img->photometric) {
		case PHOTOMETRIC_RGB:
			switch (img->bitspersample) {
				case 8:
					if (img->alpha == EXTRASAMPLE_ASSOCALPHA)
						img->put.contig = putRGBAAcontig8bittile;
					else if (img->alpha == EXTRASAMPLE_UNASSALPHA)
					{
						if (BuildMapUaToAa(img))
							img->put.contig = putRGBUAcontig8bittile;
					}
					else
						img->put.contig = putRGBcontig8bittile;
					break;
				case 16:
					if (img->alpha == EXTRASAMPLE_ASSOCALPHA)
					{
						if (BuildMapBitdepth16To8(img))
							img->put.contig = putRGBAAcontig16bittile;
					}
					else if (img->alpha == EXTRASAMPLE_UNASSALPHA)
					{
						if (BuildMapBitdepth16To8(img) &&
						    BuildMapUaToAa(img))
							img->put.contig = putRGBUAcontig16bittile;
					}
					else
					{
						if (BuildMapBitdepth16To8(img))
							img->put.contig = putRGBcontig16bittile;
					}
					break;
			}
			break;
		case PHOTOMETRIC_SEPARATED:
			if (buildMap(img)) {
				if (img->bitspersample == 8) {
					if (!img->Map)
						img->put.contig = putRGBcontig8bitCMYKtile;
					else
						img->put.contig = putRGBcontig8bitCMYKMaptile;
				}
			}
			break;
		case PHOTOMETRIC_PALETTE:
			if (buildMap(img)) {
				switch (img->bitspersample) {
					case 8:
						img->put.contig = put8bitcmaptile;
						break;
					case 4:
						img->put.contig = put4bitcmaptile;
						break;
					case 2:
						img->put.contig = put2bitcmaptile;
						break;
					case 1:
						img->put.contig = put1bitcmaptile;
						break;
				}
			}
			break;
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK:
			if (buildMap(img)) {
				switch (img->bitspersample) {
					case 16:
						img->put.contig = put16bitbwtile;
						break;
					case 8:
						if (img->alpha && img->samplesperpixel == 2)
							img->put.contig = putagreytile;
						else
							img->put.contig = putgreytile;
						break;
					case 4:
						img->put.contig = put4bitbwtile;
						break;
					case 2:
						img->put.contig = put2bitbwtile;
						break;
					case 1:
						img->put.contig = put1bitbwtile;
						break;
				}
			}
			break;
		case PHOTOMETRIC_YCBCR:
			if ((img->bitspersample==8) && (img->samplesperpixel==3))
			{
				if (initYCbCrConversion(img)!=0)
				{
					/*
					 * The 6.0 spec says that subsampling must be
					 * one of 1, 2, or 4, and that vertical subsampling
					 * must always be <= horizontal subsampling; so
					 * there are only a few possibilities and we just
					 * enumerate the cases.
					 * Joris: added support for the [1,2] case, nonetheless, to accommodate
					 * some OJPEG files
					 */
					uint16 SubsamplingHor;
					uint16 SubsamplingVer;
					TIFFGetFieldDefaulted(img->tif, TIFFTAG_YCBCRSUBSAMPLING, &SubsamplingHor, &SubsamplingVer);
					switch ((SubsamplingHor<<4)|SubsamplingVer) {
						case 0x44:
							img->put.contig = putcontig8bitYCbCr44tile;
							break;
						case 0x42:
							img->put.contig = putcontig8bitYCbCr42tile;
							break;
						case 0x41:
							img->put.contig = putcontig8bitYCbCr41tile;
							break;
						case 0x22:
							img->put.contig = putcontig8bitYCbCr22tile;
							break;
						case 0x21:
							img->put.contig = putcontig8bitYCbCr21tile;
							break;
						case 0x12:
							img->put.contig = putcontig8bitYCbCr12tile;
							break;
						case 0x11:
							img->put.contig = putcontig8bitYCbCr11tile;
							break;
					}
				}
			}
			break;
		case PHOTOMETRIC_CIELAB:
			if (buildMap(img)) {
				if (img->bitspersample == 8)
					img->put.contig = initCIELabConversion(img);
				break;
			}
	}
	return ((img->get!=NULL) && (img->put.contig!=NULL));
}