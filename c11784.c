int crxSetupSubbandData(CrxImage *img, CrxPlaneComp *planeComp,
                        const CrxTile *tile, uint32_t mdatOffset)
{
  long compDataSize = 0;
  long waveletDataOffset = 0;
  long compCoeffDataOffset = 0;
  int32_t toSubbands = 3 * img->levels + 1;
  int32_t transformWidth = 0;

  CrxSubband *subbands = planeComp->subBands;

  // calculate sizes
  for (int32_t subbandNum = 0; subbandNum < toSubbands; subbandNum++)
  {
    subbands[subbandNum].bandSize =
        subbands[subbandNum].width * sizeof(int32_t); // 4bytes
    compDataSize += subbands[subbandNum].bandSize;
  }

  if (img->levels)
  {
    int32_t encLevels = img->levels ? img->levels : 1;
    waveletDataOffset = (compDataSize + 7) & ~7;
    compDataSize =
        (sizeof(CrxWaveletTransform) * encLevels + waveletDataOffset + 7) & ~7;
    compCoeffDataOffset = compDataSize;

    // calc wavelet line buffer sizes (always at one level up from current)
    for (int level = 0; level < img->levels; ++level)
      if (level < img->levels - 1)
        compDataSize += 8 * sizeof(int32_t) *
                        planeComp->subBands[3 * (level + 1) + 2].width;
      else
        compDataSize += 8 * sizeof(int32_t) * tile->width;
  }

  // buffer allocation
  planeComp->compBuf = (uint8_t *)malloc(compDataSize);
  if (!planeComp->compBuf)
    return -1;

  // subbands buffer and sizes initialisation
  uint64_t subbandMdatOffset = img->mdatOffset + mdatOffset;
  uint8_t *subbandBuf = planeComp->compBuf;

  for (int32_t subbandNum = 0; subbandNum < toSubbands; subbandNum++)
  {
    subbands[subbandNum].bandBuf = subbandBuf;
    subbandBuf += subbands[subbandNum].bandSize;
    subbands[subbandNum].mdatOffset =
        subbandMdatOffset + subbands[subbandNum].dataOffset;
  }

  // wavelet data initialisation
  if (img->levels)
  {
    CrxWaveletTransform *waveletTransforms =
        (CrxWaveletTransform *)(planeComp->compBuf + waveletDataOffset);
    int32_t *paramData = (int32_t *)(planeComp->compBuf + compCoeffDataOffset);

    planeComp->waveletTransform = waveletTransforms;
    waveletTransforms[0].subband0Buf = (int32_t *)subbands->bandBuf;

    for (int level = 0; level < img->levels; ++level)
    {
      int32_t band = 3 * level + 1;

      if (level >= img->levels - 1)
      {
        waveletTransforms[level].height = tile->height;
        transformWidth = tile->width;
      }
      else
      {
        waveletTransforms[level].height = subbands[band + 3].height;
        transformWidth = subbands[band + 4].width;
      }
      waveletTransforms[level].width = transformWidth;
      waveletTransforms[level].lineBuf[0] = paramData;
      waveletTransforms[level].lineBuf[1] =
          waveletTransforms[level].lineBuf[0] + transformWidth;
      waveletTransforms[level].lineBuf[2] =
          waveletTransforms[level].lineBuf[1] + transformWidth;
      waveletTransforms[level].lineBuf[3] =
          waveletTransforms[level].lineBuf[2] + transformWidth;
      waveletTransforms[level].lineBuf[4] =
          waveletTransforms[level].lineBuf[3] + transformWidth;
      waveletTransforms[level].lineBuf[5] =
          waveletTransforms[level].lineBuf[4] + transformWidth;
      waveletTransforms[level].lineBuf[6] =
          waveletTransforms[level].lineBuf[5] + transformWidth;
      waveletTransforms[level].lineBuf[7] =
          waveletTransforms[level].lineBuf[6] + transformWidth;
      waveletTransforms[level].curLine = 0;
      waveletTransforms[level].curH = 0;
      waveletTransforms[level].fltTapH = 0;
      waveletTransforms[level].subband1Buf = (int32_t *)subbands[band].bandBuf;
      waveletTransforms[level].subband2Buf =
          (int32_t *)subbands[band + 1].bandBuf;
      waveletTransforms[level].subband3Buf =
          (int32_t *)subbands[band + 2].bandBuf;

      paramData = waveletTransforms[level].lineBuf[7] + transformWidth;
    }
  }

  // decoding params and bitstream initialisation
  for (int32_t subbandNum = 0; subbandNum < toSubbands; subbandNum++)
  {
    if (subbands[subbandNum].dataSize)
    {
      int32_t supportsPartial = 0;
      uint32_t roundedBitsMask = 0;

      if (planeComp->supportsPartial && subbandNum == 0)
      {
        roundedBitsMask = planeComp->roundedBitsMask;
        supportsPartial = 1;
      }
      if (crxParamInit(&subbands[subbandNum].bandParam,
                       subbands[subbandNum].mdatOffset,
                       subbands[subbandNum].dataSize,
                       subbands[subbandNum].width, subbands[subbandNum].height,
                       supportsPartial, roundedBitsMask, img->input))
        return -1;
    }
  }

  return 0;
}