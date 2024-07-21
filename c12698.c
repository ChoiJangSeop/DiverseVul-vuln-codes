static void gf_dump_vrml_sffield(GF_SceneDumper *sdump, u32 type, void *ptr, Bool is_mf, GF_Node *node)
{
	switch (type) {
	case GF_SG_VRML_SFBOOL:
		gf_fprintf(sdump->trace, "%s", * ((SFBool *)ptr) ? "true" : "false");
		break;
	case GF_SG_VRML_SFINT32:
		gf_fprintf(sdump->trace, "%d", * ((SFInt32 *)ptr) );
		break;
	case GF_SG_VRML_SFFLOAT:
		gf_fprintf(sdump->trace, "%g", FIX2FLT( * ((SFFloat *)ptr) ) );
		break;
	case GF_SG_VRML_SFDOUBLE:
		gf_fprintf(sdump->trace, "%g", * ((SFDouble *)ptr) );
		break;
	case GF_SG_VRML_SFTIME:
		gf_fprintf(sdump->trace, "%g", * ((SFTime *)ptr) );
		break;
	case GF_SG_VRML_SFCOLOR:
		gf_fprintf(sdump->trace, "%g %g %g", FIX2FLT( ((SFColor *)ptr)->red ), FIX2FLT( ((SFColor *)ptr)->green ), FIX2FLT( ((SFColor *)ptr)->blue ));
		break;
	case GF_SG_VRML_SFCOLORRGBA:
		gf_fprintf(sdump->trace, "%g %g %g %g", FIX2FLT( ((SFColorRGBA *)ptr)->red ), FIX2FLT( ((SFColorRGBA *)ptr)->green ), FIX2FLT( ((SFColorRGBA *)ptr)->blue ), FIX2FLT( ((SFColorRGBA *)ptr)->alpha ));
		break;
	case GF_SG_VRML_SFVEC2F:
		gf_fprintf(sdump->trace, "%g %g", FIX2FLT( ((SFVec2f *)ptr)->x ), FIX2FLT( ((SFVec2f *)ptr)->y ));
		break;
	case GF_SG_VRML_SFVEC2D:
		gf_fprintf(sdump->trace, "%g %g", ((SFVec2d *)ptr)->x, ((SFVec2d *)ptr)->y);
		break;
	case GF_SG_VRML_SFVEC3F:
		gf_fprintf(sdump->trace, "%g %g %g", FIX2FLT( ((SFVec3f *)ptr)->x ), FIX2FLT( ((SFVec3f *)ptr)->y ), FIX2FLT( ((SFVec3f *)ptr)->z ));
		break;
	case GF_SG_VRML_SFVEC3D:
		gf_fprintf(sdump->trace, "%g %g %g", ((SFVec3d *)ptr)->x, ((SFVec3d *)ptr)->y, ((SFVec3d *)ptr)->z);
		break;
	case GF_SG_VRML_SFROTATION:
		gf_fprintf(sdump->trace, "%g %g %g %g", FIX2FLT( ((SFRotation *)ptr)->x ), FIX2FLT( ((SFRotation *)ptr)->y ), FIX2FLT( ((SFRotation *)ptr)->z ), FIX2FLT( ((SFRotation *)ptr)->q ) );
		break;

	case GF_SG_VRML_SFATTRREF:
	{
		SFAttrRef *ar = (SFAttrRef *)ptr;
		if (ar->node) {
			GF_FieldInfo pinfo;
			gf_node_get_field(ar->node, ar->fieldIndex, &pinfo);
			scene_dump_vrml_id(sdump, ar->node);
			gf_fprintf(sdump->trace, ".%s", pinfo.name);
		}
	}
	break;
	case GF_SG_VRML_SFSCRIPT:
	{
		u32 len, i;
		char *str;
		str = (char*)((SFScript *)ptr)->script_text;
		if (!str) {
			if (!sdump->XMLDump) {
				gf_fprintf(sdump->trace, "\"\"");
			}
			break;
		}
		len = (u32)strlen(str);

		if (!sdump->XMLDump) {
			gf_fprintf(sdump->trace, "\"%s\"", str);
		}
		else {
			u16 *uniLine;

			uniLine = (u16*)gf_malloc(sizeof(short) * (len + 1));
			len = gf_utf8_mbstowcs(uniLine, len, (const char **)&str);

			if (len != GF_UTF8_FAIL) {
				for (i = 0; i<len; i++) {

					switch (uniLine[i]) {
					case '&':
						gf_fprintf(sdump->trace, "&amp;");
						break;
					case '<':
						gf_fprintf(sdump->trace, "&lt;");
						break;
					case '>':
						gf_fprintf(sdump->trace, "&gt;");
						break;
					case '\'':
					case '"':
						gf_fprintf(sdump->trace, "&apos;");
						break;
					case 0:
						break;
						/*FIXME: how the heck can we preserve newlines and spaces of JavaScript in
						an XML attribute in any viewer ? */
					default:
						if (uniLine[i]<128) {
							gf_fprintf(sdump->trace, "%c", (u8)uniLine[i]);
						}
						else {
							gf_fprintf(sdump->trace, "&#%d;", uniLine[i]);
						}
						break;
					}
			}
		}
			gf_free(uniLine);
	}
		DUMP_IND(sdump);
	}
	break;

	case GF_SG_VRML_SFSTRING:
	{
		char *str;
		if (sdump->XMLDump) {
			if (is_mf) gf_fprintf(sdump->trace, sdump->X3DDump ? "\"" : "&quot;");
		} else {
			gf_fprintf(sdump->trace, "\"");
		}
		/*dump in unicode*/
		str = ((SFString *)ptr)->buffer;

		if (node && (gf_node_get_tag(node)==TAG_MPEG4_BitWrapper)) {
			u32 bufsize = 50+ ((M_BitWrapper*)node)->buffer_len * 2;
			str = gf_malloc(sizeof(char)* bufsize);
			if (str) {
				s32 res;
				strcpy(str, "data:application/octet-string;base64,");
				res = gf_base64_encode(((M_BitWrapper*)node)->buffer.buffer, ((M_BitWrapper*)node)->buffer_len, str+37, bufsize-37);
				if (res<0) {
					gf_free(str);
					str = NULL;
				} else {
					str[res+37] = 0;
				}
			}
		}
		if (str && str[0]) {
			if (sdump->XMLDump) {
				scene_dump_utf_string(sdump, 1, str);
			} else if (!strchr(str, '\"')) {
				gf_fprintf(sdump->trace, "%s", str);
			} else {
				u32 i, len = (u32)strlen(str);
				for (i=0; i<len; i++) {
					if (str[i]=='\"') gf_fputc('\\', sdump->trace);
					gf_fputc(str[i], sdump->trace);
				}
			}
		}
		if (node && (gf_node_get_tag(node)==TAG_MPEG4_BitWrapper)) {
			if (str) gf_free(str);
		}

		if (sdump->XMLDump) {
			if (is_mf) gf_fprintf(sdump->trace, sdump->X3DDump ? "\"" : "&quot;");
		} else {
			gf_fprintf(sdump->trace, "\"");
		}
	}
	break;

	case GF_SG_VRML_SFURL:
		if (((SFURL *)ptr)->url) {
#if 0
			u32 len;
			char *str;
			short uniLine[5000];
			str = ((SFURL *)ptr)->url;
			len = gf_utf8_mbstowcs(uniLine, 5000, (const char **) &str);
			if (len != GF_UTF8_FAIL) {
				gf_fprintf(sdump->trace, sdump->XMLDump ? (sdump->X3DDump ?  "'" : "&quot;") : "\"");
				fwprintf(sdump->trace, (unsigned short *) uniLine);
				gf_fprintf(sdump->trace, sdump->XMLDump ? (sdump->X3DDump ?  "'" : "&quot;") : "\"");
			}
#else
			gf_fprintf(sdump->trace, sdump->XMLDump ? (sdump->X3DDump ?  "'" : "&quot;") : "\"");
			gf_fprintf(sdump->trace, "%s", ((SFURL *)ptr)->url);
			gf_fprintf(sdump->trace, sdump->XMLDump ? (sdump->X3DDump ?  "'" : "&quot;") : "\"");
#endif
		} else {
			if (sdump->XMLDump) {
				gf_fprintf(sdump->trace, "&quot;od://od%d&quot;", ((SFURL *)ptr)->OD_ID);
			} else {
				gf_fprintf(sdump->trace, "od:%d", ((SFURL *)ptr)->OD_ID);
			}
		}
		break;
	case GF_SG_VRML_SFIMAGE:
	{
		u32 i, count;
		SFImage *img = (SFImage *)ptr;
		gf_fprintf(sdump->trace, "%d %d %d", img->width, img->height, img->numComponents);
		count = img->width * img->height * img->numComponents;
		for (i=0; i<count; ) {
			switch (img->numComponents) {
			case 1:
				gf_fprintf(sdump->trace, " 0x%02X", img->pixels[i]);
				i++;
				break;
			case 2:
				gf_fprintf(sdump->trace, " 0x%02X%02X", img->pixels[i], img->pixels[i+1]);
				i+=2;
				break;
			case 3:
				gf_fprintf(sdump->trace, " 0x%02X%02X%02X", img->pixels[i], img->pixels[i+1], img->pixels[i+2]);
				i+=3;
				break;
			case 4:
				gf_fprintf(sdump->trace, " 0x%02X%02X%02X%02X", img->pixels[i], img->pixels[i+1], img->pixels[i+2], img->pixels[i+3]);
				i+=4;
				break;
			}
		}
	}
	break;
	}
}