void ACLosslessScan::ParseMCU(struct Line **prev,struct Line **top)
{ 
#if ACCUSOFT_CODE
  UBYTE c;
  //
  // Parse a single MCU, which is now a group of pixels.
  for(c = 0;c < m_ucCount;c++) {
    struct QMContextSet &contextset = m_Context[m_ucContext[c]];
    struct Line *line = top[c];
    struct Line *pline= prev[c];
    UBYTE ym = m_ucMCUHeight[c];
    ULONG  x = m_ulX[c];
    class PredictorBase *mcupred = m_pPredict[c];
    LONG *lp = line->m_pData + x;
    LONG *pp = (pline)?(pline->m_pData + x):(NULL);
    //
    // Parse MCUwidth * MCUheight coefficients starting at the line top.
    do {
      class PredictorBase *pred = mcupred;
      UBYTE xm = m_ucMCUWidth[c];
      do {
        // Decode now the difference between the predicted value and
        // the real value.
        LONG v;
        //
        // Get the sign coding context.
        struct QMContextSet::ContextZeroSet &zset = contextset.ClassifySignZero(m_plDa[c][ym-1],m_plDb[c][x],
                                                                                m_ucSmall[c],m_ucLarge[c]);
        //
        if (m_Coder.Get(zset.S0)) {
          LONG sz   = 0;
          bool sign = m_Coder.Get(zset.SS); // true for negative.
          //
          if (m_Coder.Get((sign)?(zset.SN):(zset.SP))) {
            struct QMContextSet::MagnitudeSet &mset = contextset.ClassifyMagnitude(m_plDb[c][x],m_ucLarge[c]);
            int  i = 0;
            LONG m = 2;
            //
            while(m_Coder.Get(mset.X[i])) {
              m <<= 1;
              i++;
            }
            //
            m >>= 1;
            sz  = m;
            while((m >>= 1)) {
              if (m_Coder.Get(mset.M[i])) {
                sz |= m;
              }
            }
          }
          //
          if (sign) {
            v = -sz - 1;
          } else {
            v =  sz + 1;
          }
        } else {
          v = 0;
        }
        //
        // Use the prediction to fill in the sample.
        lp[0] = pred->DecodeSample(v,lp,pp);
        // Update Da and Db.
        // Is this a bug? 32768 does not exist, but -32768 does. The streams
        // seem to use -32768 instead.
        m_plDb[c][x]    = v;
        m_plDa[c][ym-1] = v;
        //
        // One pixel done. Proceed to the next in the MCU. Note that
        // the lines have been extended such that always a complete MCU is present.
      } while(--xm && (lp++,pp++,x++,pred = pred->MoveRight(),true));
      //
      // Go to the next line.
    } while(--ym && (pp = line->m_pData + (x = m_ulX[c]),line = (line->m_pNext)?(line->m_pNext):(line),
                     lp = line->m_pData + x,mcupred = mcupred->MoveDown(),true));
  }
#else
  NOREF(prev);
  NOREF(top);
#endif
}