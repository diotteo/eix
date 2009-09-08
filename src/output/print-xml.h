// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Bob Shaffer II <bob.shaffer.2 at gmail.com>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#ifndef EIX__PRINT_XML_H__
#define EIX__PRINT_XML_H__ 1

#include <portage/package.h>

class DBHeader;
class VarDbPkg;
class SetStability;

class PrintXml {
	protected:
		bool started;

		const DBHeader *hdr;
		VarDbPkg *var_db_pkg;
		const SetStability *stability;
		std::string portdir;

		eix::ptr_list<Package>::size_type count;
		std::string curcat;

		void clear()
		{ started = false; }

	public:
		void init(const DBHeader *header, VarDbPkg *vardb, const SetStability *set_stability, const std::string &port_dir)
		{
			hdr = header;
			var_db_pkg = vardb;
			stability = set_stability;
			portdir = port_dir;
			clear();
		}

		PrintXml(const DBHeader *header, VarDbPkg *vardb, const SetStability *set_stability, const std::string &port_dir)
		{ init(header, vardb, set_stability, port_dir); }

		PrintXml() : hdr(NULL), var_db_pkg(NULL), stability(NULL)
		{ clear(); }

		void start();
		void package(const Package *pkg);
		void finish();
		static std::string escape_string(const std::string &s);

		~PrintXml()
		{ finish(); }
};

#endif /* EIX__PRINT_XML_H__ */
