ciConstant ciEnv::get_constant_by_index_impl(const constantPoolHandle& cpool,
                                             int pool_index, int cache_index,
                                             ciInstanceKlass* accessor) {
  bool ignore_will_link;
  int index = pool_index;
  if (cache_index >= 0) {
    assert(index < 0, "only one kind of index at a time");
    index = cpool->object_to_cp_index(cache_index);
    oop obj = cpool->resolved_references()->obj_at(cache_index);
    if (obj != NULL) {
      if (obj == Universe::the_null_sentinel()) {
        return ciConstant(T_OBJECT, get_object(NULL));
      }
      BasicType bt = T_OBJECT;
      if (cpool->tag_at(index).is_dynamic_constant()) {
        bt = Signature::basic_type(cpool->uncached_signature_ref_at(index));
      }
      if (!is_reference_type(bt)) {
        // we have to unbox the primitive value
        if (!is_java_primitive(bt)) {
          return ciConstant();
        }
        jvalue value;
        BasicType bt2 = java_lang_boxing_object::get_value(obj, &value);
        assert(bt2 == bt, "");
        switch (bt2) {
        case T_DOUBLE:  return ciConstant(value.d);
        case T_FLOAT:   return ciConstant(value.f);
        case T_LONG:    return ciConstant(value.j);
        case T_INT:     return ciConstant(bt2, value.i);
        case T_SHORT:   return ciConstant(bt2, value.s);
        case T_BYTE:    return ciConstant(bt2, value.b);
        case T_CHAR:    return ciConstant(bt2, value.c);
        case T_BOOLEAN: return ciConstant(bt2, value.z);
        default:  return ciConstant();
        }
      }
      ciObject* ciobj = get_object(obj);
      if (ciobj->is_array()) {
        return ciConstant(T_ARRAY, ciobj);
      } else {
        assert(ciobj->is_instance(), "should be an instance");
        return ciConstant(T_OBJECT, ciobj);
      }
    }
  }
  constantTag tag = cpool->tag_at(index);
  if (tag.is_int()) {
    return ciConstant(T_INT, (jint)cpool->int_at(index));
  } else if (tag.is_long()) {
    return ciConstant((jlong)cpool->long_at(index));
  } else if (tag.is_float()) {
    return ciConstant((jfloat)cpool->float_at(index));
  } else if (tag.is_double()) {
    return ciConstant((jdouble)cpool->double_at(index));
  } else if (tag.is_string()) {
    EXCEPTION_CONTEXT;
    oop string = NULL;
    assert(cache_index >= 0, "should have a cache index");
    string = cpool->string_at(index, cache_index, THREAD);
    if (HAS_PENDING_EXCEPTION) {
      CLEAR_PENDING_EXCEPTION;
      record_out_of_memory_failure();
      return ciConstant();
    }
    ciObject* constant = get_object(string);
    if (constant->is_array()) {
      return ciConstant(T_ARRAY, constant);
    } else {
      assert (constant->is_instance(), "must be an instance, or not? ");
      return ciConstant(T_OBJECT, constant);
    }
  } else if (tag.is_unresolved_klass_in_error()) {
    return ciConstant(T_OBJECT, get_unloaded_klass_mirror(NULL));
  } else if (tag.is_klass() || tag.is_unresolved_klass()) {
    ciKlass* klass = get_klass_by_index_impl(cpool, index, ignore_will_link, accessor);
    assert (klass->is_instance_klass() || klass->is_array_klass(),
            "must be an instance or array klass ");
    return ciConstant(T_OBJECT, klass->java_mirror());
  } else if (tag.is_method_type() || tag.is_method_type_in_error()) {
    // must execute Java code to link this CP entry into cache[i].f1
    ciSymbol* signature = get_symbol(cpool->method_type_signature_at(index));
    ciObject* ciobj = get_unloaded_method_type_constant(signature);
    return ciConstant(T_OBJECT, ciobj);
  } else if (tag.is_method_handle() || tag.is_method_handle_in_error()) {
    // must execute Java code to link this CP entry into cache[i].f1
    int ref_kind        = cpool->method_handle_ref_kind_at(index);
    int callee_index    = cpool->method_handle_klass_index_at(index);
    ciKlass* callee     = get_klass_by_index_impl(cpool, callee_index, ignore_will_link, accessor);
    ciSymbol* name      = get_symbol(cpool->method_handle_name_ref_at(index));
    ciSymbol* signature = get_symbol(cpool->method_handle_signature_ref_at(index));
    ciObject* ciobj     = get_unloaded_method_handle_constant(callee, name, signature, ref_kind);
    return ciConstant(T_OBJECT, ciobj);
  } else if (tag.is_dynamic_constant() || tag.is_dynamic_constant_in_error()) {
    return ciConstant(); // not supported
  } else {
    assert(false, "unknown tag: %d (%s)", tag.value(), tag.internal_name());
    return ciConstant();
  }
}