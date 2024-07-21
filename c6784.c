static HashTable *zend_generator_get_gc(zval *object, zval **table, int *n) /* {{{ */
{
	zend_generator *generator = (zend_generator*) Z_OBJ_P(object);
	zend_execute_data *execute_data = generator->execute_data;
	zend_op_array *op_array;
	zval *gc_buffer;
	uint32_t gc_buffer_size;

	if (!execute_data) {
		/* If the generator has been closed, it can only hold on to three values: The value, key
		 * and retval. These three zvals are stored sequentially starting at &generator->value. */
		*table = &generator->value;
		*n = 3;
		return NULL;
	}

	op_array = &EX(func)->op_array;
	gc_buffer_size = calc_gc_buffer_size(generator);
	if (generator->gc_buffer_size < gc_buffer_size) {
		generator->gc_buffer = safe_erealloc(generator->gc_buffer, sizeof(zval), gc_buffer_size, 0);
		generator->gc_buffer_size = gc_buffer_size;
	}

	*n = gc_buffer_size;
	*table = gc_buffer = generator->gc_buffer;

	ZVAL_COPY_VALUE(gc_buffer++, &generator->value);
	ZVAL_COPY_VALUE(gc_buffer++, &generator->key);
	ZVAL_COPY_VALUE(gc_buffer++, &generator->retval);
	ZVAL_COPY_VALUE(gc_buffer++, &generator->values);

	if (!(EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE)) {
		uint32_t i, num_cvs = EX(func)->op_array.last_var;
		for (i = 0; i < num_cvs; i++) {
			ZVAL_COPY_VALUE(gc_buffer++, EX_VAR_NUM(i));
		}
	}

	if (EX_CALL_INFO() & ZEND_CALL_FREE_EXTRA_ARGS) {
		zval *zv = EX_VAR_NUM(op_array->last_var + op_array->T);
		zval *end = zv + (EX_NUM_ARGS() - op_array->num_args);
		while (zv != end) {
			ZVAL_COPY_VALUE(gc_buffer++, zv++);
		}
	}

	if (Z_TYPE(execute_data->This) == IS_OBJECT) {
		ZVAL_OBJ(gc_buffer++, Z_OBJ(execute_data->This));
	}
	if (EX_CALL_INFO() & ZEND_CALL_CLOSURE) {
		ZVAL_OBJ(gc_buffer++, (zend_object *) EX(func)->common.prototype);
	}

	if (generator->node.children == 0) {
		zend_generator *root = generator->node.ptr.root;
		while (root != generator) {
			ZVAL_OBJ(gc_buffer++, &root->std);
			root = zend_generator_get_child(&root->node, generator);
		}
	}

	if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
		return execute_data->symbol_table;
	} else {
		return NULL;
	}
}