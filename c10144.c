MagickPrivate void XTextViewWidget(Display *display,
  const XResourceInfo *resource_info,XWindows *windows,
  const MagickBooleanType mono,const char *title,const char **textlist)
{
#define DismissButtonText  "Dismiss"

  char
    primary_selection[MagickPathExtent];

  int
    i;

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    height,
    lines,
    text_width,
    visible_lines,
    width;

  size_t
    delay,
    state;

  XEvent
    event;

  XFontStruct
    *font_info,
    *text_info;

  XTextProperty
    window_name;

  XWidgetInfo
    dismiss_info,
    expose_info,
    list_info,
    north_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info;

  XWindowChanges
    window_changes;

  /*
    Convert text string to a text list.
  */
  assert(display != (Display *) NULL);
  assert(resource_info != (XResourceInfo *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(title != (const char *) NULL);
  assert(textlist != (const char **) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",title);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (textlist == (const char **) NULL)
    {
      XNoticeWidget(display,windows,"No text to view:",(char *) NULL);
      return;
    }
  /*
    Determine Text View widget attributes.
  */
  font_info=windows->widget.font_info;
  text_info=(XFontStruct *) NULL;
  if (mono != MagickFalse)
    text_info=XBestFont(display,resource_info,MagickTrue);
  if (text_info == (XFontStruct *) NULL)
    text_info=windows->widget.font_info;
  text_width=0;
  for (i=0; textlist[i] != (char *) NULL; i++)
    if (WidgetTextWidth(text_info,(char *) textlist[i]) > text_width)
      text_width=(unsigned int) XTextWidth(text_info,(char *) textlist[i],
        MagickMin(Extent(textlist[i]),160));
  lines=(unsigned int) i;
  width=WidgetTextWidth(font_info,DismissButtonText);
  width+=QuantumMargin;
  height=(unsigned int) (text_info->ascent+text_info->descent);
  /*
    Position Text View widget.
  */
  windows->widget.width=(unsigned int) (MagickMin((int) text_width,
    (int) MaxTextWidth)+5*QuantumMargin);
  windows->widget.min_width=(unsigned int) (MinTextWidth+4*QuantumMargin);
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int) (MagickMin(MagickMax((int) lines,3),32)*
    height+((13*height) >> 1)+((9*QuantumMargin) >> 1));
  windows->widget.min_height=(unsigned int) (3*height+((13*height) >> 1)+((9*
    QuantumMargin) >> 1));
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map Text View widget.
  */
  (void) CopyMagickString(windows->widget.name,title,MagickPathExtent);
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
  (void) XReconfigureWMWindow(display,windows->widget.id,
    windows->widget.screen,(unsigned int) mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  XGetWidgetInfo((char *) NULL,&selection_info);
  visible_lines=0;
  delay=SuspendTime << 2;
  height=(unsigned int) (font_info->ascent+font_info->descent);
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
        XGetWidgetInfo(DismissButtonText,&dismiss_info);
        dismiss_info.width=width;
        dismiss_info.height=(unsigned int) ((3*height) >> 1);
        dismiss_info.x=(int) windows->widget.width-dismiss_info.width-
          QuantumMargin-2;
        dismiss_info.y=(int) windows->widget.height-dismiss_info.height-
          QuantumMargin;
        /*
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int) (dismiss_info.y-((5*QuantumMargin) >>
          1));
        scroll_info.x=(int) windows->widget.width-QuantumMargin-
          scroll_info.width;
        scroll_info.y=(3*QuantumMargin) >> 1;
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
        visible_lines=scroll_info.height/(text_info->ascent+text_info->descent+
          ((text_info->ascent+text_info->descent) >> 3));
        if (lines > visible_lines)
          slider_info.height=(unsigned int) (visible_lines*slider_info.height)/
            lines;
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
        list_info.width=(unsigned int) scroll_info.x-((3*QuantumMargin) >> 1);
        list_info.height=scroll_info.height;
        list_info.x=QuantumMargin;
        list_info.y=scroll_info.y;
        /*
          Initialize selection information.
        */
        XGetWidgetInfo((char *) NULL,&selection_info);
        selection_info.center=MagickFalse;
        selection_info.width=list_info.width;
        selection_info.height=(unsigned int)
          (9*(text_info->ascent+text_info->descent)) >> 3;
        selection_info.x=list_info.x;
        state&=(~UpdateConfigurationState);
      }
    if (state & RedrawWidgetState)
      {
        /*
          Redraw Text View window.
        */
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        XDrawBeveledButton(display,&windows->widget,&dismiss_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(~RedrawWidgetState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (lines-visible_lines))
          slider_info.id=(int) lines-visible_lines;
        if ((slider_info.id < 0) || (lines <= visible_lines))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (lines != 0)
          slider_info.y+=
            slider_info.id*(slider_info.max_y-slider_info.min_y+1)/lines;
        if (slider_info.id != selection_info.id)
          {
            /*
              Redraw scroll bar and text.
            */
            windows->widget.font_info=text_info;
            (void) XSetFont(display,windows->widget.annotate_context,
              text_info->fid);
            (void) XSetFont(display,windows->widget.highlight_context,
              text_info->fid);
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(height >> 3)+2;
            for (i=0; i < (int) visible_lines; i++)
            {
              selection_info.raised=
                (slider_info.id+i) != list_info.id ? MagickTrue : MagickFalse;
              selection_info.text=(char *) NULL;
              if ((slider_info.id+i) < (int) lines)
                selection_info.text=(char *) textlist[slider_info.id+i];
              XDrawWidgetText(display,&windows->widget,&selection_info);
              selection_info.y+=(int) selection_info.height;
            }
            windows->widget.font_info=font_info;
            (void) XSetFont(display,windows->widget.annotate_context,
              font_info->fid);
            (void) XSetFont(display,windows->widget.highlight_context,
              font_info->fid);
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
          if (slider_info.id < (int) lines)
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
          if (slider_info.id < (int) lines)
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
              slider_info.id-=(visible_lines-1);
            else
              slider_info.id+=(visible_lines-1);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(dismiss_info,event.xbutton))
          {
            /*
              User pressed Dismiss button.
            */
            dismiss_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            int
              id;

            static Time
              click_time;

            /*
              User pressed list matte.
            */
            id=slider_info.id+(event.xbutton.y-(list_info.y+(height >> 1))+1)/
              selection_info.height;
            if (id >= (int) lines)
              break;
            if (id != list_info.id)
              {
                list_info.id=id;
                click_time=event.xbutton.time;
                break;
              }
            list_info.id=id;
            if (event.xbutton.time >= (click_time+DoubleClick))
              {
                click_time=event.xbutton.time;
                break;
              }
            click_time=event.xbutton.time;
            /*
              Become the XA_PRIMARY selection owner.
            */
            (void) CopyMagickString(primary_selection,textlist[list_info.id],
              MagickPathExtent);
            (void) XSetSelectionOwner(display,XA_PRIMARY,windows->widget.id,
              event.xbutton.time);
            if (XGetSelectionOwner(display,XA_PRIMARY) != windows->widget.id)
              break;
            selection_info.id=(~0);
            list_info.id=id;
            state|=RedrawListState;
            break;
          }
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
        if (dismiss_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(dismiss_info,event.xbutton))
                state|=ExitState;
            dismiss_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
          }
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
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            dismiss_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
            state|=ExitState;
            break;
          }
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
                slider_info.id-=visible_lines;
                break;
              }
              case XK_Next:
              case XK_KP_Next:
              {
                slider_info.id+=visible_lines;
                break;
              }
              case XK_End:
              case XK_KP_End:
              {
                slider_info.id=(int) lines;
                break;
              }
            }
            state|=RedrawListState;
            break;
          }
        break;
      }
      case KeyRelease:
        break;
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
              slider_info.id=(int) (lines*(slider_info.y-slider_info.min_y+1))/
                (slider_info.max_y-slider_info.min_y+1);
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (dismiss_info.raised == MatteIsActive(dismiss_info,event.xmotion))
          {
            /*
              Dismiss button status changed.
            */
            dismiss_info.raised=
              dismiss_info.raised == MagickFalse ? MagickTrue : MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&dismiss_info);
            break;
          }
        break;
      }
      case SelectionClear:
      {
        list_info.id=(~0);
        selection_info.id=(~0);
        state|=RedrawListState;
        break;
      }
      case SelectionRequest:
      {
        XSelectionEvent
          notify;

        XSelectionRequestEvent
          *request;

        if (list_info.id == (~0))
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
  if (text_info != windows->widget.font_info)
    (void) XFreeFont(display,text_info);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
}