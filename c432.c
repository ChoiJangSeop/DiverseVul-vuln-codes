gs_grab_mouse_reset (GSGrab *grab)
{
        grab->priv->mouse_grab_window = NULL;
        grab->priv->mouse_grab_screen = NULL;
}