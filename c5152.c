bool Smb4KMountJob::fillArgs(Smb4KShare *share, QMap<QString, QVariant>& map)
{
  // Find the mount program.
  QString mount;
  QStringList paths;
  paths << "/bin";
  paths << "/sbin";
  paths << "/usr/bin";
  paths << "/usr/sbin";
  paths << "/usr/local/bin";
  paths << "/usr/local/sbin";

  for (int i = 0; i < paths.size(); ++i)
  {
    mount = KGlobal::dirs()->findExe("mount_smbfs", paths.at(i));

    if (!mount.isEmpty())
    {
      map.insert("mh_command", mount);
      break;
    }
    else
    {
      continue;
    }
  }

  if (mount.isEmpty())
  {
    Smb4KNotification::commandNotFound("mount_smbfs");
    return false;
  }
  else
  {
    // Do nothing
  }
  
  // Mount arguments.
  QMap<QString, QString> global_options = globalSambaOptions();
  Smb4KCustomOptions *options  = Smb4KCustomOptionsManager::self()->findOptions(share);

  // Set some settings for the share.
  share->setFileSystem(Smb4KShare::SMBFS);
  
  // Compile the list of arguments.
  QStringList args_list;
  
  // Workgroup
  if (!share->workgroupName().isEmpty())
  {
    args_list << "-W";
    args_list << KShell::quoteArg(share->workgroupName());
  }
  else
  {
    // Do nothing
  }
  
  // Host IP
  if (!share->hostIP().isEmpty())
  {
    args_list << "-I";
    args_list << share->hostIP();
  }
  else
  {
    // Do nothing
  }
  
  // UID
  if (options)
  {
    args_list << "-u";
    args_list << QString("%1").arg(options->uid());
  }
  else
  {
    args_list << "-u";
    args_list << QString("%1").arg((K_UID)Smb4KMountSettings::userID().toInt());
  }
  
  // GID
  if (options)
  {
    args_list << "-g";
    args_list << QString("%1").arg(options->gid());
  }
  else
  {
    args_list << "-g";
    args_list << QString("%1").arg((K_GID)Smb4KMountSettings::groupID().toInt());
  }
  
  // Character sets for the client and server
  QString client_charset, server_charset;

  switch (Smb4KMountSettings::clientCharset())
  {
    case Smb4KMountSettings::EnumClientCharset::default_charset:
    {
      client_charset = global_options["unix charset"].toLower(); // maybe empty
      break;
    }
    default:
    {
      client_charset = Smb4KMountSettings::self()->clientCharsetItem()->choices().value(Smb4KMountSettings::clientCharset()).label;
      break;
    }
  }

  switch (Smb4KMountSettings::serverCodepage())
  {
    case Smb4KMountSettings::EnumServerCodepage::default_codepage:
    {
      server_charset = global_options["dos charset"].toLower(); // maybe empty
      break;
    }
    default:
    {
      server_charset = Smb4KMountSettings::self()->serverCodepageItem()->choices().value(Smb4KMountSettings::serverCodepage()).label;
      break;
    }
  }

  if (!client_charset.isEmpty() && !server_charset.isEmpty())
  {
    args_list << "-E";
    args_list << QString("%1:%2").arg(client_charset, server_charset);
  }
  else
  {
    // Do nothing
  }
  
  // File mask
  if (!Smb4KMountSettings::fileMask().isEmpty())
  {
    args_list << "-f";
    args_list << QString("%1").arg(Smb4KMountSettings::fileMask());
  }
  else
  {
    // Do nothing
  }

  // Directory mask
  if (!Smb4KMountSettings::directoryMask().isEmpty())
  {
    args_list << "-d";
    args_list << QString("%1").arg(Smb4KMountSettings::directoryMask());
  }
  else
  {
    // Do nothing
  }
  
  // User name
  if (!share->login().isEmpty())
  {
    args_list << "-U";
    args_list << QString("%1").arg(share->login());
  }
  else
  {
    args_list << "-N";
  }
  
  // Mount options
  map.insert("mh_options", args_list);
  
  // Mount point
  map.insert("mh_mountpoint", share->canonicalPath());
  
  // UNC
  map.insert("mh_unc", !share->isHomesShare() ? share->unc() : share->homeUNC());;
  
  return true;
}