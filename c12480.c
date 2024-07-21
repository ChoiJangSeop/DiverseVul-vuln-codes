static void* swoole_unserialize_object(void *buffer, zval *return_value, zend_uchar bucket_len, zval *args, long flag)
{
    zval property;
    uint32_t arr_num = 0;
    size_t name_len = *((unsigned short*) buffer);
    if (!name_len)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "illegal unserialize data");
        return NULL;
    }
    buffer += 2;
    zend_string *class_name;
    if (flag == UNSERIALIZE_OBJECT_TO_STDCLASS) 
    {
        class_name = swoole_string_init(ZEND_STRL("StdClass"));
    } 
    else 
    {
        class_name = swoole_string_init((char*) buffer, name_len);
    }
    buffer += name_len;
    zend_class_entry *ce = swoole_try_get_ce(class_name);
    swoole_string_release(class_name);

    if (!ce)
    {
        return NULL;
    }

    buffer = get_array_real_len(buffer, bucket_len, &arr_num);
    buffer = swoole_unserialize_arr(buffer, &property, arr_num, flag);

    object_init_ex(return_value, ce);

    zval *data,*d;
    zend_string *key;
    zend_ulong index;

    
    ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(property), index, key, data)
    {
        const char *prop_name, *tmp;
        size_t prop_len;
        if (key)
        {

            if ((d = zend_hash_find(Z_OBJPROP_P(return_value), key)) != NULL)
            {
                if (Z_TYPE_P(d) == IS_INDIRECT)
                {
                    d = Z_INDIRECT_P(d);
                }
                zval_dtor(d);
                ZVAL_COPY(d, data);
            }
            else
            {
                zend_unmangle_property_name_ex(key, &tmp, &prop_name, &prop_len);
                zend_update_property(ce, return_value, prop_name, prop_len, data);
            }
//            zend_hash_update(Z_OBJPROP_P(return_value),key,data);
//            zend_update_property(ce, return_value, ZSTR_VAL(key), ZSTR_LEN(key), data);
        }
        else
        {
            zend_hash_next_index_insert(Z_OBJPROP_P(return_value), data);
        }
    }
    ZEND_HASH_FOREACH_END();
    zval_dtor(&property);

    if (ce->constructor)
    {
        //        zend_fcall_info fci = {0};
        //        zend_fcall_info_cache fcc = {0};
        //        fci.size = sizeof (zend_fcall_info);
        //        zval retval;
        //        ZVAL_UNDEF(&fci.function_name);
        //        fci.retval = &retval;
        //        fci.param_count = 0;
        //        fci.params = NULL;
        //        fci.no_separation = 1;
        //        fci.object = Z_OBJ_P(return_value);
        //
        //        zend_fcall_info_args_ex(&fci, ce->constructor, args);
        //
        //        fcc.initialized = 1;
        //        fcc.function_handler = ce->constructor;
        //        //        fcc.calling_scope = EG(scope);
        //        fcc.called_scope = Z_OBJCE_P(return_value);
        //        fcc.object = Z_OBJ_P(return_value);
        //
        //        if (zend_call_function(&fci, &fcc) == FAILURE)
        //        {
        //            zend_throw_exception_ex(NULL, 0, "could not call class constructor");
        //        }
        //        zend_fcall_info_args_clear(&fci, 1);
    }


    //call object __wakeup
    if (zend_hash_str_exists(&ce->function_table, ZEND_STRL("__wakeup")))
    {
        zval ret, wakeup;
        zend_string *fname = swoole_string_init(ZEND_STRL("__wakeup"));
        Z_STR(wakeup) = fname;
        Z_TYPE_INFO(wakeup) = IS_STRING_EX;
        call_user_function_ex(CG(function_table), return_value, &wakeup, &ret, 0, NULL, 1, NULL);
        swoole_string_release(fname);
        zval_ptr_dtor(&ret);
    }

    return buffer;

}