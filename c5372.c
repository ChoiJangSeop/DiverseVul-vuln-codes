i_alloc_struct_array(gs_memory_t * mem, uint num_elements,
                     gs_memory_type_ptr_t pstype, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj;

#ifdef MEMENTO
    if (Memento_failThisEvent())
        return NULL;
#endif

    ALLOC_CHECK_SIZE(mem,pstype);
#ifdef DEBUG
    if (pstype->enum_ptrs == basic_enum_ptrs) {
        dmprintf2(mem, "  i_alloc_struct_array: called with incorrect structure type (not element), struct='%s', client='%s'\n",
                pstype->sname, cname);
        return NULL;		/* fail */
    }
#endif
    obj = alloc_obj(imem,
                    (ulong) num_elements * pstype->ssize,
                    pstype, ALLOC_DIRECT, cname);
    if_debug7m('A', mem, "[a%d:+<.]%s %s*(%lu=%u*%u) = 0x%lx\n",
               alloc_trace_space(imem), client_name_string(cname),
               struct_type_name_string(pstype),
               (ulong) num_elements * pstype->ssize,
               num_elements, pstype->ssize, (ulong) obj);
    return (char *)obj;
}