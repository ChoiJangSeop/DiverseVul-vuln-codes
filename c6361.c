void Mounter::registerPid(int pid)
{
    pids.insert(pid);
    startTimer();
}