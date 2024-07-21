int crxFreeImageData(CrxImage *img)
{
  CrxTile *tile = img->tiles;
  int nTiles = img->tileRows * img->tileCols;

  if (img->tiles)
  {
    for (int32_t curTile = 0; curTile < nTiles; curTile++, tile++)
      if (tile[curTile].comps)
        for (int32_t curPlane = 0; curPlane < img->nPlanes; curPlane++)
          crxFreeSubbandData(img, tile[curTile].comps + curPlane);
    free(img->tiles);
    img->tiles = 0;
  }

  if (img->planeBuf)
  {
    free(img->planeBuf);
    img->planeBuf = 0;
  }

  return 0;
}