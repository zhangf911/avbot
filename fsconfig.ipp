
static fs::path configfilepath()
{
	if ( getenv ( "USERPROFILE" ) ) {
		if ( fs::exists ( fs::path ( getenv ( "USERPROFILE" ) ) / ".qqbotrc" ) )
			return fs::path ( getenv ( "USERPROFILE" ) ) / ".qqbotrc";
	}

	if ( getenv ( "HOME" ) ) {
		if ( fs::exists ( fs::path ( getenv ( "HOME" ) ) / ".qqbotrc" ) )
			return fs::path ( getenv ( "HOME" ) ) / ".qqbotrc";
	}

	if ( fs::exists ( "./qqbotrc" ) )
		return fs::path ( "./qqbotrc" );

	if ( fs::exists ( "/etc/qqbotrc" ) )
		return fs::path ( "/etc/qqbotrc" );

	throw "not configfileexit";
}

