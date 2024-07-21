_rsvg_node_finalize (RsvgNode * self)
{
    if (self->state != NULL) {
        rsvg_state_finalize (self->state);
        g_free (self->state);
    }
    if (self->children != NULL)
        g_ptr_array_free (self->children, TRUE);
    if (self->type != NULL)
        g_string_free (self->type, TRUE);
}