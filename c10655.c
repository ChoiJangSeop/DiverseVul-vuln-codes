decodeJsonStructure(void *dst, const UA_DataType *type, CtxJson *ctx, 
                    ParseCtx *parseCtx, UA_Boolean moveToken) {
    (void) moveToken;
    /* Check the recursion limit */
    if(ctx->depth > UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;

    uintptr_t ptr = (uintptr_t)dst;
    status ret = UA_STATUSCODE_GOOD;
    u8 membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    
    UA_STACKARRAY(DecodeEntry, entries, membersSize);

    for(size_t i = 0; i < membersSize && ret == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = &typelists[!m->namespaceZero][m->memberTypeIndex];

        entries[i].type = mt;
        if(!m->isArray) {
            ptr += m->padding;
            entries[i].fieldName = m->memberName;
            entries[i].fieldPointer = (void*)ptr;
            entries[i].function = decodeJsonJumpTable[mt->typeKind];
            entries[i].found = false;
            ptr += mt->memSize;
        } else {
            ptr += m->padding;
            ptr += sizeof(size_t);
            entries[i].fieldName = m->memberName;
            entries[i].fieldPointer = (void*)ptr;
            entries[i].function = (decodeJsonSignature)Array_decodeJson;
            entries[i].found = false;
            ptr += sizeof(void*);
        }
    }
    
    ret = decodeFields(ctx, parseCtx, entries, membersSize, type);

    ctx->depth--;
    return ret;
}