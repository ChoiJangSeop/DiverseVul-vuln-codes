ActionReply Smb4KMountHelper::unmount(const QVariantMap &args)
{
  ActionReply reply;
  // The mountpoint is a unique and can be used to
  // find the share.
  reply.addData("mh_mountpoint", args["mh_mountpoint"]);

  // Check if the mountpoint is valid and the file system is
  // also correct.
  bool mountpoint_ok = false;
  KMountPoint::List mountpoints = KMountPoint::currentMountPoints(KMountPoint::BasicInfoNeeded|KMountPoint::NeedMountOptions);

  for(int j = 0; j < mountpoints.size(); j++)
  {
#ifdef Q_OS_LINUX
    if (QString::compare(args["mh_mountpoint"].toString(),
                         mountpoints.at(j)->mountPoint(), Qt::CaseInsensitive) == 0 &&
        QString::compare(mountpoints.at(j)->mountType(), "cifs", Qt::CaseInsensitive) == 0)
#else
    if (QString::compare(args["mh_mountpoint"].toString(),
                         mountpoints.at(j)->mountPoint(), Qt::CaseInsensitive) == 0 &&
        QString::compare(mountpoints.at(j)->mountType(), "smbfs", Qt::CaseInsensitive) == 0)
#endif
    {
      mountpoint_ok = true;
      break;
    }
    else
    {
      continue;
    }
  }

  if (!mountpoint_ok)
  {
    reply.setErrorCode(ActionReply::HelperError);
    reply.setErrorDescription(i18n("The mountpoint is invalid."));
    return reply;
  }
  else
  {
    // Do nothing
  }

  KProcess proc(this);
  proc.setOutputChannelMode(KProcess::SeparateChannels);
  proc.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
  
  // Set the umount command here.
  QStringList command;
  command << args["mh_command"].toString();
  command << args["mh_options"].toStringList();
  command << args["mh_mountpoint"].toString();

  proc.setProgram(command);

  // Run the unmount process.
  proc.start();

  if (proc.waitForStarted(-1))
  {
    // We want to be able to terminate the process from outside.
    // Thus, we implement a loop that checks periodically, if we
    // need to kill the process.
    bool user_kill = false;

    while (!proc.waitForFinished(10))
    {
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
        reply.setErrorDescription(i18n("The unmount process crashed."));
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
    reply.setErrorDescription(i18n("The unmount process could not be started."));
  }

  return reply;
}