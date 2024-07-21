static void destroy_surface(struct swaylock_surface *surface) {
	wl_list_remove(&surface->link);
	if (surface->layer_surface != NULL) {
		zwlr_layer_surface_v1_destroy(surface->layer_surface);
	}
	if (surface->surface != NULL) {
		wl_surface_destroy(surface->surface);
	}
	destroy_buffer(&surface->buffers[0]);
	destroy_buffer(&surface->buffers[1]);
	destroy_buffer(&surface->indicator_buffers[0]);
	destroy_buffer(&surface->indicator_buffers[1]);
	wl_output_destroy(surface->output);
	free(surface);
}