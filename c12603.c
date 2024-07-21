void LosslessScan::ParseMCU(struct Line **prev,struct Line **top)
{ 
#if ACCUSOFT_CODE
  UBYTE i;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(i = 0;i < m_ucCount;i++) {
    class HuffmanDecoder *dc = m_pDCDecoder[i];
    struct Line *line = top[i];
    struct Line *pline= prev[i];
    UBYTE ym = m_ucMCUHeight[i];
    class PredictorBase *mcupred = m_pPredict[i];
    LONG *lp = line->m_pData + m_ulX[i];
    LONG *pp = (pline)?(pline->m_pData + m_ulX[i]):(NULL);
    //
    // Parse MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[i];
      do {
        LONG v;
        UBYTE symbol = dc->Get(&m_Stream);
        
        if (symbol == 0) {
          v = 0;
        } else if (symbol == 16) {
          v = -32768;
        } else {
          LONG thre = 1L << (symbol - 1);
          LONG diff = m_Stream.Get(symbol); // get the number of bits 
          if (diff < thre) {
            diff += (-1L << symbol) + 1;
          }
          v = diff;
        }
        //
        // Set the current pixel, do the inverse pointwise transformation.
        lp[0] = pred->DecodeSample(v,lp,pp);
        //
        // One pixel done. Proceed to the next in the MCU. Note that
        // the lines have been extended such that always a complete MCU is present.
      } while(--xm && (lp++,pp++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + m_ulX[i],line = (line->m_pNext)?(line->m_pNext):(line),
                     lp = line->m_pData + m_ulX[i],mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}