// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include <config.h>

#include <cstdlib>
#include <ctime>

#include <string>

#include "cache/base.h"
#include "cache/common/ebuild_exec.h"
#include "cache/common/flat_reader.h"
#include "cache/common/selectors.h"
#include "cache/metadata/metadata.h"
#include "cache/parse/parse.h"
#include "eixTk/formated.h"
#include "eixTk/i18n.h"
#include "eixTk/likely.h"
#include "eixTk/md5.h"
#include "eixTk/null.h"
#include "eixTk/stringtypes.h"
#include "eixTk/stringutils.h"
#include "eixTk/sysutils.h"
#include "eixTk/varsreader.h"
#include "portage/basicversion.h"
#include "portage/depend.h"
#include "portage/extendedversion.h"
#include "portage/package.h"
#include "portage/packagetree.h"
#include "portage/version.h"

using std::string;

bool ParseCache::initialize(const string& name) {
	WordVec names;
	split_string(&names, name, true, "#");
	if(unlikely(names.empty())) {
		return false;
	}
	WordVec::const_iterator it_name(names.begin());
	WordVec s;
	split_string(&s, *it_name, false, "|");
	if(unlikely(s.empty())) {
		return false;
	}
	try_parse = ebuild_sh = nosubst = false;
	bool try_ebuild(false), use_sh(false);
	for(WordVec::const_iterator it(s.begin()); likely(it != s.end()); ++it) {
		if(*it == "parse") {
			try_parse = true;
			nosubst = false;
		} else if(*it == "parse*") {
			try_parse = true;
			nosubst = true;
		} else if(*it == "ebuild") {
			try_ebuild = true;
			use_sh = false;
		} else if(*it == "ebuild*") {
			try_ebuild = true;
			use_sh = true;
		} else {
			return false;
		}
	}
	if(try_ebuild) {
		ebuild_sh = use_sh;
		ebuild_exec = new EbuildExec(use_sh, this);
	}
	while(++it_name != names.end()) {
		MetadataCache *p(new MetadataCache);
		if(p->initialize(*it_name)) {
			further.push_back(p);
			continue;
		}
		delete p;
		return false;
	}
	return true;
}

const char *ParseCache::getType() const {
	static string *s = NULLPTR;
	if(s == NULLPTR) {
		s = new string;
	} else {
		s->clear();
	}
	if(try_parse) {
		if(nosubst) {
			s->assign("parse*");
		} else {
			s->assign("parse");
		}
	}
	if(ebuild_exec != NULLPTR) {
		const char *t;
		if(ebuild_sh) {
			t = "ebuild*";
		} else {
			t = "ebuild";
		}
		if(s->empty()) {
			s->assign(t);
		} else {
			s->append(1, '|');
			s->append(t);
		}
	}
	for(FurtherCaches::const_iterator it(further.begin());
		likely(it != further.end()); ++it) {
		s->append(1, '#');
		s->append((*it)->getType());
	}
	return s->c_str();
}

ParseCache::~ParseCache() {
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		delete *it;
	}
	if(ebuild_exec != NULLPTR) {
		ebuild_exec->delete_cachefile();
		delete ebuild_exec;
		ebuild_exec = NULLPTR;
	}
}

void ParseCache::setScheme(const char *prefix, const char *prefixport, const std::string& scheme) {
	BasicCache::setScheme(prefix, prefixport, scheme);
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		(*it)->setScheme(prefix, prefixport, scheme);
	}
}

void ParseCache::setKey(ExtendedVersion::Overlay key) {
	BasicCache::setKey(key);
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		(*it)->setKey(key);
	}
}

void ParseCache::setOverlayName(const std::string& name) {
	BasicCache::setOverlayName(name);
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		(*it)->setOverlayName(name);
	}
}

void ParseCache::setErrorCallback(ErrorCallback error_callback) {
	BasicCache::setErrorCallback(error_callback);
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		(*it)->setErrorCallback(error_callback);
	}
}

void ParseCache::set_checking(string *str, const char *item, const VarsReader& ebuild, bool *ok) {
	bool check((ebuild_exec != NULLPTR) && (ok != NULLPTR) && (*ok));
	const string *s(ebuild.find(item));
	str->clear();
	if(s == NULLPTR) {
		if(check) {
			*ok = false;
		}
		return;
	}
	split_and_join(str, *s);
	if(!check) {
		return;
	}
	if(unlikely((str->find('`') != string::npos) ||
		(str->find("$(") != string::npos))) {
		*ok = false;
	}
}

void ParseCache::parse_exec(const char *fullpath, const string& dirpath, bool read_onetime_info, bool *have_onetime_info, Package *pkg, Version *version) {
	version->overlay_key = m_overlay_key;
	string keywords, restr, props, iuse, slot, eapi;
	bool ok(try_parse);
	if(ok || ebuild_sh) {
		VarsReader::Flags flags(VarsReader::NONE);
		if(!read_onetime_info) {
			flags |= VarsReader::ONLY_KEYWORDS_SLOT;
		}
		WordMap env;
		if(!nosubst) {
			flags |= VarsReader::INTO_MAP | VarsReader::SUBST_VARS;
			env_add_package(&env, *pkg, *version, dirpath, fullpath);
		}
		VarsReader ebuild(flags);
		if(flags & VarsReader::INTO_MAP) {
			ebuild.useMap(&env);
		}
		string errtext;
		if(!ebuild.read(fullpath, &errtext, false)) {
			m_error_callback(eix::format(_("Could not properly parse %s: %s")) % fullpath % errtext);
		}

		bool set_eapi(ebuild_sh);
		if(ok) {
			set_checking(&keywords, "KEYWORDS", ebuild, &ok);
			set_checking(&slot, "SLOT", ebuild, &ok);
			// Empty SLOT is not ok:
			if(ok && (ebuild_exec != NULLPTR) && slot.empty()) {
				ok = false;
			}
			set_checking(&restr, "RESTRICT", ebuild);
			set_checking(&props, "PROPERTIES", ebuild);
			set_checking(&iuse, "IUSE", ebuild, &ok);
			if(Depend::use_depend) {
				string depend, rdepend, pdepend, hdepend;
				set_checking(&depend, "DEPEND", ebuild);
				set_checking(&rdepend, "RDEPEND", ebuild);
				set_checking(&pdepend, "PDEPEND", ebuild);
				set_checking(&hdepend, "HDEPEND", ebuild);
				version->depend.set(depend, rdepend, pdepend, hdepend, true);
			}
			if(read_onetime_info) {
				set_checking(&(pkg->homepage), "HOMEPAGE",    ebuild, &ok);
				set_checking(&(pkg->licenses), "LICENSE",     ebuild, &ok);
				set_checking(&(pkg->desc),     "DESCRIPTION", ebuild, &ok);
				*have_onetime_info = true;
			}
			set_eapi = !ok;
		}
		if(set_eapi) {
			const string *s(ebuild.find("EAPI"));
			if(likely(s != NULLPTR)) {
				eapi = *s;
			} else {
				eapi.assign("0");
			}
		}
	}
	if(verbose) {
		const char *used_type;
		if(ok) {
			used_type = (nosubst ? "parse*" : "parse");
		} else {
			used_type = (ebuild_sh ? "ebuild*" : "ebuild");
		}
		m_error_callback(eix::format("%s/%s-%s: %s") %
			m_catname % pkg->name % version->getFull() %
			used_type);
	}
	if(!ok) {
		string *cachefile(ebuild_exec->make_cachefile(fullpath, dirpath, *pkg, *version, eapi));
		if(likely(cachefile != NULLPTR)) {
			flat_get_keywords_slot_iuse_restrict(*cachefile, &keywords, &slot, &iuse, &restr, &props, &(version->depend), m_error_callback);
			flat_read_file(cachefile->c_str(), pkg, m_error_callback);
			ebuild_exec->delete_cachefile();
		} else {
			m_error_callback(eix::format(_("Could not properly execute %s")) % fullpath);
		}
	}
	version->set_slotname(slot);
	version->set_full_keywords(keywords);
	version->set_restrict(restr);
	version->set_properties(props);
	version->set_iuse(iuse);
	pkg->addVersionFinalize(version);
}

void ParseCache::readPackage(Category *cat, const string& pkg_name, const string& directory_path, const WordVec& files) {
	bool have_onetime_info, have_pkg;

	Package *pkg(cat->findPackage(pkg_name));
	if(pkg != NULLPTR) {
		have_onetime_info = have_pkg = true;
	} else {
		have_onetime_info = have_pkg = false;
		pkg = new Package(m_catname, pkg_name);
	}

	for(WordVec::const_iterator fileit(files.begin());
		likely(fileit != files.end()); ++fileit) {
		string::size_type pos(ebuild_pos(*fileit));
		if(pos == string::npos) {
			continue;
		}

		/* Check if we can split it */
		char *ver(ExplodeAtom::split_version(fileit->substr(0, pos).c_str()));
		if(unlikely(ver == NULLPTR)) {
			m_error_callback(eix::format(_("Can't split filename of ebuild %s/%s")) %
				directory_path % (*fileit));
			continue;
		}

		/* Make version and add it to package. */
		Version *version(new Version);
		string errtext;
		BasicVersion::ParseResult r(version->parseVersion(ver, &errtext));
		if(unlikely(r != BasicVersion::parsedOK)) {
			m_error_callback(errtext);
		}
		if(unlikely(r == BasicVersion::parsedError)) {
			delete version;
			continue;
		}
		pkg->addVersionStart(version);

		string full_path(directory_path + '/' + (*fileit));

		/* For the latest version read/change corresponding data */
		bool read_onetime_info(true);
		if(have_onetime_info) {
			if(*(pkg->latest()) != *version) {
				read_onetime_info = false;
			}
		}

		time_t ebuild_time(0);
		FurtherCaches::const_iterator it(further.begin());
		for(; likely(it != further.end()); ++it) {
			const char *s((*it)->get_md5sum(pkg_name.c_str(), ver));
			if(s != NULLPTR) {
				if(verify_md5sum(full_path.c_str(), s)) {
					break;
				}
				continue;
			}
			time_t t((*it)->get_time(pkg_name.c_str(), ver));
			if(t != 0) {
				if(!ebuild_time) {
					ebuild_time = get_mtime(full_path.c_str());
				}
				if(t >= ebuild_time) {
					break;
				}
			}
		}
		if(it == further.end()) {
			parse_exec(full_path.c_str(), directory_path, read_onetime_info, &have_onetime_info, pkg, version);
		} else {
			if(verbose) {
				m_error_callback(eix::format("%s/%s-%s: %s") %
					m_catname % pkg_name % version->getFull() %
					(*it)->getType());
			}
			(*it)->get_version_info(pkg_name.c_str(), ver, version);
			if(read_onetime_info) {
				(*it)->get_common_info(pkg_name.c_str(), ver, pkg);
				have_onetime_info = true;
			}
		}

		free(ver);
	}

	if(have_onetime_info) {
		if(!have_pkg) {
			cat->addPackage(pkg);
		}
	} else {
		delete pkg;
	}
}

bool ParseCache::readCategoryPrepare(const char *cat_name) {
	m_catname = cat_name;
	further_works.clear();
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		further_works.push_back((*it)->readCategoryPrepare(cat_name));
	}
	m_catpath = m_prefix + m_scheme + '/' + cat_name;
	return scandir_cc(m_catpath, &m_packages, package_selector);
}

void ParseCache::readCategoryFinalize() {
	further_works.clear();
	for(FurtherCaches::iterator it(further.begin());
		likely(it != further.end()); ++it) {
		(*it)->readCategoryFinalize();
	}
	m_catname.clear();
	m_catpath.clear();
	m_packages.clear();
}

bool ParseCache::readCategory(Category *cat) {
	for(WordVec::const_iterator pit(m_packages.begin());
		likely(pit != m_packages.end()); ++pit) {
		string pkg_path(m_catpath + '/' + (*pit));
		WordVec files;
		if(scandir_cc(pkg_path, &files, ebuild_selector)) {
			readPackage(cat, *pit, pkg_path, files);
		}
	}
	return true;
}
