ins_compl_next_buf(buf_T *buf, int flag)
{
    static win_T *wp = NULL;

    if (flag == 'w')		// just windows
    {
	if (buf == curbuf || wp == NULL)  // first call for this flag/expansion
	    wp = curwin;
	while ((wp = (wp->w_next != NULL ? wp->w_next : firstwin)) != curwin
		&& wp->w_buffer->b_scanned)
	    ;
	buf = wp->w_buffer;
    }
    else
	// 'b' (just loaded buffers), 'u' (just non-loaded buffers) or 'U'
	// (unlisted buffers)
	// When completing whole lines skip unloaded buffers.
	while ((buf = (buf->b_next != NULL ? buf->b_next : firstbuf)) != curbuf
		&& ((flag == 'U'
			? buf->b_p_bl
			: (!buf->b_p_bl
			    || (buf->b_ml.ml_mfp == NULL) != (flag == 'u')))
		    || buf->b_scanned))
	    ;
    return buf;
}