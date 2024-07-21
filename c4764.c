gboolean vnc_color_map_lookup(VncColorMap *map,
                              guint16 idx,
                              guint16 *red,
                              guint16 *green,
                              guint16 *blue)
{
    if (idx >= (map->size + map->offset))
        return FALSE;

    *red = map->colors[idx - map->offset].red;
    *green = map->colors[idx - map->offset].green;
    *blue = map->colors[idx - map->offset].blue;

    return TRUE;
}