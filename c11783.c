int crxSetupImageData(crx_data_header_t *hdr, CrxImage *img, int16_t *outBuf,
                      uint64_t mdatOffset, uint32_t mdatSize,
                      uint8_t *mdatHdrPtr)
{
  int IncrBitTable[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0,
                          0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0};

  img->planeWidth = hdr->f_width;
  img->planeHeight = hdr->f_height;

  if (hdr->tileWidth < 0x16 || hdr->tileHeight < 0x16 ||
      img->planeWidth > 0x7FFF || img->planeHeight > 0x7FFF)
    return -1;

  img->tileCols = (img->planeWidth + hdr->tileWidth - 1) / hdr->tileWidth;
  img->tileRows = (img->planeHeight + hdr->tileHeight - 1) / hdr->tileHeight;

  if (img->tileCols > 0xFF || img->tileRows > 0xFF ||
      img->planeWidth - hdr->tileWidth * (img->tileCols - 1) < 0x16 ||
      img->planeHeight - hdr->tileHeight * (img->tileRows - 1) < 0x16)
    return -1;

  img->tiles = 0;
  img->levels = hdr->imageLevels;
  img->subbandCount = 3 * img->levels + 1; // 3 bands per level + one last LL
  img->nPlanes = hdr->nPlanes;
  img->nBits = hdr->nBits;
  img->encType = hdr->encType;
  img->samplePrecision = hdr->nBits + IncrBitTable[4 * hdr->encType + 2] + 1;
  img->mdatOffset = mdatOffset + hdr->mdatHdrSize;
  img->mdatSize = mdatSize;
  img->planeBuf = 0;
  img->outBufs[0] = img->outBufs[1] = img->outBufs[2] = img->outBufs[3] = 0;

  // The encoding type 3 needs all 4 planes to be decoded to generate row of
  // RGGB values. It seems to be using some other colour space for raw encoding
  // It is a massive buffer so ideallly it will need a different approach:
  // decode planes line by line and convert single line then without
  // intermediate plane buffer. At the moment though it's too many changes so
  // left as is.
  if (img->encType == 3 && img->nPlanes == 4 && img->nBits > 8)
  {
    img->planeBuf =
        (int16_t *)malloc(img->planeHeight * img->planeWidth * img->nPlanes *
                          ((img->samplePrecision + 7) >> 3));
    if (!img->planeBuf)
      return -1;
  }

  int32_t rowSize = 2 * img->planeWidth;

  if (img->nPlanes == 1)
    img->outBufs[0] = outBuf;
  else
    switch (hdr->cfaLayout)
    {
    case 0:
      // R G
      // G B
      img->outBufs[0] = outBuf;
      img->outBufs[1] = outBuf + 1;
      img->outBufs[2] = outBuf + rowSize;
      img->outBufs[3] = img->outBufs[2] + 1;
      break;
    case 1:
      // G R
      // B G
      img->outBufs[1] = outBuf;
      img->outBufs[0] = outBuf + 1;
      img->outBufs[3] = outBuf + rowSize;
      img->outBufs[2] = img->outBufs[3] + 1;
      break;
    case 2:
      // G B
      // R G
      img->outBufs[2] = outBuf;
      img->outBufs[3] = outBuf + 1;
      img->outBufs[0] = outBuf + rowSize;
      img->outBufs[1] = img->outBufs[0] + 1;
      break;
    case 3:
      // B G
      // G R
      img->outBufs[3] = outBuf;
      img->outBufs[2] = outBuf + 1;
      img->outBufs[1] = outBuf + rowSize;
      img->outBufs[0] = img->outBufs[1] + 1;
      break;
    }

  // read header
  return crxReadImageHeaders(hdr, img, mdatHdrPtr, mdatSize);
}