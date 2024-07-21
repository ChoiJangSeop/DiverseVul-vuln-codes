Module(asdl_seq * body, PyArena *arena)
{
    mod_ty p;
    p = (mod_ty)PyArena_Malloc(arena, sizeof(*p));
    if (!p)
        return NULL;
    p->kind = Module_kind;
    p->v.Module.body = body;
    return p;
}