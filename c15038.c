static int _FVMenuClose(FontView *fv) {
    int i;
    SplineFont *sf = fv->b.cidmaster?fv->b.cidmaster:fv->b.sf;

    if ( !SFCloseAllInstrs(fv->b.sf) )
return( false );

    if ( fv->b.nextsame!=NULL || fv->b.sf->fv!=&fv->b ) {
	/* There's another view, can close this one with no problems */
    } else if ( SFAnyChanged(sf) ) {
	i = AskChanged(fv->b.sf);
	if ( i==2 )	/* Cancel */
return( false );
	if ( i==0 && !_FVMenuSave(fv))		/* Save */
return(false);
	else
	    SFClearAutoSave(sf);		/* if they didn't save it, remove change record */
    }
    _FVCloseWindows(fv);
    if ( sf->filename!=NULL )
	RecentFilesRemember(sf->filename);
    else if ( sf->origname!=NULL )
	RecentFilesRemember(sf->origname);
    GDrawDestroyWindow(fv->gw);
return( true );
}