rsvg_new_clip_path (void)
{
    RsvgClipPath *clip_path;

    clip_path = g_new (RsvgClipPath, 1);
    _rsvg_node_init (&clip_path->super);
    clip_path->units = userSpaceOnUse;
    clip_path->super.set_atts = rsvg_clip_path_set_atts;
    clip_path->super.free = _rsvg_node_free;
    return &clip_path->super;
}