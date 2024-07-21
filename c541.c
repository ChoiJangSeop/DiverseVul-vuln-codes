static char *linetoken(FILE *stream)
{
    int ch, idx;

    while ((ch = fgetc(stream)) == ' ' || ch == '\t' ); 
    
    idx = 0;
    while (ch != EOF && ch != lineterm && idx < MAX_NAME)
    {
        ident[idx++] = ch;
        ch = fgetc(stream);
    } /* while */
    
    ungetc(ch, stream);
    ident[idx] = 0;

    return(ident);	/* returns pointer to the token */

} /* linetoken */