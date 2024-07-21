vte_sequence_handler_window_manipulation (VteTerminal *terminal, GValueArray *params)
{
	GdkScreen *gscreen;
	VteScreen *screen;
	GValue *value;
	GtkWidget *widget;
	char buf[128];
	long param, arg1, arg2;
	gint width, height;
	guint i;

	widget = &terminal->widget;
	screen = terminal->pvt->screen;

	for (i = 0; ((params != NULL) && (i < params->n_values)); i++) {
		arg1 = arg2 = -1;
		if (i + 1 < params->n_values) {
			value = g_value_array_get_nth(params, i + 1);
			if (G_VALUE_HOLDS_LONG(value)) {
				arg1 = g_value_get_long(value);
			}
		}
		if (i + 2 < params->n_values) {
			value = g_value_array_get_nth(params, i + 2);
			if (G_VALUE_HOLDS_LONG(value)) {
				arg2 = g_value_get_long(value);
			}
		}
		value = g_value_array_get_nth(params, i);
		if (!G_VALUE_HOLDS_LONG(value)) {
			continue;
		}
		param = g_value_get_long(value);
		switch (param) {
		case 1:
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Deiconifying window.\n");
			vte_terminal_emit_deiconify_window(terminal);
			break;
		case 2:
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Iconifying window.\n");
			vte_terminal_emit_iconify_window(terminal);
			break;
		case 3:
			if ((arg1 != -1) && (arg2 != -2)) {
				_vte_debug_print(VTE_DEBUG_PARSE,
						"Moving window to "
						"%ld,%ld.\n", arg1, arg2);
				vte_terminal_emit_move_window(terminal,
							      arg1, arg2);
				i += 2;
			}
			break;
		case 4:
			if ((arg1 != -1) && (arg2 != -1)) {
				_vte_debug_print(VTE_DEBUG_PARSE,
						"Resizing window "
						"(to %ldx%ld pixels).\n",
						arg2, arg1);
				vte_terminal_emit_resize_window(terminal,
								arg2 +
								VTE_PAD_WIDTH * 2,
								arg1 +
								VTE_PAD_WIDTH * 2);
				i += 2;
			}
			break;
		case 5:
			_vte_debug_print(VTE_DEBUG_PARSE, "Raising window.\n");
			vte_terminal_emit_raise_window(terminal);
			break;
		case 6:
			_vte_debug_print(VTE_DEBUG_PARSE, "Lowering window.\n");
			vte_terminal_emit_lower_window(terminal);
			break;
		case 7:
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Refreshing window.\n");
			_vte_invalidate_all(terminal);
			vte_terminal_emit_refresh_window(terminal);
			break;
		case 8:
			if ((arg1 != -1) && (arg2 != -1)) {
				_vte_debug_print(VTE_DEBUG_PARSE,
						"Resizing window "
						"(to %ld columns, %ld rows).\n",
						arg2, arg1);
				vte_terminal_emit_resize_window(terminal,
								arg2 * terminal->char_width +
								VTE_PAD_WIDTH * 2,
								arg1 * terminal->char_height +
								VTE_PAD_WIDTH * 2);
				i += 2;
			}
			break;
		case 9:
			switch (arg1) {
			case 0:
				_vte_debug_print(VTE_DEBUG_PARSE,
						"Restoring window.\n");
				vte_terminal_emit_restore_window(terminal);
				break;
			case 1:
				_vte_debug_print(VTE_DEBUG_PARSE,
						"Maximizing window.\n");
				vte_terminal_emit_maximize_window(terminal);
				break;
			default:
				break;
			}
			i++;
			break;
		case 11:
			/* If we're unmapped, then we're iconified. */
			g_snprintf(buf, sizeof(buf),
				   _VTE_CAP_CSI "%dt",
				   1 + !GTK_WIDGET_MAPPED(widget));
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Reporting window state %s.\n",
					GTK_WIDGET_MAPPED(widget) ?
					"non-iconified" : "iconified");
			vte_terminal_feed_child(terminal, buf, -1);
			break;
		case 13:
			/* Send window location, in pixels. */
			gdk_window_get_origin(widget->window,
					      &width, &height);
			g_snprintf(buf, sizeof(buf),
				   _VTE_CAP_CSI "%d;%dt",
				   width + VTE_PAD_WIDTH, height + VTE_PAD_WIDTH);
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Reporting window location"
					"(%d++,%d++).\n",
					width, height);
			vte_terminal_feed_child(terminal, buf, -1);
			break;
		case 14:
			/* Send window size, in pixels. */
			g_snprintf(buf, sizeof(buf),
				   _VTE_CAP_CSI "%d;%dt",
				   widget->allocation.height - 2 * VTE_PAD_WIDTH,
				   widget->allocation.width - 2 * VTE_PAD_WIDTH);
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Reporting window size "
					"(%dx%dn",
					width - 2 * VTE_PAD_WIDTH,
					height - 2 * VTE_PAD_WIDTH);
			vte_terminal_feed_child(terminal, buf, -1);
			break;
		case 18:
			/* Send widget size, in cells. */
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Reporting widget size.\n");
			g_snprintf(buf, sizeof(buf),
				   _VTE_CAP_CSI "%ld;%ldt",
				   terminal->row_count,
				   terminal->column_count);
			vte_terminal_feed_child(terminal, buf, -1);
			break;
		case 19:
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Reporting screen size.\n");
			gscreen = gtk_widget_get_screen(widget);
			height = gdk_screen_get_height(gscreen);
			width = gdk_screen_get_width(gscreen);
			g_snprintf(buf, sizeof(buf),
				   _VTE_CAP_CSI "%ld;%ldt",
				   height / terminal->char_height,
				   width / terminal->char_width);
			vte_terminal_feed_child(terminal, buf, -1);
			break;
		case 20:
			/* Report the icon title. */
			_vte_debug_print(VTE_DEBUG_PARSE,
				"Reporting icon title.\n");
			vte_terminal_feed_child(terminal, _VTE_CAP_OSC "LTerminal" _VTE_CAP_ST, -1);
			break;
		case 21:
			/* Report the window title. */
			_vte_debug_print(VTE_DEBUG_PARSE,
					"Reporting window title.\n");
			vte_terminal_feed_child(terminal, _VTE_CAP_OSC "LTerminal" _VTE_CAP_ST, -1);
			break;
		default:
			if (param >= 24) {
				_vte_debug_print(VTE_DEBUG_PARSE,
						"Resizing to %ld rows.\n",
					       	param);
				/* Resize to the specified number of
				 * rows. */
				vte_terminal_emit_resize_window(terminal,
								terminal->column_count * terminal->char_width +
								VTE_PAD_WIDTH * 2,
								param * terminal->char_height +
								VTE_PAD_WIDTH * 2);
			}
			break;
		}
	}
}