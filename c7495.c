PHP_METHOD(imagickkernel, frommatrix)
{
	zval *kernel_array;
	zval *origin_array;
	HashTable *inner_array;
	KernelInfo *kernel_info;
	long num_rows, num_columns = 0;
	int previous_num_columns;
	int row, column;

	HashTable *origin_array_ht;
	zval **ppzval_outer;
	zval **ppzval_inner;

	int count = 0;
	size_t origin_x, origin_y;
	zval **tmp;

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
		if (zend_hash_index_find(Z_ARRVAL_P(kernel_array), row, (void **) &ppzval_outer) != SUCCESS) {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_UNEVEN TSRMLS_CC);
			goto cleanup;
		}

		column = 0;

		if (Z_TYPE_PP(ppzval_outer) == IS_ARRAY ) {
			//parse this row
			inner_array = Z_ARRVAL_PP(ppzval_outer);
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
				if (zend_hash_index_find(inner_array, column, (void **) &ppzval_inner) != SUCCESS) {
					php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ERROR_UNEVEN TSRMLS_CC);
					goto cleanup;
				}

				if (Z_TYPE_PP(ppzval_inner) == IS_DOUBLE) {
					//It's a float lets use it.
					values[count] = Z_DVAL_PP(ppzval_inner);
				}
				else if (Z_TYPE_PP(ppzval_inner) == IS_LONG) {
					//It's a long lets use it.
					values[count] = (float)Z_LVAL_PP(ppzval_inner);
				}
				else if (Z_TYPE_PP(ppzval_inner) == IS_BOOL && Z_BVAL_PP(ppzval_inner) == 0) { 
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
		origin_array_ht = Z_ARRVAL_P(origin_array);
		if (zend_hash_index_find(origin_array_ht, 0, (void**)&tmp) == SUCCESS) {
			origin_x = Z_LVAL_PP(tmp);
		}
		else {
			php_imagick_throw_exception(IMAGICKKERNEL_CLASS, MATRIX_ORIGIN_REQUIRED TSRMLS_CC);
			goto cleanup;
		}

		if (zend_hash_index_find(origin_array_ht, 1, (void**)&tmp) == SUCCESS) {
			origin_y = Z_LVAL_PP(tmp);
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