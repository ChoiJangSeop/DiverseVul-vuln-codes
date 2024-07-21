void InstanceKlass::metaspace_pointers_do(MetaspaceClosure* it) {
  Klass::metaspace_pointers_do(it);

  if (log_is_enabled(Trace, cds)) {
    ResourceMark rm;
    log_trace(cds)("Iter(InstanceKlass): %p (%s)", this, external_name());
  }

  it->push(&_annotations);
  it->push((Klass**)&_array_klasses);
  if (!is_rewritten()) {
    it->push(&_constants, MetaspaceClosure::_writable);
  } else {
    it->push(&_constants);
  }
  it->push(&_inner_classes);
#if INCLUDE_JVMTI
  it->push(&_previous_versions);
#endif
  it->push(&_methods);
  it->push(&_default_methods);
  it->push(&_local_interfaces);
  it->push(&_transitive_interfaces);
  it->push(&_method_ordering);
  if (!is_rewritten()) {
    it->push(&_default_vtable_indices, MetaspaceClosure::_writable);
  } else {
    it->push(&_default_vtable_indices);
  }
  it->push(&_fields);

  if (itable_length() > 0) {
    itableOffsetEntry* ioe = (itableOffsetEntry*)start_of_itable();
    int method_table_offset_in_words = ioe->offset()/wordSize;
    int nof_interfaces = (method_table_offset_in_words - itable_offset_in_words())
                         / itableOffsetEntry::size();

    for (int i = 0; i < nof_interfaces; i ++, ioe ++) {
      if (ioe->interface_klass() != NULL) {
        it->push(ioe->interface_klass_addr());
        itableMethodEntry* ime = ioe->first_method_entry(this);
        int n = klassItable::method_count_for_interface(ioe->interface_klass());
        for (int index = 0; index < n; index ++) {
          it->push(ime[index].method_addr());
        }
      }
    }
  }

  it->push(&_nest_members);
  it->push(&_permitted_subclasses);
  it->push(&_record_components);
}