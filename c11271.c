int main(int argc, char **argv) {
	swaylock_log_init(LOG_ERROR);
	initialize_pw_backend(argc, argv);
	srand(time(NULL));

	enum line_mode line_mode = LM_LINE;
	state.failed_attempts = 0;
	state.args = (struct swaylock_args){
		.mode = BACKGROUND_MODE_FILL,
		.font = strdup("sans-serif"),
		.font_size = 0,
		.radius = 50,
		.thickness = 10,
		.indicator_x_position = 0,
		.indicator_y_position = 0,
		.override_indicator_x_position = false,
		.override_indicator_y_position = false,
		.ignore_empty = false,
		.show_indicator = true,
		.show_caps_lock_indicator = false,
		.show_caps_lock_text = true,
		.show_keyboard_layout = false,
		.hide_keyboard_layout = false,
		.show_failed_attempts = false,
		.indicator_idle_visible = false
	};
	wl_list_init(&state.images);
	set_default_colors(&state.args.colors);

	char *config_path = NULL;
	int result = parse_options(argc, argv, NULL, NULL, &config_path);
	if (result != 0) {
		free(config_path);
		return result;
	}
	if (!config_path) {
		config_path = get_config_path();
	}

	if (config_path) {
		swaylock_log(LOG_DEBUG, "Found config at %s", config_path);
		int config_status = load_config(config_path, &state, &line_mode);
		free(config_path);
		if (config_status != 0) {
			free(state.args.font);
			return config_status;
		}
	}

	if (argc > 1) {
		swaylock_log(LOG_DEBUG, "Parsing CLI Args");
		int result = parse_options(argc, argv, &state, &line_mode, NULL);
		if (result != 0) {
			free(state.args.font);
			return result;
		}
	}

	if (line_mode == LM_INSIDE) {
		state.args.colors.line = state.args.colors.inside;
	} else if (line_mode == LM_RING) {
		state.args.colors.line = state.args.colors.ring;
	}

#ifdef __linux__
	// Most non-linux platforms require root to mlock()
	if (mlock(state.password.buffer, sizeof(state.password.buffer)) != 0) {
		swaylock_log(LOG_ERROR, "Unable to mlock() password memory.");
		return EXIT_FAILURE;
	}
#endif

	wl_list_init(&state.surfaces);
	state.xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	state.display = wl_display_connect(NULL);
	if (!state.display) {
		free(state.args.font);
		swaylock_log(LOG_ERROR, "Unable to connect to the compositor. "
				"If your compositor is running, check or set the "
				"WAYLAND_DISPLAY environment variable.");
		return EXIT_FAILURE;
	}

	struct wl_registry *registry = wl_display_get_registry(state.display);
	wl_registry_add_listener(registry, &registry_listener, &state);
	wl_display_roundtrip(state.display);
	assert(state.compositor && state.layer_shell && state.shm);
	if (!state.input_inhibit_manager) {
		free(state.args.font);
		swaylock_log(LOG_ERROR, "Compositor does not support the input "
				"inhibitor protocol, refusing to run insecurely");
		return 1;
	}

	zwlr_input_inhibit_manager_v1_get_inhibitor(state.input_inhibit_manager);
	if (wl_display_roundtrip(state.display) == -1) {
		free(state.args.font);
		swaylock_log(LOG_ERROR, "Exiting - failed to inhibit input:"
				" is another lockscreen already running?");
		return 2;
	}

	if (state.zxdg_output_manager) {
		struct swaylock_surface *surface;
		wl_list_for_each(surface, &state.surfaces, link) {
			surface->xdg_output = zxdg_output_manager_v1_get_xdg_output(
						state.zxdg_output_manager, surface->output);
			zxdg_output_v1_add_listener(
					surface->xdg_output, &_xdg_output_listener, surface);
		}
		wl_display_roundtrip(state.display);
	} else {
		swaylock_log(LOG_INFO, "Compositor does not support zxdg output "
				"manager, images assigned to named outputs will not work");
	}

	struct swaylock_surface *surface;
	wl_list_for_each(surface, &state.surfaces, link) {
		create_layer_surface(surface);
	}

	if (state.args.daemonize) {
		wl_display_roundtrip(state.display);
		daemonize();
	}

	state.eventloop = loop_create();
	loop_add_fd(state.eventloop, wl_display_get_fd(state.display), POLLIN,
			display_in, NULL);

	loop_add_fd(state.eventloop, get_comm_reply_fd(), POLLIN, comm_in, NULL);

	state.run_display = true;
	while (state.run_display) {
		errno = 0;
		if (wl_display_flush(state.display) == -1 && errno != EAGAIN) {
			break;
		}
		loop_poll(state.eventloop);
	}

	free(state.args.font);
	return 0;
}