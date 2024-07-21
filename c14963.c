njs_vmcode_typeof(njs_vm_t *vm, njs_value_t *value, njs_value_t *invld)
{
    /* ECMAScript 5.1: null, array and regexp are objects. */

    static const njs_value_t  *types[NJS_VALUE_TYPE_MAX] = {
        &njs_string_object,
        &njs_string_undefined,
        &njs_string_boolean,
        &njs_string_number,
        &njs_string_symbol,
        &njs_string_string,
        &njs_string_data,
        &njs_string_external,
        &njs_string_invalid,
        &njs_string_undefined,
        &njs_string_undefined,
        &njs_string_undefined,
        &njs_string_undefined,
        &njs_string_undefined,
        &njs_string_undefined,
        &njs_string_undefined,

        &njs_string_object,
        &njs_string_object,
        &njs_string_function,
        &njs_string_object,
        &njs_string_object,
        &njs_string_object,
        &njs_string_object,
        &njs_string_object,
        &njs_string_object,
    };

    vm->retval = *types[value->type];

    return sizeof(njs_vmcode_2addr_t);
}