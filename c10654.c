addMultiArrayContentJSON(CtxJson *ctx, void* array, const UA_DataType *type, 
                         size_t *index, UA_UInt32 *arrayDimensions, size_t dimensionIndex, 
                         size_t dimensionSize) {
    /* Check the recursion limit */
    if(ctx->depth > UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;
    
    /* Stop recursion: The inner Arrays are written */
    status ret;
    if(dimensionIndex == (dimensionSize - 1)) {
        ret = encodeJsonArray(ctx, ((u8*)array) + (type->memSize * *index),
                              arrayDimensions[dimensionIndex], type);
        (*index) += arrayDimensions[dimensionIndex];
        return ret;
    }

    /* Recurse to the next dimension */
    ret = writeJsonArrStart(ctx);
    for(size_t i = 0; i < arrayDimensions[dimensionIndex]; i++) {
        ret |= writeJsonCommaIfNeeded(ctx);
        ret |= addMultiArrayContentJSON(ctx, array, type, index, arrayDimensions,
                                        dimensionIndex + 1, dimensionSize);
        ctx->commaNeeded[ctx->depth] = true;
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }
    ret |= writeJsonArrEnd(ctx);
    return ret;
}