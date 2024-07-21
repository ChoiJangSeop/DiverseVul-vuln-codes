njs_external_property_query(njs_vm_t *vm, njs_property_query_t *pq,
    njs_value_t *value)
{
    njs_object_prop_t   *prop;
    njs_exotic_slots_t  *slots;

    slots = njs_object_slots(value);

    if (njs_slow_path(slots->prop_handler == NULL)) {
        return NJS_DECLINED;
    }

    prop = &pq->scratch;

    njs_memzero(prop, sizeof(njs_object_prop_t));

    /*
     * njs_memzero() does also:
     *   prop->type = NJS_PROPERTY;
     *   prop->writable = 0;
     *   prop->configurable = 0;
     *   njs_set_null(&prop->getter);
     *   njs_set_null(&prop->setter);
     */

    prop->value.data.magic32 = slots->magic32;
    prop->name = pq->key;

    pq->lhq.value = prop;

    prop->writable = slots->writable;
    prop->configurable = slots->configurable;
    prop->enumerable = slots->enumerable;

    switch (pq->query) {

    case NJS_PROPERTY_QUERY_GET:
        return slots->prop_handler(vm, prop, value, NULL, &prop->value);

    case NJS_PROPERTY_QUERY_SET:
        if (slots->writable == 0) {
            return NJS_OK;
        }

        break;

    case NJS_PROPERTY_QUERY_DELETE:
        if (slots->configurable == 0) {
            return NJS_OK;
        }

        break;
    }

    prop->type = NJS_PROPERTY_HANDLER;
    prop->value.data.u.prop_handler = slots->prop_handler;

    return NJS_OK;
}