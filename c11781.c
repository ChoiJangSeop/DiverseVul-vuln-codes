static void simple_decode_row(x3f_info_t *I, x3f_directory_entry_t *DE,
                              int bits, int row, int row_stride)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  x3f_huffman_t *HUF = ID->huffman;

  uint32_t *data = (uint32_t *)((unsigned char *)ID->data + row * row_stride);

  uint16_t c[3] = {0, 0, 0};
  int col;

  uint32_t mask = 0;

  switch (bits)
  {
  case 8:
    mask = 0x0ff;
    break;
  case 9:
    mask = 0x1ff;
    break;
  case 10:
    mask = 0x3ff;
    break;
  case 11:
    mask = 0x7ff;
    break;
  case 12:
    mask = 0xfff;
    break;
  default:
    mask = 0;
    /* TODO: Shouldn't this be treated as a fatal error? */
    throw LIBRAW_EXCEPTION_IO_CORRUPT;
    break;
  }

  for (col = 0; col < ID->columns; col++)
  {
    int color;
    uint32_t val = data[col];

    for (color = 0; color < 3; color++)
    {
      uint16_t c_fix;
      c[color] += get_simple_diff(HUF, (val >> (color * bits)) & mask);

      switch (ID->type_format)
      {
      case X3F_IMAGE_RAW_HUFFMAN_X530:
      case X3F_IMAGE_RAW_HUFFMAN_10BIT:
        c_fix = (int16_t)c[color] > 0 ? c[color] : 0;

        HUF->x3rgb16.data[3 * (row * ID->columns + col) + color] = c_fix;
        break;
      case X3F_IMAGE_THUMB_HUFFMAN:
        c_fix = (int8_t)c[color] > 0 ? c[color] : 0;

        HUF->rgb8.data[3 * (row * ID->columns + col) + color] = c_fix;
        break;
      default:
        /* TODO: Shouldn't this be treated as a fatal error? */
        throw LIBRAW_EXCEPTION_IO_CORRUPT;
      }
    }
  }
}