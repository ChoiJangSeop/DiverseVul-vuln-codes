MagickExport void XListBrowserWidget(Display *display,XWindows *windows,
  XWindowInfo *window_info,const char *const *list,const char *action,
  const char *query,char *reply)
{
#define CancelButtonText  "Cancel"

  char
    primary_selection[MaxTextExtent];

  int
    x;

  int
    i;

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    entries,
    height,
    text_width,
    visible_entries,
    width;

  size_t
    delay,
    state;

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
    list_info,
    north_info,
    reply_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info;

  XWindowChanges
    window_changes;

  /*
    Count the number of entries in the list.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(window_info != (XWindowInfo *) NULL);
  assert(list != (const char **) NULL);
  assert(action != (char *) NULL);
  assert(query != (char *) NULL);
  assert(reply != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (list == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to browse:",(char *) NULL);
      return;
    }
  for (entries=0; ; entries++)
    if (list[entries] == (char *) NULL)
      break;
  /*
    Determine Font Browser widget attributes.
  */
  font_info=window_info->font_info;
  text_width=WidgetTextWidth(font_info,(char *) query);
  for (i=0; i < (int) entries; i++)
    if (WidgetTextWidth(font_info,(char *) list[i]) > text_width)
      text_width=WidgetTextWidth(font_info,(char *) list[i]);
  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  width+=QuantumMargin;
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position List Browser widget.
  */
  window_info->width=(unsigned int) MagickMin((int) text_width,(int)
    MaxTextWidth)+((9*QuantumMargin) >> 1);
  window_info->min_width=(unsigned int) (MinTextWidth+4*QuantumMargin);
  if (window_info->width < window_info->min_width)
    window_info->width=window_info->min_width;
  window_info->height=(unsigned int)
    (((81*height) >> 2)+((13*QuantumMargin) >> 1)+4);
  window_info->min_height=(unsigned int)
    (((23*height) >> 1)+((13*QuantumMargin) >> 1)+4);
  if (window_info->height < window_info->min_height)
    window_info->height=window_info->min_height;
  XConstrainWindowPosition(display,window_info);
  /*
    Map List Browser widget.
  */
  (void) CopyMagickString(window_info->name,"Browse",MaxTextExtent);
  status=XStringListToTextProperty(&window_info->name,1,&window_name);
  if (status != False)
    {
      XSetWMName(display,window_info->id,&window_name);
      XSetWMIconName(display,windows->widget.id,&window_name);
      (void) XFree((void *) window_name.value);
    }
  window_changes.width=(int) window_info->width;
  window_changes.height=(int) window_info->height;
  window_changes.x=window_info->x;
  window_changes.y=window_info->y;
  (void) XReconfigureWMWindow(display,window_info->id,window_info->screen,mask,
    &window_changes);
  (void) XMapRaised(display,window_info->id);
  window_info->mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_entries=0;
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
          (window_info->width-cancel_info.width-QuantumMargin-2);
        cancel_info.y=(int)
          (window_info->height-cancel_info.height-QuantumMargin);
        XGetWidgetInfo(action,&action_info);
        action_info.width=width;
        action_info.height=(unsigned int) ((3*height) >> 1);
        action_info.x=cancel_info.x-(cancel_info.width+(QuantumMargin >> 1)+
          (action_info.bevel_width << 1));
        action_info.y=cancel_info.y;
        /*
          Initialize reply information.
        */
        XGetWidgetInfo(reply,&reply_info);
        reply_info.raised=MagickFalse;
        reply_info.bevel_width--;
        reply_info.width=window_info->width-((4*QuantumMargin) >> 1);
        reply_info.height=height << 1;
        reply_info.x=QuantumMargin;
        reply_info.y=action_info.y-reply_info.height-QuantumMargin;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int)
          (reply_info.y-((6*QuantumMargin) >> 1)-height);
        scroll_info.x=reply_info.x+(reply_info.width-scroll_info.width);
        scroll_info.y=((5*QuantumMargin) >> 1)+height-reply_info.bevel_width;
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
        visible_entries=scroll_info.height/(height+(height >> 3));
        if (entries > visible_entries)
          slider_info.height=(visible_entries*slider_info.height)/entries;
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
        if (window_info->mapped == MagickFalse)
          for (i=0; i < (int) entries; i++)
            if (LocaleCompare(list[i],reply) == 0)
              {
                list_info.id=i;
                slider_info.id=i-(visible_entries >> 1);
                if (slider_info.id < 0)
                  slider_info.id=0;
              }
        /*
          Initialize text information.
        */
        XGetWidgetInfo(query,&text_info);
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
          Redraw List Browser window.
        */
        XDrawWidgetText(display,window_info,&text_info);
        XDrawBeveledMatte(display,window_info,&list_info);
        XDrawBeveledMatte(display,window_info,&scroll_info);
        XDrawTriangleNorth(display,window_info,&north_info);
        XDrawBeveledButton(display,window_info,&slider_info);
        XDrawTriangleSouth(display,window_info,&south_info);
        XDrawBeveledMatte(display,window_info,&reply_info);
        XDrawMatteText(display,window_info,&reply_info);
        XDrawBeveledButton(display,window_info,&action_info);
        XDrawBeveledButton(display,window_info,&cancel_info);
        XHighlightWidget(display,window_info,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawActionState;
        state|=RedrawListState;
        state&=(~RedrawWidgetState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (entries-visible_entries))
          slider_info.id=(int) (entries-visible_entries);
        if ((slider_info.id < 0) || (entries <= visible_entries))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (entries > 0)
          slider_info.y+=
            slider_info.id*(slider_info.max_y-slider_info.min_y+1)/entries;
        if (slider_info.id != selection_info.id)
          {
            /*
              Redraw scroll bar and file names.
            */
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(height >> 3)+2;
            for (i=0; i < (int) visible_entries; i++)
            {
              selection_info.raised=(slider_info.id+i) != list_info.id ?
                MagickTrue : MagickFalse;
              selection_info.text=(char *) NULL;
              if ((slider_info.id+i) < (int) entries)
                selection_info.text=(char *) list[slider_info.id+i];
              XDrawWidgetText(display,window_info,&selection_info);
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
            XDrawTriangleNorth(display,window_info,&north_info);
            XDrawMatte(display,window_info,&expose_info);
            XDrawBeveledButton(display,window_info,&slider_info);
            XDrawTriangleSouth(display,window_info,&south_info);
            expose_info.y=slider_info.y;
          }
        state&=(~RedrawListState);
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
          if (slider_info.id < (int) entries)
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
          if (slider_info.id < (int) entries)
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
              slider_info.id-=(visible_entries-1);
            else
              slider_info.id+=(visible_entries-1);
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
            if (id >= (int) entries)
              break;
            (void) CopyMagickString(reply_info.text,list[id],MaxTextExtent);
            reply_info.highlight=MagickFalse;
            reply_info.marker=reply_info.text;
            reply_info.cursor=reply_info.text+Extent(reply_info.text);
            XDrawMatteText(display,window_info,&reply_info);
            selection_info.id=(~0);
            if (id == list_info.id)
              {
                action_info.raised=MagickFalse;
                XDrawBeveledButton(display,window_info,&action_info);
                state|=ExitState;
              }
            list_info.id=id;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(action_info,event.xbutton))
          {
            /*
              User pressed action button.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,window_info,&action_info);
            break;
          }
        if (MatteIsActive(cancel_info,event.xbutton))
          {
            /*
              User pressed Cancel button.
            */
            cancel_info.raised=MagickFalse;
            XDrawBeveledButton(display,window_info,&cancel_info);
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
                  MaxTextExtent);
                (void) XSetSelectionOwner(display,XA_PRIMARY,window_info->id,
                  event.xbutton.time);
                reply_info.highlight=XGetSelectionOwner(display,XA_PRIMARY) ==
                  window_info->id ? MagickTrue : MagickFalse;
              }
            XDrawMatteText(display,window_info,&reply_info);
            click_time=event.xbutton.time;
            break;
          }
        /*
          Request primary selection.
        */
        (void) XConvertSelection(display,XA_PRIMARY,XA_STRING,XA_STRING,
          window_info->id,event.xbutton.time);
        break;
      }
      case ButtonRelease:
      {
        if (window_info->mapped == MagickFalse)
          break;
        if (north_info.raised == MagickFalse)
          {
            /*
              User released up button.
            */
            delay=SuspendTime << 2;
            north_info.raised=MagickTrue;
            XDrawTriangleNorth(display,window_info,&north_info);
          }
        if (south_info.raised == MagickFalse)
          {
            /*
              User released down button.
            */
            delay=SuspendTime << 2;
            south_info.raised=MagickTrue;
            XDrawTriangleSouth(display,window_info,&south_info);
          }
        if (slider_info.active)
          {
            /*
              Stop tracking slider.
            */
            slider_info.active=MagickFalse;
            break;
          }
        if (action_info.raised == MagickFalse)
          {
            if (event.xbutton.window == window_info->id)
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
            XDrawBeveledButton(display,window_info,&action_info);
          }
        if (cancel_info.raised == MagickFalse)
          {
            if (event.xbutton.window == window_info->id)
              if (MatteIsActive(cancel_info,event.xbutton))
                {
                  *reply_info.text='\0';
                  state|=ExitState;
                }
            cancel_info.raised=MagickTrue;
            XDrawBeveledButton(display,window_info,&cancel_info);
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
        if (event.xclient.window == window_info->id)
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
        if (event.xconfigure.window != window_info->id)
          break;
        if ((event.xconfigure.width == (int) window_info->width) &&
            (event.xconfigure.height == (int) window_info->height))
          break;
        window_info->width=(unsigned int)
          MagickMax(event.xconfigure.width,(int) window_info->min_width);
        window_info->height=(unsigned int)
          MagickMax(event.xconfigure.height,(int) window_info->min_height);
        state|=UpdateConfigurationState;
        break;
      }
      case EnterNotify:
      {
        if (event.xcrossing.window != window_info->id)
          break;
        state&=(~InactiveWidgetState);
        break;
      }
      case Expose:
      {
        if (event.xexpose.window != window_info->id)
          break;
        if (event.xexpose.count != 0)
          break;
        state|=RedrawWidgetState;
        break;
      }
      case KeyPress:
      {
        static char
          command[MaxTextExtent];

        static int
          length;

        static KeySym
          key_symbol;

        /*
          Respond to a user key press.
        */
        if (event.xkey.window != window_info->id)
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
                slider_info.id-=visible_entries;
                break;
              }
              case XK_Next:
              case XK_KP_Next:
              {
                slider_info.id+=visible_entries;
                break;
              }
              case XK_End:
              case XK_KP_End:
              {
                slider_info.id=(int) entries;
                break;
              }
            }
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            /*
              Read new entry.
            */
            if (*reply_info.text == '\0')
              break;
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,window_info,&action_info);
            state|=ExitState;
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
        XDrawMatteText(display,window_info,&reply_info);
        break;
      }
      case KeyRelease:
      {
        static char
          command[MaxTextExtent];

        static KeySym
          key_symbol;

        /*
          Respond to a user key release.
        */
        if (event.xkey.window != window_info->id)
          break;
        (void) XLookupString((XKeyEvent *) &event.xkey,command,
          (int) sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if (key_symbol == XK_Control_L)
          state&=(~ControlState);
        break;
      }
      case LeaveNotify:
      {
        if (event.xcrossing.window != window_info->id)
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
              slider_info.id=(int) ((entries*(slider_info.y-
                slider_info.min_y+1))/(slider_info.max_y-slider_info.min_y+1));
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (action_info.raised == MatteIsActive(action_info,event.xmotion))
          {
            /*
              Action button status changed.
            */
            action_info.raised=action_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,window_info,&action_info);
            break;
          }
        if (cancel_info.raised == MatteIsActive(cancel_info,event.xmotion))
          {
            /*
              Cancel button status changed.
            */
            cancel_info.raised=cancel_info.raised == MagickFalse ?
              MagickTrue : MagickFalse;
            XDrawBeveledButton(display,window_info,&cancel_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        reply_info.highlight=MagickFalse;
        XDrawMatteText(display,window_info,&reply_info);
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
        status=XGetWindowProperty(display,
          event.xselection.requestor,event.xselection.property,0L,2047L,
          MagickTrue,XA_STRING,&type,&format,&length,&after,&data);
        if ((status != Success) || (type != XA_STRING) || (format == 32) ||
            (length == 0))
          break;
        if ((Extent(reply_info.text)+length) >= (MaxTextExtent-1))
          (void) XBell(display,0);
        else
          {
            /*
              Insert primary selection in reply text.
            */
            *(data+length)='\0';
            XEditText(display,&reply_info,(KeySym) XK_Insert,(char *) data,
              state);
            XDrawMatteText(display,window_info,&reply_info);
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
        (void) XSendEvent(request->display,request->requestor,False,NoEventMask,
          (XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,window_info->id,window_info->screen);
  XCheckRefreshWindows(display,windows);
}