static PrimitiveInfo *TraceStrokePolygon(const DrawInfo *draw_info,
  const PrimitiveInfo *primitive_info,ExceptionInfo *exception)
{
#define MaxStrokePad  (6*BezierQuantum+360)
#define CheckPathExtent(pad_p,pad_q) \
{   \
  if ((pad_p) > MaxBezierCoordinates) \
    stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p); \
  else \
    if ((ssize_t) (p+(pad_p)) >= (ssize_t) extent_p) \
      { \
        if (~extent_p < (pad_p)) \
          stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p); \
        else \
          { \
            extent_p+=(pad_p); \
            stroke_p=(PointInfo *) ResizeQuantumMemory(stroke_p,extent_p+ \
              MaxStrokePad,sizeof(*stroke_p)); \
          } \
      } \
  if ((pad_q) > MaxBezierCoordinates) \
    stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q); \
  else \
    if ((ssize_t) (q+(pad_q)) >= (ssize_t) extent_q) \
      { \
        if (~extent_q < (pad_q)) \
          stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q); \
        else \
          { \
            extent_q+=(pad_q); \
            stroke_q=(PointInfo *) ResizeQuantumMemory(stroke_q,extent_q+ \
              MaxStrokePad,sizeof(*stroke_q)); \
          } \
      } \
  if ((stroke_p == (PointInfo *) NULL) || (stroke_q == (PointInfo *) NULL)) \
    { \
      if (stroke_p != (PointInfo *) NULL) \
        stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p); \
      if (stroke_q != (PointInfo *) NULL) \
        stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q); \
      polygon_primitive=(PrimitiveInfo *) \
        RelinquishMagickMemory(polygon_primitive); \
      (void) ThrowMagickException(exception,GetMagickModule(), \
        ResourceLimitError,"MemoryAllocationFailed","`%s'",""); \
      return((PrimitiveInfo *) NULL); \
    } \
}

  typedef struct _StrokeSegment
  {
    double
      p,
      q;
  } StrokeSegment;

  double
    delta_theta,
    dot_product,
    mid,
    miterlimit;

  MagickBooleanType
    closed_path;

  PointInfo
    box_p[5],
    box_q[5],
    center,
    offset,
    *stroke_p,
    *stroke_q;

  PrimitiveInfo
    *polygon_primitive,
    *stroke_polygon;

  ssize_t
    i;

  size_t
    arc_segments,
    extent_p,
    extent_q,
    number_vertices;

  ssize_t
    j,
    n,
    p,
    q;

  StrokeSegment
    dx = {0.0, 0.0},
    dy = {0.0, 0.0},
    inverse_slope = {0.0, 0.0},
    slope = {0.0, 0.0},
    theta = {0.0, 0.0};

  /*
    Allocate paths.
  */
  number_vertices=primitive_info->coordinates;
  polygon_primitive=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    number_vertices+2UL,sizeof(*polygon_primitive));
  if (polygon_primitive == (PrimitiveInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PrimitiveInfo *) NULL);
    }
  (void) memcpy(polygon_primitive,primitive_info,(size_t) number_vertices*
    sizeof(*polygon_primitive));
  offset.x=primitive_info[number_vertices-1].point.x-primitive_info[0].point.x;
  offset.y=primitive_info[number_vertices-1].point.y-primitive_info[0].point.y;
  closed_path=(fabs(offset.x) < MagickEpsilon) &&
    (fabs(offset.y) < MagickEpsilon) ?  MagickTrue : MagickFalse;
  if (((draw_info->linejoin == RoundJoin) ||
       (draw_info->linejoin == MiterJoin)) && (closed_path != MagickFalse))
    {
      polygon_primitive[number_vertices]=primitive_info[1];
      number_vertices++;
    }
  polygon_primitive[number_vertices].primitive=UndefinedPrimitive;
  /*
    Compute the slope for the first line segment, p.
  */
  dx.p=0.0;
  dy.p=0.0;
  for (n=1; n < (ssize_t) number_vertices; n++)
  {
    dx.p=polygon_primitive[n].point.x-polygon_primitive[0].point.x;
    dy.p=polygon_primitive[n].point.y-polygon_primitive[0].point.y;
    if ((fabs(dx.p) >= MagickEpsilon) || (fabs(dy.p) >= MagickEpsilon))
      break;
  }
  if (n == (ssize_t) number_vertices)
    {
      if ((draw_info->linecap != RoundCap) || (closed_path != MagickFalse))
        {
          /*
            Zero length subpath.
          */
          stroke_polygon=(PrimitiveInfo *) AcquireCriticalMemory(
            sizeof(*stroke_polygon));
          stroke_polygon[0]=polygon_primitive[0];
          stroke_polygon[0].coordinates=0;
          polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(
            polygon_primitive);
          return(stroke_polygon);
        }
      n=(ssize_t) number_vertices-1L;
    }
  extent_p=2*number_vertices;
  extent_q=2*number_vertices;
  stroke_p=(PointInfo *) AcquireQuantumMemory((size_t) extent_p+MaxStrokePad,
    sizeof(*stroke_p));
  stroke_q=(PointInfo *) AcquireQuantumMemory((size_t) extent_q+MaxStrokePad,
    sizeof(*stroke_q));
  if ((stroke_p == (PointInfo *) NULL) || (stroke_q == (PointInfo *) NULL))
    {
      if (stroke_p != (PointInfo *) NULL)
        stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
      if (stroke_q != (PointInfo *) NULL)
        stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
      polygon_primitive=(PrimitiveInfo *)
        RelinquishMagickMemory(polygon_primitive);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      return((PrimitiveInfo *) NULL);
    }
  slope.p=0.0;
  inverse_slope.p=0.0;
  if (fabs(dx.p) < MagickEpsilon)
    {
      if (dx.p >= 0.0)
        slope.p=dy.p < 0.0 ? -1.0/MagickEpsilon : 1.0/MagickEpsilon;
      else
        slope.p=dy.p < 0.0 ? 1.0/MagickEpsilon : -1.0/MagickEpsilon;
    }
  else
    if (fabs(dy.p) < MagickEpsilon)
      {
        if (dy.p >= 0.0)
          inverse_slope.p=dx.p < 0.0 ? -1.0/MagickEpsilon : 1.0/MagickEpsilon;
        else
          inverse_slope.p=dx.p < 0.0 ? 1.0/MagickEpsilon : -1.0/MagickEpsilon;
      }
    else
      {
        slope.p=dy.p/dx.p;
        inverse_slope.p=(-1.0/slope.p);
      }
  mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
  miterlimit=(double) (draw_info->miterlimit*draw_info->miterlimit*mid*mid);
  if ((draw_info->linecap == SquareCap) && (closed_path == MagickFalse))
    (void) TraceSquareLinecap(polygon_primitive,number_vertices,mid);
  offset.x=sqrt((double) (mid*mid/(inverse_slope.p*inverse_slope.p+1.0)));
  offset.y=(double) (offset.x*inverse_slope.p);
  if ((dy.p*offset.x-dx.p*offset.y) > 0.0)
    {
      box_p[0].x=polygon_primitive[0].point.x-offset.x;
      box_p[0].y=polygon_primitive[0].point.y-offset.x*inverse_slope.p;
      box_p[1].x=polygon_primitive[n].point.x-offset.x;
      box_p[1].y=polygon_primitive[n].point.y-offset.x*inverse_slope.p;
      box_q[0].x=polygon_primitive[0].point.x+offset.x;
      box_q[0].y=polygon_primitive[0].point.y+offset.x*inverse_slope.p;
      box_q[1].x=polygon_primitive[n].point.x+offset.x;
      box_q[1].y=polygon_primitive[n].point.y+offset.x*inverse_slope.p;
    }
  else
    {
      box_p[0].x=polygon_primitive[0].point.x+offset.x;
      box_p[0].y=polygon_primitive[0].point.y+offset.y;
      box_p[1].x=polygon_primitive[n].point.x+offset.x;
      box_p[1].y=polygon_primitive[n].point.y+offset.y;
      box_q[0].x=polygon_primitive[0].point.x-offset.x;
      box_q[0].y=polygon_primitive[0].point.y-offset.y;
      box_q[1].x=polygon_primitive[n].point.x-offset.x;
      box_q[1].y=polygon_primitive[n].point.y-offset.y;
    }
  /*
    Create strokes for the line join attribute: bevel, miter, round.
  */
  p=0;
  q=0;
  stroke_q[p++]=box_q[0];
  stroke_p[q++]=box_p[0];
  for (i=(ssize_t) n+1; i < (ssize_t) number_vertices; i++)
  {
    /*
      Compute the slope for this line segment, q.
    */
    dx.q=polygon_primitive[i].point.x-polygon_primitive[n].point.x;
    dy.q=polygon_primitive[i].point.y-polygon_primitive[n].point.y;
    dot_product=dx.q*dx.q+dy.q*dy.q;
    if (dot_product < 0.25)
      continue;
    slope.q=0.0;
    inverse_slope.q=0.0;
    if (fabs(dx.q) < MagickEpsilon)
      {
        if (dx.q >= 0.0)
          slope.q=dy.q < 0.0 ? -1.0/MagickEpsilon : 1.0/MagickEpsilon;
        else
          slope.q=dy.q < 0.0 ? 1.0/MagickEpsilon : -1.0/MagickEpsilon;
      }
    else
      if (fabs(dy.q) < MagickEpsilon)
        {
          if (dy.q >= 0.0)
            inverse_slope.q=dx.q < 0.0 ? -1.0/MagickEpsilon : 1.0/MagickEpsilon;
          else
            inverse_slope.q=dx.q < 0.0 ? 1.0/MagickEpsilon : -1.0/MagickEpsilon;
        }
      else
        {
          slope.q=dy.q/dx.q;
          inverse_slope.q=(-1.0/slope.q);
        }
    offset.x=sqrt((double) (mid*mid/(inverse_slope.q*inverse_slope.q+1.0)));
    offset.y=(double) (offset.x*inverse_slope.q);
    dot_product=dy.q*offset.x-dx.q*offset.y;
    if (dot_product > 0.0)
      {
        box_p[2].x=polygon_primitive[n].point.x-offset.x;
        box_p[2].y=polygon_primitive[n].point.y-offset.y;
        box_p[3].x=polygon_primitive[i].point.x-offset.x;
        box_p[3].y=polygon_primitive[i].point.y-offset.y;
        box_q[2].x=polygon_primitive[n].point.x+offset.x;
        box_q[2].y=polygon_primitive[n].point.y+offset.y;
        box_q[3].x=polygon_primitive[i].point.x+offset.x;
        box_q[3].y=polygon_primitive[i].point.y+offset.y;
      }
    else
      {
        box_p[2].x=polygon_primitive[n].point.x+offset.x;
        box_p[2].y=polygon_primitive[n].point.y+offset.y;
        box_p[3].x=polygon_primitive[i].point.x+offset.x;
        box_p[3].y=polygon_primitive[i].point.y+offset.y;
        box_q[2].x=polygon_primitive[n].point.x-offset.x;
        box_q[2].y=polygon_primitive[n].point.y-offset.y;
        box_q[3].x=polygon_primitive[i].point.x-offset.x;
        box_q[3].y=polygon_primitive[i].point.y-offset.y;
      }
    if (fabs((double) (slope.p-slope.q)) < MagickEpsilon)
      {
        box_p[4]=box_p[1];
        box_q[4]=box_q[1];
      }
    else
      {
        box_p[4].x=(double) ((slope.p*box_p[0].x-box_p[0].y-slope.q*box_p[3].x+
          box_p[3].y)/(slope.p-slope.q));
        box_p[4].y=(double) (slope.p*(box_p[4].x-box_p[0].x)+box_p[0].y);
        box_q[4].x=(double) ((slope.p*box_q[0].x-box_q[0].y-slope.q*box_q[3].x+
          box_q[3].y)/(slope.p-slope.q));
        box_q[4].y=(double) (slope.p*(box_q[4].x-box_q[0].x)+box_q[0].y);
      }
    CheckPathExtent(MaxStrokePad,MaxStrokePad);
    dot_product=dx.q*dy.p-dx.p*dy.q;
    if (dot_product <= 0.0)
      switch (draw_info->linejoin)
      {
        case BevelJoin:
        {
          stroke_q[q++]=box_q[1];
          stroke_q[q++]=box_q[2];
          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            stroke_p[p++]=box_p[4];
          else
            {
              stroke_p[p++]=box_p[1];
              stroke_p[p++]=box_p[2];
            }
          break;
        }
        case MiterJoin:
        {
          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            {
              stroke_q[q++]=box_q[4];
              stroke_p[p++]=box_p[4];
            }
          else
            {
              stroke_q[q++]=box_q[1];
              stroke_q[q++]=box_q[2];
              stroke_p[p++]=box_p[1];
              stroke_p[p++]=box_p[2];
            }
          break;
        }
        case RoundJoin:
        {
          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            stroke_p[p++]=box_p[4];
          else
            {
              stroke_p[p++]=box_p[1];
              stroke_p[p++]=box_p[2];
            }
          center=polygon_primitive[n].point;
          theta.p=atan2(box_q[1].y-center.y,box_q[1].x-center.x);
          theta.q=atan2(box_q[2].y-center.y,box_q[2].x-center.x);
          if (theta.q < theta.p)
            theta.q+=2.0*MagickPI;
          arc_segments=(size_t) CastDoubleToLong(ceil((double) ((theta.q-
            theta.p)/(2.0*sqrt(PerceptibleReciprocal(mid))))));
          CheckPathExtent(MaxStrokePad,arc_segments+MaxStrokePad);
          stroke_q[q].x=box_q[1].x;
          stroke_q[q].y=box_q[1].y;
          q++;
          for (j=1; j < (ssize_t) arc_segments; j++)
          {
            delta_theta=(double) (j*(theta.q-theta.p)/arc_segments);
            stroke_q[q].x=(double) (center.x+mid*cos(fmod((double)
              (theta.p+delta_theta),DegreesToRadians(360.0))));
            stroke_q[q].y=(double) (center.y+mid*sin(fmod((double)
              (theta.p+delta_theta),DegreesToRadians(360.0))));
            q++;
          }
          stroke_q[q++]=box_q[2];
          break;
        }
        default:
          break;
      }
    else
      switch (draw_info->linejoin)
      {
        case BevelJoin:
        {
          stroke_p[p++]=box_p[1];
          stroke_p[p++]=box_p[2];
          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            stroke_q[q++]=box_q[4];
          else
            {
              stroke_q[q++]=box_q[1];
              stroke_q[q++]=box_q[2];
            }
          break;
        }
        case MiterJoin:
        {
          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            {
              stroke_q[q++]=box_q[4];
              stroke_p[p++]=box_p[4];
            }
          else
            {
              stroke_q[q++]=box_q[1];
              stroke_q[q++]=box_q[2];
              stroke_p[p++]=box_p[1];
              stroke_p[p++]=box_p[2];
            }
          break;
        }
        case RoundJoin:
        {
          dot_product=(box_q[4].x-box_p[4].x)*(box_q[4].x-box_p[4].x)+
            (box_q[4].y-box_p[4].y)*(box_q[4].y-box_p[4].y);
          if (dot_product <= miterlimit)
            stroke_q[q++]=box_q[4];
          else
            {
              stroke_q[q++]=box_q[1];
              stroke_q[q++]=box_q[2];
            }
          center=polygon_primitive[n].point;
          theta.p=atan2(box_p[1].y-center.y,box_p[1].x-center.x);
          theta.q=atan2(box_p[2].y-center.y,box_p[2].x-center.x);
          if (theta.p < theta.q)
            theta.p+=2.0*MagickPI;
          arc_segments=(size_t) CastDoubleToLong(ceil((double) ((theta.p-
            theta.q)/(2.0*sqrt((double) (1.0/mid))))));
          CheckPathExtent(arc_segments+MaxStrokePad,MaxStrokePad);
          stroke_p[p++]=box_p[1];
          for (j=1; j < (ssize_t) arc_segments; j++)
          {
            delta_theta=(double) (j*(theta.q-theta.p)/arc_segments);
            stroke_p[p].x=(double) (center.x+mid*cos(fmod((double)
              (theta.p+delta_theta),DegreesToRadians(360.0))));
            stroke_p[p].y=(double) (center.y+mid*sin(fmod((double)
              (theta.p+delta_theta),DegreesToRadians(360.0))));
            p++;
          }
          stroke_p[p++]=box_p[2];
          break;
        }
        default:
          break;
      }
    slope.p=slope.q;
    inverse_slope.p=inverse_slope.q;
    box_p[0]=box_p[2];
    box_p[1]=box_p[3];
    box_q[0]=box_q[2];
    box_q[1]=box_q[3];
    dx.p=dx.q;
    dy.p=dy.q;
    n=i;
  }
  stroke_p[p++]=box_p[1];
  stroke_q[q++]=box_q[1];
  /*
    Trace stroked polygon.
  */
  stroke_polygon=(PrimitiveInfo *) AcquireQuantumMemory((size_t)
    (p+q+2UL*closed_path+2UL),sizeof(*stroke_polygon));
  if (stroke_polygon == (PrimitiveInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","");
      stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
      stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
      polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(
        polygon_primitive);
      return(stroke_polygon);
    }
  for (i=0; i < (ssize_t) p; i++)
  {
    stroke_polygon[i]=polygon_primitive[0];
    stroke_polygon[i].point=stroke_p[i];
  }
  if (closed_path != MagickFalse)
    {
      stroke_polygon[i]=polygon_primitive[0];
      stroke_polygon[i].point=stroke_polygon[0].point;
      i++;
    }
  for ( ; i < (ssize_t) (p+q+closed_path); i++)
  {
    stroke_polygon[i]=polygon_primitive[0];
    stroke_polygon[i].point=stroke_q[p+q+closed_path-(i+1)];
  }
  if (closed_path != MagickFalse)
    {
      stroke_polygon[i]=polygon_primitive[0];
      stroke_polygon[i].point=stroke_polygon[p+closed_path].point;
      i++;
    }
  stroke_polygon[i]=polygon_primitive[0];
  stroke_polygon[i].point=stroke_polygon[0].point;
  i++;
  stroke_polygon[i].primitive=UndefinedPrimitive;
  stroke_polygon[0].coordinates=(size_t) (p+q+2*closed_path+1);
  stroke_p=(PointInfo *) RelinquishMagickMemory(stroke_p);
  stroke_q=(PointInfo *) RelinquishMagickMemory(stroke_q);
  polygon_primitive=(PrimitiveInfo *) RelinquishMagickMemory(polygon_primitive);
  return(stroke_polygon);
}