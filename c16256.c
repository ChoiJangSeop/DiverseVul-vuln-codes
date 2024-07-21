_rsvg_handle_allow_load (RsvgHandle *handle,
                         const char *uri,
                         GError **error)
{
    RsvgLoadPolicy policy = handle->priv->load_policy;

    if (policy == RSVG_LOAD_POLICY_ALL_PERMISSIVE)
        return TRUE;

    return TRUE;
}