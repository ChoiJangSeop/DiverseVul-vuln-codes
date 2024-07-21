_rsvg_node_init (RsvgNode * self)
{
    self->parent = NULL;
    self->children = g_ptr_array_new ();
    self->state = g_new (RsvgState, 1);
    rsvg_state_init (self->state);
    self->free = _rsvg_node_free;
    self->draw = _rsvg_node_draw_nothing;
    self->set_atts = _rsvg_node_dont_set_atts;
    self->type = NULL;
}