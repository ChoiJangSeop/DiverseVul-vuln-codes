MagickExport MagickBooleanType AnnotateImage(Image *image,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  char
    *p,
    primitive[MagickPathExtent],
    *text,
    **textlist;

  DrawInfo
    *annotate,
    *annotate_info;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  PointInfo
    offset;

  RectangleInfo
    geometry;

  register ssize_t
    i;

  TypeMetric
    metrics;

  size_t
    height,
    number_lines;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickCoreSignature);
  if (draw_info->text == (char *) NULL)
    return(MagickFalse);
  if (*draw_info->text == '\0')
    return(MagickTrue);
  annotate=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  text=annotate->text;
  annotate->text=(char *) NULL;
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  number_lines=1;
  for (p=text; *p != '\0'; p++)
    if (*p == '\n')
      number_lines++;
  textlist=AcquireQuantumMemory(number_lines+1,sizeof(*textlist));
  if (textlist == (char **) NULL)
    {
      annotate_info=DestroyDrawInfo(annotate_info);
      annotate=DestroyDrawInfo(annotate);
      return(MagickFalse);
    }
  p=text;
  for (i=0; i < number_lines; i++)
  {
    char
      *q;

    textlist[i]=p;
    for (q=p; *q != '\0'; q++)
      if ((*q == '\r') || (*q == '\n'))
        break;
    if (*q == '\r')
      {
        *q='\0';
        q++;
      }
    *q='\0';
    p=q+1;
  }
  textlist[i]=(char *) NULL;
  SetGeometry(image,&geometry);
  SetGeometryInfo(&geometry_info);
  if (annotate_info->geometry != (char *) NULL)
    {
      (void) ParsePageGeometry(image,annotate_info->geometry,&geometry,
        exception);
      (void) ParseGeometry(annotate_info->geometry,&geometry_info);
    }
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    {
      annotate_info=DestroyDrawInfo(annotate_info);
      annotate=DestroyDrawInfo(annotate);
      textlist=(char **) RelinquishMagickMemory(textlist);
      return(MagickFalse);
    }
  if (IsGrayColorspace(image->colorspace) != MagickFalse)
    (void) SetImageColorspace(image,sRGBColorspace,exception);
  status=MagickTrue;
  (void) memset(&metrics,0,sizeof(metrics));
  for (i=0; textlist[i] != (char *) NULL; i++)
  {
    if (*textlist[i] == '\0')
      continue;
    /*
      Position text relative to image.
    */
    annotate_info->affine.tx=geometry_info.xi-image->page.x;
    annotate_info->affine.ty=geometry_info.psi-image->page.y;
    (void) CloneString(&annotate->text,textlist[i]);
    if ((metrics.width == 0) || (annotate->gravity != NorthWestGravity))
      (void) GetTypeMetrics(image,annotate,&metrics,exception);
    height=(ssize_t) (metrics.ascent-metrics.descent+
      draw_info->interline_spacing+0.5);
    switch (annotate->gravity)
    {
      case UndefinedGravity:
      default:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height;
        break;
      }
      case NorthWestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+i*
          annotate_info->affine.sy*height+annotate_info->affine.sy*
          metrics.ascent;
        break;
      }
      case NorthGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+
          geometry.width/2.0+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+i*
          annotate_info->affine.sy*height+annotate_info->affine.sy*
          metrics.ascent-annotate_info->affine.rx*metrics.width/2.0;
        break;
      }
      case NorthEastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*annotate_info->affine.tx+
          geometry.width+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent)-1.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+i*
          annotate_info->affine.sy*height+annotate_info->affine.sy*
          metrics.ascent-annotate_info->affine.rx*metrics.width;
        break;
      }
      case WestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+
          geometry.height/2.0+i*annotate_info->affine.sy*height+
          annotate_info->affine.sy*(metrics.ascent+metrics.descent-
          (number_lines-1.0)*height)/2.0;
        break;
      }
      case CenterGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+
          geometry.width/2.0+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+
          geometry.height/2.0+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width/2.0+annotate_info->affine.sy*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        break;
      }
      case EastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*annotate_info->affine.tx+
          geometry.width+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width+
          annotate_info->affine.ry*(metrics.ascent+metrics.descent-
          (number_lines-1.0)*height)/2.0-1.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+
          geometry.height/2.0+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width+
          annotate_info->affine.sy*(metrics.ascent+metrics.descent-
          (number_lines-1.0)*height)/2.0;
        break;
      }
      case SouthWestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height-annotate_info->affine.ry*
          (number_lines-1.0)*height;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*annotate_info->affine.ty+
          geometry.height+i*annotate_info->affine.sy*height-
          annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
        break;
      }
      case SouthGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+
          geometry.width/2.0+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0-
          annotate_info->affine.ry*(number_lines-1.0)*height/2.0;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*annotate_info->affine.ty+
          geometry.height+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width/2.0-
          annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
        break;
      }
      case SouthEastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*annotate_info->affine.tx+
          geometry.width+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width-
          annotate_info->affine.ry*(number_lines-1.0)*height-1.0;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*annotate_info->affine.ty+
          geometry.height+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width-
          annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
        break;
      }
    }
    switch (annotate->align)
    {
      case LeftAlign:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height;
        break;
      }
      case CenterAlign:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width/2.0;
        break;
      }
      case RightAlign:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width;
        break;
      }
      default:
        break;
    }
    if (draw_info->undercolor.alpha != TransparentAlpha)
      {
        DrawInfo
          *undercolor_info;

        /*
          Text box.
        */
        undercolor_info=CloneDrawInfo((ImageInfo *) NULL,(DrawInfo *) NULL);
        undercolor_info->fill=draw_info->undercolor;
        undercolor_info->affine=draw_info->affine;
        undercolor_info->affine.tx=offset.x-draw_info->affine.ry*metrics.ascent;
        undercolor_info->affine.ty=offset.y-draw_info->affine.sy*metrics.ascent;
        (void) FormatLocaleString(primitive,MagickPathExtent,
          "rectangle 0.0,0.0 %g,%g",metrics.origin.x,(double) height);
        (void) CloneString(&undercolor_info->primitive,primitive);
        (void) DrawImage(image,undercolor_info,exception);
        (void) DestroyDrawInfo(undercolor_info);
      }
    annotate_info->affine.tx=offset.x;
    annotate_info->affine.ty=offset.y;
    (void) FormatLocaleString(primitive,MagickPathExtent,"stroke-width %g "
      "line 0,0 %g,0",metrics.underline_thickness,metrics.width);
    if (annotate->decorate == OverlineDecoration)
      {
        annotate_info->affine.ty-=(draw_info->affine.sy*(metrics.ascent+
          metrics.descent-metrics.underline_position));
        (void) CloneString(&annotate_info->primitive,primitive);
        (void) DrawImage(image,annotate_info,exception);
      }
    else
      if (annotate->decorate == UnderlineDecoration)
        {
          annotate_info->affine.ty-=(draw_info->affine.sy*
            metrics.underline_position);
          (void) CloneString(&annotate_info->primitive,primitive);
          (void) DrawImage(image,annotate_info,exception);
        }
    /*
      Annotate image with text.
    */
    status=RenderType(image,annotate,&offset,&metrics,exception);
    if (status == MagickFalse)
      break;
    if (annotate->decorate == LineThroughDecoration)
      {
        annotate_info->affine.ty-=(draw_info->affine.sy*(height+
          metrics.underline_position+metrics.descent)/2.0);
        (void) CloneString(&annotate_info->primitive,primitive);
        (void) DrawImage(image,annotate_info,exception);
      }
  }
  /*
    Relinquish resources.
  */
  annotate_info=DestroyDrawInfo(annotate_info);
  annotate=DestroyDrawInfo(annotate);
  textlist=(char **) RelinquishMagickMemory(textlist);
  return(status);
}