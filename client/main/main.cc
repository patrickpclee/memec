#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include "client.hh"
#include "../../common/util/option.hh"
#include "../../lib/death_handler/death_handler.h"

int main( int argc, char **argv ) {
	int opt, ret = 0;
	bool verbose = false;
	char *path = NULL, path_default[] = "bin/config/local";
	Client *master = 0;
	OptionList options;
	struct option_t tmpOption;
	static struct option long_options[] = {
		{ "path", required_argument, NULL, 'p' },
		{ "option", required_argument, NULL, 'o' },
		{ "help", no_argument, NULL, 'h' },
		{ "verbose", no_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
	};
	Debug::DeathHandler dh;

	//////////////////////////////////
	// Parse command-line arguments //
	//////////////////////////////////
	opterr = 0;
	while( ( opt = getopt_long( argc, argv,
	                            "p:o:hv",
	                            long_options, NULL ) ) != -1 ) {
		switch( opt ) {
			case 'p':
				path = optarg;
				break;
			case 'o':
				tmpOption.section = 0;
				tmpOption.name = 0;
				tmpOption.value = 0;
				for ( int i = optind - 1, j = 0; i < argc && argv[ i ][ 0 ] != '-'; i++, j++ ) {
					switch( j ) {
						case 0: tmpOption.section = argv[ i ]; break;
						case 1: tmpOption.name = argv[ i ]; break;
						case 2: tmpOption.value = argv[ i ]; break;
					}
				}
				if ( tmpOption.value )
					options.push_back( tmpOption );
				break;
			case 'h':
				ret = 0;
				goto usage;
			case 'v':
				verbose = true;
				break;
			default:
				goto usage;
		}
	}
	path = path == NULL ? path_default : path;

	////////////////////////////////
	// Pass control to the Client //
	////////////////////////////////
	master = Client::getInstance();
	if ( ! master->init( path, options, verbose ) ) {
		fprintf( stderr, "Error: Cannot initialize master.\n" );
		return 1;
	}
	if ( ! master->start() ) {
		fprintf( stderr, "Error: Cannot start master.\n" );
		// return 1;
	}
	master->interactive();
	master->stop();

	return 0;

usage:
	fprintf(
		stderr,
		"Usage: %s [OPTION]...\n"
		"Start a coordinator.\n\n"
		"Mandatory arguments to long options are mandatory for short "
		"options too.\n"
		"  -p, --path         Specify the path to the directory containing the config files\n"
		"  -o, --option       Override the options in the config file of master\n"
		"  -v, --verbose      Show configuration\n"
		"  -h, --help         Display this help and exit\n",
		argv[ 0 ]
	);

	return ret;
}
