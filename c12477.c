static void swoole_serialize_arr(seriaString *buffer, zend_array *zvalue)
{
    zval *data;
    zend_string *key;
    zend_ulong index;
    swPoolstr *swStr = NULL;
    zend_uchar is_pack = zvalue->u.flags & HASH_FLAG_PACKED;

    ZEND_HASH_FOREACH_KEY_VAL(zvalue, index, key, data)
    {
        SBucketType type = {0};
        type.data_type = Z_TYPE_P(data);
        //start point
        size_t p = buffer->offset;

        if (is_pack && zvalue->nNextFreeElement == zvalue->nNumOfElements)
        {
            type.key_type = KEY_TYPE_INDEX;
            type.key_len = 0;
            SERIA_SET_ENTRY_TYPE(buffer, type);
        }
        else
        {
            //seria key
            if (key)
            {
                type.key_type = KEY_TYPE_STRING;
                if ((swStr = swoole_mini_filter_find(key)))
                {
                    type.key_len = 3; //means use same string
                    SERIA_SET_ENTRY_TYPE(buffer, type);
                    if (swStr->offset & 4)
                    {
                        SERIA_SET_ENTRY_SIZE4(buffer, swStr->offset);
                    }
                    else
                    {
                        SERIA_SET_ENTRY_SHORT(buffer, swStr->offset);
                    }
                }
                else
                {
                    if (key->len <= 0xff)
                    {
                        type.key_len = 1;
                        SERIA_SET_ENTRY_TYPE(buffer, type);
                        swoole_mini_filter_add(key, buffer->offset, 1);
                        SERIA_SET_ENTRY_TYPE(buffer, key->len);
                        swoole_string_cpy(buffer, key->val, key->len);
                    }
                    else if (key->len <= 0xffff)
                    {//if more than this  don't need optimize
                        type.key_len = 2;
                        SERIA_SET_ENTRY_TYPE(buffer, type);
                        swoole_mini_filter_add(key, buffer->offset, 2);
                        SERIA_SET_ENTRY_SHORT(buffer, key->len);
                        swoole_string_cpy(buffer, key->val, key->len);
                    }
                    else
                    {
                        type.key_len = 0;
                        SERIA_SET_ENTRY_TYPE(buffer, type);
                        swoole_mini_filter_add(key, buffer->offset, 3);
                        swoole_string_cpy(buffer, key + XtOffsetOf(zend_string, len), sizeof (size_t) + key->len);
                    }
                }
            }
            else
            {
                type.key_type = KEY_TYPE_INDEX;
                if (index <= 0xff)
                {
                    type.key_len = 1;
                    SERIA_SET_ENTRY_TYPE(buffer, type);
                    SERIA_SET_ENTRY_TYPE(buffer, index);
                }
                else if (index <= 0xffff)
                {
                    type.key_len = 2;
                    SERIA_SET_ENTRY_TYPE(buffer, type);
                    SERIA_SET_ENTRY_SHORT(buffer, index);
                }
                else
                {
                    type.key_len = 3;
                    SERIA_SET_ENTRY_TYPE(buffer, type);
                    SERIA_SET_ENTRY_ULONG(buffer, index);
                }

            }
        }
        //seria data
try_again:
        switch (Z_TYPE_P(data))
        {
            case IS_STRING:
            {
                if ((swStr = swoole_mini_filter_find(Z_STR_P(data))))
                {
                    ((SBucketType*) (buffer->buffer + p))->data_len = 3; //means use same string
                    if (swStr->offset & 4)
                    {
                        SERIA_SET_ENTRY_SIZE4(buffer, swStr->offset);
                    }
                    else
                    {
                        SERIA_SET_ENTRY_SHORT(buffer, swStr->offset);
                    }
                }
                else
                {
                    if (Z_STRLEN_P(data) <= 0xff)
                    {
                        ((SBucketType*) (buffer->buffer + p))->data_len = 1;
                        swoole_mini_filter_add(Z_STR_P(data), buffer->offset, 1);
                        SERIA_SET_ENTRY_TYPE(buffer, Z_STRLEN_P(data));
                        swoole_string_cpy(buffer, Z_STRVAL_P(data), Z_STRLEN_P(data));
                    }
                    else if (Z_STRLEN_P(data) <= 0xffff)
                    {
                        ((SBucketType*) (buffer->buffer + p))->data_len = 2;
                        swoole_mini_filter_add(Z_STR_P(data), buffer->offset, 2);
                        SERIA_SET_ENTRY_SHORT(buffer, Z_STRLEN_P(data));
                        swoole_string_cpy(buffer, Z_STRVAL_P(data), Z_STRLEN_P(data));
                    }
                    else
                    {//if more than this  don't need optimize
                        ((SBucketType*) (buffer->buffer + p))->data_len = 0;
                        swoole_mini_filter_add(Z_STR_P(data), buffer->offset, 3);
                        swoole_string_cpy(buffer, (char*) Z_STR_P(data) + XtOffsetOf(zend_string, len), sizeof (size_t) + Z_STRLEN_P(data));
                    }
                }
                break;
            }
            case IS_LONG:
            {
                SBucketType* long_type = (SBucketType*) (buffer->buffer + p);
                swoole_serialize_long(buffer, data, long_type);
                break;
            }
            case IS_DOUBLE:
                swoole_set_zend_value(buffer, &(data->value));
                break;
            case IS_REFERENCE:
                data = Z_REFVAL_P(data);
                ((SBucketType*) (buffer->buffer + p))->data_type = Z_TYPE_P(data);
                goto try_again;
                break;
            case IS_ARRAY:
            {
                zend_array *ht = Z_ARRVAL_P(data);

                if (GC_IS_RECURSIVE(ht))
                {
                    ((SBucketType*) (buffer->buffer + p))->data_type = IS_NULL;//reset type null
                    php_error_docref(NULL TSRMLS_CC, E_NOTICE, "the array has cycle ref");
                }
                else
                {
                    seria_array_type(ht, buffer, p, buffer->offset);
                    if (ZEND_HASH_APPLY_PROTECTION(ht))
                    {
                        GC_PROTECT_RECURSION(ht);
                        swoole_serialize_arr(buffer, ht);
                        GC_UNPROTECT_RECURSION(ht);
                    }
                    else
                    {
                        swoole_serialize_arr(buffer, ht);
                    }

                }
                break;
            }
                //object propterty table is this type
            case IS_INDIRECT:
                data = Z_INDIRECT_P(data);
                zend_uchar type = Z_TYPE_P(data);
                ((SBucketType*) (buffer->buffer + p))->data_type = (type == IS_UNDEF ? IS_NULL : type);
                goto try_again;
                break;
            case IS_OBJECT:
            {
                /*
                 * layout
                 * type | key | namelen | name | bucket len |buckets
                 */
                ((SBucketType*) (buffer->buffer + p))->data_type = IS_UNDEF;

                if (ZEND_HASH_APPLY_PROTECTION(Z_OBJPROP_P(data)))
                {
                    GC_PROTECT_RECURSION(Z_OBJPROP_P(data));
                    swoole_serialize_object(buffer, data, p);
                    GC_UNPROTECT_RECURSION(Z_OBJPROP_P(data));
                }
                else
                {
                    swoole_serialize_object(buffer, data, p);
                }

                break;
            }
            default://
                break;

        }

    }
    ZEND_HASH_FOREACH_END();
}