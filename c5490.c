gss_process_context_token (minor_status,
                           context_handle,
                           token_buffer)

OM_uint32 *		minor_status;
gss_ctx_id_t		context_handle;
gss_buffer_t		token_buffer;

{
    OM_uint32		status;
    gss_union_ctx_id_t	ctx;
    gss_mechanism	mech;

    if (minor_status == NULL)
	return (GSS_S_CALL_INACCESSIBLE_WRITE);
    *minor_status = 0;

    if (context_handle == GSS_C_NO_CONTEXT)
	return (GSS_S_CALL_INACCESSIBLE_READ | GSS_S_NO_CONTEXT);

    if (token_buffer == GSS_C_NO_BUFFER)
	return (GSS_S_CALL_INACCESSIBLE_READ);

    if (GSS_EMPTY_BUFFER(token_buffer))
	return (GSS_S_CALL_INACCESSIBLE_READ);

    /*
     * select the approprate underlying mechanism routine and
     * call it.
     */

    ctx = (gss_union_ctx_id_t) context_handle;
    mech = gssint_get_mechanism (ctx->mech_type);

    if (mech) {

	if (mech->gss_process_context_token) {
	    status = mech->gss_process_context_token(
						    minor_status,
						    ctx->internal_ctx_id,
						    token_buffer);
	    if (status != GSS_S_COMPLETE)
		map_error(minor_status, mech);
	} else
	    status = GSS_S_UNAVAILABLE;

	return(status);
    }

    return (GSS_S_BAD_MECH);
}