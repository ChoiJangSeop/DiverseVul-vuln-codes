static void vgacon_scrolldelta(struct vc_data *c, int lines)
{
	int start, end, count, soff;

	if (!lines) {
		vgacon_restore_screen(c);
		return;
	}

	if (!vgacon_scrollback_cur->data)
		return;

	if (!vgacon_scrollback_cur->save) {
		vgacon_cursor(c, CM_ERASE);
		vgacon_save_screen(c);
		c->vc_origin = (unsigned long)c->vc_screenbuf;
		vgacon_scrollback_cur->save = 1;
	}

	vgacon_scrollback_cur->restore = 0;
	start = vgacon_scrollback_cur->cur + lines;
	end = start + abs(lines);

	if (start < 0)
		start = 0;

	if (start > vgacon_scrollback_cur->cnt)
		start = vgacon_scrollback_cur->cnt;

	if (end < 0)
		end = 0;

	if (end > vgacon_scrollback_cur->cnt)
		end = vgacon_scrollback_cur->cnt;

	vgacon_scrollback_cur->cur = start;
	count = end - start;
	soff = vgacon_scrollback_cur->tail -
		((vgacon_scrollback_cur->cnt - end) * c->vc_size_row);
	soff -= count * c->vc_size_row;

	if (soff < 0)
		soff += vgacon_scrollback_cur->size;

	count = vgacon_scrollback_cur->cnt - start;

	if (count > c->vc_rows)
		count = c->vc_rows;

	if (count) {
		int copysize;

		int diff = c->vc_rows - count;
		void *d = (void *) c->vc_visible_origin;
		void *s = (void *) c->vc_screenbuf;

		count *= c->vc_size_row;
		/* how much memory to end of buffer left? */
		copysize = min(count, vgacon_scrollback_cur->size - soff);
		scr_memcpyw(d, vgacon_scrollback_cur->data + soff, copysize);
		d += copysize;
		count -= copysize;

		if (count) {
			scr_memcpyw(d, vgacon_scrollback_cur->data, count);
			d += count;
		}

		if (diff)
			scr_memcpyw(d, s, diff * c->vc_size_row);
	} else
		vgacon_cursor(c, CM_MOVE);
}