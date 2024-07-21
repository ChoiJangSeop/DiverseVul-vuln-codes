irc_server_set_prefix_modes_chars (struct t_irc_server *server,
                                   const char *prefix)
{
    char *pos;
    int i, length_modes, length_chars;

    if (!server || !prefix)
        return;

    /* free previous values */
    if (server->prefix_modes)
    {
        free (server->prefix_modes);
        server->prefix_modes = NULL;
    }
    if (server->prefix_chars)
    {
        free (server->prefix_chars);
        server->prefix_chars = NULL;
    }

    /* assign new values */
    pos = strchr (prefix, ')');
    if (pos)
    {
        server->prefix_modes = weechat_strndup (prefix + 1,
                                                pos - prefix - 1);
        if (server->prefix_modes)
        {
            pos++;
            length_modes = strlen (server->prefix_modes);
            length_chars = strlen (pos);
            server->prefix_chars = malloc (length_modes + 1);
            if (server->prefix_chars)
            {
                for (i = 0; i < length_modes; i++)
                {
                    server->prefix_chars[i] = (i < length_chars) ? pos[i] : ' ';
                }
                server->prefix_chars[length_modes] = '\0';
            }
            else
            {
                free (server->prefix_modes);
                server->prefix_modes = NULL;
            }
        }
    }
}