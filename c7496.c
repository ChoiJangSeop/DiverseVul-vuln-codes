PHP_METHOD(imagickkernel, frommatrix)
{
	zval *kernel_array;
	zval *origin_array;
	HashTable *inner_array;
	KernelInfo *kernel_info;
	long num_rows, num_columns = 0;
	int previous_num_columns;
	int row, column;

	zval *pzval_outer;
	zval *pzval_inner;

	int count = 0;
	size_t origin_x, origin_y;
	zval *tmp;

	KernelValueType *values = NULL;
	double notanumber = sqrt((double)-1.0);  /* Special Value : Not A Number */

	previous_num_columns = -1;
	count = 0;
	row = 0;
	origin_array = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|a", &kernel_array, &origin_array) == FAILURE) {
		return;
	}

	num_rows = zend_hash_num_elements(Z_ARRVAL_P(kernel_array));

	if (num_rows == 0) {
		//error - array has zero elements.
		php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_EMPTY TSRMLS_CC);
		return;
	}


	for (row=0 ; row<num_rows ; row++) {
		pzval_outer = zend_hash_index_find(Z_ARRVAL_P(kernel_array), row);
		if (pzval_outer == NULL) {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_UNEVEN TSRMLS_CC);
			goto cleanup;
		}
		ZVAL_DEREF(pzval_outer);

		column = 0;

		if (Z_TYPE_P(pzval_outer) == IS_ARRAY ) {
			inner_array = Z_ARRVAL_P(pzval_outer);
			num_columns = zend_hash_num_elements(inner_array);

			if (num_columns == 0) {
				php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_EMPTY TSRMLS_CC);
				goto cleanup;
			}

			if (values == NULL) {
				values = (KernelValueType *)AcquireAlignedMemory(num_columns, num_rows*sizeof(KernelValueType));
			}

			if (previous_num_columns != -1) {
				if (previous_num_columns != num_columns) {
					php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_UNEVEN TSRMLS_CC);
					goto cleanup;
				}
			}

			previous_num_columns = num_columns;

			for (column=0; column<num_columns ; column++) { 
				pzval_inner = zend_hash_index_find(inner_array, column);
				if (pzval_inner == NULL) {
					php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_UNEVEN TSRMLS_CC);
					goto cleanup;
				}

				ZVAL_DEREF(pzval_inner);
				if (Z_TYPE_P(pzval_inner) == IS_DOUBLE) {
					//It's a float lets use it.
					values[count] = Z_DVAL_P(pzval_inner);
				}
				else if (Z_TYPE_P(pzval_inner) == IS_LONG) {
					//It's a long lets use it.
					values[count] = (float)Z_LVAL_P(pzval_inner);
				}
				else if (Z_TYPE_P(pzval_inner) == IS_FALSE) { 
					//It's false, use nan
					values[count] = notanumber;
				}
				else {
					php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_BAD_VALUE TSRMLS_CC);
					goto cleanup;
				}
				count++;
			}
		}
		else {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_UNEVEN TSRMLS_CC);
			goto cleanup;
		}
	}

	if (origin_array == NULL) {
		if (((num_columns%2) == 0) || ((num_rows%2) == 0)) {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ORIGIN_REQUIRED TSRMLS_CC);
			goto cleanup;
		}
		origin_x = (num_columns - 1) >> 1;
		origin_y = (num_rows - 1) >> 1;
	}
	else {
		HashTable *origin_array_ht;
		origin_array_ht = Z_ARRVAL_P(origin_array);
		tmp = zend_hash_index_find(origin_array_ht, 0);
		if (tmp != NULL) {
			ZVAL_DEREF(tmp);
			origin_x = Z_LVAL_P(tmp);
		}
		else {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ORIGIN_REQUIRED TSRMLS_CC);
			goto cleanup;
		}
		tmp = zend_hash_index_find(origin_array_ht, 1);
		if (tmp != NULL) {
			ZVAL_DEREF(tmp);
			origin_y = Z_LVAL_P(tmp);
		}
		else {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ORIGIN_REQUIRED TSRMLS_CC);
			goto cleanup;
		}
	}

	kernel_info = imagick_createKernel(values, num_columns, num_rows, origin_x, origin_y);
	createKernelZval(return_value, kernel_info TSRMLS_CC);

	return;

cleanup:
	if (values != NULL) {
		RelinquishAlignedMemory(values);
	}
}