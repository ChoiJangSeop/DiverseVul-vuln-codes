MagickExport Image *DistortImage(const Image *image, DistortMethod method,
  const size_t number_arguments,const double *arguments,
  MagickBooleanType bestfit,ExceptionInfo *exception)
{
#define DistortImageTag  "Distort/Image"

  double
    *coeff,
    output_scaling;

  Image
    *distort_image;

  RectangleInfo
    geometry;  /* geometry of the distorted space viewport */

  MagickBooleanType
    viewport_given;

  PixelInfo
    invalid;  /* the color to assign when distort result is invalid */

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  /*
    Handle Special Compound Distortions
  */
  if ( method == ResizeDistortion )
    {
      if ( number_arguments != 2 )
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                    "InvalidArgument","%s : '%s'","Resize",
                    "Invalid number of args: 2 only");
          return((Image *) NULL);
        }
      distort_image=DistortResizeImage(image,(size_t)arguments[0],
         (size_t)arguments[1], exception);
      return(distort_image);
    }

  /*
    Convert input arguments (usually as control points for reverse mapping)
    into mapping coefficients to apply the distortion.

    Note that some distortions are mapped to other distortions,
    and as such do not require specific code after this point.
  */
  coeff = GenerateCoefficients(image, &method, number_arguments,
      arguments, 0, exception);
  if ( coeff == (double *) NULL )
    return((Image *) NULL);

  /*
    Determine the size and offset for a 'bestfit' destination.
    Usally the four corners of the source image is enough.
  */

  /* default output image bounds, when no 'bestfit' is requested */
  geometry.width=image->columns;
  geometry.height=image->rows;
  geometry.x=0;
  geometry.y=0;

  if ( method == ArcDistortion ) {
    bestfit = MagickTrue;  /* always calculate a 'best fit' viewport */
  }

  /* Work out the 'best fit', (required for ArcDistortion) */
  if ( bestfit ) {
    PointInfo
      s,d,min,max;  /* source, dest coords --mapping--> min, max coords */

    MagickBooleanType
      fix_bounds = MagickTrue;   /* enlarge bounds for VP handling */

    s.x=s.y=min.x=max.x=min.y=max.y=0.0;   /* keep compiler happy */

/* defines to figure out the bounds of the distorted image */
#define InitalBounds(p) \
{ \
  /* printf("%lg,%lg -> %lg,%lg\n", s.x,s.y, d.x,d.y); */ \
  min.x = max.x = p.x; \
  min.y = max.y = p.y; \
}
#define ExpandBounds(p) \
{ \
  /* printf("%lg,%lg -> %lg,%lg\n", s.x,s.y, d.x,d.y); */ \
  min.x = MagickMin(min.x,p.x); \
  max.x = MagickMax(max.x,p.x); \
  min.y = MagickMin(min.y,p.y); \
  max.y = MagickMax(max.y,p.y); \
}

    switch (method)
    {
      case AffineDistortion:
      case RigidAffineDistortion:
      { double inverse[6];
        InvertAffineCoefficients(coeff, inverse);
        s.x = (double) image->page.x;
        s.y = (double) image->page.y;
        d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
        d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
        InitalBounds(d);
        s.x = (double) image->page.x+image->columns;
        s.y = (double) image->page.y;
        d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
        d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
        ExpandBounds(d);
        s.x = (double) image->page.x;
        s.y = (double) image->page.y+image->rows;
        d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
        d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
        ExpandBounds(d);
        s.x = (double) image->page.x+image->columns;
        s.y = (double) image->page.y+image->rows;
        d.x = inverse[0]*s.x+inverse[1]*s.y+inverse[2];
        d.y = inverse[3]*s.x+inverse[4]*s.y+inverse[5];
        ExpandBounds(d);
        break;
      }
      case PerspectiveDistortion:
      { double inverse[8], scale;
        InvertPerspectiveCoefficients(coeff, inverse);
        s.x = (double) image->page.x;
        s.y = (double) image->page.y;
        scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
        scale=PerceptibleReciprocal(scale);
        d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
        d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
        InitalBounds(d);
        s.x = (double) image->page.x+image->columns;
        s.y = (double) image->page.y;
        scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
        scale=PerceptibleReciprocal(scale);
        d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
        d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
        ExpandBounds(d);
        s.x = (double) image->page.x;
        s.y = (double) image->page.y+image->rows;
        scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
        scale=PerceptibleReciprocal(scale);
        d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
        d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
        ExpandBounds(d);
        s.x = (double) image->page.x+image->columns;
        s.y = (double) image->page.y+image->rows;
        scale=inverse[6]*s.x+inverse[7]*s.y+1.0;
        scale=PerceptibleReciprocal(scale);
        d.x = scale*(inverse[0]*s.x+inverse[1]*s.y+inverse[2]);
        d.y = scale*(inverse[3]*s.x+inverse[4]*s.y+inverse[5]);
        ExpandBounds(d);
        break;
      }
      case ArcDistortion:
      { double a, ca, sa;
        /* Forward Map Corners */
        a = coeff[0]-coeff[1]/2; ca = cos(a); sa = sin(a);
        d.x = coeff[2]*ca;
        d.y = coeff[2]*sa;
        InitalBounds(d);
        d.x = (coeff[2]-coeff[3])*ca;
        d.y = (coeff[2]-coeff[3])*sa;
        ExpandBounds(d);
        a = coeff[0]+coeff[1]/2; ca = cos(a); sa = sin(a);
        d.x = coeff[2]*ca;
        d.y = coeff[2]*sa;
        ExpandBounds(d);
        d.x = (coeff[2]-coeff[3])*ca;
        d.y = (coeff[2]-coeff[3])*sa;
        ExpandBounds(d);
        /* Orthogonal points along top of arc */
        for( a=(double) (ceil((double) ((coeff[0]-coeff[1]/2.0)/MagickPI2))*MagickPI2);
               a<(coeff[0]+coeff[1]/2.0); a+=MagickPI2 ) {
          ca = cos(a); sa = sin(a);
          d.x = coeff[2]*ca;
          d.y = coeff[2]*sa;
          ExpandBounds(d);
        }
        /*
          Convert the angle_to_width and radius_to_height
          to appropriate scaling factors, to allow faster processing
          in the mapping function.
        */
        coeff[1] = (double) (Magick2PI*image->columns/coeff[1]);
        coeff[3] = (double)image->rows/coeff[3];
        break;
      }
      case PolarDistortion:
      {
        if (number_arguments < 2)
          coeff[2] = coeff[3] = 0.0;
        min.x = coeff[2]-coeff[0];
        max.x = coeff[2]+coeff[0];
        min.y = coeff[3]-coeff[0];
        max.y = coeff[3]+coeff[0];
        /* should be about 1.0 if Rmin = 0 */
        coeff[7]=(double) geometry.height/(coeff[0]-coeff[1]);
        break;
      }
      case DePolarDistortion:
      {
        /* direct calculation as it needs to tile correctly
         * for reversibility in a DePolar-Polar cycle */
        fix_bounds = MagickFalse;
        geometry.x = geometry.y = 0;
        geometry.height = (size_t) ceil(coeff[0]-coeff[1]);
        geometry.width = (size_t)
                  ceil((coeff[0]-coeff[1])*(coeff[5]-coeff[4])*0.5);
        /* correct scaling factors relative to new size */
        coeff[6]=(coeff[5]-coeff[4])/geometry.width; /* changed width */
        coeff[7]=(coeff[0]-coeff[1])/geometry.height; /* should be about 1.0 */
        break;
      }
      case Cylinder2PlaneDistortion:
      {
        /* direct calculation so center of distortion is either a pixel
         * center, or pixel edge. This allows for reversibility of the
         * distortion */
        geometry.x = geometry.y = 0;
        geometry.width = (size_t) ceil( 2.0*coeff[1]*tan(coeff[0]/2.0) );
        geometry.height = (size_t) ceil( 2.0*coeff[3]/cos(coeff[0]/2.0) );
        /* correct center of distortion relative to new size */
        coeff[4] = (double) geometry.width/2.0;
        coeff[5] = (double) geometry.height/2.0;
        fix_bounds = MagickFalse;
        break;
      }
      case Plane2CylinderDistortion:
      {
        /* direct calculation center is either pixel center, or pixel edge
         * so as to allow reversibility of the image distortion */
        geometry.x = geometry.y = 0;
        geometry.width = (size_t) ceil(coeff[0]*coeff[1]);  /* FOV * radius */
        geometry.height = (size_t) (2*coeff[3]);              /* input image height */
        /* correct center of distortion relative to new size */
        coeff[4] = (double) geometry.width/2.0;
        coeff[5] = (double) geometry.height/2.0;
        fix_bounds = MagickFalse;
        break;
      }
      case ShepardsDistortion:
      case BilinearForwardDistortion:
      case BilinearReverseDistortion:
#if 0
      case QuadrilateralDistortion:
#endif
      case PolynomialDistortion:
      case BarrelDistortion:
      case BarrelInverseDistortion:
      default:
        /* no calculated bestfit available for these distortions */
        bestfit = MagickFalse;
        fix_bounds = MagickFalse;
        break;
    }

    /* Set the output image geometry to calculated 'bestfit'.
       Yes this tends to 'over do' the file image size, ON PURPOSE!
       Do not do this for DePolar which needs to be exact for virtual tiling.
    */
    if ( fix_bounds ) {
      geometry.x = (ssize_t) floor(min.x-0.5);
      geometry.y = (ssize_t) floor(min.y-0.5);
      geometry.width=(size_t) ceil(max.x-geometry.x+0.5);
      geometry.height=(size_t) ceil(max.y-geometry.y+0.5);
    }

  }  /* end bestfit destination image calculations */

  /* The user provided a 'viewport' expert option which may
     overrides some parts of the current output image geometry.
     This also overrides its default 'bestfit' setting.
  */
  { const char *artifact=GetImageArtifact(image,"distort:viewport");
    viewport_given = MagickFalse;
    if ( artifact != (const char *) NULL ) {
      MagickStatusType flags=ParseAbsoluteGeometry(artifact,&geometry);
      if (flags==NoValue)
        (void) ThrowMagickException(exception,GetMagickModule(),
             OptionWarning,"InvalidSetting","'%s' '%s'",
             "distort:viewport",artifact);
      else
        viewport_given = MagickTrue;
    }
  }

  /* Verbose output */
  if (IsStringTrue(GetImageArtifact(image,"verbose")) != MagickFalse) {
    ssize_t
       i;
    char image_gen[MagickPathExtent];
    const char *lookup;

    /* Set destination image size and virtual offset */
    if ( bestfit || viewport_given ) {
      (void) FormatLocaleString(image_gen,MagickPathExtent,
        "  -size %.20gx%.20g -page %+.20g%+.20g xc: +insert \\\n",
        (double) geometry.width,(double) geometry.height,(double) geometry.x,
        (double) geometry.y);
      lookup="v.p{xx-v.page.x-0.5,yy-v.page.y-0.5}";
    }
    else {
      image_gen[0] = '\0';             /* no destination to generate */
      lookup = "p{xx-page.x-0.5,yy-page.y-0.5}"; /* simplify lookup */
    }

    switch (method)
    {
      case AffineDistortion:
      case RigidAffineDistortion:
      {
        double
          *inverse;

        inverse=(double *) AcquireQuantumMemory(6,sizeof(*inverse));
        if (inverse == (double *) NULL)
          {
            coeff=(double *) RelinquishMagickMemory(coeff);
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","%s","DistortImages");
            return((Image *) NULL);
          }
        InvertAffineCoefficients(coeff, inverse);
        CoefficientsToAffineArgs(inverse);
        (void) FormatLocaleFile(stderr, "Affine projection:\n");
        (void) FormatLocaleFile(stderr,
          "  -distort AffineProjection \\\n    '");
        for (i=0; i < 5; i++)
          (void) FormatLocaleFile(stderr, "%.*g,",GetMagickPrecision(),
            inverse[i]);
        (void) FormatLocaleFile(stderr, "%.*g'\n",GetMagickPrecision(),
          inverse[5]);
        (void) FormatLocaleFile(stderr,
          "Equivalent scale, rotation(deg), translation:\n");
        (void) FormatLocaleFile(stderr,"  %.*g,%.*g,%.*g,%.*g\n",
          GetMagickPrecision(),sqrt(inverse[0]*inverse[0]+
          inverse[1]*inverse[1]),GetMagickPrecision(),
          RadiansToDegrees(atan2(inverse[1],inverse[0])),
          GetMagickPrecision(),inverse[4],GetMagickPrecision(),inverse[5]);
        inverse=(double *) RelinquishMagickMemory(inverse);
        (void) FormatLocaleFile(stderr,"Affine distort, FX equivalent:\n");
        (void) FormatLocaleFile(stderr, "%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
        (void) FormatLocaleFile(stderr,"       xx=%+.*g*ii %+.*g*jj %+.*g;\n",
          GetMagickPrecision(),coeff[0],GetMagickPrecision(),coeff[1],
          GetMagickPrecision(),coeff[2]);
        (void) FormatLocaleFile(stderr,"       yy=%+.*g*ii %+.*g*jj %+.*g;\n",
          GetMagickPrecision(),coeff[3],GetMagickPrecision(),coeff[4],
          GetMagickPrecision(),coeff[5]);
        (void) FormatLocaleFile(stderr,"       %s' \\\n",lookup);
        break;
      }
      case PerspectiveDistortion:
      {
        double
          *inverse;

        inverse=(double *) AcquireQuantumMemory(8,sizeof(*inverse));
        if (inverse == (double *) NULL)
          {
            coeff=(double *) RelinquishMagickMemory(coeff);
            (void) ThrowMagickException(exception,GetMagickModule(),
              ResourceLimitError,"MemoryAllocationFailed","%s",
              "DistortCoefficients");
            return((Image *) NULL);
          }
        InvertPerspectiveCoefficients(coeff, inverse);
        (void) FormatLocaleFile(stderr,"Perspective Projection:\n");
        (void) FormatLocaleFile(stderr,
          "  -distort PerspectiveProjection \\\n      '");
        for (i=0; i < 4; i++)
          (void) FormatLocaleFile(stderr, "%.*g, ",GetMagickPrecision(),
            inverse[i]);
        (void) FormatLocaleFile(stderr, "\n       ");
        for ( ; i < 7; i++)
          (void) FormatLocaleFile(stderr, "%.*g, ",GetMagickPrecision(),
            inverse[i]);
        (void) FormatLocaleFile(stderr, "%.*g'\n",GetMagickPrecision(),
          inverse[7]);
        inverse=(double *) RelinquishMagickMemory(inverse);
        (void) FormatLocaleFile(stderr,"Perspective Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%.1024s",image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
        (void) FormatLocaleFile(stderr,"       rr=%+.*g*ii %+.*g*jj + 1;\n",
          GetMagickPrecision(),coeff[6],GetMagickPrecision(),coeff[7]);
        (void) FormatLocaleFile(stderr,
          "       xx=(%+.*g*ii %+.*g*jj %+.*g)/rr;\n",
          GetMagickPrecision(),coeff[0],GetMagickPrecision(),coeff[1],
          GetMagickPrecision(),coeff[2]);
        (void) FormatLocaleFile(stderr,
          "       yy=(%+.*g*ii %+.*g*jj %+.*g)/rr;\n",
          GetMagickPrecision(),coeff[3],GetMagickPrecision(),coeff[4],
          GetMagickPrecision(),coeff[5]);
        (void) FormatLocaleFile(stderr,"       rr%s0 ? %s : blue' \\\n",
          coeff[8] < 0.0 ? "<" : ">", lookup);
        break;
      }
      case BilinearForwardDistortion:
      {
        (void) FormatLocaleFile(stderr,"BilinearForward Mapping Equations:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,"    i = %+lf*x %+lf*y %+lf*x*y %+lf;\n",
          coeff[0],coeff[1],coeff[2],coeff[3]);
        (void) FormatLocaleFile(stderr,"    j = %+lf*x %+lf*y %+lf*x*y %+lf;\n",
          coeff[4],coeff[5],coeff[6],coeff[7]);
#if 0
        /* for debugging */
        (void) FormatLocaleFile(stderr, "   c8 = %+lf  c9 = 2*a = %+lf;\n",
            coeff[8], coeff[9]);
#endif
        (void) FormatLocaleFile(stderr,
          "BilinearForward Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x%+lf; jj=j+page.y%+lf;\n",0.5-coeff[3],0.5-
          coeff[7]);
        (void) FormatLocaleFile(stderr,"       bb=%lf*ii %+lf*jj %+lf;\n",
          coeff[6], -coeff[2], coeff[8]);
        /* Handle Special degenerate (non-quadratic) or trapezoidal case */
        if (coeff[9] != 0)
          {
            (void) FormatLocaleFile(stderr,
              "       rt=bb*bb %+lf*(%lf*ii%+lf*jj);\n",-2*coeff[9],coeff[4],
              -coeff[0]);
          (void) FormatLocaleFile(stderr,
            "       yy=( -bb + sqrt(rt) ) / %lf;\n",coeff[9]);
          }
        else
          (void) FormatLocaleFile(stderr,"       yy=(%lf*ii%+lf*jj)/bb;\n",
            -coeff[4],coeff[0]);
        (void) FormatLocaleFile(stderr,
          "       xx=(ii %+lf*yy)/(%lf %+lf*yy);\n",-coeff[1],coeff[0],
          coeff[2]);
        if ( coeff[9] != 0 )
          (void) FormatLocaleFile(stderr,"       (rt < 0 ) ? red : %s'\n",
            lookup);
        else
          (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
        break;
      }
      case BilinearReverseDistortion:
      {
#if 0
        (void) FormatLocaleFile(stderr, "Polynomial Projection Distort:\n");
        (void) FormatLocaleFile(stderr, "  -distort PolynomialProjection \\\n");
        (void) FormatLocaleFile(stderr, "      '1.5, %lf, %lf, %lf, %lf,\n",
            coeff[3], coeff[0], coeff[1], coeff[2]);
        (void) FormatLocaleFile(stderr, "            %lf, %lf, %lf, %lf'\n",
            coeff[7], coeff[4], coeff[5], coeff[6]);
#endif
        (void) FormatLocaleFile(stderr,
          "BilinearReverse Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
        (void) FormatLocaleFile(stderr,
          "       xx=%+lf*ii %+lf*jj %+lf*ii*jj %+lf;\n",coeff[0],coeff[1],
          coeff[2], coeff[3]);
        (void) FormatLocaleFile(stderr,
           "       yy=%+lf*ii %+lf*jj %+lf*ii*jj %+lf;\n",coeff[4],coeff[5],
           coeff[6], coeff[7]);
        (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
        break;
      }
      case PolynomialDistortion:
      {
        size_t nterms = (size_t) coeff[1];
        (void) FormatLocaleFile(stderr,
          "Polynomial (order %lg, terms %lu), FX Equivelent\n",coeff[0],
          (unsigned long) nterms);
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x+0.5; jj=j+page.y+0.5;\n");
        (void) FormatLocaleFile(stderr, "       xx =");
        for (i=0; i < (ssize_t) nterms; i++)
        {
          if ((i != 0) && (i%4 == 0))
            (void) FormatLocaleFile(stderr, "\n         ");
          (void) FormatLocaleFile(stderr," %+lf%s",coeff[2+i],
            poly_basis_str(i));
        }
        (void) FormatLocaleFile(stderr,";\n       yy =");
        for (i=0; i < (ssize_t) nterms; i++)
        {
          if ((i != 0) && (i%4 == 0))
            (void) FormatLocaleFile(stderr,"\n         ");
          (void) FormatLocaleFile(stderr," %+lf%s",coeff[2+i+nterms],
            poly_basis_str(i));
        }
        (void) FormatLocaleFile(stderr,";\n       %s' \\\n", lookup);
        break;
      }
      case ArcDistortion:
      {
        (void) FormatLocaleFile(stderr,"Arc Distort, Internal Coefficients:\n");
        for (i=0; i < 5; i++)
          (void) FormatLocaleFile(stderr,
            "  c%.20g = %+lf\n",(double) i,coeff[i]);
        (void) FormatLocaleFile(stderr,"Arc Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,"  -fx 'ii=i+page.x; jj=j+page.y;\n");
        (void) FormatLocaleFile(stderr,"       xx=(atan2(jj,ii)%+lf)/(2*pi);\n",
          -coeff[0]);
        (void) FormatLocaleFile(stderr,"       xx=xx-round(xx);\n");
        (void) FormatLocaleFile(stderr,"       xx=xx*%lf %+lf;\n",coeff[1],
          coeff[4]);
        (void) FormatLocaleFile(stderr,
          "       yy=(%lf - hypot(ii,jj)) * %lf;\n",coeff[2],coeff[3]);
        (void) FormatLocaleFile(stderr,"       v.p{xx-.5,yy-.5}' \\\n");
        break;
      }
      case PolarDistortion:
      {
        (void) FormatLocaleFile(stderr,"Polar Distort, Internal Coefficents\n");
        for (i=0; i < 8; i++)
          (void) FormatLocaleFile(stderr,"  c%.20g = %+lf\n",(double) i,
            coeff[i]);
        (void) FormatLocaleFile(stderr,"Polar Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x%+lf; jj=j+page.y%+lf;\n",-coeff[2],-coeff[3]);
        (void) FormatLocaleFile(stderr,"       xx=(atan2(ii,jj)%+lf)/(2*pi);\n",
          -(coeff[4]+coeff[5])/2 );
        (void) FormatLocaleFile(stderr,"       xx=xx-round(xx);\n");
        (void) FormatLocaleFile(stderr,"       xx=xx*2*pi*%lf + v.w/2;\n",
          coeff[6] );
        (void) FormatLocaleFile(stderr,"       yy=(hypot(ii,jj)%+lf)*%lf;\n",
          -coeff[1],coeff[7] );
        (void) FormatLocaleFile(stderr,"       v.p{xx-.5,yy-.5}' \\\n");
        break;
      }
      case DePolarDistortion:
      {
        (void) FormatLocaleFile(stderr,
          "DePolar Distort, Internal Coefficents\n");
        for (i=0; i < 8; i++)
          (void) FormatLocaleFile(stderr,"  c%.20g = %+lf\n",(double) i,
            coeff[i]);
        (void) FormatLocaleFile(stderr,"DePolar Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,"  -fx 'aa=(i+.5)*%lf %+lf;\n",
          coeff[6],+coeff[4]);
        (void) FormatLocaleFile(stderr,"       rr=(j+.5)*%lf %+lf;\n",
          coeff[7],+coeff[1]);
        (void) FormatLocaleFile(stderr,"       xx=rr*sin(aa) %+lf;\n",
          coeff[2]);
        (void) FormatLocaleFile(stderr,"       yy=rr*cos(aa) %+lf;\n",
          coeff[3]);
        (void) FormatLocaleFile(stderr,"       v.p{xx-.5,yy-.5}' \\\n");
        break;
      }
      case Cylinder2PlaneDistortion:
      {
        (void) FormatLocaleFile(stderr,
          "Cylinder to Plane Distort, Internal Coefficents\n");
        (void) FormatLocaleFile(stderr,"  cylinder_radius = %+lf\n",coeff[1]);
        (void) FormatLocaleFile(stderr,
          "Cylinder to Plane Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr, "%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x%+lf+0.5; jj=j+page.y%+lf+0.5;\n",-coeff[4],
          -coeff[5]);
        (void) FormatLocaleFile(stderr,"       aa=atan(ii/%+lf);\n",coeff[1]);
        (void) FormatLocaleFile(stderr,"       xx=%lf*aa%+lf;\n",
          coeff[1],coeff[2]);
        (void) FormatLocaleFile(stderr,"       yy=jj*cos(aa)%+lf;\n",coeff[3]);
        (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
        break;
      }
      case Plane2CylinderDistortion:
      {
        (void) FormatLocaleFile(stderr,
          "Plane to Cylinder Distort, Internal Coefficents\n");
        (void) FormatLocaleFile(stderr,"  cylinder_radius = %+lf\n",coeff[1]);
        (void) FormatLocaleFile(stderr,
          "Plane to Cylinder Distort, FX Equivelent:\n");
        (void) FormatLocaleFile(stderr,"%s", image_gen);
        (void) FormatLocaleFile(stderr,
          "  -fx 'ii=i+page.x%+lf+0.5; jj=j+page.y%+lf+0.5;\n",-coeff[4],
          -coeff[5]);
        (void) FormatLocaleFile(stderr,"       ii=ii/%+lf;\n",coeff[1]);
        (void) FormatLocaleFile(stderr,"       xx=%lf*tan(ii)%+lf;\n",coeff[1],
          coeff[2] );
        (void) FormatLocaleFile(stderr,"       yy=jj/cos(ii)%+lf;\n",coeff[3]);
        (void) FormatLocaleFile(stderr,"       %s' \\\n", lookup);
        break;
      }
      case BarrelDistortion:
      case BarrelInverseDistortion:
      {
        double
          xc,
          yc;

        /*
          NOTE: This does the barrel roll in pixel coords not image coords
          The internal distortion must do it in image coordinates,
          so that is what the center coeff (8,9) is given in.
        */
        xc=((double)image->columns-1.0)/2.0+image->page.x;
        yc=((double)image->rows-1.0)/2.0+image->page.y;
        (void) FormatLocaleFile(stderr, "Barrel%s Distort, FX Equivelent:\n",
          method == BarrelDistortion ? "" : "Inv");
        (void) FormatLocaleFile(stderr, "%s", image_gen);
        if ( fabs(coeff[8]-xc-0.5) < 0.1 && fabs(coeff[9]-yc-0.5) < 0.1 )
          (void) FormatLocaleFile(stderr,"  -fx 'xc=(w-1)/2;  yc=(h-1)/2;\n");
        else
          (void) FormatLocaleFile(stderr,"  -fx 'xc=%lf;  yc=%lf;\n",coeff[8]-
            0.5,coeff[9]-0.5);
        (void) FormatLocaleFile(stderr,
          "       ii=i-xc;  jj=j-yc;  rr=hypot(ii,jj);\n");
        (void) FormatLocaleFile(stderr,
          "       ii=ii%s(%lf*rr*rr*rr %+lf*rr*rr %+lf*rr %+lf);\n",
          method == BarrelDistortion ? "*" : "/",coeff[0],coeff[1],coeff[2],
          coeff[3]);
        (void) FormatLocaleFile(stderr,
          "       jj=jj%s(%lf*rr*rr*rr %+lf*rr*rr %+lf*rr %+lf);\n",
          method == BarrelDistortion ? "*" : "/",coeff[4],coeff[5],coeff[6],
          coeff[7]);
        (void) FormatLocaleFile(stderr,"       v.p{fx*ii+xc,fy*jj+yc}' \\\n");
      }
      default:
        break;
    }
  }
  /*
    The user provided a 'scale' expert option will scale the output image size,
    by the factor given allowing for super-sampling of the distorted image
    space.  Any scaling factors must naturally be halved as a result.
  */
  { const char *artifact;
    artifact=GetImageArtifact(image,"distort:scale");
    output_scaling = 1.0;
    if (artifact != (const char *) NULL) {
      output_scaling = fabs(StringToDouble(artifact,(char **) NULL));
      geometry.width=(size_t) (output_scaling*geometry.width+0.5);
      geometry.height=(size_t) (output_scaling*geometry.height+0.5);
      geometry.x=(ssize_t) (output_scaling*geometry.x+0.5);
      geometry.y=(ssize_t) (output_scaling*geometry.y+0.5);
      if ( output_scaling < 0.1 ) {
        coeff = (double *) RelinquishMagickMemory(coeff);
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
                "InvalidArgument","%s", "-set option:distort:scale" );
        return((Image *) NULL);
      }
      output_scaling = 1/output_scaling;
    }
  }
#define ScaleFilter(F,A,B,C,D) \
    ScaleResampleFilter( (F), \
      output_scaling*(A), output_scaling*(B), \
      output_scaling*(C), output_scaling*(D) )

  /*
    Initialize the distort image attributes.
  */
  distort_image=CloneImage(image,geometry.width,geometry.height,MagickTrue,
    exception);
  if (distort_image == (Image *) NULL)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      return((Image *) NULL);
    }
  /* if image is ColorMapped - change it to DirectClass */
  if (SetImageStorageClass(distort_image,DirectClass,exception) == MagickFalse)
    {
      coeff=(double *) RelinquishMagickMemory(coeff);
      distort_image=DestroyImage(distort_image);
      return((Image *) NULL);
    }
  if ((IsPixelInfoGray(&distort_image->background_color) == MagickFalse) &&
      (IsGrayColorspace(distort_image->colorspace) != MagickFalse))
    (void) SetImageColorspace(distort_image,sRGBColorspace,exception);
  if (distort_image->background_color.alpha_trait != UndefinedPixelTrait)
    distort_image->alpha_trait=BlendPixelTrait;
  distort_image->page.x=geometry.x;
  distort_image->page.y=geometry.y;
  ConformPixelInfo(distort_image,&distort_image->matte_color,&invalid,
    exception);

  { /* ----- MAIN CODE -----
       Sample the source image to each pixel in the distort image.
     */
    CacheView
      *distort_view;

    MagickBooleanType
      status;

    MagickOffsetType
      progress;

    PixelInfo
      zero;

    ResampleFilter
      **magick_restrict resample_filter;

    ssize_t
      j;

    status=MagickTrue;
    progress=0;
    GetPixelInfo(distort_image,&zero);
    resample_filter=AcquireResampleFilterThreadSet(image,
      UndefinedVirtualPixelMethod,MagickFalse,exception);
    distort_view=AcquireAuthenticCacheView(distort_image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp parallel for schedule(static) shared(progress,status) \
      magick_number_threads(image,distort_image,distort_image->rows,1)
#endif
    for (j=0; j < (ssize_t) distort_image->rows; j++)
    {
      const int
        id = GetOpenMPThreadId();

      double
        validity;  /* how mathematically valid is this the mapping */

      MagickBooleanType
        sync;

      PixelInfo
        pixel;    /* pixel color to assign to distorted image */

      PointInfo
        d,
        s;  /* transform destination image x,y  to source image x,y */

      ssize_t
        i;

      Quantum
        *magick_restrict q;

      q=QueueCacheViewAuthenticPixels(distort_view,0,j,distort_image->columns,1,
        exception);
      if (q == (Quantum *) NULL)
        {
          status=MagickFalse;
          continue;
        }
      pixel=zero;

      /* Define constant scaling vectors for Affine Distortions
        Other methods are either variable, or use interpolated lookup
      */
      switch (method)
      {
        case AffineDistortion:
        case RigidAffineDistortion:
          ScaleFilter( resample_filter[id],
            coeff[0], coeff[1],
            coeff[3], coeff[4] );
          break;
        default:
          break;
      }

      /* Initialize default pixel validity
      *    negative:         pixel is invalid  output 'matte_color'
      *    0.0 to 1.0:       antialiased, mix with resample output
      *    1.0 or greater:   use resampled output.
      */
      validity = 1.0;

      for (i=0; i < (ssize_t) distort_image->columns; i++)
      {
        /* map pixel coordinate to distortion space coordinate */
        d.x = (double) (geometry.x+i+0.5)*output_scaling;
        d.y = (double) (geometry.y+j+0.5)*output_scaling;
        s = d;  /* default is a no-op mapping */
        switch (method)
        {
          case AffineDistortion:
          case RigidAffineDistortion:
          {
            s.x=coeff[0]*d.x+coeff[1]*d.y+coeff[2];
            s.y=coeff[3]*d.x+coeff[4]*d.y+coeff[5];
            /* Affine partial derivitives are constant -- set above */
            break;
          }
          case PerspectiveDistortion:
          {
            double
              p,q,r,abs_r,abs_c6,abs_c7,scale;
            /* perspective is a ratio of affines */
            p=coeff[0]*d.x+coeff[1]*d.y+coeff[2];
            q=coeff[3]*d.x+coeff[4]*d.y+coeff[5];
            r=coeff[6]*d.x+coeff[7]*d.y+1.0;
            /* Pixel Validity -- is it a 'sky' or 'ground' pixel */
            validity = (r*coeff[8] < 0.0) ? 0.0 : 1.0;
            /* Determine horizon anti-alias blending */
            abs_r = fabs(r)*2;
            abs_c6 = fabs(coeff[6]);
            abs_c7 = fabs(coeff[7]);
            if ( abs_c6 > abs_c7 ) {
              if ( abs_r < abs_c6*output_scaling )
                validity = 0.5 - coeff[8]*r/(coeff[6]*output_scaling);
            }
            else if ( abs_r < abs_c7*output_scaling )
              validity = 0.5 - coeff[8]*r/(coeff[7]*output_scaling);
            /* Perspective Sampling Point (if valid) */
            if ( validity > 0.0 ) {
              /* divide by r affine, for perspective scaling */
              scale = 1.0/r;
              s.x = p*scale;
              s.y = q*scale;
              /* Perspective Partial Derivatives or Scaling Vectors */
              scale *= scale;
              ScaleFilter( resample_filter[id],
                (r*coeff[0] - p*coeff[6])*scale,
                (r*coeff[1] - p*coeff[7])*scale,
                (r*coeff[3] - q*coeff[6])*scale,
                (r*coeff[4] - q*coeff[7])*scale );
            }
            break;
          }
          case BilinearReverseDistortion:
          {
            /* Reversed Mapped is just a simple polynomial */
            s.x=coeff[0]*d.x+coeff[1]*d.y+coeff[2]*d.x*d.y+coeff[3];
            s.y=coeff[4]*d.x+coeff[5]*d.y
                    +coeff[6]*d.x*d.y+coeff[7];
            /* Bilinear partial derivitives of scaling vectors */
            ScaleFilter( resample_filter[id],
                coeff[0] + coeff[2]*d.y,
                coeff[1] + coeff[2]*d.x,
                coeff[4] + coeff[6]*d.y,
                coeff[5] + coeff[6]*d.x );
            break;
          }
          case BilinearForwardDistortion:
          {
            /* Forward mapped needs reversed polynomial equations
             * which unfortunatally requires a square root!  */
            double b,c;
            d.x -= coeff[3];  d.y -= coeff[7];
            b = coeff[6]*d.x - coeff[2]*d.y + coeff[8];
            c = coeff[4]*d.x - coeff[0]*d.y;

            validity = 1.0;
            /* Handle Special degenerate (non-quadratic) case
             * Currently without horizon anti-alising */
            if ( fabs(coeff[9]) < MagickEpsilon )
              s.y =  -c/b;
            else {
              c = b*b - 2*coeff[9]*c;
              if ( c < 0.0 )
                validity = 0.0;
              else
                s.y = ( -b + sqrt(c) )/coeff[9];
            }
            if ( validity > 0.0 )
              s.x = ( d.x - coeff[1]*s.y) / ( coeff[0] + coeff[2]*s.y );

            /* NOTE: the sign of the square root should be -ve for parts
                     where the source image becomes 'flipped' or 'mirrored'.
               FUTURE: Horizon handling
               FUTURE: Scaling factors or Deritives (how?)
            */
            break;
          }
#if 0
          case BilinearDistortion:
            /* Bilinear mapping of any Quadrilateral to any Quadrilateral */
            /* UNDER DEVELOPMENT */
            break;
#endif
          case PolynomialDistortion:
          {
            /* multi-ordered polynomial */
            ssize_t
              k;

            ssize_t
              nterms=(ssize_t)coeff[1];

            PointInfo
              du,dv; /* the du,dv vectors from unit dx,dy -- derivatives */

            s.x=s.y=du.x=du.y=dv.x=dv.y=0.0;
            for(k=0; k < nterms; k++) {
              s.x  += poly_basis_fn(k,d.x,d.y)*coeff[2+k];
              du.x += poly_basis_dx(k,d.x,d.y)*coeff[2+k];
              du.y += poly_basis_dy(k,d.x,d.y)*coeff[2+k];
              s.y  += poly_basis_fn(k,d.x,d.y)*coeff[2+k+nterms];
              dv.x += poly_basis_dx(k,d.x,d.y)*coeff[2+k+nterms];
              dv.y += poly_basis_dy(k,d.x,d.y)*coeff[2+k+nterms];
            }
            ScaleFilter( resample_filter[id], du.x,du.y,dv.x,dv.y );
            break;
          }
          case ArcDistortion:
          {
            /* what is the angle and radius in the destination image */
            s.x  = (double) ((atan2(d.y,d.x) - coeff[0])/Magick2PI);
            s.x -= MagickRound(s.x);     /* angle */
            s.y  = hypot(d.x,d.y);       /* radius */

            /* Arc Distortion Partial Scaling Vectors
              Are derived by mapping the perpendicular unit vectors
              dR  and  dA*R*2PI  rather than trying to map dx and dy
              The results is a very simple orthogonal aligned ellipse.
            */
            if ( s.y > MagickEpsilon )
              ScaleFilter( resample_filter[id],
                  (double) (coeff[1]/(Magick2PI*s.y)), 0, 0, coeff[3] );
            else
              ScaleFilter( resample_filter[id],
                  distort_image->columns*2, 0, 0, coeff[3] );

            /* now scale the angle and radius for source image lookup point */
            s.x = s.x*coeff[1] + coeff[4] + image->page.x +0.5;
            s.y = (coeff[2] - s.y) * coeff[3] + image->page.y;
            break;
          }
          case PolarDistortion:
          { /* 2D Cartesain to Polar View */
            d.x -= coeff[2];
            d.y -= coeff[3];
            s.x  = atan2(d.x,d.y) - (coeff[4]+coeff[5])/2;
            s.x /= Magick2PI;
            s.x -= MagickRound(s.x);
            s.x *= Magick2PI;       /* angle - relative to centerline */
            s.y  = hypot(d.x,d.y);  /* radius */

            /* Polar Scaling vectors are based on mapping dR and dA vectors
               This results in very simple orthogonal scaling vectors
            */
            if ( s.y > MagickEpsilon )
              ScaleFilter( resample_filter[id],
                (double) (coeff[6]/(Magick2PI*s.y)), 0, 0, coeff[7] );
            else
              ScaleFilter( resample_filter[id],
                  distort_image->columns*2, 0, 0, coeff[7] );

            /* now finish mapping radius/angle to source x,y coords */
            s.x = s.x*coeff[6] + (double)image->columns/2.0 + image->page.x;
            s.y = (s.y-coeff[1])*coeff[7] + image->page.y;
            break;
          }
          case DePolarDistortion:
          { /* @D Polar to Carteasain  */
            /* ignore all destination virtual offsets */
            d.x = ((double)i+0.5)*output_scaling*coeff[6]+coeff[4];
            d.y = ((double)j+0.5)*output_scaling*coeff[7]+coeff[1];
            s.x = d.y*sin(d.x) + coeff[2];
            s.y = d.y*cos(d.x) + coeff[3];
            /* derivatives are usless - better to use SuperSampling */
            break;
          }
          case Cylinder2PlaneDistortion:
          { /* 3D Cylinder to Tangential Plane */
            double ax, cx;
            /* relative to center of distortion */
            d.x -= coeff[4]; d.y -= coeff[5];
            d.x /= coeff[1];        /* x' = x/r */
            ax=atan(d.x);           /* aa = atan(x/r) = u/r  */
            cx=cos(ax);             /* cx = cos(atan(x/r)) = 1/sqrt(x^2+u^2) */
            s.x = coeff[1]*ax;      /* u  = r*atan(x/r) */
            s.y = d.y*cx;           /* v  = y*cos(u/r) */
            /* derivatives... (see personnal notes) */
            ScaleFilter( resample_filter[id],
                  1.0/(1.0+d.x*d.x), 0.0, -d.x*s.y*cx*cx/coeff[1], s.y/d.y );
#if 0
if ( i == 0 && j == 0 ) {
  fprintf(stderr, "x=%lf  y=%lf  u=%lf  v=%lf\n", d.x*coeff[1], d.y, s.x, s.y);
  fprintf(stderr, "phi = %lf\n", (double)(ax * 180.0/MagickPI) );
  fprintf(stderr, "du/dx=%lf  du/dx=%lf  dv/dx=%lf  dv/dy=%lf\n",
                1.0/(1.0+d.x*d.x), 0.0, -d.x*s.y*cx*cx/coeff[1], s.y/d.y );
  fflush(stderr); }
#endif
            /* add center of distortion in source */
            s.x += coeff[2]; s.y += coeff[3];
            break;
          }
          case Plane2CylinderDistortion:
          { /* 3D Cylinder to Tangential Plane */
            /* relative to center of distortion */
            d.x -= coeff[4]; d.y -= coeff[5];

            /* is pixel valid - horizon of a infinite Virtual-Pixel Plane
             * (see Anthony Thyssen's personal note) */
            validity = (double) (coeff[1]*MagickPI2 - fabs(d.x))/output_scaling + 0.5;

            if ( validity > 0.0 ) {
              double cx,tx;
              d.x /= coeff[1];           /* x'= x/r */
              cx = 1/cos(d.x);           /* cx = 1/cos(x/r) */
              tx = tan(d.x);             /* tx = tan(x/r) */
              s.x = coeff[1]*tx;         /* u = r * tan(x/r) */
              s.y = d.y*cx;              /* v = y / cos(x/r) */
              /* derivatives...  (see Anthony Thyssen's personal notes) */
              ScaleFilter( resample_filter[id],
                    cx*cx, 0.0, s.y*cx/coeff[1], cx );
#if 0
/*if ( i == 0 && j == 0 )*/
if ( d.x == 0.5 && d.y == 0.5 ) {
  fprintf(stderr, "x=%lf  y=%lf  u=%lf  v=%lf\n", d.x*coeff[1], d.y, s.x, s.y);
  fprintf(stderr, "radius = %lf  phi = %lf  validity = %lf\n",
      coeff[1],  (double)(d.x * 180.0/MagickPI), validity );
  fprintf(stderr, "du/dx=%lf  du/dx=%lf  dv/dx=%lf  dv/dy=%lf\n",
      cx*cx, 0.0, s.y*cx/coeff[1], cx);
  fflush(stderr); }
#endif
            }
            /* add center of distortion in source */
            s.x += coeff[2]; s.y += coeff[3];
            break;
          }
          case BarrelDistortion:
          case BarrelInverseDistortion:
          { /* Lens Barrel Distionion Correction */
            double r,fx,fy,gx,gy;
            /* Radial Polynomial Distortion (de-normalized) */
            d.x -= coeff[8];
            d.y -= coeff[9];
            r = sqrt(d.x*d.x+d.y*d.y);
            if ( r > MagickEpsilon ) {
              fx = ((coeff[0]*r + coeff[1])*r + coeff[2])*r + coeff[3];
              fy = ((coeff[4]*r + coeff[5])*r + coeff[6])*r + coeff[7];
              gx = ((3*coeff[0]*r + 2*coeff[1])*r + coeff[2])/r;
              gy = ((3*coeff[4]*r + 2*coeff[5])*r + coeff[6])/r;
              /* adjust functions and scaling for 'inverse' form */
              if ( method == BarrelInverseDistortion ) {
                fx = 1/fx;  fy = 1/fy;
                gx *= -fx*fx;  gy *= -fy*fy;
              }
              /* Set the source pixel to lookup and EWA derivative vectors */
              s.x = d.x*fx + coeff[8];
              s.y = d.y*fy + coeff[9];
              ScaleFilter( resample_filter[id],
                  gx*d.x*d.x + fx, gx*d.x*d.y,
                  gy*d.x*d.y,      gy*d.y*d.y + fy );
            }
            else {
              /* Special handling to avoid divide by zero when r==0
              **
              ** The source and destination pixels match in this case
              ** which was set at the top of the loop using  s = d;
              ** otherwise...   s.x=coeff[8]; s.y=coeff[9];
              */
              if ( method == BarrelDistortion )
                ScaleFilter( resample_filter[id],
                     coeff[3], 0, 0, coeff[7] );
              else /* method == BarrelInverseDistortion */
                /* FUTURE, trap for D==0 causing division by zero */
                ScaleFilter( resample_filter[id],
                     1.0/coeff[3], 0, 0, 1.0/coeff[7] );
            }
            break;
          }
          case ShepardsDistortion:
          { /* Shepards Method, or Inverse Weighted Distance for
               displacement around the destination image control points
               The input arguments are the coefficents to the function.
               This is more of a 'displacement' function rather than an
               absolute distortion function.

               Note: We can not determine derivatives using shepards method
               so only a point sample interpolatation can be used.
            */
            size_t
              i;
            double
              denominator;

            denominator = s.x = s.y = 0;
            for(i=0; i<number_arguments; i+=4) {
              double weight =
                  ((double)d.x-arguments[i+2])*((double)d.x-arguments[i+2])
                + ((double)d.y-arguments[i+3])*((double)d.y-arguments[i+3]);
              weight = pow(weight,coeff[0]); /* shepards power factor */
              weight = ( weight < 1.0 ) ? 1.0 : 1.0/weight;

              s.x += (arguments[ i ]-arguments[i+2])*weight;
              s.y += (arguments[i+1]-arguments[i+3])*weight;
              denominator += weight;
            }
            s.x /= denominator;
            s.y /= denominator;
            s.x += d.x;   /* make it as relative displacement */
            s.y += d.y;
            break;
          }
          default:
            break; /* use the default no-op given above */
        }
        /* map virtual canvas location back to real image coordinate */
        if ( bestfit && method != ArcDistortion ) {
          s.x -= image->page.x;
          s.y -= image->page.y;
        }
        s.x -= 0.5;
        s.y -= 0.5;

        if ( validity <= 0.0 ) {
          /* result of distortion is an invalid pixel - don't resample */
          SetPixelViaPixelInfo(distort_image,&invalid,q);
        }
        else {
          /* resample the source image to find its correct color */
          (void) ResamplePixelColor(resample_filter[id],s.x,s.y,&pixel,
            exception);
          /* if validity between 0.0 and 1.0 mix result with invalid pixel */
          if ( validity < 1.0 ) {
            /* Do a blend of sample color and invalid pixel */
            /* should this be a 'Blend', or an 'Over' compose */
            CompositePixelInfoBlend(&pixel,validity,&invalid,(1.0-validity),
              &pixel);
          }
          SetPixelViaPixelInfo(distort_image,&pixel,q);
        }
        q+=GetPixelChannels(distort_image);
      }
      sync=SyncCacheViewAuthenticPixels(distort_view,exception);
      if (sync == MagickFalse)
        status=MagickFalse;
      if (image->progress_monitor != (MagickProgressMonitor) NULL)
        {
          MagickBooleanType
            proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
          #pragma omp atomic
#endif
          progress++;
          proceed=SetImageProgress(image,DistortImageTag,progress,image->rows);
          if (proceed == MagickFalse)
            status=MagickFalse;
        }
    }
    distort_view=DestroyCacheView(distort_view);
    resample_filter=DestroyResampleFilterThreadSet(resample_filter);

    if (status == MagickFalse)
      distort_image=DestroyImage(distort_image);
  }

  /* Arc does not return an offset unless 'bestfit' is in effect
     And the user has not provided an overriding 'viewport'.
   */
  if ( method == ArcDistortion && !bestfit && !viewport_given ) {
    distort_image->page.x = 0;
    distort_image->page.y = 0;
  }
  coeff=(double *) RelinquishMagickMemory(coeff);
  return(distort_image);
}