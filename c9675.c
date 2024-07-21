int pdf_is_pdf(FILE *fp)
{
    int   is_pdf;
    char *header;

    header = get_header(fp);

    if (header && strstr(header, "%PDF-"))
      is_pdf = 1;
    else 
      is_pdf = 0;

    free(header);
    return is_pdf;
}