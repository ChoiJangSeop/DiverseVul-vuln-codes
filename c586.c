void 
mono_reflection_create_dynamic_method (MonoReflectionDynamicMethod *mb)
{
	ReflectionMethodBuilder rmb;
	MonoMethodSignature *sig;
	MonoClass *klass;
	GSList *l;
	int i;

	sig = dynamic_method_to_signature (mb);

	reflection_methodbuilder_from_dynamic_method (&rmb, mb);

	/*
	 * Resolve references.
	 */
	/* 
	 * Every second entry in the refs array is reserved for storing handle_class,
	 * which is needed by the ldtoken implementation in the JIT.
	 */
	rmb.nrefs = mb->nrefs;
	rmb.refs = g_new0 (gpointer, mb->nrefs + 1);
	for (i = 0; i < mb->nrefs; i += 2) {
		MonoClass *handle_class;
		gpointer ref;
		MonoObject *obj = mono_array_get (mb->refs, MonoObject*, i);

		if (strcmp (obj->vtable->klass->name, "DynamicMethod") == 0) {
			MonoReflectionDynamicMethod *method = (MonoReflectionDynamicMethod*)obj;
			/*
			 * The referenced DynamicMethod should already be created by the managed
			 * code, except in the case of circular references. In that case, we store
			 * method in the refs array, and fix it up later when the referenced 
			 * DynamicMethod is created.
			 */
			if (method->mhandle) {
				ref = method->mhandle;
			} else {
				/* FIXME: GC object stored in unmanaged memory */
				ref = method;

				/* FIXME: GC object stored in unmanaged memory */
				method->referenced_by = g_slist_append (method->referenced_by, mb);
			}
			handle_class = mono_defaults.methodhandle_class;
		} else {
			MonoException *ex = NULL;
			ref = resolve_object (mb->module->image, obj, &handle_class, NULL);
			if (!ref)
				ex = mono_get_exception_type_load (NULL, NULL);
			else if (mono_security_get_mode () == MONO_SECURITY_MODE_CORE_CLR)
				ex = mono_security_core_clr_ensure_dynamic_method_resolved_object (ref, handle_class);

			if (ex) {
				g_free (rmb.refs);
				mono_raise_exception (ex);
				return;
			}
		}

		rmb.refs [i] = ref; /* FIXME: GC object stored in unmanaged memory (change also resolve_object() signature) */
		rmb.refs [i + 1] = handle_class;
	}		

	klass = mb->owner ? mono_class_from_mono_type (mono_reflection_type_get_handle ((MonoReflectionType*)mb->owner)) : mono_defaults.object_class;

	mb->mhandle = reflection_methodbuilder_to_mono_method (klass, &rmb, sig);

	/* Fix up refs entries pointing at us */
	for (l = mb->referenced_by; l; l = l->next) {
		MonoReflectionDynamicMethod *method = (MonoReflectionDynamicMethod*)l->data;
		MonoMethodWrapper *wrapper = (MonoMethodWrapper*)method->mhandle;
		gpointer *data;
		
		g_assert (method->mhandle);

		data = (gpointer*)wrapper->method_data;
		for (i = 0; i < GPOINTER_TO_UINT (data [0]); i += 2) {
			if ((data [i + 1] == mb) && (data [i + 1 + 1] == mono_defaults.methodhandle_class))
				data [i + 1] = mb->mhandle;
		}
	}
	g_slist_free (mb->referenced_by);

	g_free (rmb.refs);

	/* ilgen is no longer needed */
	mb->ilgen = NULL;