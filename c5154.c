bool Smb4KUnmountJob::createUnmountAction(Smb4KShare *share, Action *action)
{
  Q_ASSERT(share);
  Q_ASSERT(action);
  
  if (!share || !action)
  {
    return false;
  }
  else
  {
    // Do nothing
  }
  
  // Find the umount program.
  QString umount;
  QStringList paths;
  paths << "/bin";
  paths << "/sbin";
  paths << "/usr/bin";
  paths << "/usr/sbin";
  paths << "/usr/local/bin";
  paths << "/usr/local/sbin";

  for ( int i = 0; i < paths.size(); ++i )
  {
    umount = KGlobal::dirs()->findExe("umount", paths.at(i));

    if (!umount.isEmpty())
    {
      break;
    }
    else
    {
      continue;
    }
  }

  if (umount.isEmpty() && !m_silent)
  {
    Smb4KNotification::commandNotFound("umount");
    return false;
  }
  else
  {
    // Do nothing
  }

  QStringList options;
#if defined(Q_OS_LINUX)
  if (m_force)
  {
    options << "-l"; // lazy unmount
  }
  else
  {
    // Do nothing
  }
#endif

  action->setName("net.sourceforge.smb4k.mounthelper.unmount");
  action->setHelperID("net.sourceforge.smb4k.mounthelper");
  action->addArgument("mh_command", umount);
  action->addArgument("mh_url", share->url().url());
  action->addArgument("mh_mountpoint", share->canonicalPath());
  action->addArgument("mh_options", options);

  return true;
}