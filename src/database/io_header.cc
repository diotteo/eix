// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include <config.h>

#include <sys/types.h>

#include <cstring>

#include <string>
#include <vector>

#include "database/header.h"
#include "database/io.h"
#include "eixTk/auto_list.h"
#include "eixTk/diagnostics.h"
#include "eixTk/eixint.h"
#include "eixTk/formated.h"
#include "eixTk/i18n.h"
#include "eixTk/likely.h"
#include "eixTk/null.h"
#include "eixTk/stringutils.h"
#include "portage/depend.h"
#include "portage/extendedversion.h"

using std::string;
using std::vector;

bool Database::read_header(DBHeader *hdr, string *errtext) {
	size_t magic_len(strlen(DBHeader::magic));
	eix::auto_list<char> buf(new char[magic_len + 1]);
	buf.get()[magic_len] = 0;
	if(unlikely(!read_string_plain(buf.get(), magic_len, errtext))) {
		return false;
	}
	if(unlikely(strcmp(DBHeader::magic, buf.get()) != 0)) {
		char c(buf.get()[0]);
		// Until version 30 the first char is the version:
GCC_DIAG_OFF(sign-conversion)
		hdr->version = (((c > 0) && (c <= 30)) ? c : 0);
GCC_DIAG_ON(sign-conversion)
	} else if(unlikely(!read_num(&(hdr->version), errtext))) {
		return false;
	}
	if(unlikely(!hdr->isCurrent())) {
		if(errtext != NULLPTR) {
			*errtext = eix::format((hdr->version > DBHeader::current) ?
			_("cachefile uses newer format %s (current is %s)") :
			_("cachefile uses obsolete format %s (current is %s)"))
			% hdr->version % DBHeader::current;
		}
		return false;
	}

	if(unlikely(!read_num(&(hdr->size), errtext))) {
		return false;
	}

	ExtendedVersion::Overlay overlay_sz;
	if(unlikely(!read_num(&(overlay_sz), errtext))) {
		return false;
	}
	for(; likely(overlay_sz != 0); --overlay_sz) {
		string path;
		if(unlikely(!read_string(&path, errtext))) {
			return false;
		}
		string ov;
		if(unlikely(!read_string(&ov, errtext))) {
			return false;
		}
		hdr->addOverlay(OverlayIdent(path.c_str(), ov.c_str()));
	}

	if(unlikely(!read_hash(&(hdr->license_hash), errtext))) {
		return false;
	}
	if(unlikely(!read_hash(&(hdr->keywords_hash), errtext))) {
		return false;
	}
	if(unlikely(!read_hash(&(hdr->iuse_hash), errtext))) {
		return false;
	}
	if(unlikely(!read_hash(&(hdr->slot_hash), errtext))) {
		return false;
	}

	vector<string>::size_type sets_sz;
	if(unlikely(!read_num(&sets_sz, errtext))) {
		return false;
	}
	for(; likely(sets_sz != 0); --sets_sz) {
		string s;
		if(unlikely(!read_string(&s, errtext))) {
			return false;
		}
		hdr->world_sets.push_back(s);
	}

	eix::UNumber use_dep_num;
	if(unlikely(!read_num(&use_dep_num, errtext))) {
		return false;
	}
	if((hdr->use_depend = (use_dep_num != 0))) {
		eix::OffsetType len;
		if(unlikely(!read_num(&len, errtext))) {
			return false;
		}
		if(Depend::use_depend) {
			if(unlikely(!read_hash(&(hdr->depend_hash), errtext))) {
				return false;
			}
		} else if(len != 0) {
			if(unlikely(!seekrel(len, errtext))) {
				return false;
			}
		}
	}
	return true;
}

bool Database::read_hash(StringHash *hash, string *errtext) {
	hash->init(false);
	StringHash::size_type i;
	if(unlikely(!read_num(&i, errtext))) {
		return false;
	}
	for(; likely(i != 0); --i) {
		string s;
		if(unlikely(!read_string(&s, errtext))) {
			return false;
		}
		hash->store_string(s);
	}
	hash->finalize();
	return true;
}
