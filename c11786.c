int crxReadImageHeaders(crx_data_header_t *hdr, CrxImage *img, uint8_t *mdatPtr,
                        uint32_t mdatSize)
{
  int nTiles = img->tileRows * img->tileCols;

  if (!nTiles)
    return -1;

  if (!img->tiles)
  {
    img->tiles = (CrxTile *)malloc(
        sizeof(CrxTile) * nTiles +
        sizeof(CrxPlaneComp) * nTiles * img->nPlanes +
        sizeof(CrxSubband) * nTiles * img->nPlanes * img->subbandCount);
    if (!img->tiles)
      return -1;

    // memory areas in allocated chunk
    CrxTile *tile = img->tiles;
    CrxPlaneComp *comps = (CrxPlaneComp *)(tile + nTiles);
    CrxSubband *bands = (CrxSubband *)(comps + img->nPlanes * nTiles);

    for (int curTile = 0; curTile < nTiles; curTile++, tile++)
    {
      tile->tileFlag = 0; // tile neighbouring flags
      tile->tileNumber = curTile;
      tile->tileSize = 0;
      tile->comps = comps + curTile * img->nPlanes;

      if ((curTile + 1) % img->tileCols)
      {
        // not the last tile in a tile row
        tile->width = hdr->tileWidth;
        if (img->tileCols > 1)
        {
          tile->tileFlag = E_HAS_TILES_ON_THE_RIGHT;
          if (curTile % img->tileCols)
            // not the first tile in tile row
            tile->tileFlag |= E_HAS_TILES_ON_THE_LEFT;
        }
      }
      else
      {
        // last tile in a tile row
        tile->width = img->planeWidth - hdr->tileWidth * (img->tileCols - 1);
        if (img->tileCols > 1)
          tile->tileFlag = E_HAS_TILES_ON_THE_LEFT;
      }
      if (curTile < nTiles - img->tileCols)
      {
        // in first tile row
        tile->height = hdr->tileHeight;
        if (img->tileRows > 1)
        {
          tile->tileFlag |= E_HAS_TILES_ON_THE_BOTTOM;
          if (curTile >= img->tileCols)
            tile->tileFlag |= E_HAS_TILES_ON_THE_TOP;
        }
      }
      else
      {
        // non first tile row
        tile->height = img->planeHeight - hdr->tileHeight * (img->tileRows - 1);
        if (img->tileRows > 1)
          tile->tileFlag |= E_HAS_TILES_ON_THE_TOP;
      }
      if (img->nPlanes)
      {
        CrxPlaneComp *comp = tile->comps;
        CrxSubband *band = bands + curTile * img->nPlanes * img->subbandCount;

        for (int curComp = 0; curComp < img->nPlanes; curComp++, comp++)
        {
          comp->compNumber = curComp;
          comp->supportsPartial = 1;
          comp->tileFlag = tile->tileFlag;
          comp->subBands = band;
          comp->compBuf = 0;
          comp->waveletTransform = 0;
          if (img->subbandCount)
          {
            for (int curBand = 0; curBand < img->subbandCount;
                 curBand++, band++)
            {
              band->supportsPartial = 0;
              band->quantValue = 4;
              band->bandParam = 0;
              band->dataSize = 0;
            }
          }
        }
      }
    }
  }

  uint32_t tileOffset = 0;
  uint32_t dataSize = mdatSize;
  uint8_t *dataPtr = mdatPtr;
  CrxTile *tile = img->tiles;

  for (int curTile = 0; curTile < nTiles; curTile++, tile++)
  {
    if (dataSize < 0xC)
      return -1;

    if (LibRaw::sgetn(2, dataPtr) != 0xFF01)
      return -1;
    if (LibRaw::sgetn(2, dataPtr + 8) != curTile)
      return -1;

    dataSize -= 0xC;

    tile->tileSize = LibRaw::sgetn(4, dataPtr + 4);
    tile->dataOffset = tileOffset;

    int32_t hdrExtraBytes = LibRaw::sgetn(2, dataPtr + 2) - 8;
    tileOffset += tile->tileSize;
    dataPtr += hdrExtraBytes + 0xC;
    dataSize -= hdrExtraBytes;

    uint32_t compOffset = 0;
    CrxPlaneComp *comp = tile->comps;

    for (int compNum = 0; compNum < img->nPlanes; compNum++, comp++)
    {
      if (dataSize < 0xC)
        return -1;

      if (LibRaw::sgetn(2, dataPtr) != 0xFF02)
        return -1;
      if (compNum != dataPtr[8] >> 4)
        return -1;

      comp->compSize = LibRaw::sgetn(4, dataPtr + 4);

      int32_t compHdrRoundedBits = (dataPtr[8] >> 1) & 3;
      comp->supportsPartial = (dataPtr[8] & 8) != 0;

      comp->dataOffset = compOffset;
      comp->tileFlag = tile->tileFlag;

      compOffset += comp->compSize;
      dataSize -= 0xC;
      dataPtr += 0xC;

      comp->roundedBitsMask = 0;

      if (compHdrRoundedBits)
      {
        if (img->levels || !comp->supportsPartial)
          return -1;

        comp->roundedBitsMask = 1 << (compHdrRoundedBits - 1);
      }

      if (crxReadSubbandHeaders(hdr, img, tile, comp, &dataPtr, &dataSize))
        return -1;
    }
  }
  return 0;
}