static int wddx_stack_init(wddx_stack *stack)
{
	stack->top = 0;
	stack->elements = (void **) safe_emalloc(sizeof(void **), STACK_BLOCK_SIZE, 0);
	stack->max = STACK_BLOCK_SIZE;
	stack->varname = NULL;
	stack->done = 0;

	return SUCCESS;
}