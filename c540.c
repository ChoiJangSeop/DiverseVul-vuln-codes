static char *token(FILE *stream)
{
    int ch, idx;

    /* skip over white space */
    while ((ch = fgetc(stream)) == ' ' || ch == lineterm || 
            ch == ',' || ch == '\t' || ch == ';');
    
    idx = 0;
    while (ch != EOF && ch != ' ' && ch != lineterm 
           && ch != '\t' && ch != ':' && ch != ';' && idx < MAX_NAME)
    {
        ident[idx++] = ch;
        ch = fgetc(stream);
    } /* while */

    if (ch == EOF && idx < 1) return ((char *)NULL);
    if (idx >= 1 && ch != ':' ) ungetc(ch, stream);
    if (idx < 1 ) ident[idx++] = ch;	/* single-character token */
    ident[idx] = 0;
    
    return(ident);	/* returns pointer to the token */

} /* token */