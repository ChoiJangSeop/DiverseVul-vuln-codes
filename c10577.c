Mat_VarFree(matvar_t *matvar)
{
    size_t nelems = 0;

    if ( NULL == matvar )
        return;
    if ( NULL != matvar->dims ) {
        nelems = 1;
        SafeMulDims(matvar, &nelems);
        free(matvar->dims);
    }
    if ( NULL != matvar->data ) {
        switch (matvar->class_type ) {
            case MAT_C_STRUCT:
                if ( !matvar->mem_conserve ) {
                    matvar_t **fields = (matvar_t**)matvar->data;
                    size_t nelems_x_nfields, i;
                    SafeMul(&nelems_x_nfields, nelems, matvar->internal->num_fields);
                    for ( i = 0; i < nelems_x_nfields; i++ )
                        Mat_VarFree(fields[i]);

                    free(matvar->data);
                }
                break;
            case MAT_C_CELL:
                if ( !matvar->mem_conserve ) {
                    matvar_t **cells = (matvar_t**)matvar->data;
                    size_t i;
                    for ( i = 0; i < nelems; i++ )
                        Mat_VarFree(cells[i]);

                    free(matvar->data);
                }
                break;
            case MAT_C_SPARSE:
                if ( !matvar->mem_conserve ) {
                    mat_sparse_t *sparse;
                    sparse = (mat_sparse_t*)matvar->data;
                    if ( sparse->ir != NULL )
                        free(sparse->ir);
                    if ( sparse->jc != NULL )
                        free(sparse->jc);
                    if ( matvar->isComplex && NULL != sparse->data ) {
                        ComplexFree((mat_complex_split_t*)sparse->data);
                    } else if ( sparse->data != NULL ) {
                        free(sparse->data);
                    }
                    free(sparse);
                }
                break;
            case MAT_C_DOUBLE:
            case MAT_C_SINGLE:
            case MAT_C_INT64:
            case MAT_C_UINT64:
            case MAT_C_INT32:
            case MAT_C_UINT32:
            case MAT_C_INT16:
            case MAT_C_UINT16:
            case MAT_C_INT8:
            case MAT_C_UINT8:
            case MAT_C_CHAR:
                if ( !matvar->mem_conserve ) {
                    if ( matvar->isComplex ) {
                        ComplexFree((mat_complex_split_t*)matvar->data);
                    } else {
                        free(matvar->data);
                    }
                }
                break;
            case MAT_C_FUNCTION:
                if ( !matvar->mem_conserve ) {
                    free(matvar->data);
                }
                break;
            case MAT_C_EMPTY:
            case MAT_C_OBJECT:
            case MAT_C_OPAQUE:
                break;
        }
    }

    if ( NULL != matvar->internal ) {
#if HAVE_ZLIB
        if ( matvar->compression == MAT_COMPRESSION_ZLIB ) {
            inflateEnd(matvar->internal->z);
            free(matvar->internal->z);
            if ( matvar->class_type == MAT_C_SPARSE && NULL != matvar->internal->data ) {
                mat_sparse_t *sparse;
                sparse = (mat_sparse_t*)matvar->internal->data;
                if ( sparse->ir != NULL )
                    free(sparse->ir);
                if ( sparse->jc != NULL )
                    free(sparse->jc);
                if ( matvar->isComplex && NULL != sparse->data ) {
                    ComplexFree((mat_complex_split_t*)sparse->data);
                } else if ( sparse->data != NULL ) {
                    free(sparse->data);
                }
                free(sparse);
            }
            else if ( matvar->isComplex && NULL != matvar->internal->data ) {
                ComplexFree((mat_complex_split_t*)matvar->internal->data);
            } else if ( NULL != matvar->internal->data ) {
                free(matvar->internal->data);
            }
        }
#endif
#if defined(MAT73) && MAT73
        if ( -1 < matvar->internal->id ) {
            switch ( H5Iget_type(matvar->internal->id) ) {
                case H5I_GROUP:
                    H5Gclose(matvar->internal->id);
                    matvar->internal->id = -1;
                    break;
                case H5I_DATASET:
                    H5Dclose(matvar->internal->id);
                    matvar->internal->id = -1;
                    break;
                default:
                    break;
            }
        }
        if ( 0 < matvar->internal->hdf5_ref ) {
            switch ( H5Iget_type(matvar->internal->id) ) {
                case H5I_GROUP:
                    H5Gclose(matvar->internal->id);
                    matvar->internal->hdf5_ref = -1;
                    break;
                case H5I_DATASET:
                    H5Dclose(matvar->internal->id);
                    matvar->internal->hdf5_ref = -1;
                    break;
                default:
                    break;
            }
        }
        if ( NULL != matvar->internal->hdf5_name ) {
            free(matvar->internal->hdf5_name);
            matvar->internal->hdf5_name = NULL;
        }
#endif
        if ( NULL != matvar->internal->fieldnames &&
             matvar->internal->num_fields > 0 ) {
            size_t i;
            for ( i = 0; i < matvar->internal->num_fields; i++ ) {
                if ( NULL != matvar->internal->fieldnames[i] )
                    free(matvar->internal->fieldnames[i]);
            }
            free(matvar->internal->fieldnames);
        }
        free(matvar->internal);
        matvar->internal = NULL;
    }
    if ( NULL != matvar->name )
        free(matvar->name);
    free(matvar);
}