rsvg_new_node_component_transfer_function (char channel)
{
    RsvgNodeComponentTransferFunc *filter;

    filter = g_new (RsvgNodeComponentTransferFunc, 1);
    _rsvg_node_init (&filter->super);
    filter->super.free = rsvg_component_transfer_function_free;
    filter->super.set_atts = rsvg_node_component_transfer_function_set_atts;
    filter->function = identity_component_transfer_func;
    filter->nbTableValues = 0;
    return (RsvgNode *) filter;
}