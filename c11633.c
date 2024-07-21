void LinkResolver::resolve_handle_call(CallInfo& result,
                                       const LinkInfo& link_info,
                                       TRAPS) {
  // JSR 292:  this must be an implicitly generated method MethodHandle.invokeExact(*...) or similar
  Klass* resolved_klass = link_info.resolved_klass();
  assert(resolved_klass == SystemDictionary::MethodHandle_klass() ||
         resolved_klass == SystemDictionary::VarHandle_klass(), "");
  assert(MethodHandles::is_signature_polymorphic_name(link_info.name()), "");
  Handle       resolved_appendix;
  Handle       resolved_method_type;
  methodHandle resolved_method = lookup_polymorphic_method(link_info,
                                       &resolved_appendix, &resolved_method_type, CHECK);
  result.set_handle(resolved_klass, resolved_method, resolved_appendix, resolved_method_type, CHECK);
}