njs_is_data_descriptor(njs_object_prop_t *prop)
{
    return prop->writable != NJS_ATTRIBUTE_UNSET || njs_is_valid(&prop->value);
}