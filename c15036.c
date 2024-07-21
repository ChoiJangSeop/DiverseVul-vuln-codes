static int sd_e_h(GWindow gw, GEvent *event) {
    struct sd_data *sd = GDrawGetUserData(gw);

    if ( sd==NULL )
return( true );
    
    if ( event->type==et_close ) {
	SD_DoCancel( sd );
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("scripting.html");
return( true );
	}
return( false );
    } else if ( event->type == et_map )	/* Above palettes */
	GDrawRaise(gw);
    else if ( event->type == et_resize )
	GDrawRequestExpose(gw,NULL,false);
return( true );
}