gif_get_lzw (GifContext *context)
{
	guchar *dest, *temp;
	gint lower_bound, upper_bound; /* bounds for emitting the area_updated signal */
	gboolean bound_flag;
	gint first_pass; /* bounds for emitting the area_updated signal */
	gint v;

	if (context->frame == NULL) {
                context->frame = g_new (GdkPixbufFrame, 1);

                context->frame->composited = NULL;
                context->frame->revert = NULL;
                
                context->frame->pixbuf =
                        gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                        TRUE,
                                        8,
                                        context->frame_len,
                                        context->frame_height);
                
                context->frame->x_offset = context->x_offset;
                context->frame->y_offset = context->y_offset;
                context->frame->need_recomposite = TRUE;
                
                /* GIF delay is in hundredths, we want thousandths */
                context->frame->delay_time = context->gif89.delay_time * 10;
                context->frame->elapsed = context->animation->total_time;
                context->animation->total_time += context->frame->delay_time;
                
                switch (context->gif89.disposal) {
                case 0:
                case 1:
                        context->frame->action = GDK_PIXBUF_FRAME_RETAIN;
                        break;
                case 2:
                        context->frame->action = GDK_PIXBUF_FRAME_DISPOSE;
                        break;
                case 3:
                        context->frame->action = GDK_PIXBUF_FRAME_REVERT;
                        break;
                default:
                        context->frame->action = GDK_PIXBUF_FRAME_RETAIN;
                        break;
                }

                context->frame->bg_transparent = (context->gif89.transparent == context->background_index);
                
                {
                        /* Update animation size */
                        int w, h;
                        
                        context->animation->n_frames ++;
                        context->animation->frames = g_list_append (context->animation->frames, context->frame);

                        w = context->frame->x_offset +
                                gdk_pixbuf_get_width (context->frame->pixbuf);
                        h = context->frame->y_offset +
                                gdk_pixbuf_get_height (context->frame->pixbuf);
                        if (w > context->animation->width)
                                context->animation->width = w;
                        if (h > context->animation->height)
                                context->animation->height = h;
                }

                /* Only call prepare_func for the first frame */
		if (context->animation->frames->next == NULL) { 
                        if (context->prepare_func)
                                (* context->prepare_func) (context->frame->pixbuf,
                                                           GDK_PIXBUF_ANIMATION (context->animation),
                                                           context->user_data);
                } else {
                        /* Otherwise init frame with last frame */
                        GList *link;
                        GdkPixbufFrame *prev_frame;
                        
                        link = g_list_find (context->animation->frames, context->frame);

                        prev_frame = link->prev->data;

                        gdk_pixbuf_gif_anim_frame_composite (context->animation, prev_frame);

                        gdk_pixbuf_copy_area (prev_frame->composited,
                                              context->frame->x_offset,
                                              context->frame->y_offset,
                                              gdk_pixbuf_get_width (context->frame->pixbuf),
                                              gdk_pixbuf_get_height (context->frame->pixbuf),
                                              context->frame->pixbuf,
                                              0, 0);
                }
        }

	dest = gdk_pixbuf_get_pixels (context->frame->pixbuf);

	bound_flag = FALSE;
	lower_bound = upper_bound = context->draw_ypos;
	first_pass = context->draw_pass;

	while (TRUE) {
                guchar (*cmap)[MAXCOLORMAPSIZE];

                if (context->frame_cmap_active)
                        cmap = context->frame_color_map;
                else
                        cmap = context->global_color_map;
                
		v = lzw_read_byte (context);
		if (v < 0) {
			goto finished_data;
		}
		bound_flag = TRUE;

                g_assert (gdk_pixbuf_get_has_alpha (context->frame->pixbuf));
                
                temp = dest + context->draw_ypos * gdk_pixbuf_get_rowstride (context->frame->pixbuf) + context->draw_xpos * 4;
                *temp = cmap [0][(guchar) v];
                *(temp+1) = cmap [1][(guchar) v];
                *(temp+2) = cmap [2][(guchar) v];
                *(temp+3) = (guchar) ((v == context->gif89.transparent) ? 0 : 255);

		if (context->prepare_func && context->frame_interlace)
			gif_fill_in_lines (context, dest, v);

		context->draw_xpos++;
                
		if (context->draw_xpos == context->frame_len) {
			context->draw_xpos = 0;
			if (context->frame_interlace) {
				switch (context->draw_pass) {
				case 0:
				case 1:
					context->draw_ypos += 8;
					break;
				case 2:
					context->draw_ypos += 4;
					break;
				case 3:
					context->draw_ypos += 2;
					break;
				}

				if (context->draw_ypos >= context->frame_height) {
					context->draw_pass++;
					switch (context->draw_pass) {
					case 1:
						context->draw_ypos = 4;
						break;
					case 2:
						context->draw_ypos = 2;
						break;
					case 3:
						context->draw_ypos = 1;
						break;
					default:
						goto done;
					}
				}
			} else {
				context->draw_ypos++;
			}
			if (context->draw_pass != first_pass) {
				if (context->draw_ypos > lower_bound) {
					lower_bound = 0;
					upper_bound = context->frame_height;
				} else {
                                        
				}
			} else
				upper_bound = context->draw_ypos;
		}
		if (context->draw_ypos >= context->frame_height)
			break;
	}

 done:

        context->state = GIF_GET_NEXT_STEP;

        v = 0;

 finished_data:
        
        if (bound_flag)
                context->frame->need_recomposite = TRUE;
        
	if (bound_flag && context->update_func) {
		if (lower_bound <= upper_bound && first_pass == context->draw_pass) {
			(* context->update_func)
				(context->frame->pixbuf,
				 0, lower_bound,
				 gdk_pixbuf_get_width (context->frame->pixbuf),
				 upper_bound - lower_bound,
				 context->user_data);
		} else {
			if (lower_bound <= upper_bound) {
				(* context->update_func)
					(context->frame->pixbuf,
					 context->frame->x_offset,
                                         context->frame->y_offset,
					 gdk_pixbuf_get_width (context->frame->pixbuf),
					 gdk_pixbuf_get_height (context->frame->pixbuf),
					 context->user_data);
			} else {
				(* context->update_func)
					(context->frame->pixbuf,
					 context->frame->x_offset,
                                         context->frame->y_offset,
					 gdk_pixbuf_get_width (context->frame->pixbuf),
					 upper_bound,
					 context->user_data);
				(* context->update_func)
					(context->frame->pixbuf,
					 context->frame->x_offset,
                                         lower_bound + context->frame->y_offset,
					 gdk_pixbuf_get_width (context->frame->pixbuf),
					 gdk_pixbuf_get_height (context->frame->pixbuf),
					 context->user_data);
			}
		}
	}

	if (context->state == GIF_GET_NEXT_STEP) {
                /* Will be freed with context->animation, we are just
                 * marking that we're done with it (no current frame)
                 */
		context->frame = NULL;
                context->frame_cmap_active = FALSE;
	}
	
	return v;
}