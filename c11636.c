void LinkResolver::resolve_handle_call(CallInfo& result, KlassHandle resolved_klass,
                                       Symbol* method_name, Symbol* method_signature,
                                       KlassHandle current_klass,
                                       TRAPS) {
  // JSR 292:  this must be an implicitly generated method MethodHandle.invokeExact(*...) or similar
  assert(resolved_klass() == SystemDictionary::MethodHandle_klass(), "");
  assert(MethodHandles::is_signature_polymorphic_name(method_name), "");
  methodHandle resolved_method;
  Handle       resolved_appendix;
  Handle       resolved_method_type;
  lookup_polymorphic_method(resolved_method, resolved_klass,
                            method_name, method_signature,
                            current_klass, &resolved_appendix, &resolved_method_type, CHECK);
  result.set_handle(resolved_method, resolved_appendix, resolved_method_type, CHECK);
}