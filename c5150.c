ActionReply Smb4KMountHelper::mount(const QVariantMap &args)
{
  ActionReply reply;
  // The mountpoint is a unique and can be used to
  // find the share.
  reply.addData("mh_mountpoint", args["mh_mountpoint"]);
  
  KProcess proc(this);
  proc.setOutputChannelMode(KProcess::SeparateChannels);
  proc.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
#if defined(Q_OS_LINUX)
  proc.setEnv("PASSWD", args["mh_url"].toUrl().password(), true);
#endif
  // We need this to avoid a translated password prompt.
  proc.setEnv("LANG", "C");
  // If the location of a Kerberos ticket is passed, it needs to
  // be passed to the process environment here.
  if (args.contains("mh_krb5ticket"))
  {
    proc.setEnv("KRB5CCNAME", args["mh_krb5ticket"].toString());
  }
  else
  {
    // Do nothing
  }
  
  // Set the mount command here.
  QStringList command;
#if defined(Q_OS_LINUX)
  command << args["mh_command"].toString();
  command << args["mh_unc"].toString();
  command << args["mh_mountpoint"].toString();
  command << args["mh_options"].toStringList();
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
  command << args["mh_command"].toString();
  command << args["mh_options"].toStringList();
  command << args["mh_unc"].toString();
  command << args["mh_mountpoint"].toString();
#else
  // Do nothing.
#endif
  proc.setProgram(command);

  // Run the mount process.
  proc.start();

  if (proc.waitForStarted(-1))
  {
    bool user_kill = false;

    while (!proc.waitForFinished(10))
    {
      // Check if there is a password prompt. If there is one, pass
      // the password to it.
      QByteArray out = proc.readAllStandardError();
      
      if (out.startsWith("Password:"))
      {
        proc.write(args["mh_url"].toUrl().password().toUtf8());
        proc.write("\r");
      }
      else
      {
        // Do nothing
      }

      // We want to be able to terminate the process from outside.
      // Thus, we implement a loop that checks periodically, if we
      // need to kill the process.
      if (HelperSupport::isStopped())
      {
        proc.kill();
        user_kill = true;
        break;
      }
      else
      {
        // Do nothing
      }
    }

    if (proc.exitStatus() == KProcess::CrashExit)
    {
      if (!user_kill)
      {
        reply.setErrorCode(ActionReply::HelperError);
        reply.setErrorDescription(i18n("The mount process crashed."));
        return reply;
      }
      else
      {
        // Do nothing
      }
    }
    else
    {
      // Check if there is output on stderr.
      QString stdErr = QString::fromUtf8(proc.readAllStandardError());
      reply.addData("mh_error_message", stdErr.trimmed());
    }
  }
  else
  {
    reply.setErrorCode(ActionReply::HelperError);
    reply.setErrorDescription(i18n("The mount process could not be started."));
  }

  return reply;
}