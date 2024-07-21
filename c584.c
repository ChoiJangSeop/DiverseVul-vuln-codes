void
mono_reflection_destroy_dynamic_method (MonoReflectionDynamicMethod *mb)
{
	g_assert (mb);

	if (mb->mhandle)
		mono_runtime_free_method (
			mono_object_get_domain ((MonoObject*)mb), mb->mhandle);