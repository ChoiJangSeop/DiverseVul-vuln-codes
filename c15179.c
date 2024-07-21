InsecureRandomDataProvider::random()
{
    if (! this->seeded_random)
    {
	// Seed the random number generator with something simple, but
	// just to be interesting, don't use the unmodified current
	// time.  It would be better if this were a more secure seed.
        QUtil::srandom(QUtil::get_current_time() ^ 0xcccc);
	this->seeded_random = true;
    }

#  ifdef HAVE_RANDOM
    return ::random();
#  else
    return rand();
#  endif
}