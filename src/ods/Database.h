/*
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is Adriano dos Santos Fernandes.
 * Portions created by the Initial Developer are Copyright (C) 2011 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 */

#ifndef FBSTUFF_ODS_DATABASE_H
#define FBSTUFF_ODS_DATABASE_H

#include "Ods.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <map>

namespace fbods
{

//------------------------------------------------------------------------------


class Database
{
public:
	Database(const char* filename)
		: file(filename, std::ios::in | std::ios::binary)
	{
		if (file.fail())
			throw std::runtime_error(std::string("Cannot open ") + filename);

		file.read(reinterpret_cast<char*>(&header), sizeof(header));

		relationPointer.insert(std::make_pair(RELATION_ID_PAGES, header.pages));
	}

	void readPage(unsigned number, void* data)
	{
		file.seekg(header.pageSize * number, std::ios::beg);
		file.read(static_cast<char*>(data), header.pageSize);
	}

public:
	enum RelationId
	{
		RELATION_ID_PAGES = 0,
		RELATION_ID_INDEX_SEGMENTS = 3,
		RELATION_ID_RELATIONS = 6
	};

	std::fstream file;
	std::map<RelationId, unsigned> relationPointer;
	HeaderPage header;
};

template <int LENGTH>
struct VarChar
{
	boost::uint16_t length;
	char data[LENGTH];
};


//------------------------------------------------------------------------------

}	// fbods

#endif	// FBSTUFF_ODS_DATABASE_H
