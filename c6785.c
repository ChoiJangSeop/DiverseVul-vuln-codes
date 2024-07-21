static uint32_t calc_gc_buffer_size(zend_generator *generator) /* {{{ */
{
	uint32_t size = 4; /* value, key, retval, values */
	if (generator->execute_data) {
		zend_execute_data *execute_data = generator->execute_data;
		zend_op_array *op_array = &EX(func)->op_array;

		/* Compiled variables */
		if (!(EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE)) {
			size += op_array->last_var;
		}
		/* Extra args */
		if (EX_CALL_INFO() & ZEND_CALL_FREE_EXTRA_ARGS) {
			size += EX_NUM_ARGS() - op_array->num_args;
		}
		size += Z_TYPE(execute_data->This) == IS_OBJECT; /* $this */
		size += (EX_CALL_INFO() & ZEND_CALL_CLOSURE) != 0; /* Closure object */

		/* Yield from root references */
		if (generator->node.children == 0) {
			zend_generator *root = generator->node.ptr.root;
			while (root != generator) {
				root = zend_generator_get_child(&root->node, generator);
				size++;
			}
		}
	}
	return size;
}