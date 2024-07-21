void free_G_buffers(__G)     /* releases all memory allocated in global vars */
    __GDEF
{
#ifndef SFX
    unsigned i;
#endif

#ifdef SYSTEM_SPECIFIC_DTOR
    SYSTEM_SPECIFIC_DTOR(__G);
#endif

    inflate_free(__G);
    checkdir(__G__ (char *)NULL, END);

#ifdef DYNALLOC_CRCTAB
    if (CRC_32_TAB) {
        free_crc_table();
        CRC_32_TAB = NULL;
    }
#endif

   if (G.key != (char *)NULL) {
        free(G.key);
        G.key = (char *)NULL;
   }

   if (G.extra_field != (uch *)NULL) {
        free(G.extra_field);
        G.extra_field = (uch *)NULL;
   }

#if (!defined(VMS) && !defined(SMALL_MEM))
    /* VMS uses its own buffer scheme for textmode flush() */
    if (G.outbuf2) {
        free(G.outbuf2);   /* malloc'd ONLY if unshrink and -a */
        G.outbuf2 = (uch *)NULL;
    }
#endif

    if (G.outbuf)
        free(G.outbuf);
    if (G.inbuf)
        free(G.inbuf);
    G.inbuf = G.outbuf = (uch *)NULL;

#ifdef UNICODE_SUPPORT
    if (G.filename_full) {
        free(G.filename_full);
        G.filename_full = (char *)NULL;
        G.fnfull_bufsize = 0;
    }
#endif /* UNICODE_SUPPORT */

#ifndef SFX
    for (i = 0; i < DIR_BLKSIZ; i++) {
        if (G.info[i].cfilname != (char Far *)NULL) {
            zffree(G.info[i].cfilname);
            G.info[i].cfilname = (char Far *)NULL;
        }
    }
#endif

#ifdef MALLOC_WORK
    if (G.area.Slide) {
        free(G.area.Slide);
        G.area.Slide = (uch *)NULL;
    }
#endif

} /* end function free_G_buffers() */