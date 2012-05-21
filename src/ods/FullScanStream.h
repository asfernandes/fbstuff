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

#ifndef FBSTUFF_ODS_FULL_SCAN_STREAM_H
#define FBSTUFF_ODS_FULL_SCAN_STREAM_H

#include "Database.h"
#include <string>
#include <map>
#include <boost/scoped_array.hpp>

namespace fbods
{

//------------------------------------------------------------------------------


class FullScanStream
{
public:
	FullScanStream(Database* aDatabase, Database::RelationId aRelationId);
	FullScanStream(Database* aDatabase, const char* relationName);

private:
	void init();

public:
	bool fetch(void* recordBuffer);

private:
	void readPointer();

private:
	Database* database;
	Database::RelationId relationId;
	bool first;
	unsigned pointerNum;
	unsigned dataNum;
	boost::scoped_array<char> pointerScope;
	boost::scoped_array<char> dataScope;
	PointerPage* pointer;
	DataPage* data;
};


//------------------------------------------------------------------------------

}	// fbods

#endif	// FBSTUFF_ODS_FULL_SCAN_STREAM_H
