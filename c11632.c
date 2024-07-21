void LinkResolver::resolve_handle_call(CallInfo& result,
                                       const LinkInfo& link_info,
                                       TRAPS) {
  // JSR 292:  this must be an implicitly generated method MethodHandle.invokeExact(*...) or similar
  Klass* resolved_klass = link_info.resolved_klass();
  assert(resolved_klass == vmClasses::MethodHandle_klass() ||
         resolved_klass == vmClasses::VarHandle_klass(), "");
  assert(MethodHandles::is_signature_polymorphic_name(link_info.name()), "");
  Handle resolved_appendix;
  Method* resolved_method = lookup_polymorphic_method(link_info, &resolved_appendix, CHECK);
  result.set_handle(resolved_klass, methodHandle(THREAD, resolved_method), resolved_appendix, CHECK);
}