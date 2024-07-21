read_rle_pixel(tga_source_ptr sinfo)
/* Read one Targa pixel from the input file, expanding RLE data as needed */
{
  register FILE *infile = sinfo->pub.input_file;
  register int i;

  /* Duplicate previously read pixel? */
  if (sinfo->dup_pixel_count > 0) {
    sinfo->dup_pixel_count--;
    return;
  }

  /* Time to read RLE block header? */
  if (--sinfo->block_count < 0) { /* decrement pixels remaining in block */
    i = read_byte(sinfo);
    if (i & 0x80) {             /* Start of duplicate-pixel block? */
      sinfo->dup_pixel_count = i & 0x7F; /* number of dups after this one */
      sinfo->block_count = 0;   /* then read new block header */
    } else {
      sinfo->block_count = i & 0x7F; /* number of pixels after this one */
    }
  }

  /* Read next pixel */
  for (i = 0; i < sinfo->pixel_size; i++) {
    sinfo->tga_pixel[i] = (U_CHAR)getc(infile);
  }
}