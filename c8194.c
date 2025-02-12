Mat_VarReadNextInfo4(mat_t *mat)
{
    int       M,O,data_type,class_type;
    mat_int32_t tmp;
    long      nBytes;
    size_t    readresult;
    matvar_t *matvar = NULL;
    union {
        mat_uint32_t u;
        mat_uint8_t  c[4];
    } endian;

    if ( mat == NULL || mat->fp == NULL )
        return NULL;
    else if ( NULL == (matvar = Mat_VarCalloc()) )
        return NULL;

    readresult = fread(&tmp,sizeof(int),1,(FILE*)mat->fp);
    if ( 1 != readresult ) {
        Mat_VarFree(matvar);
        return NULL;
    }

    endian.u = 0x01020304;

    /* See if MOPT may need byteswapping */
    if ( tmp < 0 || tmp > 4052 ) {
        if ( Mat_int32Swap(&tmp) > 4052 ) {
            Mat_VarFree(matvar);
            return NULL;
        }
    }

    M = (int)floor(tmp / 1000.0);
    switch ( M ) {
        case 0:
            /* IEEE little endian */
            mat->byteswap = endian.c[0] != 4;
            break;
        case 1:
            /* IEEE big endian */
            mat->byteswap = endian.c[0] != 1;
            break;
        default:
            /* VAX, Cray, or bogus */
            Mat_VarFree(matvar);
            return NULL;
    }

    tmp -= M*1000;
    O = (int)floor(tmp / 100.0);
    /* O must be zero */
    if ( 0 != O ) {
        Mat_VarFree(matvar);
        return NULL;
    }

    tmp -= O*100;
    data_type = (int)floor(tmp / 10.0);
    /* Convert the V4 data type */
    switch ( data_type ) {
        case 0:
            matvar->data_type = MAT_T_DOUBLE;
            break;
        case 1:
            matvar->data_type = MAT_T_SINGLE;
            break;
        case 2:
            matvar->data_type = MAT_T_INT32;
            break;
        case 3:
            matvar->data_type = MAT_T_INT16;
            break;
        case 4:
            matvar->data_type = MAT_T_UINT16;
            break;
        case 5:
            matvar->data_type = MAT_T_UINT8;
            break;
        default:
            Mat_VarFree(matvar);
            return NULL;
    }

    tmp -= data_type*10;
    class_type = (int)floor(tmp / 1.0);
    switch ( class_type ) {
        case 0:
            matvar->class_type = MAT_C_DOUBLE;
            break;
        case 1:
            matvar->class_type = MAT_C_CHAR;
            break;
        case 2:
            matvar->class_type = MAT_C_SPARSE;
            break;
        default:
            Mat_VarFree(matvar);
            return NULL;
    }

    matvar->rank = 2;
    matvar->dims = (size_t*)calloc(2, sizeof(*matvar->dims));
    if ( NULL == matvar->dims ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    readresult = fread(&tmp,sizeof(int),1,(FILE*)mat->fp);
    if ( mat->byteswap )
        Mat_int32Swap(&tmp);
    matvar->dims[0] = tmp;
    if ( 1 != readresult ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    readresult = fread(&tmp,sizeof(int),1,(FILE*)mat->fp);
    if ( mat->byteswap )
        Mat_int32Swap(&tmp);
    matvar->dims[1] = tmp;
    if ( 1 != readresult ) {
        Mat_VarFree(matvar);
        return NULL;
    }

    readresult = fread(&(matvar->isComplex),sizeof(int),1,(FILE*)mat->fp);
    if ( 1 != readresult ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    if ( matvar->isComplex && MAT_C_CHAR == matvar->class_type ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    readresult = fread(&tmp,sizeof(int),1,(FILE*)mat->fp);
    if ( 1 != readresult ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    if ( mat->byteswap )
        Mat_int32Swap(&tmp);
    /* Check that the length of the variable name is at least 1 */
    if ( tmp < 1 ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    matvar->name = (char*)malloc(tmp);
    if ( NULL == matvar->name ) {
        Mat_VarFree(matvar);
        return NULL;
    }
    readresult = fread(matvar->name,1,tmp,(FILE*)mat->fp);
    if ( tmp != readresult ) {
        Mat_VarFree(matvar);
        return NULL;
    }

    matvar->internal->datapos = ftell((FILE*)mat->fp);
    if ( matvar->internal->datapos == -1L ) {
        Mat_VarFree(matvar);
        Mat_Critical("Couldn't determine file position");
        return NULL;
    }
    {
        int err;
        size_t tmp2 = Mat_SizeOf(matvar->data_type);
        if ( matvar->isComplex )
            tmp2 *= 2;
        err = SafeMulDims(matvar, &tmp2);
        if ( err ) {
            Mat_VarFree(matvar);
            Mat_Critical("Integer multiplication overflow");
            return NULL;
        }

        nBytes = (long)tmp2;
    }
    (void)fseek((FILE*)mat->fp,nBytes,SEEK_CUR);

    return matvar;
}