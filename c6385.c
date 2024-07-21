void Mounter::startTimer()
{
    if (!timer) {
        timer=new QTimer(this);
        connect(timer, SIGNAL(timeout()), SLOT(timeout()));
    }
    timer->start(30000);
}