activate_parameters_install_free (ActivateParametersInstall *parameters_install)
{
	if (parameters_install->slot_info) {
		g_object_remove_weak_pointer (G_OBJECT (parameters_install->slot_info), (gpointer *)&parameters_install->slot_info);
	}
	if (parameters_install->parent_window) {
		g_object_remove_weak_pointer (G_OBJECT (parameters_install->parent_window), (gpointer *)&parameters_install->parent_window);
	}
	nautilus_file_list_free (parameters_install->files);
	g_free (parameters_install->activation_directory);
	g_free (parameters_install);
}