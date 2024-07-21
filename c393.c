mark_trusted_callback (NautilusFile  *file,
		       GFile         *result_location,
		       GError        *error,
		       gpointer       callback_data)
{
	ActivateParametersDesktop *parameters;

	parameters = callback_data;
	if (error) {
		eel_show_error_dialog (_("Unable to mark launcher trusted (executable)"),
				       error->message,
				       parameters->parent_window);
	}
	
	activate_parameters_desktop_free (parameters);
}