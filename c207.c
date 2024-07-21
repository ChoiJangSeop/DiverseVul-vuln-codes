_dbus_validate_signature_with_reason (const DBusString *type_str,
                                      int               type_pos,
                                      int               len)
{
  const unsigned char *p;
  const unsigned char *end;
  int last;
  int struct_depth;
  int array_depth;
  int dict_entry_depth;
  DBusValidity result;

  int element_count;
  DBusList *element_count_stack;

  result = DBUS_VALID;
  element_count_stack = NULL;

  if (!_dbus_list_append (&element_count_stack, _DBUS_INT_TO_POINTER (0)))
    {
      result = DBUS_VALIDITY_UNKNOWN_OOM_ERROR;
      goto out;
    }

  _dbus_assert (type_str != NULL);
  _dbus_assert (type_pos < _DBUS_INT32_MAX - len);
  _dbus_assert (len >= 0);
  _dbus_assert (type_pos >= 0);

  if (len > DBUS_MAXIMUM_SIGNATURE_LENGTH)
    {
      result = DBUS_INVALID_SIGNATURE_TOO_LONG;
      goto out;
    }

  p = _dbus_string_get_const_data_len (type_str, type_pos, 0);

  end = _dbus_string_get_const_data_len (type_str, type_pos + len, 0);
  struct_depth = 0;
  array_depth = 0;
  dict_entry_depth = 0;
  last = DBUS_TYPE_INVALID;

  while (p != end)
    {
      switch (*p)
        {
        case DBUS_TYPE_BYTE:
        case DBUS_TYPE_BOOLEAN:
        case DBUS_TYPE_INT16:
        case DBUS_TYPE_UINT16:
        case DBUS_TYPE_INT32:
        case DBUS_TYPE_UINT32:
        case DBUS_TYPE_INT64:
        case DBUS_TYPE_UINT64:
        case DBUS_TYPE_DOUBLE:
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_VARIANT:
          break;

        case DBUS_TYPE_ARRAY:
          array_depth += 1;
          if (array_depth > DBUS_MAXIMUM_TYPE_RECURSION_DEPTH)
            {
              result = DBUS_INVALID_EXCEEDED_MAXIMUM_ARRAY_RECURSION;
              goto out;
            }
          break;

        case DBUS_STRUCT_BEGIN_CHAR:
          struct_depth += 1;

          if (struct_depth > DBUS_MAXIMUM_TYPE_RECURSION_DEPTH)
            {
              result = DBUS_INVALID_EXCEEDED_MAXIMUM_STRUCT_RECURSION;
              goto out;
            }
          
          if (!_dbus_list_append (&element_count_stack, 
                             _DBUS_INT_TO_POINTER (0)))
            {
              result = DBUS_VALIDITY_UNKNOWN_OOM_ERROR;
              goto out;
            }

          break;

        case DBUS_STRUCT_END_CHAR:
          if (struct_depth == 0)
            {
              result = DBUS_INVALID_STRUCT_ENDED_BUT_NOT_STARTED;
              goto out;
            }

          if (last == DBUS_STRUCT_BEGIN_CHAR)
            {
              result = DBUS_INVALID_STRUCT_HAS_NO_FIELDS;
              goto out;
            }

          _dbus_list_pop_last (&element_count_stack);

          struct_depth -= 1;
          break;

        case DBUS_DICT_ENTRY_BEGIN_CHAR:
          if (last != DBUS_TYPE_ARRAY)
            {
              result = DBUS_INVALID_DICT_ENTRY_NOT_INSIDE_ARRAY;
              goto out;
            }
            
          dict_entry_depth += 1;

          if (dict_entry_depth > DBUS_MAXIMUM_TYPE_RECURSION_DEPTH)
            {
              result = DBUS_INVALID_EXCEEDED_MAXIMUM_DICT_ENTRY_RECURSION;
              goto out;
            }

          if (!_dbus_list_append (&element_count_stack, 
                             _DBUS_INT_TO_POINTER (0)))
            {
              result = DBUS_VALIDITY_UNKNOWN_OOM_ERROR;
              goto out;
            }

          break;

        case DBUS_DICT_ENTRY_END_CHAR:
          if (dict_entry_depth == 0)
            {
              result = DBUS_INVALID_DICT_ENTRY_ENDED_BUT_NOT_STARTED;
              goto out;
            }
            
          dict_entry_depth -= 1;

          element_count = 
            _DBUS_POINTER_TO_INT (_dbus_list_pop_last (&element_count_stack));

          if (element_count != 2)
            {
              if (element_count == 0)
                result = DBUS_INVALID_DICT_ENTRY_HAS_NO_FIELDS;
              else if (element_count == 1)
                result = DBUS_INVALID_DICT_ENTRY_HAS_ONLY_ONE_FIELD;
              else
                result = DBUS_INVALID_DICT_ENTRY_HAS_TOO_MANY_FIELDS;
              
              goto out;
            }
          break;
          
        case DBUS_TYPE_STRUCT:     /* doesn't appear in signatures */
        case DBUS_TYPE_DICT_ENTRY: /* ditto */
        default:
          result = DBUS_INVALID_UNKNOWN_TYPECODE;
	  goto out;
        }

      if (*p != DBUS_TYPE_ARRAY && 
          *p != DBUS_DICT_ENTRY_BEGIN_CHAR && 
	  *p != DBUS_STRUCT_BEGIN_CHAR) 
        {
          element_count = 
            _DBUS_POINTER_TO_INT (_dbus_list_pop_last (&element_count_stack));

          ++element_count;

          if (!_dbus_list_append (&element_count_stack, 
                             _DBUS_INT_TO_POINTER (element_count)))
            {
              result = DBUS_VALIDITY_UNKNOWN_OOM_ERROR;
              goto out;
            }
        }
      
      if (array_depth > 0)
        {
          if (*p == DBUS_TYPE_ARRAY && p != end)
            {
	       const char *p1;
	       p1 = p + 1;
               if (*p1 == DBUS_STRUCT_END_CHAR ||
                   *p1 == DBUS_DICT_ENTRY_END_CHAR)
                 {
                   result = DBUS_INVALID_MISSING_ARRAY_ELEMENT_TYPE;
                   goto out;
                 }
            }
          else
	    {
              array_depth = 0;
	    }
        }

      if (last == DBUS_DICT_ENTRY_BEGIN_CHAR &&
          !dbus_type_is_basic (*p))
        {
          result = DBUS_INVALID_DICT_KEY_MUST_BE_BASIC_TYPE;
          goto out;
        }
        
      last = *p;
      ++p;
    }


  if (array_depth > 0)
    {
      result = DBUS_INVALID_MISSING_ARRAY_ELEMENT_TYPE;
      goto out;
    }
    
  if (struct_depth > 0)
    {
       result = DBUS_INVALID_STRUCT_STARTED_BUT_NOT_ENDED;
       goto out;
    }
    
  if (dict_entry_depth > 0)
    {
      result =  DBUS_INVALID_DICT_ENTRY_STARTED_BUT_NOT_ENDED;
      goto out;
    }
    
  _dbus_assert (last != DBUS_TYPE_ARRAY);
  _dbus_assert (last != DBUS_STRUCT_BEGIN_CHAR);
  _dbus_assert (last != DBUS_DICT_ENTRY_BEGIN_CHAR);

  result = DBUS_VALID;

out:
  _dbus_list_clear (&element_count_stack);
  return result;
}