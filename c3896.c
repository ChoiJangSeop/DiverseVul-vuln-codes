static MagickBooleanType DrawDashPolygon(const DrawInfo *draw_info,
  const PrimitiveInfo *primitive_info,Image *image,ExceptionInfo *exception)
{
  DrawInfo
    *clone_info;

  double
    length,
    maximum_length,
    offset,
    scale,
    total_length;

  MagickStatusType
    status;

  PrimitiveInfo
    *dash_polygon;

  register ssize_t
    i;

  register double
    dx,
    dy;

  size_t
    number_vertices;

  ssize_t
    j,
    n;

  assert(draw_info != (const DrawInfo *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    begin draw-dash");
  for (i=0; primitive_info[i].primitive != UndefinedPrimitive; i++) ;
  number_vertices=(size_t) i;
  dash_polygon=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    (2UL*number_vertices+1UL),sizeof(*dash_polygon));
  if (dash_polygon == (PrimitiveInfo *) NULL)
    return(MagickFalse);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  clone_info->miterlimit=0;
  dash_polygon[0]=primitive_info[0];
  scale=ExpandAffine(&draw_info->affine);
  length=scale*(draw_info->dash_pattern[0]-0.5);
  offset=draw_info->dash_offset != 0.0 ? scale*draw_info->dash_offset : 0.0;
  j=1;
  for (n=0; offset > 0.0; j=0)
  {
    if (draw_info->dash_pattern[n] <= 0.0)
      break;
    length=scale*(draw_info->dash_pattern[n]+(n == 0 ? -0.5 : 0.5));
    if (offset > length)
      {
        offset-=length;
        n++;
        length=scale*(draw_info->dash_pattern[n]+0.5);
        continue;
      }
    if (offset < length)
      {
        length-=offset;
        offset=0.0;
        break;
      }
    offset=0.0;
    n++;
  }
  status=MagickTrue;
  maximum_length=0.0;
  total_length=0.0;
  for (i=1; (i < number_vertices) && (length >= 0.0); i++)
  {
    dx=primitive_info[i].point.x-primitive_info[i-1].point.x;
    dy=primitive_info[i].point.y-primitive_info[i-1].point.y;
    maximum_length=hypot((double) dx,dy);
    if (length == 0.0)
      {
        n++;
        if (draw_info->dash_pattern[n] == 0.0)
          n=0;
        length=scale*(draw_info->dash_pattern[n]+(n == 0 ? -0.5 : 0.5));
      }
    for (total_length=0.0; (length >= 0.0) && (maximum_length >= (total_length+length)); )
    {
      total_length+=length;
      if ((n & 0x01) != 0)
        {
          dash_polygon[0]=primitive_info[0];
          dash_polygon[0].point.x=(double) (primitive_info[i-1].point.x+dx*
            total_length/maximum_length);
          dash_polygon[0].point.y=(double) (primitive_info[i-1].point.y+dy*
            total_length/maximum_length);
          j=1;
        }
      else
        {
          if ((j+1) > (ssize_t) (2*number_vertices))
            break;
          dash_polygon[j]=primitive_info[i-1];
          dash_polygon[j].point.x=(double) (primitive_info[i-1].point.x+dx*
            total_length/maximum_length);
          dash_polygon[j].point.y=(double) (primitive_info[i-1].point.y+dy*
            total_length/maximum_length);
          dash_polygon[j].coordinates=1;
          j++;
          dash_polygon[0].coordinates=(size_t) j;
          dash_polygon[j].primitive=UndefinedPrimitive;
          status&=DrawStrokePolygon(image,clone_info,dash_polygon,exception);
        }
      n++;
      if (draw_info->dash_pattern[n] == 0.0)
        n=0;
      length=scale*(draw_info->dash_pattern[n]+(n == 0 ? -0.5 : 0.5));
    }
    length-=(maximum_length-total_length);
    if ((n & 0x01) != 0)
      continue;
    dash_polygon[j]=primitive_info[i];
    dash_polygon[j].coordinates=1;
    j++;
  }
  if ((total_length <= maximum_length) && ((n & 0x01) == 0) && (j > 1))
    {
      dash_polygon[j]=primitive_info[i-1];
      dash_polygon[j].point.x+=MagickEpsilon;
      dash_polygon[j].point.y+=MagickEpsilon;
      dash_polygon[j].coordinates=1;
      j++;
      dash_polygon[0].coordinates=(size_t) j;
      dash_polygon[j].primitive=UndefinedPrimitive;
      status&=DrawStrokePolygon(image,clone_info,dash_polygon,exception);
    }
  dash_polygon=(PrimitiveInfo *) RelinquishMagickMemory(dash_polygon);
  clone_info=DestroyDrawInfo(clone_info);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"    end draw-dash");
  return(status != 0 ? MagickTrue : MagickFalse);
}