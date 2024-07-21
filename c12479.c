PHPAPI int php_swoole_unserialize(void *buffer, size_t len, zval *return_value, zval *object_args, long flag)
{
    SBucketType type = *(SBucketType*) (buffer);
    zend_uchar real_type = type.data_type;
    buffer += sizeof (SBucketType);
    switch (real_type)
    {
        case IS_NULL:
        case IS_TRUE:
        case IS_FALSE:
            Z_TYPE_INFO_P(return_value) = real_type;
            break;
        case IS_LONG:
            swoole_unserialize_long(buffer, return_value, type);
            Z_TYPE_INFO_P(return_value) = real_type;
            break;
        case IS_DOUBLE:
            swoole_unserialize_raw(buffer, return_value);
            Z_TYPE_INFO_P(return_value) = real_type;
            break;
        case IS_STRING:
            len -= sizeof (SBucketType);
            zend_string *str = swoole_unserialize_string(buffer, len);
            ZVAL_STR(return_value, str);
            break;
        case IS_ARRAY:
        {
            if (swoole_seria_check_eof(buffer, len) < 0)
            {
                  php_error_docref(NULL TSRMLS_CC, E_NOTICE, "detect the error eof");
                  return SW_FALSE;
            }
            unser_start = buffer - sizeof (SBucketType);
            uint32_t num = 0;
            buffer = get_array_real_len(buffer, type.data_len, &num);
            if (!swoole_unserialize_arr(buffer, return_value, num, flag))
            {
                return SW_FALSE;
            }
            break;
        }
        case IS_UNDEF:
            if (swoole_seria_check_eof(buffer, len) < 0)
            {
                  php_error_docref(NULL TSRMLS_CC, E_NOTICE, "detect the error eof");
                  return SW_FALSE;
            }
            unser_start = buffer - sizeof (SBucketType);
            if (!swoole_unserialize_object(buffer, return_value, type.data_len, object_args, flag))
            {
                return SW_FALSE;
            }
            break;
        default:
            php_error_docref(NULL TSRMLS_CC, E_NOTICE, "the type is not supported by swoole serialize.");
            return SW_FALSE;
    }

    return SW_TRUE;
}