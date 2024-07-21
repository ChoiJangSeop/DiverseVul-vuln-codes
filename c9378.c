bjc_compress(const byte *row, uint raster, byte *compressed)
{
  const byte *end_row = row;
  register const byte *exam = row;
  register byte *cptr = compressed; /* output pointer into compressed bytes */

  end_row += raster;

  while ( exam < end_row ) {
    /* Search ahead in the input looking for a run */
    /* of at least 4 identical bytes. */
    const byte *compr = exam;
    const byte *end_dis;
    const byte *next;
    register byte test, test2;

    test = *exam;
    while ( exam < end_row ) {
      test2 = *++exam;
      if ( test == test2 )
          break;
      test = test2;
    }

    /* Find out how long the run is */
    end_dis = exam - 1;
    if ( exam == end_row ) { /* no run */
      next = --end_row;
    } else {

      next = exam + 1;
      while ( next < end_row && *next == test ) next++;
    }

    /* Now [compr..end_dis) should be encoded as dissimilar, */
    /* and [end_dis..next) should be encoded as similar. */
    /* Note that either of these ranges may be empty. */

    for ( ; ; ) { /* Encode up to 128 dissimilar bytes */
      uint count = end_dis - compr; /* uint for faster switch */
      switch ( count ) { /* Use memcpy only if it's worthwhile. */
      case 6: cptr[6] = compr[5];
      case 5: cptr[5] = compr[4];
      case 4: cptr[4] = compr[3];
      case 3: cptr[3] = compr[2];
      case 2: cptr[2] = compr[1];
      case 1: cptr[1] = compr[0];
        *cptr = count - 1;
        cptr += count + 1;
      case 0: /* all done */
        break;
      default:
        if ( count > 128 ) count = 128;
        *cptr++ = count - 1;
        memcpy(cptr, compr, count);
        cptr += count, compr += count;
        continue;
      }
      break;
    }

    { /* Encode up to 128 similar bytes. */
      /* Note that count may be <0 at end of row. */
      int count = next - end_dis;
      if (next < end_row || test != 0)
        while ( count > 0 ) {

          int this = (count > 128 ? 128 : count);
          *cptr++ = 257 - this;
          *cptr++ = (byte)test;
          count -= this;
        }
      exam = next;
    }
  }
  return (uint)(cptr - compressed);
}