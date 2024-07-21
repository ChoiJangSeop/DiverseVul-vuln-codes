static void get_xref_linear_skipped(FILE *fp, xref_t *xref)
{
    int  err;
    char ch, buf[256];

    if (xref->start != 0)
      return;

    /* Special case (Linearized PDF with initial startxref at 0) */
    xref->is_linear = 1;

    /* Seek to %%EOF */
    if ((xref->end = get_next_eof(fp)) < 0)
      return;

    /* Locate the trailer */ 
    err = 0; 
    while (!(err = ferror(fp)) && fread(buf, 1, 8, fp))
    {
        if (strncmp(buf, "trailer", strlen("trailer")) == 0)
          break;
        else if ((ftell(fp) - 9) < 0)
          return;

        fseek(fp, -9, SEEK_CUR);
    }

    if (err)
      return;

    /* If we found 'trailer' look backwards for 'xref' */
    ch = 0;
    while (SAFE_F(fp, ((ch = fgetc(fp)) != 'x')))
      fseek(fp, -2, SEEK_CUR);

    if (ch == 'x')
    {
        xref->start = ftell(fp) - 1;
        fseek(fp, -1, SEEK_CUR);
    }

    /* Now continue to next eof ... */
    fseek(fp, xref->start, SEEK_SET);
}