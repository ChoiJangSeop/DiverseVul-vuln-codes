rsvg_new_filter_primitive_light_source (char type)
{
    RsvgNodeLightSource *data;
    data = g_new (RsvgNodeLightSource, 1);
    _rsvg_node_init (&data->super);
    data->super.free = _rsvg_node_free;
    data->super.set_atts = rsvg_filter_primitive_light_source_set_atts;
    data->specularExponent = 1;
    if (type == 's')
        data->type = SPOTLIGHT;
    else if (type == 'd')
        data->type = DISTANTLIGHT;
    else
        data->type = POINTLIGHT;
    data->limitingconeAngle = 180;
    return &data->super;
}