int main( int argc, char **argv )
{
	options.add("convert-sqlite3", ki18n("Convert the current SQLite 2.x database to SQLite 3 and exit") , 0 );
	options.add( 0, KLocalizedString(), 0 );    


	KAboutData about( "krecipes", 0, ki18n( "Krecipes" ), version, ki18n( "The KDE Cookbook" ), KAboutData::License_GPL, ki18n( "(C) 2003 Unai Garro\n(C) 2004-2006 Jason Kivlighn"), ki18n("This product is RecipeML compatible.\nYou can get more information about this file format in:\nhttp://www.formatdata.com/recipeml" ), "http://krecipes.sourceforge.net/", "panfaust@gmail.com" );
	about.addAuthor( ki18n("Unai Garro"), KLocalizedString(), "ugarro@gmail.com", 0 );
	about.addAuthor( ki18n("Jason Kivlighn"), KLocalizedString(), "jkivlighn@gmail.com", 0 );
	about.addAuthor( ki18n("Cyril Bosselut"), KLocalizedString(), "bosselut@b1project.com", "http://b1project.com" );

	about.addCredit( ki18n("Colleen Beamer"), ki18n("Testing, bug reports, suggestions"), "colleen.beamer@gmail.com", 0 );
	about.addCredit( ki18n("Robert Wadley"), ki18n("Icons and artwork"), "rob@robntina.fastmail.us", 0 );
	about.addAuthor( ki18n("Daniel Sauvé"), ki18n("Porting to KDE4"), "megametres@gmail.com", "http://metres.homelinux.com" );

        about.addAuthor( ki18n("Laurent Montel"), ki18n("Porting to KDE4"), "montel@kde.org", 0 );
        about.addAuthor( ki18n("José Manuel Santamaría Lema"), ki18n("Porting to KDE4, current maintainer"), "panfaust@gmail.com", 0 );
        about.addAuthor( ki18n("Martin Engelmann"), ki18n("Porting to KDE4, developer"), "murphi.oss@googlemail.com", 0 );
	
	about.addCredit( ki18n("Patrick Spendrin"), ki18n("Patches to make Krecipes work under Windows"), "ps_ml@gmx.de", 0 );
	about.addCredit( ki18n("Mike Ferguson"), ki18n("Help with bugs, patches"), "", 0 );
	about.addCredit( ki18n("Warren Severin"), ki18n("Code to export recipes to *.mx2 files"), "", 0 );
	
	about.setTranslator( ki18n( "INSERT YOUR NAME HERE" ), ki18n( "INSERT YOUR EMAIL ADDRESS" ) );
	KCmdLineArgs::init( argc, argv, &about );
	KCmdLineArgs::addCmdLineOptions( options );
	KUniqueApplication::addCmdLineOptions();

	if ( !KUniqueApplication::start() ) {
		std::cerr << "Krecipes is already running!" << std::endl;
		return 0;
	}

	KUniqueApplication app;

	// see if we are starting with session management
	if ( app.isSessionRestored() ) {
		RESTORE( Krecipes );
	}
	else {
		// no session.. just start up normally
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

		QApplication::flush();

		if ( args->isSet("convert-sqlite3") ) {
			ConvertSQLite3 sqliteConverter;
			sqliteConverter.convert();
			return 0;
		}

		Krecipes * widget = new Krecipes;
		app.setTopWidget( widget );
		widget->show();

		args->clear();
	}

	return app.exec();
}