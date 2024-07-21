bool ExtractUnixLink50(CommandData *Cmd,const wchar *Name,FileHeader *hd)
{
  char Target[NM];
  WideToChar(hd->RedirName,Target,ASIZE(Target));
  if (hd->RedirType==FSREDIR_WINSYMLINK || hd->RedirType==FSREDIR_JUNCTION)
  {
    // Cannot create Windows absolute path symlinks in Unix. Only relative path
    // Windows symlinks can be created here. RAR 5.0 used \??\ prefix
    // for Windows absolute symlinks, since RAR 5.1 /??/ is used.
    // We escape ? as \? to avoid "trigraph" warning
    if (strncmp(Target,"\\??\\",4)==0 || strncmp(Target,"/\?\?/",4)==0)
      return false;
    DosSlashToUnix(Target,Target,ASIZE(Target));
  }
  // Use hd->FileName instead of LinkName, since LinkName can include
  // the destination path as a prefix, which can confuse
  // IsRelativeSymlinkSafe algorithm.
  if (!Cmd->AbsoluteLinks && (IsFullPath(Target) ||
      !IsRelativeSymlinkSafe(Cmd,hd->FileName,Name,hd->RedirName)))
    return false;
  return UnixSymlink(Cmd,Target,Name,&hd->mtime,&hd->atime);
}