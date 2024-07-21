pcsc_vendor_specific_init (int slot)
{
  unsigned char buf[256];
  pcsc_dword_t len;
  int sw;
  int vendor = 0;
  int product = 0;
  pcsc_dword_t get_tlv_ioctl = (pcsc_dword_t)-1;
  unsigned char *p;

  len = sizeof (buf);
  sw = control_pcsc (slot, CM_IOCTL_GET_FEATURE_REQUEST, NULL, 0, buf, &len);
  if (sw)
    {
      log_error ("pcsc_vendor_specific_init: GET_FEATURE_REQUEST failed: %d\n",
                 sw);
      return SW_NOT_SUPPORTED;
    }
  else
    {
      p = buf;
      while (p < buf + len)
        {
          unsigned char code = *p++;
          int l = *p++;
          unsigned int v = 0;

          if (l == 1)
            v = p[0];
          else if (l == 2)
            v = ((p[0] << 8) | p[1]);
          else if (l == 4)
            v = ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);

          if (code == FEATURE_VERIFY_PIN_DIRECT)
            reader_table[slot].pcsc.verify_ioctl = v;
          else if (code == FEATURE_MODIFY_PIN_DIRECT)
            reader_table[slot].pcsc.modify_ioctl = v;
          else if (code == FEATURE_GET_TLV_PROPERTIES)
            get_tlv_ioctl = v;

          if (DBG_CARD_IO)
            log_debug ("feature: code=%02X, len=%d, v=%02X\n", code, l, v);

          p += l;
        }
    }

  if (get_tlv_ioctl == (pcsc_dword_t)-1)
    {
      /*
       * For system which doesn't support GET_TLV_PROPERTIES,
       * we put some heuristics here.
       */
      if (reader_table[slot].rdrname)
        {
          if (strstr (reader_table[slot].rdrname, "SPRx32"))
            {
              reader_table[slot].is_spr532 = 1;
              reader_table[slot].pinpad_varlen_supported = 1;
            }
          else if (strstr (reader_table[slot].rdrname, "ST-2xxx")
                   || strstr (reader_table[slot].rdrname, "cyberJack")
                   || strstr (reader_table[slot].rdrname, "DIGIPASS")
                   || strstr (reader_table[slot].rdrname, "Gnuk")
                   || strstr (reader_table[slot].rdrname, "KAAN"))
            reader_table[slot].pinpad_varlen_supported = 1;
        }

      return 0;
    }

  len = sizeof (buf);
  sw = control_pcsc (slot, get_tlv_ioctl, NULL, 0, buf, &len);
  if (sw)
    {
      log_error ("pcsc_vendor_specific_init: GET_TLV_IOCTL failed: %d\n", sw);
      return SW_NOT_SUPPORTED;
    }

  p = buf;
  while (p < buf + len)
    {
      unsigned char tag = *p++;
      int l = *p++;
      unsigned int v = 0;

      /* Umm... here is little endian, while the encoding above is big.  */
      if (l == 1)
        v = p[0];
      else if (l == 2)
        v = ((p[1] << 8) | p[0]);
      else if (l == 4)
        v = ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);

      if (tag == PCSCv2_PART10_PROPERTY_bMinPINSize)
        reader_table[slot].pcsc.pinmin = v;
      else if (tag == PCSCv2_PART10_PROPERTY_bMaxPINSize)
        reader_table[slot].pcsc.pinmax = v;
      else if (tag == PCSCv2_PART10_PROPERTY_wIdVendor)
        vendor = v;
      else if (tag == PCSCv2_PART10_PROPERTY_wIdProduct)
        product = v;

      if (DBG_CARD_IO)
        log_debug ("TLV properties: tag=%02X, len=%d, v=%08X\n", tag, l, v);

      p += l;
    }

  if (vendor == VENDOR_VEGA && product == VEGA_ALPHA)
    {
      /*
       * Please read the comment of ccid_vendor_specific_init in
       * ccid-driver.c.
       */
      const unsigned char cmd[] = { '\xb5', '\x01', '\x00', '\x03', '\x00' };
      sw = control_pcsc (slot, CM_IOCTL_VENDOR_IFD_EXCHANGE,
                         cmd, sizeof (cmd), NULL, 0);
      if (sw)
        return SW_NOT_SUPPORTED;
    }
  else if (vendor == VENDOR_SCM && product == SCM_SPR532) /* SCM SPR532 */
    {
      reader_table[slot].is_spr532 = 1;
      reader_table[slot].pinpad_varlen_supported = 1;
    }
  else if ((vendor == 0x046a && product == 0x003e)  /* Cherry ST-2xxx */
           || vendor == 0x0c4b /* Tested with Reiner cyberJack GO */
           || vendor == 0x1a44 /* Tested with Vasco DIGIPASS 920 */
           || vendor == 0x234b /* Tested with FSIJ Gnuk Token */
           || vendor == 0x0d46 /* Tested with KAAN Advanced??? */)
    reader_table[slot].pinpad_varlen_supported = 1;

  return 0;
}