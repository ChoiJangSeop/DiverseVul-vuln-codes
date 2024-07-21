gs_grab_keyboard_reset (GSGrab *grab)
{
        grab->priv->keyboard_grab_window = NULL;
        grab->priv->keyboard_grab_screen = NULL;
}