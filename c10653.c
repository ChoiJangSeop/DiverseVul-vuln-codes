encodeJsonStructure(const void *src, const UA_DataType *type, CtxJson *ctx) {
    /* Check the recursion limit */
    if(ctx->depth > UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;

    status ret = writeJsonObjStart(ctx);

    uintptr_t ptr = (uintptr_t) src;
    u8 membersSize = type->membersSize;
    const UA_DataType * typelists[2] = {UA_TYPES, &type[-type->typeIndex]};
    for(size_t i = 0; i < membersSize && ret == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = &typelists[!m->namespaceZero][m->memberTypeIndex];

        if(m->memberName != NULL && *m->memberName != 0)
            ret |= writeJsonKey(ctx, m->memberName);

        if(!m->isArray) {
            ptr += m->padding;
            size_t memSize = mt->memSize;
            ret |= encodeJsonJumpTable[mt->typeKind]((const void*) ptr, mt, ctx);
            ptr += memSize;
        } else {
            ptr += m->padding;
            const size_t length = *((const size_t*) ptr);
            ptr += sizeof (size_t);
            ret |= encodeJsonArray(ctx, *(void * const *)ptr, length, mt);
            ptr += sizeof (void*);
        }
    }

    ret |= writeJsonObjEnd(ctx);

    ctx->depth--;
    return ret;
}