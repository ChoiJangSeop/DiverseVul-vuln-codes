Handle MethodHandles::resolve_MemberName(Handle mname, KlassHandle caller, TRAPS) {
  Handle empty;
  assert(java_lang_invoke_MemberName::is_instance(mname()), "");

  if (java_lang_invoke_MemberName::vmtarget(mname()) != NULL) {
    // Already resolved.
    DEBUG_ONLY(int vmindex = java_lang_invoke_MemberName::vmindex(mname()));
    assert(vmindex >= Method::nonvirtual_vtable_index, "");
    return mname;
  }

  Handle defc_oop(THREAD, java_lang_invoke_MemberName::clazz(mname()));
  Handle name_str(THREAD, java_lang_invoke_MemberName::name( mname()));
  Handle type_str(THREAD, java_lang_invoke_MemberName::type( mname()));
  int    flags    =       java_lang_invoke_MemberName::flags(mname());
  int    ref_kind =       (flags >> REFERENCE_KIND_SHIFT) & REFERENCE_KIND_MASK;
  if (!ref_kind_is_valid(ref_kind)) {
    THROW_MSG_(vmSymbols::java_lang_InternalError(), "obsolete MemberName format", empty);
  }

  DEBUG_ONLY(int old_vmindex);
  assert((old_vmindex = java_lang_invoke_MemberName::vmindex(mname())) == 0, "clean input");

  if (defc_oop.is_null() || name_str.is_null() || type_str.is_null()) {
    THROW_MSG_(vmSymbols::java_lang_IllegalArgumentException(), "nothing to resolve", empty);
  }

  instanceKlassHandle defc;
  {
    Klass* defc_klass = java_lang_Class::as_Klass(defc_oop());
    if (defc_klass == NULL)  return empty;  // a primitive; no resolution possible
    if (!defc_klass->oop_is_instance()) {
      if (!defc_klass->oop_is_array())  return empty;
      defc_klass = SystemDictionary::Object_klass();
    }
    defc = instanceKlassHandle(THREAD, defc_klass);
  }
  if (defc.is_null()) {
    THROW_MSG_(vmSymbols::java_lang_InternalError(), "primitive class", empty);
  }
  defc->link_class(CHECK_(empty));  // possible safepoint

  // convert the external string name to an internal symbol
  TempNewSymbol name = java_lang_String::as_symbol_or_null(name_str());
  if (name == NULL)  return empty;  // no such name
  if (name == vmSymbols::class_initializer_name())
    return empty; // illegal name

  vmIntrinsics::ID mh_invoke_id = vmIntrinsics::_none;
  if ((flags & ALL_KINDS) == IS_METHOD &&
      (defc() == SystemDictionary::MethodHandle_klass()) &&
      (ref_kind == JVM_REF_invokeVirtual ||
       ref_kind == JVM_REF_invokeSpecial ||
       // static invocation mode is required for _linkToVirtual, etc.:
       ref_kind == JVM_REF_invokeStatic)) {
    vmIntrinsics::ID iid = signature_polymorphic_name_id(name);
    if (iid != vmIntrinsics::_none &&
        ((ref_kind == JVM_REF_invokeStatic) == is_signature_polymorphic_static(iid))) {
      // Virtual methods invoke and invokeExact, plus internal invokers like _invokeBasic.
      // For a static reference it could an internal linkage routine like _linkToVirtual, etc.
      mh_invoke_id = iid;
    }
  }

  // convert the external string or reflective type to an internal signature
  TempNewSymbol type = lookup_signature(type_str(), (mh_invoke_id != vmIntrinsics::_none), CHECK_(empty));
  if (type == NULL)  return empty;  // no such signature exists in the VM

  // Time to do the lookup.
  switch (flags & ALL_KINDS) {
  case IS_METHOD:
    {
      CallInfo result;
      {
        assert(!HAS_PENDING_EXCEPTION, "");
        if (ref_kind == JVM_REF_invokeStatic) {
          LinkResolver::resolve_static_call(result,
                        defc, name, type, caller, caller.not_null(), false, THREAD);
        } else if (ref_kind == JVM_REF_invokeInterface) {
          LinkResolver::resolve_interface_call(result, Handle(), defc,
                        defc, name, type, caller, caller.not_null(), false, THREAD);
        } else if (mh_invoke_id != vmIntrinsics::_none) {
          assert(!is_signature_polymorphic_static(mh_invoke_id), "");
          LinkResolver::resolve_handle_call(result,
                        defc, name, type, caller, THREAD);
        } else if (ref_kind == JVM_REF_invokeSpecial) {
          LinkResolver::resolve_special_call(result,
                        Handle(), defc, name, type, caller, caller.not_null(), THREAD);
        } else if (ref_kind == JVM_REF_invokeVirtual) {
          LinkResolver::resolve_virtual_call(result, Handle(), defc,
                        defc, name, type, caller, caller.not_null(), false, THREAD);
        } else {
          assert(false, err_msg("ref_kind=%d", ref_kind));
        }
        if (HAS_PENDING_EXCEPTION) {
          return empty;
        }
      }
      if (result.resolved_appendix().not_null()) {
        // The resolved MemberName must not be accompanied by an appendix argument,
        // since there is no way to bind this value into the MemberName.
        // Caller is responsible to prevent this from happening.
        THROW_MSG_(vmSymbols::java_lang_InternalError(), "appendix", empty);
      }
      oop mname2 = init_method_MemberName(mname, result);
      return Handle(THREAD, mname2);
    }
  case IS_CONSTRUCTOR:
    {
      CallInfo result;
      {
        assert(!HAS_PENDING_EXCEPTION, "");
        if (name == vmSymbols::object_initializer_name()) {
          LinkResolver::resolve_special_call(result,
                        Handle(), defc, name, type, caller, caller.not_null(), THREAD);
        } else {
          break;                // will throw after end of switch
        }
        if (HAS_PENDING_EXCEPTION) {
          return empty;
        }
      }
      assert(result.is_statically_bound(), "");
      oop mname2 = init_method_MemberName(mname, result);
      return Handle(THREAD, mname2);
    }
  case IS_FIELD:
    {
      fieldDescriptor result; // find_field initializes fd if found
      {
        assert(!HAS_PENDING_EXCEPTION, "");
        LinkResolver::resolve_field(result, defc, name, type, caller, Bytecodes::_nop, false, false, THREAD);
        if (HAS_PENDING_EXCEPTION) {
          return empty;
        }
      }
      oop mname2 = init_field_MemberName(mname, result, ref_kind_is_setter(ref_kind));
      return Handle(THREAD, mname2);
    }
  default:
    THROW_MSG_(vmSymbols::java_lang_InternalError(), "unrecognized MemberName format", empty);
  }

  return empty;
}