static void test_iterators()
{
    json_t *object, *foo, *bar, *baz;
    void *iter;

    if(json_object_iter(NULL))
        fail("able to iterate over NULL");

    if(json_object_iter_next(NULL, NULL))
        fail("able to increment an iterator on a NULL object");

    object = json_object();
    foo = json_string("foo");
    bar = json_string("bar");
    baz = json_string("baz");
    if(!object || !foo || !bar || !bar)
        fail("unable to create values");

    if(json_object_iter_next(object, NULL))
        fail("able to increment a NULL iterator");

    if(json_object_set(object, "a", foo) ||
       json_object_set(object, "b", bar) ||
       json_object_set(object, "c", baz))
        fail("unable to populate object");

    iter = json_object_iter(object);
    if(!iter)
        fail("unable to get iterator");
    if(strcmp(json_object_iter_key(iter), "a"))
        fail("iterating failed: wrong key");
    if(json_object_iter_value(iter) != foo)
        fail("iterating failed: wrong value");

    iter = json_object_iter_next(object, iter);
    if(!iter)
        fail("unable to increment iterator");
    if(strcmp(json_object_iter_key(iter), "b"))
        fail("iterating failed: wrong key");
    if(json_object_iter_value(iter) != bar)
        fail("iterating failed: wrong value");

    iter = json_object_iter_next(object, iter);
    if(!iter)
        fail("unable to increment iterator");
    if(strcmp(json_object_iter_key(iter), "c"))
        fail("iterating failed: wrong key");
    if(json_object_iter_value(iter) != baz)
        fail("iterating failed: wrong value");

    if(json_object_iter_next(object, iter) != NULL)
        fail("able to iterate over the end");

    if(json_object_iter_at(object, "foo"))
        fail("json_object_iter_at() succeeds for non-existent key");

    iter = json_object_iter_at(object, "b");
    if(!iter)
        fail("json_object_iter_at() fails for an existing key");

    if(strcmp(json_object_iter_key(iter), "b"))
        fail("iterating failed: wrong key");
    if(json_object_iter_value(iter) != bar)
        fail("iterating failed: wrong value");

    iter = json_object_iter_next(object, iter);
    if(!iter)
        fail("unable to increment iterator");
    if(strcmp(json_object_iter_key(iter), "c"))
        fail("iterating failed: wrong key");
    if(json_object_iter_value(iter) != baz)
        fail("iterating failed: wrong value");

    if(json_object_iter_set(object, iter, bar))
        fail("unable to set value at iterator");

    if(strcmp(json_object_iter_key(iter), "c"))
        fail("json_object_iter_key() fails after json_object_iter_set()");
    if(json_object_iter_value(iter) != bar)
        fail("json_object_iter_value() fails after json_object_iter_set()");
    if(json_object_get(object, "c") != bar)
        fail("json_object_get() fails after json_object_iter_set()");

    json_decref(object);
    json_decref(foo);
    json_decref(bar);
    json_decref(baz);
}