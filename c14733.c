void ftoa_bounded_extra(JsVarFloat val,char *str, size_t len, int radix, int fractionalDigits) {
  const JsVarFloat stopAtError = 0.0000001;
  if (isnan(val)) strncpy(str,"NaN",len);
  else if (!isfinite(val)) {
    if (val<0) strncpy(str,"-Infinity",len);
    else strncpy(str,"Infinity",len);
  } else {
    if (val<0) {
      if (--len <= 0) { *str=0; return; } // bounds check
      *(str++) = '-';
      val = -val;
    }

    // what if we're really close to an integer? Just use that...
    if (((JsVarInt)(val+stopAtError)) == (1+(JsVarInt)val))
      val = (JsVarFloat)(1+(JsVarInt)val);

    JsVarFloat d = 1;
    while (d*radix <= val) d*=radix;
    while (d >= 1) {
      int v = (int)(val / d);
      val -= v*d;
      if (--len <= 0) { *str=0; return; } // bounds check
      *(str++) = itoch(v);
      d /= radix;
    }
#ifndef USE_NO_FLOATS
    if (((fractionalDigits<0) && val>0) || fractionalDigits>0) {
      bool hasPt = false;
      val*=radix;
      while (((fractionalDigits<0) && (fractionalDigits>-12) && (val > stopAtError)) || (fractionalDigits > 0)) {
        int v = (int)(val+((fractionalDigits==1) ? 0.4 : 0.00000001) );
        val = (val-v)*radix;
	if (v==radix) v=radix-1;
        if (!hasPt) {	
	  hasPt = true;
          if (--len <= 0) { *str=0; return; } // bounds check
          *(str++)='.';
        }
        if (--len <= 0) { *str=0; return; } // bounds check
        *(str++)=itoch(v);
        fractionalDigits--;
      }
    }
#endif

    *(str++)=0;
  }
}