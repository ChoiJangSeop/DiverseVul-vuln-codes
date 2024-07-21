JANET_CORE_FN(cfun_array_new_filled,
              "(array/new-filled count &opt value)",
              "Creates a new array of `count` elements, all set to `value`, which defaults to nil. Returns the new array.") {
    janet_arity(argc, 1, 2);
    int32_t count = janet_getinteger(argv, 0);
    Janet x = (argc == 2) ? argv[1] : janet_wrap_nil();
    JanetArray *array = janet_array(count);
    for (int32_t i = 0; i < count; i++) {
        array->data[i] = x;
    }
    array->count = count;
    return janet_wrap_array(array);
}