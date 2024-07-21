static void GTextFieldSave(GTextField *gt,int utf8) {
    unichar_t *ret;
    char *cret;
    FILE *file;
    unichar_t *pt;

    if ( _ggadget_use_gettext ) {
	char *temp = GWidgetOpenFile8(_("Save"),NULL,"*.{txt,py}",NULL,NULL);
	ret = utf82u_copy(temp);
	free(temp);
    } else
	ret = GWidgetSaveAsFile(GStringGetResource(_STR_Save,NULL),NULL,
		txt,NULL,NULL);

    if ( ret==NULL )
return;
    cret = u2def_copy(ret);
    free(ret);
    file = fopen(cret,"w");
    if ( file==NULL ) {
	if ( _ggadget_use_gettext )
	    GWidgetError8(_("Could not open file"), _("Could not open %.100s"),cret);
	else
	    GWidgetError(errort,error,cret);
	free(cret);
return;
    }
    free(cret);

    if ( utf8 ) {
	putc(0xef,file);		/* Zero width something or other. Marks this as unicode, utf8 */
	putc(0xbb,file);
	putc(0xbf,file);
	for ( pt = gt->text ; *pt; ++pt ) {
	    if ( *pt<0x80 )
		putc(*pt,file);
	    else if ( *pt<0x800 ) {
		putc(0xc0 | (*pt>>6), file);
		putc(0x80 | (*pt&0x3f), file);
	    } else if ( *pt>=0xd800 && *pt<0xdc00 && pt[1]>=0xdc00 && pt[1]<0xe000 ) {
		int u = ((*pt>>6)&0xf)+1, y = ((*pt&3)<<4) | ((pt[1]>>6)&0xf);
		putc( 0xf0 | (u>>2),file );
		putc( 0x80 | ((u&3)<<4) | ((*pt>>2)&0xf),file );
		putc( 0x80 | y,file );
		putc( 0x80 | (pt[1]&0x3f),file );
	    } else {
		putc( 0xe0 | (*pt>>12),file );
		putc( 0x80 | ((*pt>>6)&0x3f),file );
		putc( 0x80 | (*pt&0x3f),file );
	    }
	}
    } else {
	putc(0xfeff>>8,file);		/* Zero width something or other. Marks this as unicode */
	putc(0xfeff&0xff,file);
	for ( pt = gt->text ; *pt; ++pt ) {
	    putc(*pt>>8,file);
	    putc(*pt&0xff,file);
	}
    }
    fclose(file);
}