ciConstant ciEnv::get_constant_by_index_impl(constantPoolHandle cpool,
                                             int pool_index, int cache_index,
                                             ciInstanceKlass* accessor) {
  bool ignore_will_link;
  EXCEPTION_CONTEXT;
  int index = pool_index;
  if (cache_index >= 0) {
    assert(index < 0, "only one kind of index at a time");
    oop obj = cpool->resolved_references()->obj_at(cache_index);
    if (obj != NULL) {
      ciObject* ciobj = get_object(obj);
      if (ciobj->is_array()) {
        return ciConstant(T_ARRAY, ciobj);
      } else {
        assert(ciobj->is_instance(), "should be an instance");
        return ciConstant(T_OBJECT, ciobj);
      }
    }
    index = cpool->object_to_cp_index(cache_index);
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
    oop string = NULL;
    assert(cache_index >= 0, "should have a cache index");
    if (cpool->is_pseudo_string_at(index)) {
      string = cpool->pseudo_string_at(index, cache_index);
    } else {
      string = cpool->string_at(index, cache_index, THREAD);
      if (HAS_PENDING_EXCEPTION) {
        CLEAR_PENDING_EXCEPTION;
        record_out_of_memory_failure();
        return ciConstant();
      }
    }
    ciObject* constant = get_object(string);
    if (constant->is_array()) {
      return ciConstant(T_ARRAY, constant);
    } else {
      assert (constant->is_instance(), "must be an instance, or not? ");
      return ciConstant(T_OBJECT, constant);
    }
  } else if (tag.is_klass() || tag.is_unresolved_klass()) {
    // 4881222: allow ldc to take a class type
    ciKlass* klass = get_klass_by_index_impl(cpool, index, ignore_will_link, accessor);
    if (HAS_PENDING_EXCEPTION) {
      CLEAR_PENDING_EXCEPTION;
      record_out_of_memory_failure();
      return ciConstant();
    }
    assert (klass->is_instance_klass() || klass->is_array_klass(),
            "must be an instance or array klass ");
    return ciConstant(T_OBJECT, klass->java_mirror());
  } else if (tag.is_method_type()) {
    // must execute Java code to link this CP entry into cache[i].f1
    ciSymbol* signature = get_symbol(cpool->method_type_signature_at(index));
    ciObject* ciobj = get_unloaded_method_type_constant(signature);
    return ciConstant(T_OBJECT, ciobj);
  } else if (tag.is_method_handle()) {
    // must execute Java code to link this CP entry into cache[i].f1
    int ref_kind        = cpool->method_handle_ref_kind_at(index);
    int callee_index    = cpool->method_handle_klass_index_at(index);
    ciKlass* callee     = get_klass_by_index_impl(cpool, callee_index, ignore_will_link, accessor);
    ciSymbol* name      = get_symbol(cpool->method_handle_name_ref_at(index));
    ciSymbol* signature = get_symbol(cpool->method_handle_signature_ref_at(index));
    ciObject* ciobj     = get_unloaded_method_handle_constant(callee, name, signature, ref_kind);
    return ciConstant(T_OBJECT, ciobj);
  } else {
    ShouldNotReachHere();
    return ciConstant();
  }
}