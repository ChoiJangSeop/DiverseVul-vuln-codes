GBool CairoOutputDev::tilingPatternFill(GfxState *state, Object *str,
					int paintType, Dict *resDict,
					double *mat, double *bbox,
					int x0, int y0, int x1, int y1,
					double xStep, double yStep)
{
  PDFRectangle box;
  Gfx *gfx;
  cairo_pattern_t *pattern;
  cairo_surface_t *surface;
  cairo_matrix_t matrix;
  cairo_t *old_cairo;
  double xMin, yMin, xMax, yMax;
  double width, height;
  int surface_width, surface_height;

  width = bbox[2] - bbox[0];
  height = bbox[3] - bbox[1];

  if (xStep != width || yStep != height)
    return gFalse;
  /* TODO: implement the other cases here too */

  surface_width = (int) ceil (width);
  surface_height = (int) ceil (height);

  surface = cairo_surface_create_similar (cairo_get_target (cairo),
					  CAIRO_CONTENT_COLOR_ALPHA,
					  surface_width, surface_height);
  if (cairo_surface_status (surface))
    return gFalse;

  old_cairo = cairo;
  cairo = cairo_create (surface);
  cairo_surface_destroy (surface);

  box.x1 = bbox[0]; box.y1 = bbox[1];
  box.x2 = bbox[2]; box.y2 = bbox[3];
  gfx = new Gfx(xref, this, resDict, catalog, &box, NULL);
  gfx->display(str);
  delete gfx;

  pattern = cairo_pattern_create_for_surface (cairo_get_target (cairo));
  cairo_destroy (cairo);
  cairo = old_cairo;
  if (cairo_pattern_status (pattern))
    return gFalse;

  state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
  cairo_rectangle (cairo, xMin, yMin, xMax - xMin, yMax - yMin);

  cairo_matrix_init_scale (&matrix, surface_width / width, surface_height / height);
  cairo_pattern_set_matrix (pattern, &matrix);

  cairo_matrix_init (&matrix, mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
  cairo_transform (cairo, &matrix);
  cairo_set_source (cairo, pattern);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_fill (cairo);

  cairo_pattern_destroy (pattern);

  return gTrue;
}