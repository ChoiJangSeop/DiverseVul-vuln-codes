MagickExport void XFileBrowserWidget(Display *display,XWindows *windows,
  const char *action,char *reply)
{
#define CancelButtonText  "Cancel"
#define DirectoryText  "Directory:"
#define FilenameText  "File name:"
#define GrabButtonText  "Grab"
#define FormatButtonText  "Format"
#define HomeButtonText  "Home"
#define UpButtonText  "Up"

  char
    *directory,
    **filelist,
    home_directory[MaxTextExtent],
    primary_selection[MaxTextExtent],
    text[MaxTextExtent],
    working_path[MaxTextExtent];

  int
    x,
    y;

  ssize_t
    i;

  static char
    glob_pattern[MaxTextExtent] = "*",
    format[MaxTextExtent] = "miff";

  static MagickStatusType
    mask = (MagickStatusType) (CWWidth | CWHeight | CWX | CWY);

  Status
    status;

  unsigned int
    anomaly,
    height,
    text_width,
    visible_files,
    width;

  size_t
    delay,
    files,
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
    special_info,
    list_info,
    home_info,
    north_info,
    reply_info,
    scroll_info,
    selection_info,
    slider_info,
    south_info,
    text_info,
    up_info;

  XWindowChanges
    window_changes;

  /*
    Read filelist from current directory.
  */
  assert(display != (Display *) NULL);
  assert(windows != (XWindows *) NULL);
  assert(action != (char *) NULL);
  assert(reply != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",action);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  directory=getcwd(home_directory,MaxTextExtent);
  (void) directory;
  (void) CopyMagickString(working_path,home_directory,MaxTextExtent);
  filelist=ListFiles(working_path,glob_pattern,&files);
  if (filelist == (char **) NULL)
    {
      /*
        Directory read failed.
      */
      XNoticeWidget(display,windows,"Unable to read directory:",working_path);
      (void) XDialogWidget(display,windows,action,"Enter filename:",reply);
      return;
    }
  /*
    Determine File Browser widget attributes.
  */
  font_info=windows->widget.font_info;
  text_width=0;
  for (i=0; i < (ssize_t) files; i++)
    if (WidgetTextWidth(font_info,filelist[i]) > text_width)
      text_width=WidgetTextWidth(font_info,filelist[i]);
  width=WidgetTextWidth(font_info,(char *) action);
  if (WidgetTextWidth(font_info,GrabButtonText) > width)
    width=WidgetTextWidth(font_info,GrabButtonText);
  if (WidgetTextWidth(font_info,FormatButtonText) > width)
    width=WidgetTextWidth(font_info,FormatButtonText);
  if (WidgetTextWidth(font_info,CancelButtonText) > width)
    width=WidgetTextWidth(font_info,CancelButtonText);
  if (WidgetTextWidth(font_info,HomeButtonText) > width)
    width=WidgetTextWidth(font_info,HomeButtonText);
  if (WidgetTextWidth(font_info,UpButtonText) > width)
    width=WidgetTextWidth(font_info,UpButtonText);
  width+=QuantumMargin;
  if (WidgetTextWidth(font_info,DirectoryText) > width)
    width=WidgetTextWidth(font_info,DirectoryText);
  if (WidgetTextWidth(font_info,FilenameText) > width)
    width=WidgetTextWidth(font_info,FilenameText);
  height=(unsigned int) (font_info->ascent+font_info->descent);
  /*
    Position File Browser widget.
  */
  windows->widget.width=width+MagickMin((int) text_width,(int) MaxTextWidth)+
    6*QuantumMargin;
  windows->widget.min_width=width+MinTextWidth+4*QuantumMargin;
  if (windows->widget.width < windows->widget.min_width)
    windows->widget.width=windows->widget.min_width;
  windows->widget.height=(unsigned int)
    (((81*height) >> 2)+((13*QuantumMargin) >> 1)+4);
  windows->widget.min_height=(unsigned int)
    (((23*height) >> 1)+((13*QuantumMargin) >> 1)+4);
  if (windows->widget.height < windows->widget.min_height)
    windows->widget.height=windows->widget.min_height;
  XConstrainWindowPosition(display,&windows->widget);
  /*
    Map File Browser widget.
  */
  (void) CopyMagickString(windows->widget.name,"Browse and Select a File",
    MaxTextExtent);
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
    windows->widget.screen,mask,&window_changes);
  (void) XMapRaised(display,windows->widget.id);
  windows->widget.mapped=MagickFalse;
  /*
    Respond to X events.
  */
  XGetWidgetInfo((char *) NULL,&slider_info);
  XGetWidgetInfo((char *) NULL,&north_info);
  XGetWidgetInfo((char *) NULL,&south_info);
  XGetWidgetInfo((char *) NULL,&expose_info);
  visible_files=0;
  anomaly=(LocaleCompare(action,"Composite") == 0) ||
    (LocaleCompare(action,"Open") == 0) || (LocaleCompare(action,"Map") == 0);
  *reply='\0';
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
        XGetWidgetInfo(GrabButtonText,&special_info);
        special_info.width=width;
        special_info.height=(unsigned int) ((3*height) >> 1);
        special_info.x=action_info.x-(action_info.width+(QuantumMargin >> 1)+
          (special_info.bevel_width << 1));
        special_info.y=action_info.y;
        if (anomaly == MagickFalse)
          {
            char
              *p;

            special_info.text=(char *) FormatButtonText;
            p=reply+Extent(reply)-1;
            while ((p > (reply+1)) && (*(p-1) != '.'))
              p--;
            if ((p > (reply+1)) && (*(p-1) == '.'))
              (void) CopyMagickString(format,p,MaxTextExtent);
          }
        XGetWidgetInfo(UpButtonText,&up_info);
        up_info.width=width;
        up_info.height=(unsigned int) ((3*height) >> 1);
        up_info.x=QuantumMargin;
        up_info.y=((5*QuantumMargin) >> 1)+height;
        XGetWidgetInfo(HomeButtonText,&home_info);
        home_info.width=width;
        home_info.height=(unsigned int) ((3*height) >> 1);
        home_info.x=QuantumMargin;
        home_info.y=up_info.y+up_info.height+QuantumMargin;
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
          Initialize scroll information.
        */
        XGetWidgetInfo((char *) NULL,&scroll_info);
        scroll_info.bevel_width--;
        scroll_info.width=height;
        scroll_info.height=(unsigned int)
          (reply_info.y-up_info.y-(QuantumMargin >> 1));
        scroll_info.x=reply_info.x+(reply_info.width-scroll_info.width);
        scroll_info.y=up_info.y-reply_info.bevel_width;
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
        visible_files=scroll_info.height/(height+(height >> 3));
        if (files > visible_files)
          slider_info.height=(unsigned int)
            ((visible_files*slider_info.height)/files);
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
          Redraw File Browser window.
        */
        x=QuantumMargin;
        y=text_info.y+((text_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,DirectoryText,
          Extent(DirectoryText));
        (void) CopyMagickString(text_info.text,working_path,MaxTextExtent);
        (void) ConcatenateMagickString(text_info.text,DirectorySeparator,
          MaxTextExtent);
        (void) ConcatenateMagickString(text_info.text,glob_pattern,
          MaxTextExtent);
        XDrawWidgetText(display,&windows->widget,&text_info);
        XDrawBeveledButton(display,&windows->widget,&up_info);
        XDrawBeveledButton(display,&windows->widget,&home_info);
        XDrawBeveledMatte(display,&windows->widget,&list_info);
        XDrawBeveledMatte(display,&windows->widget,&scroll_info);
        XDrawTriangleNorth(display,&windows->widget,&north_info);
        XDrawBeveledButton(display,&windows->widget,&slider_info);
        XDrawTriangleSouth(display,&windows->widget,&south_info);
        x=QuantumMargin;
        y=reply_info.y+((reply_info.height-height) >> 1)+font_info->ascent;
        (void) XDrawString(display,windows->widget.id,
          windows->widget.annotate_context,x,y,FilenameText,
          Extent(FilenameText));
        XDrawBeveledMatte(display,&windows->widget,&reply_info);
        XDrawMatteText(display,&windows->widget,&reply_info);
        XDrawBeveledButton(display,&windows->widget,&special_info);
        XDrawBeveledButton(display,&windows->widget,&action_info);
        XDrawBeveledButton(display,&windows->widget,&cancel_info);
        XHighlightWidget(display,&windows->widget,BorderOffset,BorderOffset);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(~RedrawWidgetState);
      }
    if (state & UpdateListState)
      {
        char
          **checklist;

        size_t
          number_files;

        /*
          Update file list.
        */
        checklist=ListFiles(working_path,glob_pattern,&number_files);
        if (checklist == (char **) NULL)
          {
            /*
              Reply is a filename, exit.
            */
            action_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&action_info);
            break;
          }
        for (i=0; i < (ssize_t) files; i++)
          filelist[i]=DestroyString(filelist[i]);
        if (filelist != (char **) NULL)
          filelist=(char **) RelinquishMagickMemory(filelist);
        filelist=checklist;
        files=number_files;
        /*
          Update file list.
        */
        slider_info.height=
          scroll_info.height-((slider_info.min_y-scroll_info.y+1) << 1)+1;
        if (files > visible_files)
          slider_info.height=(unsigned int)
            ((visible_files*slider_info.height)/files);
        slider_info.max_y=south_info.y-south_info.bevel_width-
          slider_info.bevel_width-2;
        slider_info.id=0;
        slider_info.y=slider_info.min_y;
        expose_info.y=slider_info.y;
        selection_info.id=(~0);
        list_info.id=(~0);
        state|=RedrawListState;
        /*
          Redraw directory name & reply.
        */
        if (IsGlob(reply_info.text) == MagickFalse)
          {
            *reply_info.text='\0';
            reply_info.cursor=reply_info.text;
          }
        (void) CopyMagickString(text_info.text,working_path,MaxTextExtent);
        (void) ConcatenateMagickString(text_info.text,DirectorySeparator,
          MaxTextExtent);
        (void) ConcatenateMagickString(text_info.text,glob_pattern,
          MaxTextExtent);
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
          Jump scroll to match user filename.
        */
        list_info.id=(~0);
        for (i=0; i < (ssize_t) files; i++)
          if (LocaleCompare(filelist[i],reply) >= 0)
            {
              list_info.id=(int)
                (LocaleCompare(filelist[i],reply) == 0 ? i : ~0);
              break;
            }
        if ((i < (ssize_t) slider_info.id) ||
            (i >= (ssize_t) (slider_info.id+visible_files)))
          slider_info.id=(int) i-(visible_files >> 1);
        selection_info.id=(~0);
        state|=RedrawListState;
        state&=(~JumpListState);
      }
    if (state & RedrawListState)
      {
        /*
          Determine slider id and position.
        */
        if (slider_info.id >= (int) (files-visible_files))
          slider_info.id=(int) (files-visible_files);
        if ((slider_info.id < 0) || (files <= visible_files))
          slider_info.id=0;
        slider_info.y=slider_info.min_y;
        if (files > 0)
          slider_info.y+=((ssize_t) slider_info.id*(slider_info.max_y-
            slider_info.min_y+1)/files);
        if (slider_info.id != selection_info.id)
          {
            /*
              Redraw scroll bar and file names.
            */
            selection_info.id=slider_info.id;
            selection_info.y=list_info.y+(height >> 3)+2;
            for (i=0; i < (ssize_t) visible_files; i++)
            {
              selection_info.raised=(int) (slider_info.id+i) != list_info.id ?
                MagickTrue : MagickFalse;
              selection_info.text=(char *) NULL;
              if ((slider_info.id+i) < (ssize_t) files)
                selection_info.text=filelist[slider_info.id+i];
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
          if (slider_info.id < (int) files)
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
          if (slider_info.id < (int) files)
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
              slider_info.id-=(visible_files-1);
            else
              slider_info.id+=(visible_files-1);
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(list_info,event.xbutton))
          {
            int
              id;

            /*
              User pressed file matte.
            */
            id=slider_info.id+(event.xbutton.y-(list_info.y+(height >> 1))+1)/
              selection_info.height;
            if (id >= (int) files)
              break;
            (void) CopyMagickString(reply_info.text,filelist[id],MaxTextExtent);
            reply_info.highlight=MagickFalse;
            reply_info.marker=reply_info.text;
            reply_info.cursor=reply_info.text+Extent(reply_info.text);
            XDrawMatteText(display,&windows->widget,&reply_info);
            if (id == list_info.id)
              {
                char
                  *p;

                p=reply_info.text+strlen(reply_info.text)-1;
                if (*p == *DirectorySeparator)
                  ChopPathComponents(reply_info.text,1);
                (void) ConcatenateMagickString(working_path,DirectorySeparator,
                  MaxTextExtent);
                (void) ConcatenateMagickString(working_path,reply_info.text,
                  MaxTextExtent);
                *reply='\0';
                state|=UpdateListState;
              }
            selection_info.id=(~0);
            list_info.id=id;
            state|=RedrawListState;
            break;
          }
        if (MatteIsActive(up_info,event.xbutton))
          {
            /*
              User pressed Up button.
            */
            up_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&up_info);
            break;
          }
        if (MatteIsActive(home_info,event.xbutton))
          {
            /*
              User pressed Home button.
            */
            home_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&home_info);
            break;
          }
        if (MatteIsActive(special_info,event.xbutton))
          {
            /*
              User pressed Special button.
            */
            special_info.raised=MagickFalse;
            XDrawBeveledButton(display,&windows->widget,&special_info);
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
            for (i=1; i <= (ssize_t) Extent(reply_info.marker); i++)
              if (XTextWidth(font_info,reply_info.marker,(int) i) > x)
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
        if (up_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(up_info,event.xbutton))
                {
                  ChopPathComponents(working_path,1);
                  if (*working_path == '\0')
                    (void) CopyMagickString(working_path,DirectorySeparator,
                      MaxTextExtent);
                  state|=UpdateListState;
                }
            up_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&up_info);
          }
        if (home_info.raised == MagickFalse)
          {
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(home_info,event.xbutton))
                {
                  (void) CopyMagickString(working_path,home_directory,
                    MaxTextExtent);
                  state|=UpdateListState;
                }
            home_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&home_info);
          }
        if (special_info.raised == MagickFalse)
          {
            if (anomaly == MagickFalse)
              {
                char
                  **formats;

                ExceptionInfo
                  *exception;

                size_t
                  number_formats;

                /*
                  Let user select image format.
                */
                exception=AcquireExceptionInfo();
                formats=GetMagickList("*",&number_formats,exception);
                exception=DestroyExceptionInfo(exception);
                if (formats == (char **) NULL)
                  break;
                (void) XCheckDefineCursor(display,windows->widget.id,
                  windows->widget.busy_cursor);
                windows->popup.x=windows->widget.x+60;
                windows->popup.y=windows->widget.y+60;
                XListBrowserWidget(display,windows,&windows->popup,
                  (const char **) formats,"Select","Select image format type:",
                  format);
                XSetCursorState(display,windows,MagickTrue);
                (void) XCheckDefineCursor(display,windows->widget.id,
                  windows->widget.cursor);
                LocaleLower(format);
                AppendImageFormat(format,reply_info.text);
                reply_info.cursor=reply_info.text+Extent(reply_info.text);
                XDrawMatteText(display,&windows->widget,&reply_info);
                special_info.raised=MagickTrue;
                XDrawBeveledButton(display,&windows->widget,&special_info);
                for (i=0; i < (ssize_t) number_formats; i++)
                  formats[i]=DestroyString(formats[i]);
                formats=(char **) RelinquishMagickMemory(formats);
                break;
              }
            if (event.xbutton.window == windows->widget.id)
              if (MatteIsActive(special_info,event.xbutton))
                {
                  (void) CopyMagickString(working_path,"x:",MaxTextExtent);
                  state|=ExitState;
                }
            special_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&special_info);
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
                  *reply='\0';
                  state|=ExitState;
                }
            cancel_info.raised=MagickTrue;
            XDrawBeveledButton(display,&windows->widget,&cancel_info);
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
          command[MaxTextExtent];

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
                slider_info.id-=visible_files;
                break;
              }
              case XK_Next:
              case XK_KP_Next:
              {
                slider_info.id+=visible_files;
                break;
              }
              case XK_End:
              case XK_KP_End:
              {
                slider_info.id=(int) files;
                break;
              }
            }
            state|=RedrawListState;
            break;
          }
        if ((key_symbol == XK_Return) || (key_symbol == XK_KP_Enter))
          {
            /*
              Read new directory or glob patterm.
            */
            if (*reply_info.text == '\0')
              break;
            if (IsGlob(reply_info.text))
              (void) CopyMagickString(glob_pattern,reply_info.text,
                MaxTextExtent);
            else
              {
                (void) ConcatenateMagickString(working_path,DirectorySeparator,
                  MaxTextExtent);
                (void) ConcatenateMagickString(working_path,reply_info.text,
                  MaxTextExtent);
                if (*working_path == '~')
                  ExpandFilename(working_path);
                *reply='\0';
              }
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
              slider_info.id=(int) ((files*(slider_info.y-slider_info.min_y+1))/
                (slider_info.max_y-slider_info.min_y+1));
            state|=RedrawListState;
            break;
          }
        if (state & InactiveWidgetState)
          break;
        if (up_info.raised == MatteIsActive(up_info,event.xmotion))
          {
            /*
              Up button status changed.
            */
            up_info.raised=!up_info.raised;
            XDrawBeveledButton(display,&windows->widget,&up_info);
            break;
          }
        if (home_info.raised == MatteIsActive(home_info,event.xmotion))
          {
            /*
              Home button status changed.
            */
            home_info.raised=!home_info.raised;
            XDrawBeveledButton(display,&windows->widget,&home_info);
            break;
          }
        if (special_info.raised == MatteIsActive(special_info,event.xmotion))
          {
            /*
              Grab button status changed.
            */
            special_info.raised=!special_info.raised;
            XDrawBeveledButton(display,&windows->widget,&special_info);
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
        notify.display=request->display;
        notify.requestor=request->requestor;
        notify.selection=request->selection;
        notify.target=request->target;
        notify.time=request->time;
        if (request->property == None)
          notify.property=request->target;
        else
          notify.property=request->property;
        (void) XSendEvent(request->display,request->requestor,False,0,
          (XEvent *) &notify);
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  XSetCursorState(display,windows,MagickFalse);
  (void) XWithdrawWindow(display,windows->widget.id,windows->widget.screen);
  XCheckRefreshWindows(display,windows);
  /*
    Free file list.
  */
  for (i=0; i < (ssize_t) files; i++)
    filelist[i]=DestroyString(filelist[i]);
  if (filelist != (char **) NULL)
    filelist=(char **) RelinquishMagickMemory(filelist);
  if (*reply != '\0')
    {
      (void) ConcatenateMagickString(working_path,DirectorySeparator,
        MaxTextExtent);
      (void) ConcatenateMagickString(working_path,reply,MaxTextExtent);
    }
  (void) CopyMagickString(reply,working_path,MaxTextExtent);
  if (*reply == '~')
    ExpandFilename(reply);
}