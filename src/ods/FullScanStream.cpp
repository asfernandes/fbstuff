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
 * Portions created by the Initial Developer are Copyright (C) 2012 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 */

#include "FullScanStream.h"
#include <map>
#include <stdexcept>
#include <string>
#include <boost/algorithm/string.hpp>

namespace fbods
{

using std::map;
using std::runtime_error;
using std::string;

//------------------------------------------------------------------------------


FullScanStream::FullScanStream(Database* aDatabase, Database::RelationId aRelationId)
	: database(aDatabase),
	  relationId(aRelationId),
	  first(true),
	  pointerNum(0),
	  dataNum(0)
{
	init();
}

FullScanStream::FullScanStream(Database* aDatabase, const char* relationName)
	: database(aDatabase),
	  first(true),
	  pointerNum(0),
	  dataNum(0)
{
	string strRelationName(relationName);

	FullScanStream scan(database, Database::RELATION_ID_RELATIONS);

	struct RdbRelations
	{
		boost::uint32_t nullFlags[1];
		boost::uint64_t viewBlr;
		boost::uint64_t viewSource;
		boost::uint64_t description;
		boost::int16_t relationId;
		boost::int16_t systemFlag;
		boost::int16_t dbkeyLength;
		boost::int16_t format;
		boost::int16_t fieldId;
		char relationName[31];
		char securityClass[31];
		VarChar<255> externalFile;
		boost::uint64_t runtime;
		boost::uint64_t externalDescription;
		char ownerName[31];
		char defaultClass[31];
		boost::int16_t flags;
		boost::int16_t relationType;
	} rdbRelations;

	while (scan.fetch(&rdbRelations))
	{
		string str(rdbRelations.relationName, sizeof(rdbRelations.relationName));
		boost::algorithm::trim(str);

		if (str == strRelationName)
		{
			relationId = static_cast<Database::RelationId>(rdbRelations.relationId);
			init();
			return;
		}
	}

	throw runtime_error(string("Relation ") + relationName + " not found");
}

void FullScanStream::init()
{
	map<Database::RelationId, unsigned>::iterator it = database->relationPointer.find(relationId);
	unsigned firstPointer;

	if (it != database->relationPointer.end())
		firstPointer = it->second;
	else
	{
		FullScanStream scan(database, Database::RELATION_ID_PAGES);

		struct RdbPages
		{
			boost::uint32_t nullFlags[1];
			boost::int32_t pageNumber;
			boost::int16_t relationId;
			boost::int32_t pageSequence;
			boost::int16_t pageType;
		} rdbPages;

		firstPointer = 0;

		while (scan.fetch(&rdbPages))
		{
			if (rdbPages.relationId == static_cast<boost::int16_t>(relationId) &&
				rdbPages.pageType == PageHeader::TYPE_POINTER &&
				rdbPages.pageSequence == 0)
			{
				firstPointer = rdbPages.pageNumber;
				break;
			}
		}

		if (firstPointer == 0)
		{
			char s[32];
			sprintf(s, "%d", static_cast<boost::int16_t>(relationId));
			throw runtime_error(string("Pointer page for relation id ") + s +
				" has not been found");
		}
	}

	pointerScope.reset(new char[database->header.pageSize]);
	dataScope.reset(new char[database->header.pageSize]);

	pointer = reinterpret_cast<PointerPage*>(pointerScope.get());
	data = reinterpret_cast<DataPage*>(dataScope.get());

	database->readPage(firstPointer, pointer);
	readPointer();
}

bool FullScanStream::fetch(void* recordBuffer)
{
	boost::uint8_t* raw = reinterpret_cast<boost::uint8_t*>(data);
	RecordHeader* record;

	do
	{
		if (first)
			first = false;
		else
			++dataNum;

		while (!(dataNum < data->count))
		{
			if (++pointerNum < pointer->count)
			{
				dataNum = 0;
				readPointer();
			}
			else if (pointer->next != 0)
			{
				pointerNum = 0;
				dataNum = 0;

				database->readPage(pointer->next, pointer);
				readPointer();
			}
			else
				return false;
		}

		record = reinterpret_cast<RecordHeader*>(&raw[data->rpt[dataNum].offset]);
	} while (record->backPage != 0 ||
		(record->flags & (RecordHeader::FLAG_DELETED | RecordHeader::FLAG_BLOB)));

	boost::uint8_t* recordStart = record->data;
	boost::uint8_t* recordEnd = record->data + data->rpt[dataNum].length -
		offsetof(RecordHeader, data);

	boost::uint8_t* pt = static_cast<boost::uint8_t*>(recordBuffer);

	for (const boost::uint8_t* p = recordStart; p != recordEnd;)
	{
		if (*p & 0x80)
		{
			boost::uint8_t n = -boost::int8_t(*p++);

			while (n-- > 0)
				*pt++ = *p;

			++p;
		}
		else
		{
			boost::uint8_t n = *p++;

			while (n-- > 0)
				*pt++ = *p++;
		}
	}

	/***
	cout << "\t\t\toffset: " << data->rpt[dataNum].offset <<
		", length: " << data->rpt[dataNum].length <<
		", transaction: " << record->transaction <<
		", page: " << record->page <<
		", line: " << record->line <<
		", flags: " << hex << record->flags << dec <<
		", format: " << int(record->format) <<
		", size: " << (pt - static_cast<boost::uint8_t*>(recordBuffer)) << endl << "\t\t\t\t";

	cout << hex;

	for (boost::uint8_t* p = recordStart; p != recordEnd; ++p)
		cout << " " << setw(2) << int(*p);

	cout << endl << "\t\t\t\t";

	for (recordEnd = pt, pt = static_cast<boost::uint8_t*>(recordBuffer); pt != recordEnd; ++pt)
		cout << " " << setw(2) << int(*pt);

	cout << dec;

	cout << endl;
	***/

	return true;
}

void FullScanStream::readPointer()
{
	database->readPage(pointer->page[pointerNum], data);

	/***
	cout << "\tpage: " << pointer->page[pointerNum] << endl;
	cout << "\t\ttype: " << int(data->pageHeader.type) <<
		", sequence: " << data->sequence <<
		", count: " << data->count << endl;
	***/
}


//------------------------------------------------------------------------------

}	// fbods
