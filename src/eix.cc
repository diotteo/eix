// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#include <config.h>
#include <main/main.h>

#include <eixrc/global.h>

#include <output/print-xml.h>
#include <output/formatstring.h>
#include <output/formatstring-print.h>

#include <portage/conf/cascadingprofile.h>
#include <portage/conf/portagesettings.h>
#include <portage/set_stability.h>

#include <eixTk/argsreader.h>
#include <eixTk/stringutils.h>
#include <eixTk/utils.h>
#include <eixTk/filenames.h>

#include "cli.h"

#include <database/header.h>
#include <database/package_reader.h>

#define VAR_DB_PKG "/var/db/pkg/"

using namespace std;

static int  is_current_dbversion(const char *filename);
static void print_vector(const vector<string> &vec);
static void print_unused(const string &filename, const string &excludefiles, const eix::ptr_list<Package> &packagelist, bool test_empty = false);
static void print_removed(const string &dirname, const string &excludefiles, const eix::ptr_list<Package> &packagelist);

/** Show a short help screen with options and commands. */
static void
dump_help(int exit_code)
{
	printf(_("Usage: %s [options] EXPRESSION\n"
			"\n"
			"Search for packages in the index generated by eix-update.\n"
			"EXPRESSION is true or false. Packages for which the EXPRESSION gives true,\n"
			"are included in the final report.\n"
			"\n"
			"An EXPRESSION can be:\n"
			"    EXPRESSION [-o|-a] EXPRESSION\n"
			"    [local-options] PATTERN\n"
			"\n"
			"Global:\n"
			"   Exclusive options:\n"
			"     -h, --help            show this screen and exit\n"
			"     -V, --version         show version and exit\n"
			"     --dump                dump variables to stdout\n"
			"     --dump-defaults       dump default values of variables\n"
			"     --print               print the expanded value of a variable\n"
			"     --print-all-useflags  print all IUSE words used in some version\n"
			"     --print-all-keywords  print all KEYWORDS used in some version\n"
			"     --print-all-slots     print all SLOT strings used in some version\n"
			"     --print-all-provides  print all PROVIDE strings used in some package\n"
			"     --print-all-licenses  print all LICENSE strings used in some package\n"
			"     --print-world-sets    print the world sets\n"
			"     --print-overlay-path  print the path of specified overlay\n"
			"     --print-overlay-label print label of specified overlay\n"
			"\n"
			"   Special:\n"
			"     -t  --test-non-matching Before other output, print non-matching entries\n"
			"                           of /etc/portage/package.* and non-matching names\n"
			"                           of installed packages; this option is best\n"
			"                           combined with -T to clean up /etc/portage/package.*\n"
			"     -Q, --quick (toggle)  don't read unguessable slots of installed packages\n"
			"         --care            always read slots of installed packages\n"
			"         --cache-file      use another cache-file instead of %s\n"
			"\n"
			"   Output:\n"
			"     -q, --quiet (toggle)   (no) output\n"
			"     -n, --nocolor          do not use ANSI color codes\n"
			"     -F, --force-color      force colorful output\n"
			"     -*, --pure-packages    Omit printing of overlay names and package number\n"
			"     --only-names           -* with format <category>/<name>\n"
			"     --xml (toggle)         output results in XML format\n"
			"     -c, --compact (toggle) compact search results\n"
			"     -v, --verbose (toggle) verbose search results\n"
			"     -x, --versionsort  (toggle) sort output by slots/versions\n"
			"     -l, --versionlines (toggle) print available versions line-by-line\n"
			"                            and print IUSE on a per-version base.\n"
			"         --format           format string for normal output\n"
			"         --format-compact   format string for compact output\n"
			"         --format-verbose   format string for verbose output\n"
			"\n"
			"Local:\n"
			"  Miscellaneous:\n"
			"    -I, --installed       Next expression only matches installed packages.\n"
			"    -i, --multi-installed Match packages installed in several versions.\n"
			"    -d, --dup-packages    Match duplicated packages.\n"
			"    -D, --dup-versions    Match packages with duplicated versions.\n"
			"    -1, --slotted         Match packages with a nontrivial slot.\n"
			"    -2, --slots           Match packages with two different slots.\n"
			"    -u, --upgrade[+-]     Match packages without best slotted version.\n"
			"                          +: settings from LOCAL_PORTAGE_CONFIG=true\n"
			"                          -: settings from LOCAL_PORTAGE_CONFIG=false\n"
			"    --stable[+-]          Match packages with a stable version\n"
			"    --testing[+-]         Match packages with a testing or stable version.\n"
			"    --non-masked[+-]      Match packages with a non-masked version.\n"
			"    --system[+-]          Match system packages.\n"
			"    --installed-unstable  Match packages with a non-stable installed version.\n"
			"    --installed-testing   Match packages with a testing    installed version.\n"
			"    --installed-masked    Match packages with a masked     installed version.\n"
			"    --world               Match world packages.\n"
			"    --world-all           Match packages of world or of a world set.\n"
			"    --world-set           Match packages of a world set.\n"
			"    -O, --overlay                        Match packages from overlays.\n"
			"    --in-overlay OVERLAY                 Match packages from OVERLAY.\n"
			"    --only-in-overlay OVERLAY            Match packages only in OVERLAY.\n"
			"    -J, --installed-overlay Match packages installed from overlays.\n"
			"    --installed-from-overlay OVERLAY     Packages installed from OVERLAY.\n"
			"    --installed-in-some-overlay          Packages with an installed version\n"
			"                                         provided by some overlay.\n"
			"    --installed-in-overlay OVERLAY       Packages with an installed version\n"
			"                                         provided from OVERLAY.\n"
			"    --restrict-fetch          Match packages with RESTRICT=fetch.\n"
			"    --restrict-mirror         Match packages with RESTRICT=mirror.\n"
			"    --restrict-primaryuri     Match packages with RESTRICT=primaryuri.\n"
			"    --restrict-binchecks      Match packages with RESTRICT=binchecks.\n"
			"    --restrict-strip          Match packages with RESTRICT=strip.\n"
			"    --restrict-test           Match packages with RESTRICT=test.\n"
			"    --restrict-userpriv       Match packages with RESTRICT=userpriv.\n"
			"    --restrict-installsources Match packages with RESTRICT=installsources.\n"
			"    --restrict-bindist        Match packages with RESTRICT=bindist.\n"
			"    --properties-interactive  Match packages with PROPERTIES=interactive.\n"
			"    --properties-live         Match packages with PROPERTIES=live.\n"
			"    --properties-virtual      Match packages with PROPERTIES=virtual.\n"
			"    --properties-set          Match packages with PROPERTIES=set.\n"
			"    -T, --test-obsolete   Match packages with obsolete entries in\n"
			"                          /etc/portage/package.* (see man eix).\n"
			"                          Use -t to check non-existing packages.\n"
			"    -!, --not (toggle)    Invert the expression.\n"
			"    -|, --pipe            Use input from pipe of emerge -pv\n"
			"\n"
			"  Search Fields:\n"
			"    -S, --description       description\n"
			"    -A, --category-name     \"category/name\"\n"
			"    -C, --category          category\n"
			"    -s, --name              name (default)\n"
			"    -H, --homepage          homepage\n"
			"    -L, --license           license\n"
			"    -P, --provide           provides\n"
			"    --set                   local package set name\n"
			"    --slot                  slot\n"
			"    --installed-slot        slot of installed version\n"
			"    -U, --use               useflag (of the ebuild)\n"
			"    --installed-with-use    enabled useflag (of installed package)\n"
			"    --installed-without-use disabled useflag (of installed package)\n"
			"\n"
			"  Type of Pattern:\n"
			"    -r, --regex           Pattern is a regexp (default)\n"
			"    -e, --exact           Pattern is the exact string\n"
			"    -z, --substring       Pattern is a substring\n"
			"    -b, --begin           Pattern is the beginning of the string\n"
			"        --end             Pattern is the end       of the string\n"
			"    -p, --pattern         Pattern is a wildcards-pattern\n"
			"    -f [m], --fuzzy [m]   Use fuzzy-search with a max. levenshtein-distance m.\n"
			"\n"
			"This program is covered by the GNU General Public License. See COPYING for\n"
			"further information.\n"),
		EIX_CACHEFILE, program_name.c_str());

	if(exit_code != -1) {
		exit(exit_code);
	}
}

static const char *format_normal, *format_verbose, *format_compact;
static const char *eix_cachefile = NULL;
static const char *var_to_print = NULL;
static const char *overlaypath_to_print = NULL;
static const char *overlaylabel_to_print = NULL;

enum OverlayMode
{
	mode_list_used_renumbered  = 0,
	mode_list_used             = 1,
	mode_list_all_if_any       = 2,
	mode_list_all              = 3,
	mode_list_none             = 4
};

static OverlayMode overlay_mode;

static PrintFormat format(get_package_property);

/** Local options for argument reading. */
static struct LocalOptions {
	bool be_quiet,
		 quick,
		 care,
		 verbose_output,
		 compact_output,
		 show_help,
		 show_version,
		 pure_packages,
		 only_names,
		 dump_eixrc,
		 dump_defaults,
		 xml,
		 test_unused,
		 do_debug,
		 ignore_etc_portage,
		 is_current,
		 hash_iuse,
		 hash_keywords,
		 hash_slot,
		 hash_provide,
		 hash_license,
		 world_sets;
} rc_options;

/** Arguments and shortopts. */
static struct Option long_options[] = {
	// Global options
	Option("quiet",        'q',     Option::BOOLEAN,       &rc_options.be_quiet),
	Option("quick",        'Q',     Option::BOOLEAN,       &rc_options.quick),
	Option("care",         O_CARE,  Option::BOOLEAN_T,     &rc_options.care),

	Option("nocolor",      'n',     Option::BOOLEAN_T,     &format.no_color),
	Option("force-color",  'F',     Option::BOOLEAN_F,     &format.no_color),
	Option("versionlines", 'l',     Option::BOOLEAN,       &format.style_version_lines),
	Option("versionsort",  'x',     Option::BOOLEAN,       &format.slot_sorted),
	Option("pure-packages",'*',     Option::BOOLEAN,       &rc_options.pure_packages),
	Option("only-names",O_ONLY_NAMES,Option::BOOLEAN,      &rc_options.only_names),

	Option("verbose",      'v',     Option::BOOLEAN,       &rc_options.verbose_output),
	Option("compact",      'c',     Option::BOOLEAN,       &rc_options.compact_output),
	Option("xml",          O_XML,   Option::BOOLEAN,       &rc_options.xml),
	Option("help",         'h',     Option::BOOLEAN_T,     &rc_options.show_help),
	Option("version",      'V',     Option::BOOLEAN_T,     &rc_options.show_version),
	Option("dump",         O_DUMP,  Option::BOOLEAN_T,     &rc_options.dump_eixrc),
	Option("dump-defaults",O_DUMP_DEFAULTS,Option::BOOLEAN_T,&rc_options.dump_defaults),
	Option("test-non-matching",'t', Option::BOOLEAN_T,     &rc_options.test_unused),
	Option("debug",        O_DEBUG, Option::BOOLEAN_T,     &rc_options.do_debug),

	Option("is-current",   O_CURRENT, Option::BOOLEAN_T,   &rc_options.is_current),

	Option("print-all-useflags", O_HASH_IUSE,     Option::BOOLEAN_T, &rc_options.hash_iuse),
	Option("print-all-keywords", O_HASH_KEYWORDS, Option::BOOLEAN_T, &rc_options.hash_keywords),
	Option("print-all-slots",    O_HASH_SLOT,     Option::BOOLEAN_T, &rc_options.hash_slot),
	Option("print-all-provides", O_HASH_PROVIDE,  Option::BOOLEAN_T, &rc_options.hash_provide),
	Option("print-all-licenses", O_HASH_LICENSE,  Option::BOOLEAN_T, &rc_options.hash_license),
	Option("print-world-sets",   O_WORLD_SETS,    Option::BOOLEAN_T, &rc_options.world_sets),

	Option("ignore-etc-portage",  O_IGNORE_ETC_PORTAGE, Option::BOOLEAN_T,  &rc_options.ignore_etc_portage),

	Option("print",        O_PRINT_VAR,     Option::STRING,   &var_to_print),

	Option("print-overlay-path",   O_PRINT_OPATH, Option::STRING, &overlaypath_to_print),
	Option("print-overlay-label", O_PRINT_OLABEL, Option::STRING, &overlaylabel_to_print),

	Option("format",         O_FMT,         Option::STRING,   &format_normal),
	Option("format-verbose", O_FMT_VERBOSE, Option::STRING,   &format_verbose),
	Option("format-compact", O_FMT_COMPACT, Option::STRING,   &format_compact),

	Option("cache-file",     O_EIX_CACHEFILE,Option::STRING,  &eix_cachefile),

	// Options for criteria
	Option("installed",     'I'),
	Option("multi-installed",'i'),
	Option("slotted",       '1'),
	Option("slots",         '2'),
	Option("upgrade",       'u'),
	Option("upgrade+",      O_UPGRADE_LOCAL),
	Option("upgrade-",      O_UPGRADE_NONLOCAL),
	Option("stable",        O_STABLE_DEFAULT),
	Option("testing",       O_TESTING_DEFAULT),
	Option("non-masked",    O_NONMASKED_DEFAULT),
	Option("world",         O_WORLD),
	Option("world-all",     O_WORLD_ALL),
	Option("world-set",     O_WORLD_SET),
	Option("system",        O_SYSTEM_DEFAULT),
	Option("stable+",       O_STABLE_LOCAL),
	Option("testing+",      O_TESTING_LOCAL),
	Option("non-masked+",   O_NONMASKED_LOCAL),
	Option("system+",       O_SYSTEM_LOCAL),
	Option("stable-",       O_STABLE_NONLOCAL),
	Option("testing-",      O_TESTING_NONLOCAL),
	Option("non-masked-",   O_NONMASKED_NONLOCAL),
	Option("system-",       O_SYSTEM_NONLOCAL),
	Option("installed-unstable", O_INSTALLED_UNSTABLE),
	Option("installed-testing",  O_INSTALLED_TESTING),
	Option("installed-masked",   O_INSTALLED_MASKED),
	Option("overlay",              'O'),
	Option("installed-overlay",    'J'),
	Option("installed-from-overlay",O_FROM_OVERLAY,     Option::KEEP_STRING_OPTIONAL),
	Option("in-overlay",           O_OVERLAY,           Option::KEEP_STRING_OPTIONAL),
	Option("only-in-overlay",      O_ONLY_OVERLAY,      Option::KEEP_STRING_OPTIONAL),
	Option("installed-in-some-overlay", O_INSTALLED_SOME),
	Option("installed-in-overlay", O_INSTALLED_OVERLAY, Option::KEEP_STRING_OPTIONAL),
	Option("restrict-fetch",         O_RESTRICT_FETCH),
	Option("restrict-mirror",        O_RESTRICT_MIRROR),
	Option("restrict-primaryuri",    O_RESTRICT_PRIMARYURI),
	Option("restrict-binchecks",     O_RESTRICT_BINCHECKS),
	Option("restrict-strip",         O_RESTRICT_STRIP),
	Option("restrict-test",          O_RESTRICT_TEST),
	Option("restrict-userpriv",      O_RESTRICT_USERPRIV),
	Option("restrict-installsources",O_RESTRICT_INSTALLSOURCES),
	Option("restrict-bindist",       O_RESTRICT_BINDIST),
	Option("properties-interactive", O_PROPERTIES_INTERACTIVE),
	Option("properties-live",        O_PROPERTIES_LIVE),
	Option("properties-virtual",     O_PROPERTIES_VIRTUAL),
	Option("properties-set",         O_PROPERTIES_SET),
	Option("dup-packages",  'd'),
	Option("dup-versions",  'D'),
	Option("test-obsolete", 'T'),
	Option("not",           '!'),
	Option("pipe",          '|'),

	// Algorithms for a criteria
	Option("fuzzy",         'f'),
	Option("regex",         'r'),
	Option("exact",         'e'),
	Option("pattern",       'p'),
	Option("begin",         'b'),
	Option("substring",     'z'),
	Option("end",           O_END_ALGO),

	// What to match in this criteria
	Option("name",          's'),
	Option("slot",          O_SEARCH_SLOT),
	Option("installed-slot", O_INSTALLED_SLOT),
	Option("category",      'C'),
	Option("category-name", 'A'),
	Option("description",   'S'),
	Option("license",       'L'),
	Option("homepage",      'H'),
	Option("provide",       'P'),
	Option("set",           O_SEARCH_SET),
	Option("use",           'U'),
	Option("installed-with-use",    O_INSTALLED_WITH_USE),
	Option("installed-without-use", O_INSTALLED_WITHOUT_USE),

	// What to do with the next one
	Option("or",            'o'),
	Option("and",           'a'),

	Option(0 , 0)
};

/** Setup default values for all global variables. */
static void
setup_defaults()
{
	EixRc &rc = get_eixrc(NULL);

	// Setup defaults
	(void) memset(&rc_options, 0, sizeof(rc_options));

	rc_options.quick           = rc.getBool("QUICKMODE");
	rc_options.be_quiet        = rc.getBool("QUIETMODE");
	rc_options.care            = rc.getBool("CAREMODE");

	format_verbose             = rc["FORMAT_VERBOSE"].c_str();
	format_compact             = rc["FORMAT_COMPACT"].c_str();
	format_normal              = rc["FORMAT"].c_str();
	string s                   = rc["DEFAULT_FORMAT"];
	if((strcasecmp(s.c_str(), "FORMAT_VERBOSE") == 0) ||
	   (strcasecmp(s.c_str(), "verbose") == 0)) {
		rc_options.verbose_output = true;
	}
	else if((strcasecmp(s.c_str(), "FORMAT_COMPACT") == 0) ||
	   (strcasecmp(s.c_str(), "compact") == 0)) {
		rc_options.compact_output = true;
	}
	format.setupResources(rc);
	format.no_color            = !isatty(1) && !rc.getBool("FORCE_USECOLORS");
	format.style_version_lines = rc.getBool("STYLE_VERSION_LINES");
	format.slot_sorted         = !rc.getBool("STYLE_VERSION_SORTED");
	format.recommend_mode      = rc.getLocalMode("RECOMMEND_LOCAL_MODE");

	string overlay = rc["OVERLAYS_LIST"];
	if(overlay.find("if") != string::npos)
		overlay_mode = mode_list_all_if_any;
	else if(overlay.find("number") != string::npos)
		overlay_mode = mode_list_used_renumbered;
	else if(overlay.find("used") != string::npos)
		overlay_mode = mode_list_used;
	else if((overlay.find("no") != string::npos) ||
		(overlay.find("false") != string::npos))
		overlay_mode = mode_list_none;
	else
		overlay_mode = mode_list_all;
}

static bool
print_overlay_table(PrintFormat &fmt, DBHeader &header, vector<bool> *overlay_used)
{
	bool printed_overlay = false;
	for(Version::Overlay i = (overlay_mode == mode_list_all) ? 0 : 1;
		i != header.countOverlays(); ++i) {
		if(i && overlay_used) {
			if(!((*overlay_used)[i-1]))
				continue;
		}
		cout << fmt.overlay_keytext(i) << " ";
		cout << header.getOverlay(i).human_readable() << "\n";
		printed_overlay = true;
	}
	return printed_overlay;
}

static void
set_format()
{
	string varname;
	try {
		if(rc_options.verbose_output) {
			varname = "FORMAT_VERBOSE";
			format.setFormat(format_verbose);
		}
		else if(rc_options.compact_output) {
			varname = "FORMAT_COMPACT";
			format.setFormat(format_compact);
		}
		else {
			varname = "FORMAT";
			format.setFormat(format_normal);
		}
	}
	catch(const ExBasic &e) {
		cerr << eix::format(_("Problems while parsing %s: %s\n"))
			% varname % e << endl;
		exit(1);
	}
}

int
run_eix(int argc, char** argv)
{
	EixRc &eixrc = get_eixrc(EIX_VARS_PREFIX);

	// Setup defaults for all global variables like rc_options
	setup_defaults();

	// Read our options from the commandline.
	ArgumentReader argreader(argc, argv, long_options);

	if(var_to_print) {
		eixrc.print_var(var_to_print);
		exit(0);
	}

	string cachefile;
	if(eix_cachefile)
		cachefile = eix_cachefile;
	else
		cachefile = eixrc["EIX_CACHEFILE"];

	// Only check if the versions uses the current layout
	if(rc_options.is_current) {
		return is_current_dbversion(cachefile.c_str());
	}

	// Dump eixrc-stuff
	if(rc_options.dump_eixrc || rc_options.dump_defaults) {
		eixrc.dumpDefaults(stdout, rc_options.dump_defaults);
		exit(0);
	}

	// Show help screen
	if(rc_options.show_help) {
		dump_help(0);
	}

	// Show version
	if(rc_options.show_version) {
		dump_version(0);
	}

	// Honour a STFU
	if(rc_options.be_quiet) {
		close(1);
	}

	if(rc_options.only_names) {
		rc_options.pure_packages = true;
		format.setFormat("<category>/<name>");
	}
	else
		set_format();

	format.setupColors();

	if(rc_options.pure_packages) {
		overlay_mode = mode_list_none;
	}

	PortageSettings portagesettings(eixrc, true, false);

	string var_db_pkg = eixrc["EPREFIX_INSTALLED"] + VAR_DB_PKG;
	VarDbPkg varpkg_db(var_db_pkg, !rc_options.quick, rc_options.care,
		eixrc.getBool("RESTRICT_INSTALLED"),
		eixrc.getBool("CARE_RESTRICT_INSTALLED"));
	varpkg_db.check_installed_overlays = eixrc.getBoolText("CHECK_INSTALLED_OVERLAYS", "repository");

	MarkedList *marked_list = NULL;
	Matchatom *query;

	/* Open database file */
	FILE *fp = fopen(cachefile.c_str(), "rb");
	if(!fp) {
		cerr << eix::format(_(
			"Can't open the database file %s for reading (mode = 'rb')\n"
			"Did you forget to create it with 'eix-update'?"))
			% cachefile << endl;
		exit(1);
	}

	DBHeader header;

	if(!io::read_header(fp, header)) {
		fclose(fp);
		cerr << eix::format(_(
			"%s was created with an incompatible eix-update:\n"
			"It uses database format %s (current is %s).\n"
			"Please run 'eix-update' and try again."))
			% cachefile % header.version % DBHeader::current
			<< endl;
		exit(1);
	}
	portagesettings.store_world_sets(&(header.world_sets));

	if(rc_options.hash_iuse) {
		fclose(fp);
		header.iuse_hash.output();
		exit(0);
	}
	if(rc_options.hash_keywords) {
		fclose(fp);
		header.keywords_hash.output();
		exit(0);
	}
	if(rc_options.hash_slot) {
		fclose(fp);
		header.slot_hash.output();
		exit(0);
	}
	if(rc_options.hash_provide) {
		fclose(fp);
		header.provide_hash.output();
		exit(0);
	}
	if(rc_options.hash_license) {
		fclose(fp);
		header.license_hash.output();
		exit(0);
	}
	if(rc_options.world_sets) {
		fclose(fp);
		const vector<string> *p = portagesettings.get_world_sets();
		for(vector<string>::const_iterator it = p->begin(); it != p->end(); ++it)
			cout << *it << "\n";
		exit(0);
	}

	LocalMode local_mode = LOCALMODE_DEFAULT;
	if(!eixrc.getBool("LOCAL_PORTAGE_CONFIG")) {
		rc_options.ignore_etc_portage = true;
		local_mode = LOCALMODE_NONLOCAL;
	}
	else if(!rc_options.ignore_etc_portage) {
		local_mode = LOCALMODE_LOCAL;
	}
	// Save lot of time: avoid redundant remasking
	if(format.recommend_mode == local_mode)
		format.recommend_mode = LOCALMODE_DEFAULT;

	SetStability stability(&portagesettings, !rc_options.ignore_etc_portage, false, eixrc.getBool("ALWAYS_ACCEPT_KEYWORDS"));

	query = parse_cli(eixrc, varpkg_db, portagesettings, stability, header, &marked_list, argreader.begin(), argreader.end());

	eix::ptr_list<Package> matches;
	eix::ptr_list<Package> all_packages;

	PackageReader reader(fp, header, &portagesettings);
	if(overlaypath_to_print || overlaylabel_to_print) {
		fclose(fp);
		Version::Overlay num;
		const char *osearch = overlaypath_to_print;
		bool print_path = osearch;
		if(!print_path)
			osearch = overlaylabel_to_print;
		if(!header.find_overlay(&num, osearch, NULL, 0, DBHeader::OVTEST_ALL))
			exit(1);
		const OverlayIdent& overlay = header.getOverlay(num);
		if(print_path)
			cout << overlay.path;
		else
			cout << overlay.label;
		exit(0);
	}
	while(reader.next())
	{
		if(query->match(&reader))
		{
			Package *release=reader.release();
			matches.push_back(release);
			if(rc_options.test_unused)
				all_packages.push_back(release);
		}
		else
		{
			if(rc_options.test_unused)
				all_packages.push_back(reader.release());
			else
				reader.skip();
		}
	}
	fclose(fp);
	if(rc_options.test_unused)
	{
		bool empty = eixrc.getBool("TEST_FOR_EMPTY");
		cout << "\n";
		if(eixrc.getBool("TEST_KEYWORDS"))
			print_unused(eixrc.m_eprefixconf + USER_KEYWORDS_FILE,
				eixrc["KEYWORDS_NONEXISTENT"],
				all_packages);
		if(eixrc.getBool("TEST_MASK"))
			print_unused(eixrc.m_eprefixconf + USER_MASK_FILE,
				eixrc["MASK_NONEXISTENT"],
				all_packages);
		if(eixrc.getBool("TEST_UNMASK"))
			print_unused(eixrc.m_eprefixconf + USER_UNMASK_FILE,
				eixrc["UNMASK_NONEXISTENT"],
				all_packages);
		if(eixrc.getBool("TEST_USE"))
			print_unused(eixrc.m_eprefixconf + USER_USE_FILE,
				eixrc["USE_NONEXISTENT"],
				all_packages, empty);
		if(eixrc.getBool("TEST_CFLAGS"))
			print_unused(eixrc.m_eprefixconf + USER_CFLAGS_FILE,
				eixrc["CFLAGS_NONEXISTENT"],
				all_packages, empty);
		if(eixrc.getBool("TEST_REMOVED"))
			print_removed(var_db_pkg, eixrc["INSTALLED_NONEXISTENT"], all_packages);
	}

	/* Sort the found matches by rating */
	if(FuzzyAlgorithm::sort_by_levenshtein()) {
		matches.sort(FuzzyAlgorithm::compare);
	}

	format.set_marked_list(marked_list);
	if(overlay_mode != mode_list_used_renumbered)
		format.set_overlay_translations(NULL);
	if(header.countOverlays())
	{
		format.clear_virtual(header.countOverlays());
		for(Version::Overlay i = 1; i != header.countOverlays(); i++)
			format.set_as_virtual(i, is_virtual((eixrc["EPREFIX_VIRTUAL"] + header.getOverlay(i).path).c_str()));
	}
	bool need_overlay_table = false;
	vector<bool> overlay_used(header.countOverlays(), false);
	format.set_overlay_used(&overlay_used, &need_overlay_table);
	eix::ptr_list<Package>::size_type count;
	bool only_printed = eixrc.getBool("COUNT_ONLY_PRINTED");
	if(only_printed)
		count = 0;
	else
		count = matches.size();
	PrintXml *print_xml = NULL;
	if(rc_options.xml) {
		overlay_mode = mode_list_none;
		rc_options.pure_packages = true;

		print_xml = new PrintXml(&header, &varpkg_db, &stability, &eixrc,
			portagesettings["PORTDIR"]);
		print_xml->start();
	}
	for(eix::ptr_list<Package>::iterator it = matches.begin();
		it != matches.end(); ++it) {

		stability.set_stability(**it);

		if(rc_options.xml) {
			print_xml->package(*it);
			continue;
		}

		if(it->largest_overlay) {
			need_overlay_table = true;
			if(overlay_mode <= mode_list_used) {
				for(Package::iterator ver = it->begin();
					ver != it->end(); ++ver) {
					Version::Overlay key = ver->overlay_key;
					if(key>0)
						overlay_used[key - 1] = true;
				}
			}
		}
		if(overlay_mode != mode_list_used_renumbered) {
			if(format.print(*it, &header, &varpkg_db, &portagesettings, &stability)) {
				if(only_printed)
					count++;
			}
		}
	}
	switch(overlay_mode) {
		case mode_list_all:  need_overlay_table = true;  break;
		case mode_list_none: need_overlay_table = false; break;
		default: break;
	}
	vector<Version::Overlay> overlay_num(header.countOverlays(), 0);
	if(overlay_mode == mode_list_used_renumbered) {
		Version::Overlay i = 1;
		vector<bool>::iterator  uit = overlay_used.begin();
		vector<Version::Overlay>::iterator nit = overlay_num.begin();
		for(; uit != overlay_used.end(); ++uit, ++nit)
			if(*uit == true)
				*nit = i++;
		format.set_overlay_translations(&overlay_num);
		for(eix::ptr_list<Package>::iterator it = matches.begin();
			it != matches.end();
			++it) {
			if(format.print(*it, &header, &varpkg_db, &portagesettings, &stability)) {
				if(only_printed)
					count++;
			}
		}
	}
	bool printed_overlay = false;
	if(need_overlay_table)
	{
		printed_overlay = print_overlay_table(format, header,
			(overlay_mode <= mode_list_used)? &overlay_used : NULL);
	}
	if(print_xml) {
		print_xml->finish();
		delete print_xml;
	}

	short print_count_always = eixrc.getBoolText("PRINT_COUNT_ALWAYS", "never");
	if((print_count_always >= 0) && !rc_options.pure_packages)
	{
		if(!count) {
			if(print_count_always)
				cout << eix::format(_("Found %s matches.\n"))
					% eix::ptr_list<Package>::size_type(0);
			else
				cout << _("No matches found.\n");
		}
		else if(count == 1) {
			if(print_count_always) {
				if(printed_overlay)
					cout << "\n";
				cout << eix::format(_("Found %s match.\n"))
					% eix::ptr_list<Package>::size_type(1);
			}
		}
		else {
			if(printed_overlay)
				cout << "\n";
			cout <<  eix::format(_("Found %s matches.\n")) % count;
		}
	}

	// Delete old query
	delete query;

	// Delete matches
	matches.delete_and_clear();

	return EXIT_SUCCESS;
}

static int
is_current_dbversion(const char *filename) {
	DBHeader header;
	FILE *fp = fopen(filename, "rb");
	if(!fp)
	{
		cerr << eix::format(_(
			"Can't open the database file %s for reading (mode = 'rb')\n"
			"Did you forget to create it with 'eix-update'?"))
			% filename << endl;
		return 1;
	}
	bool is_current = io::read_header(fp, header);
	fclose(fp);

	return (is_current ? 0 : 1);
}

static void
print_vector(const vector<string> &vec)
{
	for(vector<string>::const_iterator it=vec.begin(); it != vec.end(); it++)
		cout << *it << "\n";
	cout << "--\n\n";
}

static void
print_unused(const string &filename, const string &excludefiles, const eix::ptr_list<Package> &packagelist, bool test_empty)
{
	vector<string> unused;
	vector<string> lines;
	set<string> excludes;
	bool know_excludes = false;
	pushback_lines(filename.c_str(), &lines, false, true);
	for(vector<string>::iterator i(lines.begin());
		i != lines.end();
		i++)
	{
		if(i->empty())
			continue;
		if(!know_excludes) {
			know_excludes = true;
			vector<string> excludelist = split_string(excludefiles, true);
			for(vector<string>::const_iterator it = excludelist.begin();
				it != excludelist.end(); ++it) {
				vector<string> excl;
				pushback_lines(it->c_str(), &excl, false, true);
				insert_list(excludes, split_string(join_vector(excl)));
			}
		}

		KeywordMask *m = NULL;

		try {
			string::size_type n = i->find_first_of("\t ");
			if(n == string::npos) {
				if(excludes.find(*i) != excludes.end())
					continue;
				if(test_empty)
				{
					unused.push_back(*i);
					continue;
				}
				m = new KeywordMask(i->c_str());
			}
			else {
				string it = i->substr(0, n);
				if(excludes.find(it) != excludes.end())
					continue;
				m = new KeywordMask(it.c_str());
			}
		}
		catch(const ExBasic &e) {
			portage_parse_error(filename, lines.begin(), i, e);
		}
		if(!m)
			continue;

		eix::ptr_list<Package>::const_iterator pi;
		for(pi = packagelist.begin(); pi != packagelist.end(); ++pi)
		{
			if(m->ismatch(**pi))
				break;
		}
		delete m;
		if(pi != packagelist.end())
			continue;
		unused.push_back(*i);
	}
	if(unused.empty()) {
		cout << eix::format(test_empty ?
			_("No non-matching or empty entries in %s.\n") :
			_("No non-matching entries in %s.\n") )
			% filename;
		return;
	}
	cout << eix::format(test_empty ?
		_("Non-matching or empty entries in %s:\n\n") :
		_("Non-matching entries in %s:\n\n"))
		% filename;
	print_vector(unused);
}

static void
print_removed(const string &dirname, const string &excludefiles, const eix::ptr_list<Package> &packagelist)
{
	/* For faster testing, we build a category->name set */
	map< string, set<string> > cat_name;
	for(eix::ptr_list<Package>::const_iterator pit = packagelist.begin();
		pit != packagelist.end(); ++pit )
		cat_name[pit->category].insert(pit->name);

	/* This will contain categories/packages to be printed */
	vector<string> failure;

	/* Read all installed packages (not versions!) and fill failures */
	set<string> excludes;
	bool know_excludes = false;
	vector<string> categories;
	pushback_files(dirname, categories, NULL, 2, true, false);
	for(vector<string>::const_iterator cit = categories.begin();
		cit != categories.end(); ++cit )
	{
		vector<string> names;
		string cat_slash = *cit + "/";
		pushback_files(dirname + cat_slash, names, NULL, 2, true, false);
		map< string, set<string> >::const_iterator cat = cat_name.find(*cit);
		const set<string> *ns = ( (cat == cat_name.end()) ? NULL : &(cat->second) );
		for(vector<string>::const_iterator nit = names.begin();
			nit != names.end(); ++nit )
		{
			char *name = ExplodeAtom::split_name(nit->c_str());
			if(!name)
				continue;
			if((!ns) || (ns->find(name) == ns->end())) {
				if(!know_excludes) {
					know_excludes = true;
					vector<string> excludelist = split_string(excludefiles, true);
					for(vector<string>::const_iterator it = excludelist.begin();
						it != excludelist.end(); ++it) {
						vector<string> excl;
						pushback_lines(it->c_str(), &excl, false, true);
						insert_list(excludes, split_string(join_vector(excl)));
					}
				}
				if(excludes.find(name) == excludes.end()) {
					string fullname = cat_slash + name;
					if(excludes.find(fullname) == excludes.end())
						failure.push_back(cat_slash + name);
				}
			}
			free(name);
		}
	}
	if(failure.empty())
	{
		cout << _("The names of all installed packages are in the database.\n\n");
		return;
	}
	cout << _("The following installed packages are not in the database:\n\n");
	print_vector(failure);
	return;
}
