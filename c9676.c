void pdf_get_version(FILE *fp, pdf_t *pdf)
{
    char *header, *c;

    header = get_header(fp);

    /* Locate version string start and make sure we dont go past header */
    if ((c = strstr(header, "%PDF-")) && 
        (c + strlen("%PDF-M.m") + 2))
    {
        pdf->pdf_major_version = atoi(c + strlen("%PDF-"));
        pdf->pdf_minor_version = atoi(c + strlen("%PDF-M."));
    }

    free(header);
}