rsvg_new_path (void)
{
    RsvgNodePath *path;
    path = g_new (RsvgNodePath, 1);
    _rsvg_node_init (&path->super);
    path->d = NULL;
    path->super.free = rsvg_node_path_free;
    path->super.draw = rsvg_node_path_draw;
    path->super.set_atts = rsvg_node_path_set_atts;

    return &path->super;
}