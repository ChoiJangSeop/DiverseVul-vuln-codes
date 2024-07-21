static void* swoole_unserialize_arr(void *buffer, zval *zvalue, uint32_t nNumOfElements, long flag)
{
    //Initialize zend array
    zend_ulong h, nIndex, max_index = 0;
    uint32_t size = cp_zend_hash_check_size(nNumOfElements);
    if (!size)
    {
        return NULL;
    }
    if (!buffer)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "illegal unserialize data");
        return NULL;
    }
    ZVAL_NEW_ARR(zvalue);
    //Initialize buckets
    zend_array *ht = Z_ARR_P(zvalue);
    ht->nTableSize = size;
    ht->nNumUsed = nNumOfElements;
    ht->nNumOfElements = nNumOfElements;
    ht->nNextFreeElement = 0;
#ifdef HASH_FLAG_APPLY_PROTECTION
    ht->u.flags = HASH_FLAG_APPLY_PROTECTION;
#endif
    ht->nTableMask = -(ht->nTableSize);
    ht->pDestructor = ZVAL_PTR_DTOR;

    GC_SET_REFCOUNT(ht, 1);
    GC_TYPE_INFO(ht) = IS_ARRAY;
    // if (ht->nNumUsed)
    //{
    //    void *arData = ecalloc(1, len);
    HT_SET_DATA_ADDR(ht, emalloc(HT_SIZE(ht)));
    ht->u.flags |= HASH_FLAG_INITIALIZED;
    int ht_hash_size = HT_HASH_SIZE((ht)->nTableMask);
    if (ht_hash_size <= 0)
    {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "illegal unserialize data");
        return NULL;
    }
    HT_HASH_RESET(ht);
    //}


    int idx;
    Bucket *p;
    for(idx = 0; idx < nNumOfElements; idx++)
    {
        if (!buffer)
        {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "illegal array unserialize data");
            return NULL;
        }
        SBucketType type = *((SBucketType*) buffer);
        buffer += sizeof (SBucketType);
        p = ht->arData + idx;
        /* Initialize key */
        if (type.key_type == KEY_TYPE_STRING)
        {
            size_t key_len;
            if (type.key_len == 3)
            {//read the same mem
                void *str_pool_addr = get_pack_string_len_addr(&buffer, &key_len);
                p->key = zend_string_init((char*) str_pool_addr, key_len, 0);
                h = zend_inline_hash_func((char*) str_pool_addr, key_len);
                p->key->h = p->h = h;
            }
            else
            {//move step
                if (type.key_len == 1)
                {
                    key_len = *((zend_uchar*) buffer);
                    buffer += sizeof (zend_uchar);
                }
                else if (type.key_len == 2)
                {
                    key_len = *((unsigned short*) buffer);
                    buffer += sizeof (unsigned short);
                }
                else
                {
                    key_len = *((size_t*) buffer);
                    buffer += sizeof (size_t);
                }
                p->key = zend_string_init((char*) buffer, key_len, 0);
                //           h = zend_inline_hash_func((char*) buffer, key_len);
                h = zend_inline_hash_func((char*) buffer, key_len);
                buffer += key_len;
                p->key->h = p->h = h;
            }
        }
        else
        {
            if (type.key_len == 0)
            {
                //means pack
                h = p->h = idx;
                p->key = NULL;
                max_index = p->h + 1;
                //                ht->u.flags |= HASH_FLAG_PACKED;
            }
            else
            {
                if (type.key_len == 1)
                {
                    h = *((zend_uchar*) buffer);
                    buffer += sizeof (zend_uchar);
                }
                else if (type.key_len == 2)
                {
                    h = *((unsigned short*) buffer);
                    buffer += sizeof (unsigned short);
                }
                else
                {
                    h = *((zend_ulong*) buffer);
                    buffer += sizeof (zend_ulong);
                }
                p->h = h;
                p->key = NULL;
                if (h >= max_index)
                {
                    max_index = h + 1;
                }
            }
        }
        /* Initialize hash */
        nIndex = h | ht->nTableMask;
        Z_NEXT(p->val) = HT_HASH(ht, nIndex);
        HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(idx);

        /* Initialize data type */
        p->val.u1.v.type = type.data_type;
        Z_TYPE_FLAGS(p->val) = 0;

        /* Initialize data */
        if (type.data_type == IS_STRING)
        {
            size_t data_len;
            if (type.data_len == 3)
            {//read the same mem
                void *str_pool_addr = get_pack_string_len_addr(&buffer, &data_len);
                p->val.value.str = zend_string_init((char*) str_pool_addr, data_len, 0);
            }
            else
            {
                if (type.data_len == 1)
                {
                    data_len = *((zend_uchar*) buffer);
                    buffer += sizeof (zend_uchar);
                }
                else if (type.data_len == 2)
                {
                    data_len = *((unsigned short*) buffer);
                    buffer += sizeof (unsigned short);
                }
                else
                {
                    data_len = *((size_t*) buffer);
                    buffer += sizeof (size_t);
                }
                p->val.value.str = zend_string_init((char*) buffer, data_len, 0);
                buffer += data_len;
            }
            Z_TYPE_INFO(p->val) = IS_STRING_EX;
        }
        else if (type.data_type == IS_ARRAY)
        {
            uint32_t num = 0;
            buffer = get_array_real_len(buffer, type.data_len, &num);
            buffer = swoole_unserialize_arr(buffer, &p->val, num, flag);
        }
        else if (type.data_type == IS_LONG)
        {
            buffer = swoole_unserialize_long(buffer, &p->val, type);
        }
        else if (type.data_type == IS_DOUBLE)
        {
            p->val.value = *((zend_value*) buffer);
            buffer += sizeof (zend_value);
        }
        else if (type.data_type == IS_UNDEF)
        {
            buffer = swoole_unserialize_object(buffer, &p->val, type.data_len, NULL, flag);
            Z_TYPE_INFO(p->val) = IS_OBJECT_EX;
        }

    }
    ht->nNextFreeElement = max_index;

    return buffer;

}