MagickPrivate void XColorBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
#define CancelButtonText  "Cancel"
#define ColornameText  "Name:"
#define ColorPatternText  "Pattern:"
#define GrabButtonText  "Grab"
#define ResetButtonText  "Reset"

  char
    **colorlist,
    primary_selection[MagickPathExtent],
    reset_pattern[MagickPathExtent],
    text[MagickPathExtent];

  ExceptionInfo
    *exception;

  int
    x,
    y;

  int
    i;

  static char
    glob_pattern[MagickPathExtent] = "*";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    height,
    text_width,
    visible_colors,
    width;

  size_t
    colors,
    delay,
    state;

  XColor
    color;

  XEvent
    event;

  XFontStruct
    *font_info;

  XTextProperty
    window_name;

  XWidgetInfo
    action_info,
    cancel_info,
    expose_info,
    grab_info,
    list_info,
    mode_info,
    north_info,
    reply_info,
    reset_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info;

  XWindowChanges
    window_changes;

  /*
    Get color list and sort in ascending order.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(reset_pattern,"*",MagickPathExtent);
  exception=AcquireExceptionInfo();
  colorlist=GetColorList(glob_pattern,&colors,exception);
  if (colorlist == (char **) NULL)
    {
      /*
        Pattern failed, obtain all the colors.
      */
      (void) CopyMagickString(glob_pattern,"*",MagickPathExtent);
      colorlist=GetColorList(glob_pattern,&colors,exception);
      if (colorlist == (char **) NULL)
        {
          XNoticeWidget(display,windows,"Unable to obtain colors names:",
            glob_pattern);
          (void) XDialogWidget(display,windows,action,"Enter color name:",
            reply);
          return;
        }
    }
  /*
    Determine Color Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=0;
  for (i=0; i < (int) colors; i++)
    if (WidgetTextWidth(font_info,colorlist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,colorlist[i]);
  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,ResetButtonText) > width)
    width=WidgetTextWidth(font_info,ResetButtonText);
  if (WidgetTextWidth(font_info,GrabButtonText) > width)
    width=WidgetTextWidth(font_info,GrabButtonText);
  width+=QuantumMargin;
  if (WidgetTextWidth(font_info,ColorPatternText) > width)
    width=WidgetTextWidth(font_info,ColorPatternText);
  if (WidgetTextWidth(font_info,ColornameText) > width)
    width=WidgetTextWidth(font_info,ColornameText);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position Color Browser widget.
  */
  windows->widget.width=(unsigned int)
    (width+MagickMin((int) text_width,(int) MaxTextWidth)+6*QuantumMargin);
  windows->widget.min_width=(unsigned int)
    (width+MinTextWidth+4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    ((81*height) >> 2)+((13*QuantumMargin) >> 1)+4;
  windows->widget.min_height=(unsigned int)
    (((23*height) >> 1)+((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Color Browser widget.
  */
  (void) CopyMagickString(windows->widget.name,"Browse and Select a Color",
    MagickPathExtent);
  status=XStringListToTextProperty(&windows->widget.name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,windows->widget.id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) windows->widget.width;
  window_changes.height=(int) windows->widget.height;
  window_changes.x=windows->widget.x;
  window_changes.y=windows->widget.y;
  (void) XReconfigureWMWindow(display,windows->widget.id,windows->widget.screen,
    mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&mode_info);
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_colors=0;
  delay=SuspendTime << 2;
  state=UpdateConfigurationState;
  do
  {
    if (state & UpdateConfigurationState)
      {
        int
          id;

        /*
          Initialize button information.
        */
        XGetWidgetInfo(CancelButtonText,&cancel_info);
        cancel_info.width=width;
        cancel_info.height=(unsigned int) ((3*height) >> 1);
        cancel_info.x=(int)
          (windows->widget.width-cancel_info.width-QuantumMargin-2);
        cancel_info.y=(int)
          (windows->widget.height-cancel_info.height-QuantumMargin);
        XGetWidgetInfo(action,&action_info);
        action_info.width=width;
        action_info.height=(unsigned int) ((3*height) >> 1);
        action_info.x=cancel_info.x-(cancel_info.width+(QuantumMargin >> 1)+
          (action_info.bevel_width << 1));
        action_info.y=cancel_info.y;
        XGetWidgetInfo(GrabButtonText,&grab_info);
        grab_info.width=width;
        grab_info.height=(unsigned int) ((3*height) >> 1);
        grab_info.x=QuantumMargin;
        grab_info.y=((5*QuantumMargin) >> 1)+height;
        XGetWidgetInfo(ResetButtonText,&reset_info);
        reset_info.width=width;
        reset_info.height=(unsigned int) ((3*height) >> 1);
        reset_info.x=QuantumMargin;
        reset_info.y=grab_info.y+grab_info.height+QuantumMargin;
        /*
          Initialize reply information.
        */
        XGetWidgetInfo(reply,&reply_info);
        reply_info.raised=MagickFalse;
        reply_info.bevel_width--;
        reply_info.width=windows->widget.width-width-((6*QuantumMargin) >> 1);
        reply_info.height=height << 1;
        reply_info.x=(int) (width+(QuantumMargin << 1));
        reply_info.y=action_info.y-reply_info.height-QuantumMargin;
        /*
          Initialize mode information.
        */
        XGetWidgetInfo((char *) NULL,&mode_info);
        mode_info.active=MagickTrue;
        mode_info.bevel_width=0;
        mode_info.width=(unsigned int) (action_info.x-(QuantumMargin << 1));
        mode_info.height=action_info.height;
        mode_info.x=QuantumMargin;
        mode_info.y=action_info.y;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int) (reply_info.y-grab_info.y-
          (QuantumMargin >> 1));
        scroll_info.x=reply_info.x+(reply_info.width-scroll_info.width);
        scroll_info.y=grab_info.y-reply_info.bevel_width;
        scroll_info.raised=MagickFalse;
        scroll_info.trough=MagickTrue;
        north_info=scroll_info;
        north_info.raised=MagickTrue;
        north_info.width-=(north_info.bevel_width << 1);
        north_info.height=north_info.width-1;
        north_info.x+=north_info.bevel_width;
        north_info.y+=north_info.bevel_width;
        south_info=north_info;
        south_info.y=scroll_info.y+scroll_info.height-scroll_info.bevel_width-
          south_info.height;
        id=slider_info.id;
        slider_info=north_info;
        slider_info.id=id;
        slider_info.width-=2;
        slider_info.min_y=north_info.y+north_info.height+north_info.bevel_width+
          slider_info.bevel_width+2;
        slider_info.height=scroll_info.height-((slider_info.min_y-
          scroll_info.y+1) << 1)+4;
        visible_colors=scroll_info.height/(height+(height >> 3));
        if (colors > visible_colors)
          slider_info.height=(unsigned int)
            ((visible_colors*slider_info.height)/colors);
        slider_info.max_y=south_info.y-south_info.bevel_width-
          slider_info.bevel_width-2;
        slider_info.x=scroll_info.x+slider_info.bevel_width+1;
        slider_info.y=slider_info.min_y;
        expose_info=scroll_info;
        expose_info.y=slider_info.y;
        /*
          Initialize list information.
        */
        XGetWidgetInfo((char *) NULL,&list_info);
        list_info.raised=MagickFalse;
        list_info.bevel_width--;
        list_info.width=(unsigned int)
          (scroll_info.x-reply_info.x-(QuantumMargin >> 1));
        list_info.height=scroll_info.height;
        list_info.x=reply_info.x;
        list_info.y=scroll_info.y;
        if (windows->widget.mapped == MagickFalse)
          state|=JumpListState;
        /*
          Initialize text information.
        */
        *text='\0';
        XGetWidgetInfo(text,&text_info);
        text_info.center=MagickFalse;
        text_info.width=reply_info.width;
        text_info.height=height;
        text_info.x=list_info.x-(QuantumMargin >> 1);
        text_info.y=QuantumMargin;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int) ((9*height) >> 3);
        selection_info.x=list_info.x;
        state&=(~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Color Browser window.
        */
        x=QuantumMargin;
        y=text_info.y+((text_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,ColorPatternText,
          Extent(ColorPatternText));
        (void) CopyMagickString(text_info.text,glob_pattern,MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawBeveledButton(display,&windows->widget,&grab_info);
        XDrawBeveledButton(display,&windows->widget,&reset_info);
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        x=QuantumMargin;
        y=reply_info.y+((reply_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,ColornameText,
          Extent(ColornameText));
        XDrawBeveledMatte(display,&windows->widget,&reply_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledButton(display,&windows->widget,&action_info);
        XDrawBeveledButton(display,&windows->widget,&cancel_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawActionState;
        state|=RedrawListState;
        state&=(~RedrawWidgetState);
      }
    if (state & UpdateListState)
      {
        char
          **checklist;

        size_t
          number_colors;

        status=XParseColor(display,windows->widget.map_info->colormap,
          glob_pattern,&color);
        if ((status != False) || (strchr(glob_pattern,'-') != (char *) NULL))
          {
            /*
              Reply is a single color name-- exit.
            */
            (void) CopyMagickString(reply,glob_pattern,MagickPathExtent);
            (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        /*
          Update color list.
        */
        checklist=GetColorList(glob_pattern,&number_colors,exception);
        if (number_colors == 0)
          {
            (void) CopyMagickString(glob_pattern,reset_pattern,MagickPathExtent);
            (void) XBell(display,0);
          }
        else
          {
            for (i=0; i < (int) colors; i++)
              colorlist[i]=DestroyString(colorlist[i]);
            if (colorlist != (char **) NULL)
              colorlist=(char **) RelinquishMagickMemory(colorlist);
            colorlist=checklist;
            colors=number_colors;
          }
        /*
          Sort color list in ascending order.
        */
        slider_info.height=
          scroll_info.height-((slider_info.min_y-scroll_info.y+1) << 1)+1;
        if (colors > visible_colors)
          slider_info.height=(unsigned int)
            ((visible_colors*slider_info.height)/colors);
        slider_info.max_y=south_info.y-south_info.bevel_width-
          slider_info.bevel_width-2;
        slider_info.id=0;
        slider_info.y=slider_info.min_y;
        expose_info.y=slider_info.y;
        selection_info.id=(~0);
        list_info.id=(~0);
        state|=RedrawListState;
        /*
          Redraw color name & reply.
        */
        *reply_info.text='\0';
        reply_info.cursor=reply_info.text;
        (void) CopyMagickString(text_info.text,glob_pattern,MagickPathExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        state&=(~UpdateListState);
      }
    if (state & JumpListState)
      {
        /*
          Jump scroll to match user color.
        */
        list_info.id=(~0);
        for (i=0; i < (int) colors; i++)
          if (LocaleCompare(colorlist[i],reply) >= 0)
            {
              list_info.id=LocaleCompare(colorlist[i],reply) == 0 ? i : ~0;
              break;
            }
        if ((i < slider_info.id) ||
            (i >= (int) (slider_info.id+visible_colors)))
          slider_info.id=i-(visible_colors >> 1);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(~JumpListState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (colors-visible_colors))
          slider_info.id=(int) (colors-visible_colors);
        if ((slider_info.id < 0) || (colors <= visible_colors))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (colors != 0)
          slider_info.y+=((ssize_t) slider_info.id*(slider_info.max_y-
            slider_info.min_y+1)/colors);
        if (slider_info.id != selection_info.id)
          {
            /*
              Redraw scroll bar and file names.
            */
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(height >> 3)+2;
            for (i=0; i < (int) visible_colors; i++)
            {
              selection_info.raised=(slider_info.id+i) != list_info.id ?
                MagickTrue : MagickFalse;
              selection_info.text=(char *) NULL;
              if ((slider_info.id+i) < (int) colors)
                selection_info.text=colorlist[slider_info.id+i];
              XDrawWidgetText(display,&windows->widget,&selection_info);
              selection_info.y+=(int) selection_info.height;
            }
            /*
              Update slider.
            */
            if (slider_info.y > expose_info.y)
              {
                expose_info.height=(unsigned int) slider_info.y-expose_info.y;
                expose_info.y=slider_info.y-expose_info.height-
                  slider_info.bevel_width-1;
              }
            else
              {
                expose_info.height=(unsigned int) expose_info.y-slider_info.y;
                expose_info.y=slider_info.y+slider_info.height+
                  slider_info.bevel_width+1;
              }
            XDrawTriangleNorth(display,&windows->widget,&north_info);
            XDrawMatte(display,&windows->widget,&expose_info);
            XDrawBeveledButton(display,&windows->widget,&slider_info);
            XDrawTriangleSouth(display,&windows->widget,&south_info);
            expose_info.y=slider_info.y;
          }
        state&=(~RedrawListState);
      }
    if (state & RedrawActionState)
      {
        static char
          colorname[MagickPathExtent];

        /*
          Display the selected color in a drawing area.
        */
        color=windows->widget.pixel_info->matte_color;
        (void) XParseColor(display,windows->widget.map_info->colormap,
          reply_info.text,&windows->widget.pixel_info->matte_color);
        XBestPixel(display,windows->widget.map_info->colormap,(XColor *) NULL,
          (unsigned int) windows->widget.visual_info->colormap_size,
          &windows->widget.pixel_info->matte_color);
        mode_info.text=colorname;
        (void) FormatLocaleString(mode_info.text,MagickPathExtent,
          "#%02x%02x%02x",windows->widget.pixel_info->matte_color.red,
          windows->widget.pixel_info->matte_color.green,
          windows->widget.pixel_info->matte_color.blue);
        XDrawBeveledButton(display,&windows->widget,&mode_info);
        windows->widget.pixel_info->matte_color=color;
        state&=(~RedrawActionState);
      }
    /*
      Wait for next event.
    */
    if (north_info.raised && south_info.raised)
      (void) XIfEvent(display,&event,XScreenEvent,(char *) windows);
    else
      {
        /*
          Brief delay before advancing scroll bar.
        */
        XDelay(display,delay);
        delay=SuspendTime;
        (void) XCheckIfEvent(display,&event,XScreenEvent,(char *) windows);
        if (north_info.raised == MagickFalse)
          if (slider_info.id > 0)
            {
              /*
                Move slider up.
              */
              slider_info.id--;
              state|=RedrawListState;
            }
        if (south_info.raised == MagickFalse)
          if (slider_info.id < (int) colors)
            {
              /*
                Move slider down.
              */
              slider_info.id++;
              state|=RedrawListState;
            }
        if (event.type != ButtonRelease)
          continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (MatteIsActive(slider_info,event.xbutton))
          {
            /*
              Track slider.
            */
            slider_info.active=MagickTrue;
            break;
          }
        if (MatteIsActive(north_info,event.xbutton))
          if (slider_info.id > 0)
            {
              /*
                Move slider up.
              */
              north_info.raised=MagickFalse;
              slider_info.id--;
              state|=RedrawListState;
              break;
            }
        if (MatteIsActive(south_info,event.xbutton))
          if (slider_info.id < (int) colors)
            {
              /*
                Move slider down.
              */
              south_info.raised=MagickFalse;
              slider_info.id++;
              state|=RedrawListState;
              break;
            }
        if (MatteIsActive(scroll_info,event.xbutton))
          {
            /*
              Move slider.
            */
            if (event.xbutton.y < slider_info.y)
              slider_info.id-=(visible_colors-1);
            else
              slider_info.id+=(visible_colors-1);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            int
              id;

            /*
              User pressed list matte.
            */
            id=slider_info.id+(event.xbutton.y-(list_info.y+(height >> 1))+1)/
              selection_info.height;
            if (id >= (int) colors)
              break;
            (void) CopyMagickString(reply_info.text,colorlist[id],
              MagickPathExtent);
            reply_info.highlight=MagickFalse;
            reply_info.marker=reply_info.text;
            reply_info.cursor=reply_info.text+Extent(reply_info.text);
            XDrawMatteText(display,&windows->widget,&reply_info);
            state|=RedrawActionState;
            if (id == list_info.id)
              {
                (void) CopyMagickString(glob_pattern,reply_info.text,
                  MagickPathExtent);
                state|=UpdateListState;
              }
            selection_info.id=(~0);
            list_info.id=id;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(grab_info,event.xbutton))
          {
            /*
              User pressed Grab button.
            */
            grab_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&grab_info);
            break;
          }
        if (MatteIsActive(reset_info,event.xbutton))
          {
            /*
              User pressed Reset button.
            */
            reset_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
            break;
          }
        if (MatteIsActive(mode_info,event.xbutton))
          {
            /*
              User pressed mode button.
            */
            if (mode_info.text != (char *) NULL)
              (void) CopyMagickString(reply_info.text,mode_info.text,
                MagickPathExtent);
            (void) CopyMagickString(primary_selection,reply_info.text,
              MagickPathExtent);
            (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
              event.xbutton.time);
            reply_info.highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
              windows->widget.id ? MagickTrue : MagickFalse;
            reply_info.marker=reply_info.text;
            reply_info.cursor=reply_info.text+Extent(reply_info.text);
            XDrawMatteText(display,&windows->widget,&reply_info);
            break;
          }
        if (MatteIsActive(action_info,event.xbutton))
          {
            /*
              User pressed action button.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        if (event.xbutton.button != Button2)
          {
            static Time
              click_time;

            /*
              Move text cursor to position of button press.
            */
            x=event.xbutton.x-reply_info.x-(QuantumMargin >> 2);
            for (i=1; i <= Extent(reply_info.marker); i++)
              if (XTextWidth(font_info,reply_info.marker,i) > x)
                break;
            reply_info.cursor=reply_info.marker+i-1;
            if (event.xbutton.time > (click_time+DoubleClick))
              reply_info.highlight=MagickFalse;
            else
              {
                /*
                  Become the XA_PRIMARY selection owner.
                */
                (void) CopyMagickString(primary_selection,reply_info.text,
                  MagickPathExtent);
                (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
                  event.xbutton.time);
                reply_info.highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
                  windows->widget.id ? MagickTrue : MagickFalse;
              }
            XDrawMatteText(display,&windows->widget,&reply_info);
            click_time=event.xbutton.time;
            break;
          }
        /*
          Request primary selection.
        */
        (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
          windows->widget.id,event.xbutton.time);
        break;
      }
      case ButtonRelease:
      {
        if (windows->widget.mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,&windows->widget,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,&windows->widget,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        if (grab_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(grab_info,event.xbutton))
                {
                  /*
                    Select a fill color from the X server.
                  */
                  (void) XGetWindowColor(display,windows,reply_info.text,
                    exception);
                  reply_info.marker=reply_info.text;
                  reply_info.cursor=reply_info.text+Extent(reply_info.text);
                  XDrawMatteText(display,&windows->widget,&reply_info);
                  state|=RedrawActionState;
                }
            grab_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&grab_info);
          }
        if (reset_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(reset_info,event.xbutton))
                {
                  (void) CopyMagickString(glob_pattern,reset_pattern,
                    MagickPathExtent);
                  state|=UpdateListState;
                }
            reset_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
          }
        if (action_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              {
                if (MatteIsActive(action_info,event.xbutton))
                  {
                    if (*reply_info.text == '\0')
                      (void) XBell(display,0);
                    else
                      state|=ExitState;
                  }
              }
            action_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&action_info);
          }
        if (cancel_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(cancel_info,event.xbutton))
                {
                  *reply_info.text='\0';
                  state|=ExitState;
                }
            cancel_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
          }
        if (MatteIsActive(reply_info,event.xbutton) == MagickFalse)
          break;
        break;
      }
      case ClientMessage:
      {
        /*
          If client window delete message, exit.
        */
        if (event.xclient.message_type != windows->wm_protocols)
          break;
        if (*event.xclient.data.l == (int) windows->wm_take_focus)
          {
            (void) XSetInputFocus(display,event.xclient.window,RevertToParent,
              (Time) event.xclient.data.l[1]);
            break;
          }
        if (*event.xclient.data.l != (int) windows->wm_delete_window)
          break;
        if (event.xclient.window == windows->widget.id)
          {
            *reply_info.text='\0';
            state|=ExitState;
            break;
          }
        break;
      }
      case ConfigureNotify:
      {
        /*
          Update widget configuration.
        */
        if (event.xconfigure.window != windows->widget.id)
          break;
        if ((event.xconfigure.width == (int) windows->widget.width) &&
            (event.xconfigure.height == (int) windows->widget.height))
          break;
        windows->widget.width=(unsigned int)
          MagickMax(event.xconfigure.width,(int) windows->widget.min_width);
        windows->widget.height=(unsigned int)
          MagickMax(event.xconfigure.height,(int) windows->widget.min_height);
        state|=UpdateConfigurationState;
        break;
      }
      case EnterNotify:
      {
        if (event.xcrossing.window != windows->widget.id)
          break;
        state&=(~InactiveWidgetState);
        break;
      }
      case Expose:
      {
        if (event.xexpose.window != windows->widget.id)
          break;
        if (event.xexpose.count != 0)
          break;
        state|=RedrawWidgetState;
        break;
      }
      case KeyPress:
      {
        static char
          command[MagickPathExtent];

        static int
          length;

        static KeySym
          key_symbol;

        /*
          Respond to a user key press.
        */
        if (event.xkey.window != windows->widget.id)
          break;
        length=XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if (AreaIsActive(scroll_info,event.xkey))
          {
            /*
              Move slider.
            */
            switch ((int) key_symbol)
            {
              case XK_Home:
              case XK_KP_Home:
              {
                slider_info.id=0;
                break;
              }
              case XK_Up:
              case XK_KP_Up:
              {
                slider_info.id--;
                break;
              }
              case XK_Down:
              case XK_KP_Down:
              {
                slider_info.id++;
                break;
              }
              case XK_Prior:
              case XK_KP_Prior:
              {
                slider_info.id-=visible_colors;
                break;
              }
              case XK_Next:
              case XK_KP_Next:
              {
                slider_info.id+=visible_colors;
                break;
              }
              case XK_End:
              case XK_KP_End:
              {
                slider_info.id=(int) colors;
                break;
              }
            }
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            /*
              Read new color or glob patterm.
            */
            if (*reply_info.text == '\0')
              break;
            (void) CopyMagickString(glob_pattern,reply_info.text,MagickPathExtent);
            state|=UpdateListState;
            break;
          }
        if (key_symbol == XK_Control_L)
          {
            state|=ControlState;
            break;
          }
        if (state & ControlState)
          switch ((int) key_symbol)
          {
            case XK_u:
            case XK_U:
            {
              /*
                Erase the entire line of text.
              */
              *reply_info.text='\0';
              reply_info.cursor=reply_info.text;
              reply_info.marker=reply_info.text;
              reply_info.highlight=MagickFalse;
              break;
            }
            default:
              break;
          }
        XEditText(display,&reply_info,key_symbol,command,state);
        XDrawMatteText(display,&windows->widget,&reply_info);
        state|=JumpListState;
        status=XParseColor(display,windows->widget.map_info->colormap,
          reply_info.text,&color);
        if (status != False)
          state|=RedrawActionState;
        break;
      }
      case KeyRelease:
      {
        static char
          command[MagickPathExtent];

        static KeySym
          key_symbol;

        /*
          Respond to a user key release.
        */
        if (event.xkey.window != windows->widget.id)
          break;
        (void) XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if (key_symbol == XK_Control_L)
          state&=(~ControlState);
        break;
      }
      case LeaveNotify:
      {
        if (event.xcrossing.window != windows->widget.id)
          break;
        state|=InactiveWidgetState;
        break;
      }
      case MapNotify:
      {
        mask&=(~CWX);
        mask&=(~CWY);
        break;
      }
      case MotionNotify:
      {
        /*
          Discard pending button motion events.
        */
        while (XCheckMaskEvent(display,ButtonMotionMask,&event)) ;
        if (slider_info.active)
          {
            /*
              Move slider matte.
            */
            slider_info.y=event.xmotion.y-
              ((slider_info.height+slider_info.bevel_width) >> 1)+1;
            if (slider_info.y < slider_info.min_y)
              slider_info.y=slider_info.min_y;
            if (slider_info.y > slider_info.max_y)
              slider_info.y=slider_info.max_y;
            slider_info.id=0;
            if (slider_info.y != slider_info.min_y)
              slider_info.id=(int) ((colors*(slider_info.y-
                slider_info.min_y+1))/(slider_info.max_y-slider_info.min_y+1));
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (grab_info.raised == MatteIsActive(grab_info,event.xmotion))
          {
            /*
              Grab button status changed.
            */
            grab_info.raised=!grab_info.raised;
            XDrawBeveledButton(display,&windows->widget,&grab_info);
            break;
          }
        if (reset_info.raised == MatteIsActive(reset_info,event.xmotion))
          {
            /*
              Reset button status changed.
            */
            reset_info.raised=!reset_info.raised;
            XDrawBeveledButton(display,&windows->widget,&reset_info);
            break;
          }
        if (action_info.raised == MatteIsActive(action_info,event.xmotion))
          {
            /*
              Action button status changed.
            */
            action_info.raised=action_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        if (cancel_info.raised == MatteIsActive(cancel_info,event.xmotion))
          {
            /*
              Cancel button status changed.
            */
            cancel_info.raised=cancel_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        reply_info.highlight=MagickFalse;
        XDrawMatteText(display,&windows->widget,&reply_info);
        break;
      }
      case SelectionNotify:
      {
        Atom
          type;

        int
          format;

        unsigned char
          *data;

        unsigned long
          after,
          length;

        /*
          Obtain response from primary selection.
        */
        if (event.xselection.property == (Atom) None)
          break;
        status=XGetWindowProperty(display,event.xselection.requestor,
          event.xselection.property,0L,2047L,MagickTrue,XA_STRING,&type,
          &format,&length,&after,&data);
        if ((status != Success) || (type != XA_STRING) || (format == 32) ||
            (length == 0))
          break;
        if ((Extent(reply_info.text)+length) >= (MagickPathExtent-1))
          (void) XBell(display,0);
        else
          {
            /*
              Insert primary selection in reply text.
            */
            *(data+length)='\0';
            XEditText(display,&reply_info,(KeySym) XK_Insert,(char *) data,
              state);
            XDrawMatteText(display,&windows->widget,&reply_info);
            state|=JumpListState;
            state|=RedrawActionState;
          }
        (void) XFree((void *) data);
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        if (reply_info.highlight == MagickFalse)
          break;
        /*
          Set primary selection.
        */
        request=(&(event.xselectionrequest));
        (void) XChangeProperty(request->display,request->requestor,
          request->property,request->target,8,PropModeReplace,
          (unsigned char *) primary_selection,Extent(primary_selection));
        notify.type=SelectionNotify;
        notify.send_event=MagickTrue;
        notify.display=request->display;
        notify.requestor=request->requestor;
        notify.selection=request->selection;
        notify.target=request->target;
        notify.time=request->time;
        if (request->property == None)
          notify.property=request->target;
        else
          notify.property=request->property;
        (void) XSendEvent(request->display,request->requestor,False,
          NoEventMask,(XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  /*
    Free color list.
  */
  for (i=0; i < (int) colors; i++)
    colorlist[i]=DestroyString(colorlist[i]);
  if (colorlist != (char **) NULL)
    colorlist=(char **) RelinquishMagickMemory(colorlist);
  exception=DestroyExceptionInfo(exception);
  if ((*reply == '\0') || (strchr(reply,'-') != (char *) NULL))
    return;
  status=XParseColor(display,windows->widget.map_info->colormap,reply,&color);
  if (status != False)
    return;
  XNoticeWidget(display,windows,"Color is unknown to X server:",reply);
  (void) CopyMagickString(reply,"gray",MagickPathExtent);
}