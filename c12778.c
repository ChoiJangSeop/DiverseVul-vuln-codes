char *parse_tags(ASS_Renderer *render_priv, char *p, char *end, double pwr)
{
    for (char *q; p < end; p = q) {
        while (*p != '\\' && p != end)
            ++p;
        if (*p != '\\')
            break;
        ++p;
        if (p != end)
            skip_spaces(&p);

        q = p;
        while (*q != '(' && *q != '\\' && q != end)
            ++q;
        if (q == p)
            continue;

        char *name_end = q;

        // Store one extra element to be able to detect excess arguments
        struct arg args[MAX_VALID_NARGS + 1];
        int nargs = 0;
        for (int i = 0; i <= MAX_VALID_NARGS; ++i)
            args[i].start = args[i].end = "";

        // Split parenthesized arguments. Do this for all tags and before
        // any non-parenthesized argument because that's what VSFilter does.
        if (*q == '(') {
            ++q;
            while (1) {
                if (q != end)
                    skip_spaces(&q);

                // Split on commas. If there is a backslash, ignore any
                // commas following it and lump everything starting from
                // the last comma, through the backslash and all the way
                // to the end of the argument string into a single argument.

                char *r = q;
                while (*r != ',' && *r != '\\' && *r != ')' && r != end)
                    ++r;

                if (*r == ',') {
                    push_arg(args, &nargs, q, r);
                    q = r + 1;
                } else {
                    // Swallow the rest of the parenthesized string. This could
                    // be either a backslash-argument or simply the last argument.
                    while (*r != ')' && r != end)
                        ++r;
                    push_arg(args, &nargs, q, r);
                    q = r;
                    // The closing parenthesis could be missing.
                    if (q != end)
                        ++q;
                    break;
                }
            }
        }

#define tag(name) (mystrcmp(&p, (name)) && (push_arg(args, &nargs, p, name_end), 1))
#define complex_tag(name) mystrcmp(&p, (name))

        // New tags introduced in vsfilter 2.39
        if (tag("xbord")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val = render_priv->state.border_x * (1 - pwr) + val * pwr;
                val = (val < 0) ? 0 : val;
            } else
                val = render_priv->state.style->Outline;
            render_priv->state.border_x = val;
        } else if (tag("ybord")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val = render_priv->state.border_y * (1 - pwr) + val * pwr;
                val = (val < 0) ? 0 : val;
            } else
                val = render_priv->state.style->Outline;
            render_priv->state.border_y = val;
        } else if (tag("xshad")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val = render_priv->state.shadow_x * (1 - pwr) + val * pwr;
            } else
                val = render_priv->state.style->Shadow;
            render_priv->state.shadow_x = val;
        } else if (tag("yshad")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val = render_priv->state.shadow_y * (1 - pwr) + val * pwr;
            } else
                val = render_priv->state.style->Shadow;
            render_priv->state.shadow_y = val;
        } else if (tag("fax")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                render_priv->state.fax =
                    val * pwr + render_priv->state.fax * (1 - pwr);
            } else
                render_priv->state.fax = 0.;
        } else if (tag("fay")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                render_priv->state.fay =
                    val * pwr + render_priv->state.fay * (1 - pwr);
            } else
                render_priv->state.fay = 0.;
        } else if (complex_tag("iclip")) {
            if (nargs == 4) {
                int x0, y0, x1, y1;
                x0 = argtoi(args[0]);
                y0 = argtoi(args[1]);
                x1 = argtoi(args[2]);
                y1 = argtoi(args[3]);
                render_priv->state.clip_x0 =
                    render_priv->state.clip_x0 * (1 - pwr) + x0 * pwr;
                render_priv->state.clip_x1 =
                    render_priv->state.clip_x1 * (1 - pwr) + x1 * pwr;
                render_priv->state.clip_y0 =
                    render_priv->state.clip_y0 * (1 - pwr) + y0 * pwr;
                render_priv->state.clip_y1 =
                    render_priv->state.clip_y1 * (1 - pwr) + y1 * pwr;
                render_priv->state.clip_mode = 1;
            } else if (!render_priv->state.clip_drawing) {
                if (parse_vector_clip(render_priv, args, nargs))
                    render_priv->state.clip_drawing_mode = 1;
            }
        } else if (tag("blur")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val = render_priv->state.blur * (1 - pwr) + val * pwr;
                val = (val < 0) ? 0 : val;
                val = (val > BLUR_MAX_RADIUS) ? BLUR_MAX_RADIUS : val;
                render_priv->state.blur = val;
            } else
                render_priv->state.blur = 0.0;
            // ASS standard tags
        } else if (tag("fscx")) {
            double val;
            if (nargs) {
                val = argtod(*args) / 100;
                val = render_priv->state.scale_x * (1 - pwr) + val * pwr;
                val = (val < 0) ? 0 : val;
            } else
                val = render_priv->state.style->ScaleX;
            render_priv->state.scale_x = val;
        } else if (tag("fscy")) {
            double val;
            if (nargs) {
                val = argtod(*args) / 100;
                val = render_priv->state.scale_y * (1 - pwr) + val * pwr;
                val = (val < 0) ? 0 : val;
            } else
                val = render_priv->state.style->ScaleY;
            render_priv->state.scale_y = val;
        } else if (tag("fsc")) {
            render_priv->state.scale_x = render_priv->state.style->ScaleX;
            render_priv->state.scale_y = render_priv->state.style->ScaleY;
        } else if (tag("fsp")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                render_priv->state.hspacing =
                    render_priv->state.hspacing * (1 - pwr) + val * pwr;
            } else
                render_priv->state.hspacing = render_priv->state.style->Spacing;
        } else if (tag("fs")) {
            double val = 0;
            if (nargs) {
                val = argtod(*args);
                if (*args->start == '+' || *args->start == '-')
                    val = render_priv->state.font_size * (1 + pwr * val / 10);
                else
                    val = render_priv->state.font_size * (1 - pwr) + val * pwr;
            }
            if (val <= 0)
                val = render_priv->state.style->FontSize;
            if (render_priv->state.font)
                change_font_size(render_priv, val);
        } else if (tag("bord")) {
            double val, xval, yval;
            if (nargs) {
                val = argtod(*args);
                xval = render_priv->state.border_x * (1 - pwr) + val * pwr;
                yval = render_priv->state.border_y * (1 - pwr) + val * pwr;
                xval = (xval < 0) ? 0 : xval;
                yval = (yval < 0) ? 0 : yval;
            } else
                xval = yval = render_priv->state.style->Outline;
            render_priv->state.border_x = xval;
            render_priv->state.border_y = yval;
        } else if (complex_tag("move")) {
            double x1, x2, y1, y2;
            long long t1, t2, delta_t, t;
            double x, y;
            double k;
            if (nargs == 4 || nargs == 6) {
                x1 = argtod(args[0]);
                y1 = argtod(args[1]);
                x2 = argtod(args[2]);
                y2 = argtod(args[3]);
                t1 = t2 = 0;
                if (nargs == 6) {
                    t1 = argtoll(args[4]);
                    t2 = argtoll(args[5]);
                    if (t1 > t2) {
                        long long tmp = t2;
                        t2 = t1;
                        t1 = tmp;
                    }
                }
            } else
                continue;
            if (t1 <= 0 && t2 <= 0) {
                t1 = 0;
                t2 = render_priv->state.event->Duration;
            }
            delta_t = t2 - t1;
            t = render_priv->time - render_priv->state.event->Start;
            if (t <= t1)
                k = 0.;
            else if (t >= t2)
                k = 1.;
            else
                k = ((double) (t - t1)) / delta_t;
            x = k * (x2 - x1) + x1;
            y = k * (y2 - y1) + y1;
            if (render_priv->state.evt_type != EVENT_POSITIONED) {
                render_priv->state.pos_x = x;
                render_priv->state.pos_y = y;
                render_priv->state.detect_collisions = 0;
                render_priv->state.evt_type = EVENT_POSITIONED;
            }
        } else if (tag("frx")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val *= M_PI / 180;
                render_priv->state.frx =
                    val * pwr + render_priv->state.frx * (1 - pwr);
            } else
                render_priv->state.frx = 0.;
        } else if (tag("fry")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val *= M_PI / 180;
                render_priv->state.fry =
                    val * pwr + render_priv->state.fry * (1 - pwr);
            } else
                render_priv->state.fry = 0.;
        } else if (tag("frz") || tag("fr")) {
            double val;
            if (nargs) {
                val = argtod(*args);
                val *= M_PI / 180;
                render_priv->state.frz =
                    val * pwr + render_priv->state.frz * (1 - pwr);
            } else
                render_priv->state.frz =
                    M_PI * render_priv->state.style->Angle / 180.;
        } else if (tag("fn")) {
            char *family;
            char *start = args->start;
            if (nargs && strncmp(start, "0", args->end - start)) {
                skip_spaces(&start);
                family = strndup(start, args->end - start);
            } else
                family = strdup(render_priv->state.style->FontName);
            free(render_priv->state.family);
            render_priv->state.family = family;
            update_font(render_priv);
        } else if (tag("alpha")) {
            int i;
            if (nargs) {
                int32_t a = parse_alpha_tag(args->start);
                for (i = 0; i < 4; ++i)
                    change_alpha(&render_priv->state.c[i], a, pwr);
            } else {
                change_alpha(&render_priv->state.c[0],
                             _a(render_priv->state.style->PrimaryColour), 1);
                change_alpha(&render_priv->state.c[1],
                             _a(render_priv->state.style->SecondaryColour), 1);
                change_alpha(&render_priv->state.c[2],
                             _a(render_priv->state.style->OutlineColour), 1);
                change_alpha(&render_priv->state.c[3],
                             _a(render_priv->state.style->BackColour), 1);
            }
            // FIXME: simplify
        } else if (tag("an")) {
            int val = argtoi(*args);
            if ((render_priv->state.parsed_tags & PARSED_A) == 0) {
                if (val >= 1 && val <= 9)
                    render_priv->state.alignment = numpad2align(val);
                else
                    render_priv->state.alignment =
                        render_priv->state.style->Alignment;
                render_priv->state.parsed_tags |= PARSED_A;
            }
        } else if (tag("a")) {
            int val = argtoi(*args);
            if ((render_priv->state.parsed_tags & PARSED_A) == 0) {
                if (val >= 1 && val <= 11)
                    // take care of a vsfilter quirk:
                    // handle illegal \a8 and \a4 like \a5
                    render_priv->state.alignment = ((val & 3) == 0) ? 5 : val;
                else
                    render_priv->state.alignment =
                        render_priv->state.style->Alignment;
                render_priv->state.parsed_tags |= PARSED_A;
            }
        } else if (complex_tag("pos")) {
            double v1, v2;
            if (nargs == 2) {
                v1 = argtod(args[0]);
                v2 = argtod(args[1]);
            } else
                continue;
            if (render_priv->state.evt_type == EVENT_POSITIONED) {
                ass_msg(render_priv->library, MSGL_V, "Subtitle has a new \\pos "
                       "after \\move or \\pos, ignoring");
            } else {
                render_priv->state.evt_type = EVENT_POSITIONED;
                render_priv->state.detect_collisions = 0;
                render_priv->state.pos_x = v1;
                render_priv->state.pos_y = v2;
            }
        } else if (complex_tag("fade") || complex_tag("fad")) {
            int a1, a2, a3;
            long long t1, t2, t3, t4;
            if (nargs == 2) {
                // 2-argument version (\fad, according to specs)
                a1 = 0xFF;
                a2 = 0;
                a3 = 0xFF;
                t1 = -1;
                t2 = argtoll(args[0]);
                t3 = argtoll(args[1]);
                t4 = -1;
            } else if (nargs == 7) {
                // 7-argument version (\fade)
                a1 = argtoi(args[0]);
                a2 = argtoi(args[1]);
                a3 = argtoi(args[2]);
                t1 = argtoll(args[3]);
                t2 = argtoll(args[4]);
                t3 = argtoll(args[5]);
                t4 = argtoll(args[6]);
            } else
                continue;
            if (t1 == -1 && t4 == -1) {
                t1 = 0;
                t4 = render_priv->state.event->Duration;
                t3 = t4 - t3;
            }
            if ((render_priv->state.parsed_tags & PARSED_FADE) == 0) {
                render_priv->state.fade =
                    interpolate_alpha(render_priv->time -
                            render_priv->state.event->Start, t1, t2,
                            t3, t4, a1, a2, a3);
                render_priv->state.parsed_tags |= PARSED_FADE;
            }
        } else if (complex_tag("org")) {
            double v1, v2;
            if (nargs == 2) {
                v1 = argtod(args[0]);
                v2 = argtod(args[1]);
            } else
                continue;
            if (!render_priv->state.have_origin) {
                render_priv->state.org_x = v1;
                render_priv->state.org_y = v2;
                render_priv->state.have_origin = 1;
                render_priv->state.detect_collisions = 0;
            }
        } else if (complex_tag("t")) {
            double accel;
            int cnt = nargs - 1;
            long long t1, t2, t, delta_t;
            double k;
            if (cnt == 3) {
                t1 = argtoll(args[0]);
                t2 = argtoll(args[1]);
                accel = argtod(args[2]);
            } else if (cnt == 2) {
                t1 = argtoll(args[0]);
                t2 = argtoll(args[1]);
                accel = 1.;
            } else if (cnt == 1) {
                t1 = 0;
                t2 = 0;
                accel = argtod(args[0]);
            } else if (cnt == 0) {
                t1 = 0;
                t2 = 0;
                accel = 1.;
            } else
                continue;
            render_priv->state.detect_collisions = 0;
            if (t2 == 0)
                t2 = render_priv->state.event->Duration;
            delta_t = t2 - t1;
            t = render_priv->time - render_priv->state.event->Start;        // FIXME: move to render_context
            if (t <= t1)
                k = 0.;
            else if (t >= t2)
                k = 1.;
            else {
                assert(delta_t != 0.);
                k = pow(((double) (t - t1)) / delta_t, accel);
            }
            p = args[cnt].start;
            p = parse_tags(render_priv, p, args[cnt].end, k);    // maybe k*pwr ? no, specs forbid nested \t's
        } else if (complex_tag("clip")) {
            if (nargs == 4) {
                int x0, y0, x1, y1;
                x0 = argtoi(args[0]);
                y0 = argtoi(args[1]);
                x1 = argtoi(args[2]);
                y1 = argtoi(args[3]);
                render_priv->state.clip_x0 =
                    render_priv->state.clip_x0 * (1 - pwr) + x0 * pwr;
                render_priv->state.clip_x1 =
                    render_priv->state.clip_x1 * (1 - pwr) + x1 * pwr;
                render_priv->state.clip_y0 =
                    render_priv->state.clip_y0 * (1 - pwr) + y0 * pwr;
                render_priv->state.clip_y1 =
                    render_priv->state.clip_y1 * (1 - pwr) + y1 * pwr;
                render_priv->state.clip_mode = 0;
            } else if (!render_priv->state.clip_drawing) {
                if (parse_vector_clip(render_priv, args, nargs))
                    render_priv->state.clip_drawing_mode = 0;
            }
        } else if (tag("c") || tag("1c")) {
            if (nargs) {
                uint32_t val = parse_color_tag(args->start);
                change_color(&render_priv->state.c[0], val, pwr);
            } else
                change_color(&render_priv->state.c[0],
                             render_priv->state.style->PrimaryColour, 1);
        } else if (tag("2c")) {
            if (nargs) {
                uint32_t val = parse_color_tag(args->start);
                change_color(&render_priv->state.c[1], val, pwr);
            } else
                change_color(&render_priv->state.c[1],
                             render_priv->state.style->SecondaryColour, 1);
        } else if (tag("3c")) {
            if (nargs) {
                uint32_t val = parse_color_tag(args->start);
                change_color(&render_priv->state.c[2], val, pwr);
            } else
                change_color(&render_priv->state.c[2],
                             render_priv->state.style->OutlineColour, 1);
        } else if (tag("4c")) {
            if (nargs) {
                uint32_t val = parse_color_tag(args->start);
                change_color(&render_priv->state.c[3], val, pwr);
            } else
                change_color(&render_priv->state.c[3],
                             render_priv->state.style->BackColour, 1);
        } else if (tag("1a")) {
            if (nargs) {
                uint32_t val = parse_alpha_tag(args->start);
                change_alpha(&render_priv->state.c[0], val, pwr);
            } else
                change_alpha(&render_priv->state.c[0],
                             _a(render_priv->state.style->PrimaryColour), 1);
        } else if (tag("2a")) {
            if (nargs) {
                uint32_t val = parse_alpha_tag(args->start);
                change_alpha(&render_priv->state.c[1], val, pwr);
            } else
                change_alpha(&render_priv->state.c[1],
                             _a(render_priv->state.style->SecondaryColour), 1);
        } else if (tag("3a")) {
            if (nargs) {
                uint32_t val = parse_alpha_tag(args->start);
                change_alpha(&render_priv->state.c[2], val, pwr);
            } else
                change_alpha(&render_priv->state.c[2],
                             _a(render_priv->state.style->OutlineColour), 1);
        } else if (tag("4a")) {
            if (nargs) {
                uint32_t val = parse_alpha_tag(args->start);
                change_alpha(&render_priv->state.c[3], val, pwr);
            } else
                change_alpha(&render_priv->state.c[3],
                             _a(render_priv->state.style->BackColour), 1);
        } else if (tag("r")) {
            if (nargs) {
                int len = args->end - args->start;
                reset_render_context(render_priv,
                        lookup_style_strict(render_priv->track, args->start, len));
            } else
                reset_render_context(render_priv, NULL);
        } else if (tag("be")) {
            double dval;
            if (nargs) {
                int val;
                dval = argtod(*args);
                // VSFilter always adds +0.5, even if the value is negative
                val = (int) (render_priv->state.be * (1 - pwr) + dval * pwr + 0.5);
                // Clamp to a safe upper limit, since high values need excessive CPU
                val = (val < 0) ? 0 : val;
                val = (val > MAX_BE) ? MAX_BE : val;
                render_priv->state.be = val;
            } else
                render_priv->state.be = 0;
        } else if (tag("b")) {
            int val = argtoi(*args);
            if (!nargs || !(val == 0 || val == 1 || val >= 100))
                val = render_priv->state.style->Bold;
            render_priv->state.bold = val;
            update_font(render_priv);
        } else if (tag("i")) {
            int val = argtoi(*args);
            if (!nargs || !(val == 0 || val == 1))
                val = render_priv->state.style->Italic;
            render_priv->state.italic = val;
            update_font(render_priv);
        } else if (tag("kf") || tag("K")) {
            double val = 100;
            if (nargs)
                val = argtod(*args);
            render_priv->state.effect_type = EF_KARAOKE_KF;
            if (render_priv->state.effect_timing)
                render_priv->state.effect_skip_timing +=
                    render_priv->state.effect_timing;
            render_priv->state.effect_timing = val * 10;
        } else if (tag("ko")) {
            double val = 100;
            if (nargs)
                val = argtod(*args);
            render_priv->state.effect_type = EF_KARAOKE_KO;
            if (render_priv->state.effect_timing)
                render_priv->state.effect_skip_timing +=
                    render_priv->state.effect_timing;
            render_priv->state.effect_timing = val * 10;
        } else if (tag("k")) {
            double val = 100;
            if (nargs)
                val = argtod(*args);
            render_priv->state.effect_type = EF_KARAOKE;
            if (render_priv->state.effect_timing)
                render_priv->state.effect_skip_timing +=
                    render_priv->state.effect_timing;
            render_priv->state.effect_timing = val * 10;
        } else if (tag("shad")) {
            double val, xval, yval;
            if (nargs) {
                val = argtod(*args);
                xval = render_priv->state.shadow_x * (1 - pwr) + val * pwr;
                yval = render_priv->state.shadow_y * (1 - pwr) + val * pwr;
                // VSFilter compatibility: clip for \shad but not for \[xy]shad
                xval = (xval < 0) ? 0 : xval;
                yval = (yval < 0) ? 0 : yval;
            } else
                xval = yval = render_priv->state.style->Shadow;
            render_priv->state.shadow_x = xval;
            render_priv->state.shadow_y = yval;
        } else if (tag("s")) {
            int val = argtoi(*args);
            if (!nargs || !(val == 0 || val == 1))
                val = render_priv->state.style->StrikeOut;
            if (val)
                render_priv->state.flags |= DECO_STRIKETHROUGH;
            else
                render_priv->state.flags &= ~DECO_STRIKETHROUGH;
        } else if (tag("u")) {
            int val = argtoi(*args);
            if (!nargs || !(val == 0 || val == 1))
                val = render_priv->state.style->Underline;
            if (val)
                render_priv->state.flags |= DECO_UNDERLINE;
            else
                render_priv->state.flags &= ~DECO_UNDERLINE;
        } else if (tag("pbo")) {
            double val = argtod(*args);
            render_priv->state.pbo = val;
        } else if (tag("p")) {
            int val = argtoi(*args);
            val = (val < 0) ? 0 : val;
            render_priv->state.drawing_scale = val;
        } else if (tag("q")) {
            int val = argtoi(*args);
            if (!nargs || !(val >= 0 && val <= 3))
                val = render_priv->track->WrapStyle;
            render_priv->state.wrap_style = val;
        } else if (tag("fe")) {
            int val;
            if (nargs)
                val = argtoi(*args);
            else
                val = render_priv->state.style->Encoding;
            render_priv->state.font_encoding = val;
        }
    }

    return p;
}