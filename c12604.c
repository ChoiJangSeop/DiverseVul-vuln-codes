void ACSequentialScan::DecodeBlock(LONG *block,
                                   LONG &prevdc,LONG &prevdiff,
                                   UBYTE small,UBYTE large,UBYTE kx,UBYTE dc,UBYTE ac)
{
  // DC coding
  if (m_ucScanStart == 0 && m_bResidual == false) {
    LONG diff;
    struct QMContextSet::DCContextZeroSet &cz = m_Context[dc].Classify(prevdiff,small,large);
    // Check whether the difference is nonzero.
    if (m_Coder.Get(cz.S0)) {
      LONG sz;
      bool sign = m_Coder.Get(cz.SS); // sign coding, is true for negative.
      //
      //
      // Positive and negative are encoded in different contexts.
      // Decode the magnitude cathegory.
      if (m_Coder.Get((sign)?(cz.SN):(cz.SP))) {
        int  i = 0;
        LONG m = 2;
        
        while(m_Coder.Get(m_Context[dc].DCMagnitude.X[i])) {
          m <<= 1;
          i++;
          if (m == 0) 
            JPG_THROW(MALFORMED_STREAM,"ACSequentialScan::DecodeBlock",
                      "QMDecoder is out of sync");
        }
        //
        // Get the MSB to decode.
        m >>= 1;
        sz  = m;
        //
        // Refinement coding of remaining bits.
        while((m >>= 1)) {
          if (m_Coder.Get(m_Context[dc].DCMagnitude.M[i])) {
            sz |= m;
          }
        }
      } else {
        sz = 0;
      }
      //
      // Done, finally, include the sign and the offset.
      if (sign) {
        diff = -sz - 1;
      } else {
        diff = sz + 1;
      }
    } else {
      // Difference is zero.
      diff = 0;
    }

    prevdiff = diff;
    if (m_bDifferential) {
      prevdc   = diff;
    } else {
      prevdc  += diff;
    }
    block[0] = prevdc << m_ucLowBit; // point transformation
  }

  if (m_ucScanStop) {
    // AC coding. No block skipping used here.
    int k = (m_ucScanStart)?(m_ucScanStart):((m_bResidual)?0:1);
    //
    // EOB decoding.
    while(k <= m_ucScanStop && !m_Coder.Get(m_Context[ac].ACZero[k-1].SE)) {
      LONG sz;
      bool sign;
      //
      // Not yet EOB. Run coding in S0: Skip over zeros.
      while(!m_Coder.Get(m_Context[ac].ACZero[k-1].S0)) {
        k++;
        if (k > m_ucScanStop)
          JPG_THROW(MALFORMED_STREAM,"ACSequentialScan::DecodeBlock",
                    "QMDecoder is out of sync");
      }
      //
      // Now decode the sign of the coefficient.
      // This happens in the uniform context.
      sign = m_Coder.Get(m_Context[ac].Uniform);
      //
      // Decode the magnitude.
      if (m_Coder.Get(m_Context[ac].ACZero[k-1].SP)) {
        // X1 coding, identical to SN and SP.
        if (m_Coder.Get(m_Context[ac].ACZero[k-1].SP)) {
          int  i = 0;
          LONG m = 4;
          struct QMContextSet::ACContextMagnitudeSet &acm = (k > kx)?(m_Context[ac].ACMagnitudeHigh):(m_Context[ac].ACMagnitudeLow);
          
          while(m_Coder.Get(acm.X[i])) {
            m <<= 1;
            i++;
            if (m == 0)
              JPG_THROW(MALFORMED_STREAM,"ACSequentialScan::DecodeBlock",
                        "QMDecoder is out of sync");
          }
          //
          // Get the MSB to decode
          m >>= 1;
          sz  = m;
          //
          // Proceed to refinement.
          while((m >>= 1)) {
            if (m_Coder.Get(acm.M[i])) {
              sz |= m;
            }
          }
        } else {
          sz = 1;
        }
      } else {
        sz = 0;
      }
      //
      // Done. Finally, include sign and offset.
      sz++;
      if (sign) 
        sz = -sz;
      block[DCT::ScanOrder[k]] = sz << m_ucLowBit;
      //
      // Proceed to the next block.
      k++;
    }
  }
}