aperture_report(gerbv_aperture_t *apertures[], int aperture_num,
		double x, double y, gerbv_image_t *img, gerbv_project_t *prj)
{
	gerbv_aperture_type_t type = apertures[aperture_num]->type;
	double *params = apertures[aperture_num]->parameter;
	gerbv_simplified_amacro_t *sim = apertures[aperture_num]->simplified;
	double *simpars = sim->parameter;

	g_message (_("    Aperture used: D%d"), aperture_num);
	g_message (_("    Aperture type: %s"),
		(type == GERBV_APTYPE_MACRO)?
			_(gerbv_aperture_type_name(sim->type)):
			_(gerbv_aperture_type_name(type)));

	switch (type) {
	case GERBV_APTYPE_CIRCLE:
		g_message (_("    Diameter: %g %s"),
			screen_units(params[0]),
			screen_units_str());
		break;
	case GERBV_APTYPE_RECTANGLE:
	case GERBV_APTYPE_OVAL:
		g_message (_("    Dimensions: %gx%g %s"),
			screen_units(params[0]),
			screen_units(params[1]),
			screen_units_str());
		break;
	case GERBV_APTYPE_MACRO: {
		switch (sim->type) {
		case GERBV_APTYPE_MACRO_CIRCLE:
			g_message (_("    Diameter: %g %s"),
				screen_units(simpars[CIRCLE_DIAMETER]),
				screen_units_str());
			x += simpars[CIRCLE_CENTER_X];
			y += simpars[CIRCLE_CENTER_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Center: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			break;

		case GERBV_APTYPE_MACRO_OUTLINE:
			g_message (_("    Number of points: %g"),
				simpars[OUTLINE_NUMBER_OF_POINTS]);
			x += simpars[OUTLINE_FIRST_X];
			y += simpars[OUTLINE_FIRST_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Start: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
				simpars[OUTLINE_ROTATION_IDX(simpars)]);
			break;

		case GERBV_APTYPE_MACRO_POLYGON:
			g_message (_("    Number of points: %g"),
				simpars[POLYGON_NUMBER_OF_POINTS]);
			g_message (_("    Diameter: %g %s"),
				screen_units(simpars[POLYGON_DIAMETER]),
				screen_units_str());
			x += simpars[POLYGON_CENTER_X];
			y += simpars[POLYGON_CENTER_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Center: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
				simpars[POLYGON_ROTATION]);
			break;

		case GERBV_APTYPE_MACRO_MOIRE:
			g_message (_("    Outside diameter: %g %s"),
				screen_units(simpars[MOIRE_OUTSIDE_DIAMETER]),
				screen_units_str());
			g_message (_("    Ring thickness: %g %s"),
				screen_units(simpars[MOIRE_CIRCLE_THICKNESS]),
				screen_units_str());
			g_message (_("    Gap width: %g %s"),
				screen_units(simpars[MOIRE_GAP_WIDTH]),
				screen_units_str());
			g_message (_("    Number of rings: %g"),
				simpars[MOIRE_NUMBER_OF_CIRCLES]);
			g_message (_("    Crosshair thickness: %g %s"),
				screen_units(
					simpars[MOIRE_CROSSHAIR_THICKNESS]),
				screen_units_str());
			g_message (_("    Crosshair length: %g %s"),
				screen_units(simpars[MOIRE_CROSSHAIR_LENGTH]),
				screen_units_str());
			x += simpars[MOIRE_CENTER_X];
			y += simpars[MOIRE_CENTER_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Center: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
				simpars[MOIRE_ROTATION]);
			break;

		case GERBV_APTYPE_MACRO_THERMAL:
			g_message (_("    Outside diameter: %g %s"),
				screen_units(simpars[THERMAL_OUTSIDE_DIAMETER]),
				screen_units_str());
			g_message (_("    Inside diameter: %g %s"),
				screen_units(simpars[THERMAL_INSIDE_DIAMETER]),
				screen_units_str());
			g_message (_("    Crosshair thickness: %g %s"),
				screen_units(
					simpars[THERMAL_CROSSHAIR_THICKNESS]),
				screen_units_str());
			x += simpars[THERMAL_CENTER_X];
			y += simpars[THERMAL_CENTER_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Center: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
				simpars[THERMAL_ROTATION]);
			break;

		case GERBV_APTYPE_MACRO_LINE20:
			g_message (_("    Width: %g %s"),
				screen_units(simpars[LINE20_WIDTH]),
				screen_units_str());
			x += simpars[LINE20_START_X];
			y += simpars[LINE20_START_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Start: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			x += simpars[LINE20_END_X];
			y += simpars[LINE20_END_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Stop: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
					simpars[LINE20_ROTATION]);
			break;

		case GERBV_APTYPE_MACRO_LINE21:
			g_message (_("    Width: %g %s"),
				screen_units(simpars[LINE21_WIDTH]),
				screen_units_str());
			g_message (_("    Height: %g %s"),
				screen_units(simpars[LINE21_HEIGHT]),
				screen_units_str());
			x += simpars[LINE21_CENTER_X];
			y += simpars[LINE21_CENTER_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Center: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
					simpars[LINE21_ROTATION]);
			break;

		case GERBV_APTYPE_MACRO_LINE22:
			g_message (_("    Width: %g %s"),
				screen_units(simpars[LINE22_WIDTH]),
				screen_units_str());
			g_message (_("    Height: %g %s"),
				screen_units(simpars[LINE22_HEIGHT]),
				screen_units_str());
			x += simpars[LINE22_LOWER_LEFT_X];
			y += simpars[LINE22_LOWER_LEFT_Y];
			gerbv_transform_coord_for_image(&x, &y, img, prj);
			g_message (_("    Lower left: (%g, %g) %s"),
				screen_units(x), screen_units(y),
				screen_units_str());
			g_message (_("    Rotation: %g deg"),
					simpars[LINE22_ROTATION]);
			break;

		default:
			break;
		}
		break;
	}
	default:
		break;
	}
}