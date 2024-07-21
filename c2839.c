PHP_FUNCTION(imagepsbbox)
{
	zval *fnt;
	long sz = 0, sp = 0, wd = 0;
	char *str;
	int i, space = 0, add_width = 0, char_width, amount_kern;
	int cur_x, cur_y, dx, dy;
	int x1, y1, x2, y2, x3, y3, x4, y4;
	int *f_ind;
	int str_len, per_char = 0;
	int argc = ZEND_NUM_ARGS();
	double angle = 0, sin_a = 0, cos_a = 0;
	BBox char_bbox, str_bbox = {0, 0, 0, 0};

	if (argc != 3 && argc != 6) {
		ZEND_WRONG_PARAM_COUNT();
	}
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "srl|lld", &str, &str_len, &fnt, &sz, &sp, &wd, &angle) == FAILURE) {
		return;
	}
	
	if (argc == 6) {
		space = sp;
		add_width = wd;
		angle = angle * M_PI / 180;
		sin_a = sin(angle);
		cos_a = cos(angle);
		per_char =  add_width || angle ? 1 : 0;
	}

	ZEND_FETCH_RESOURCE(f_ind, int *, &fnt, -1, "Type 1 font", le_ps_font);

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)
#define new_x(a, b) (int) ((a) * cos_a - (b) * sin_a)
#define new_y(a, b) (int) ((a) * sin_a + (b) * cos_a)

	if (per_char) {
		space += T1_GetCharWidth(*f_ind, ' ');
		cur_x = cur_y = 0;

		for (i = 0; i < str_len; i++) {
			if (str[i] == ' ') {
				char_bbox.llx = char_bbox.lly = char_bbox.ury = 0;
				char_bbox.urx = char_width = space;
			} else {
				char_bbox = T1_GetCharBBox(*f_ind, str[i]);
				char_width = T1_GetCharWidth(*f_ind, str[i]);
			}
			amount_kern = i ? T1_GetKerning(*f_ind, str[i - 1], str[i]) : 0;

			/* Transfer character bounding box to right place */
			x1 = new_x(char_bbox.llx, char_bbox.lly) + cur_x;
			y1 = new_y(char_bbox.llx, char_bbox.lly) + cur_y;
			x2 = new_x(char_bbox.llx, char_bbox.ury) + cur_x;
			y2 = new_y(char_bbox.llx, char_bbox.ury) + cur_y;
			x3 = new_x(char_bbox.urx, char_bbox.ury) + cur_x;
			y3 = new_y(char_bbox.urx, char_bbox.ury) + cur_y;
			x4 = new_x(char_bbox.urx, char_bbox.lly) + cur_x;
			y4 = new_y(char_bbox.urx, char_bbox.lly) + cur_y;

			/* Find min & max values and compare them with current bounding box */
			str_bbox.llx = min(str_bbox.llx, min(x1, min(x2, min(x3, x4))));
			str_bbox.lly = min(str_bbox.lly, min(y1, min(y2, min(y3, y4))));
			str_bbox.urx = max(str_bbox.urx, max(x1, max(x2, max(x3, x4))));
			str_bbox.ury = max(str_bbox.ury, max(y1, max(y2, max(y3, y4))));

			/* Move to the next base point */
			dx = new_x(char_width + add_width + amount_kern, 0);
			dy = new_y(char_width + add_width + amount_kern, 0);
			cur_x += dx;
			cur_y += dy;
			/*
			printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", x1, y1, x2, y2, x3, y3, x4, y4, char_bbox.llx, char_bbox.lly, char_bbox.urx, char_bbox.ury, char_width, amount_kern, cur_x, cur_y, dx, dy);
			*/
		}

	} else {
		str_bbox = T1_GetStringBBox(*f_ind, str, str_len, space, T1_KERNING);
	}

	if (T1_errno) {
		RETURN_FALSE;
	}

	array_init(return_value);
	/*
	printf("%d %d %d %d\n", str_bbox.llx, str_bbox.lly, str_bbox.urx, str_bbox.ury);
	*/
	add_next_index_long(return_value, (int) ceil(((double) str_bbox.llx)*sz/1000));
	add_next_index_long(return_value, (int) ceil(((double) str_bbox.lly)*sz/1000));
	add_next_index_long(return_value, (int) ceil(((double) str_bbox.urx)*sz/1000));
	add_next_index_long(return_value, (int) ceil(((double) str_bbox.ury)*sz/1000));
}