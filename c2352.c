int SSH_Access::HandleSSHMessage()
{
   int m=STALL;
   const char *b;
   int s;
   pty_recv_buf->Get(&b,&s);
   const char *eol=find_char(b,s,'\n');
   if(!eol)
   {
      const char *p="password:";
      const char *p_for="password for ";
      const char *y="(yes/no)?";
      int p_len=strlen(p);
      int p_for_len=strlen(p_for);
      int y_len=strlen(y);
      if(s>0 && b[s-1]==' ')
	 s--;
      if((s>=p_len && !strncasecmp(b+s-p_len,p,p_len))
      || (s>10 && !strncmp(b+s-2,"':",2))
      || (s>p_for_len && b[s-1]==':' && !strncasecmp(b,p_for,p_for_len)))
      {
	 if(!pass)
	 {
	    SetError(LOGIN_FAILED,_("Password required"));
	    return MOVED;
	 }
	 if(password_sent>0)
	 {
	    SetError(LOGIN_FAILED,_("Login incorrect"));
	    return MOVED;
	 }
	 pty_recv_buf->Put("XXXX");
	 pty_send_buf->Put(pass);
	 pty_send_buf->Put("\n");
	 password_sent++;
	 return m;
      }
      if(s>=y_len && !strncasecmp(b+s-y_len,y,y_len))
      {
	 pty_recv_buf->Put("yes\n");
	 pty_send_buf->Put("yes\n");
	 return m;
      }
      if(!received_greeting && recv_buf->Size()>0)
      {
	 recv_buf->Get(&b,&s);
	 eol=find_char(b,s,'\n');
	 if(eol)
	 {
	    xstring &line=xstring::get_tmp(b,eol-b);
	    if(line.eq(greeting))
	       received_greeting=true;
	    LogRecv(4,line);
	    recv_buf->Skip(eol-b+1);
	 }
      }
      LogSSHMessage();
      return m;
   }
   const char *f=N_("Host key verification failed");
   if(!strncasecmp(b,f,strlen(f)))
   {
      LogSSHMessage();
      SetError(FATAL,_(f));
      return MOVED;
   }
   if(eol>b && eol[-1]=='\r')
      eol--;
   f=N_("Name or service not known");
   int f_len=strlen(f);
   if(eol-b>=f_len && !strncasecmp(eol-f_len,f,f_len)) {
      LogSSHMessage();
      SetError(LOOKUP_ERROR,xstring::get_tmp(b,eol-b));
      return MOVED;
   }
   LogSSHMessage();
   return MOVED;
}