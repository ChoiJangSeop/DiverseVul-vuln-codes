gboolean vnc_color_map_set(VncColorMap *map,
                           guint16 idx,
                           guint16 red,
                           guint16 green,
                           guint16 blue)
{
    if (idx >= (map->size + map->offset))
        return FALSE;

    map->colors[idx - map->offset].red = red;
    map->colors[idx - map->offset].green = green;
    map->colors[idx - map->offset].blue = blue;

    return TRUE;
}