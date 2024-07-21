static int vp8dx_decode_bool(BOOL_DECODER *br, int probability) {
  unsigned int bit = 0;
  VP8_BD_VALUE value;
  unsigned int split;
  VP8_BD_VALUE bigsplit;
  int count;
  unsigned int range;

  split = 1 + (((br->range - 1) * probability) >> 8);

  if (br->count < 0) vp8dx_bool_decoder_fill(br);

  value = br->value;
  count = br->count;

  bigsplit = (VP8_BD_VALUE)split << (VP8_BD_VALUE_SIZE - 8);

  range = split;

  if (value >= bigsplit) {
    range = br->range - split;
    value = value - bigsplit;
    bit = 1;
  }

  {
    const int shift = vp8_norm[range];
    range <<= shift;
    value <<= shift;
    count -= shift;
  }
  br->value = value;
  br->count = count;
  br->range = range;

  return bit;
}