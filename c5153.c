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
    mount = KGlobal::dirs()->findExe("mount.cifs", paths.at(i));

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
    Smb4KNotification::commandNotFound("mount.cifs");
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
  share->setFileSystem(Smb4KShare::CIFS);
  
  if (options)
  {
    share->setPort(options->fileSystemPort() != Smb4KMountSettings::remoteFileSystemPort() ?
                   options->fileSystemPort() : Smb4KMountSettings::remoteFileSystemPort());
  }
  else
  {
    share->setPort(Smb4KMountSettings::remoteFileSystemPort());
  }
  
  // Compile the list of arguments, that is added to the
  // mount command via "-o ...".
  QStringList args_list;
  
  // Workgroup or domain
  if (!share->workgroupName().trimmed().isEmpty())
  {
    args_list << QString("domain=%1").arg(KShell::quoteArg(share->workgroupName()));
  }
  else
  {
    // Do nothing
  }
  
  // Host IP
  if (!share->hostIP().trimmed().isEmpty())
  {
    args_list << QString("ip=%1").arg(share->hostIP());
  }
  else
  {
    // Do nothing
  }
  
  // User name
  if (!share->login().isEmpty())
  {
    args_list << QString("username=%1").arg(share->login());
  }
  else
  {
    args_list << "guest";
  }
  
  // Client's and server's NetBIOS name
  // According to the manual page, this is only needed when port 139
  // is used. So, we only pass the NetBIOS name in that case.
  if (Smb4KMountSettings::remoteFileSystemPort() == 139 || (options && options->fileSystemPort() == 139))
  {
    // The client's NetBIOS name.
    if (!Smb4KSettings::netBIOSName().isEmpty())
    {
      args_list << QString("netbiosname=%1").arg(KShell::quoteArg(Smb4KSettings::netBIOSName()));
    }
    else
    {
      if (!global_options["netbios name"].isEmpty())
      {
        args_list << QString("netbiosname=%1").arg(KShell::quoteArg(global_options["netbios name"]));
      }
      else
      {
        // Do nothing
      }
    }

    // The server's NetBIOS name.
    // ('servern' is a synonym for 'servernetbiosname')
    args_list << QString("servern=%1").arg(KShell::quoteArg( share->hostName()));
  }
  else
  {
    // Do nothing
  }
  
  // UID
  args_list << QString("uid=%1").arg(options ? options->uid() : (K_UID)Smb4KMountSettings::userID().toInt());
  
  // Force user
  if (Smb4KMountSettings::forceUID())
  {
    args_list << "forceuid";
  }
  else
  {
    // Do nothing
  }
  
  // GID
  args_list << QString("gid=%1").arg(options ? options->gid() : (K_GID)Smb4KMountSettings::groupID().toInt());
  
  // Force GID
  if (Smb4KMountSettings::forceGID())
  {
    args_list << "forcegid";
  }
  else
  {
    // Do nothing
  }
  
  // Client character set
  switch (Smb4KMountSettings::clientCharset())
  {
    case Smb4KMountSettings::EnumClientCharset::default_charset:
    {
      if (!global_options["unix charset"].isEmpty())
      {
        args_list << QString("iocharset=%1").arg(global_options["unix charset"].toLower());
      }
      else
      {
        // Do nothing
      }
      break;
    }
    default:
    {
      args_list << QString("iocharset=%1")
                   .arg(Smb4KMountSettings::self()->clientCharsetItem()->choices().value(Smb4KMountSettings::clientCharset()).label);
      break;
    }
  }
  
  // Port. 
  args_list << QString("port=%1").arg(share->port());
  
  // Write access
  if (options)
  {
    switch (options->writeAccess())
    {
      case Smb4KCustomOptions::ReadWrite:
      {
        args_list << "rw";
        break;
      }
      case Smb4KCustomOptions::ReadOnly:
      {
        args_list << "ro";
        break;
      }
      default:
      {
        switch (Smb4KMountSettings::writeAccess())
        {
          case Smb4KMountSettings::EnumWriteAccess::ReadWrite:
          {
            args_list << "rw";
            break;
          }
          case Smb4KMountSettings::EnumWriteAccess::ReadOnly:
          {
            args_list << "ro";
            break;
          }
          default:
          {
            break;
          }
        }
        break;
      }
    }
  }
  else
  {
    switch (Smb4KMountSettings::writeAccess())
    {
      case Smb4KMountSettings::EnumWriteAccess::ReadWrite:
      {
        args_list << "rw";
        break;
      }
      case Smb4KMountSettings::EnumWriteAccess::ReadOnly:
      {
        args_list << "ro";
        break;
      }
      default:
      {
        break;
      }
    }
  }
  
  // File mask
  if (!Smb4KMountSettings::fileMask().isEmpty())
  {
    args_list << QString("file_mode=%1").arg(Smb4KMountSettings::fileMask());
  }
  else
  {
    // Do nothing
  }

  // Directory mask
  if (!Smb4KMountSettings::directoryMask().isEmpty())
  {
    args_list << QString("dir_mode=%1").arg(Smb4KMountSettings::directoryMask());
  }
  else
  {
    // Do nothing
  }
  
  // Permission checks
  if (Smb4KMountSettings::permissionChecks())
  {
    args_list << "perm";
  }
  else
  {
    args_list << "noperm";
  }
  
  // Client controls IDs
  if (Smb4KMountSettings::clientControlsIDs())
  {
    args_list << "setuids";
  }
  else
  {
    args_list << "nosetuids";
  }
  
  // Server inode numbers
  if (Smb4KMountSettings::serverInodeNumbers())
  {
    args_list << "serverino";
  }
  else
  {
    args_list << "noserverino";
  }
  
  // Cache mode
  switch (Smb4KMountSettings::cacheMode())
  {
    case Smb4KMountSettings::EnumCacheMode::None:
    {
      args_list << "cache=none";
      break;
    }
    case Smb4KMountSettings::EnumCacheMode::Strict:
    {
      args_list << "cache=strict";
      break;
    }
    case Smb4KMountSettings::EnumCacheMode::Loose:
    {
      args_list << "cache=loose";
      break;
    }
    default:
    {
      break;
    }
  }
  
  // Translate reserved characters
  if (Smb4KMountSettings::translateReservedChars())
  {
    args_list << "mapchars";
  }
  else
  {
    args_list << "nomapchars";
  }
  
  // Locking
  if (Smb4KMountSettings::noLocking())
  {
    args_list << "nolock";
  }
  else
  {
    // Do nothing
  }
  
  // Security mode
  if (options)
  {
    switch (options->securityMode())
    {
      case Smb4KCustomOptions::NoSecurityMode:
      {
        args_list << "sec=none";
        break;
      }
      case Smb4KCustomOptions::Krb5:
      {
        args_list << "sec=krb5";
        args_list << QString("cruid=%1").arg(KUser(KUser::UseRealUserID).uid());
        break;
      }
      case Smb4KCustomOptions::Krb5i:
      {
        args_list << "sec=krb5i";
        args_list << QString("cruid=%1").arg(KUser(KUser::UseRealUserID).uid());
        break;
      }
      case Smb4KCustomOptions::Ntlm:
      {
        args_list << "sec=ntlm";
        break;
      }
      case Smb4KCustomOptions::Ntlmi:
      {
        args_list << "sec=ntlmi";
        break;
      }
      case Smb4KCustomOptions::Ntlmv2:
      {
        args_list << "sec=ntlmv2";
        break;
      }
      case Smb4KCustomOptions::Ntlmv2i:
      {
        args_list << "sec=ntlmv2i";
        break;
      }
      case Smb4KCustomOptions::Ntlmssp:
      {
        args_list << "sec=ntlmssp";
        break;
      }
      case Smb4KCustomOptions::Ntlmsspi:
      {
        args_list << "sec=ntlmsspi";
        break;
      }
      default:
      {
        // Smb4KCustomOptions::DefaultSecurityMode
        break;
      }
    }
  }
  else
  {
    switch (Smb4KMountSettings::securityMode())
    {
      case Smb4KMountSettings::EnumSecurityMode::None:
      {
        args_list << "sec=none";
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Krb5:
      {
        args_list << "sec=krb5";
        args_list << QString("cruid=%1").arg(KUser(KUser::UseRealUserID).uid());
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Krb5i:
      {
        args_list << "sec=krb5i";
        args_list << QString("cruid=%1").arg(KUser(KUser::UseRealUserID).uid());
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Ntlm:
      {
        args_list << "sec=ntlm";
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Ntlmi:
      {
        args_list << "sec=ntlmi";
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Ntlmv2:
      {
        args_list << "sec=ntlmv2";
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Ntlmv2i:
      {
        args_list << "sec=ntlmv2i";
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Ntlmssp:
      {
        args_list << "sec=ntlmssp";
        break;
      }
      case Smb4KMountSettings::EnumSecurityMode::Ntlmsspi:
      {
        args_list << "sec=ntlmsspi";
        break;
      }
      default:
      {
        // Smb4KSettings::EnumSecurityMode::Default,
        break;
      }
    }
  }
  
  // SMB protocol version
  switch (Smb4KMountSettings::smbProtocolVersion())
  {
    case Smb4KMountSettings::EnumSmbProtocolVersion::OnePointZero:
    {
      args_list << "vers=1.0";
      break;
    }
    case Smb4KMountSettings::EnumSmbProtocolVersion::TwoPointZero:
    {
      args_list << "vers=2.0";
      break;
    }
    case Smb4KMountSettings::EnumSmbProtocolVersion::TwoPointOne:
    {
      args_list << "vers=2.1";
      break;
    }
    case Smb4KMountSettings::EnumSmbProtocolVersion::ThreePointZero:
    {
      args_list << "vers=3.0";
      break;
    }
    default:
    {
      break;
    }
  }
    
  // Global custom options provided by the user
  if (!Smb4KMountSettings::customCIFSOptions().isEmpty())
  {
    // SECURITY: Only pass those arguments to mount.cifs that do not pose
    // a potential security risk and that have not already been defined.
    //
    // This is, among others, the proper fix to the security issue reported
    // by Heiner Markert (aka CVE-2014-2581).
    QStringList whitelist = whitelistedMountArguments();
    QStringList list = Smb4KMountSettings::customCIFSOptions().split(',', QString::SkipEmptyParts);
    QMutableStringListIterator it(list);
    
    while (it.hasNext())
    {
      QString arg = it.next().section("=", 0, 0);
      
      if (!whitelist.contains(arg))
      {
        it.remove();
      }
      else
      {
        // Do nothing
      }
      
      args_list += list;
    }
  }
  else
  {
    // Do nothing
  }
  
  // Mount options
  QStringList mh_options;
  mh_options << "-o";
  mh_options << args_list.join(",");
  map.insert("mh_options", mh_options);
  
  // Mount point
  map.insert("mh_mountpoint", share->canonicalPath());
  
  // UNC
  map.insert("mh_unc", !share->isHomesShare() ? share->unc() : share->homeUNC());;
  
  return true;
}