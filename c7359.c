TEST_F(ZNCTest, Modpython) {
    if (QProcessEnvironment::systemEnvironment().value(
            "DISABLED_ZNC_PERL_PYTHON_TEST") == "1") {
        return;
    }
    auto znc = Run();
    znc->CanLeak();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod modpython");
    client.Write("znc loadmod pyeval");
    client.Write("PRIVMSG *pyeval :2+2");
    client.ReadUntil(":*pyeval!znc@znc.in PRIVMSG nick :4");
    client.Write("PRIVMSG *pyeval :module.GetUser().GetUserName()");
    client.ReadUntil("nick :'user'");
    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":n!u@h PRIVMSG nick :Hi\xF0, github issue #1229");
    // "replacement character"
    client.ReadUntil("Hi\xEF\xBF\xBD, github issue");
}