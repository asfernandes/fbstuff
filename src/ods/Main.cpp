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

#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <boost/cstdint.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_array.hpp>
#include <string.h>
#include <assert.h>
#include <stdio.h>

using namespace std;


struct PageHeader
{
	static const boost::int8_t TYPE_POINTER = 4;

	boost::int8_t type;
	boost::int8_t flags;
	boost::uint16_t checksum;
	boost::uint32_t generation;
	boost::uint32_t scn;
	boost::uint32_t reserved;
};

struct HeaderPage
{
	PageHeader pageHeader;
	boost::uint16_t pageSize;
	boost::uint16_t odsVersion;
	boost::int32_t pages;
	boost::uint32_t nextPage;
	boost::int32_t oldestTransaction;
	boost::int32_t oldestActive;
	boost::int32_t nextTransaction;
	boost::uint16_t sequence;
	boost::uint16_t flags;
	boost::int32_t creationDate[2];
	boost::int32_t attachmentId;
	boost::int32_t shadowCount;
	boost::int16_t implementation;
	boost::uint16_t odsMinor;
	boost::uint16_t odsMinorOriginal;
	boost::uint16_t end;
	boost::uint32_t pageBuffers;
	boost::int32_t bumpedTransaction;
	boost::int32_t oldestSnapshot;
	boost::int32_t backupPages;
	boost::int32_t misc[3];
	boost::uint8_t data[];
};

struct PointerPage
{
	PageHeader pageHeader;
	boost::int32_t sequence;
	boost::int32_t next;
	boost::uint16_t count;
	boost::uint16_t relation;
	boost::uint16_t minSpace;
	boost::uint16_t maxSpace;
	boost::int32_t page[];
};

struct DataPage
{
	PageHeader pageHeader;
	boost::int32_t sequence;
	boost::uint16_t relation;
	boost::uint16_t count;
	struct Repeat
	{
		boost::uint16_t offset;
		boost::uint16_t length;
	} rpt[];
};

struct RecordHeader
{
	static const unsigned FLAG_DELETED	= 0x01;
	static const unsigned FLAG_BLOB		= 0x10;

	boost::int32_t transaction;
	boost::int32_t backPage;
	boost::uint16_t backLine;
	boost::uint16_t flags;
	boost::uint8_t format;
	union
	{
		boost::uint8_t data[];
		/***
		struct
		{
			boost::int32_t page;
			boost::uint16_t line;
			boost::uint8_t data[];
		} fragment;
		***/
	};
};


template <int LENGTH>
struct VarChar
{
	boost::uint16_t length;
	char data[LENGTH];
};


class Database
{
public:
	Database(const char* filename)
		: file(filename, ios::in | ios::binary)
	{
		if (file.fail())
			throw runtime_error(string("Cannot open ") + filename);

		file.read(reinterpret_cast<char*>(&header), sizeof(header));

		relationPointer.insert(make_pair(RELATION_ID_PAGES, header.pages));
	}

	void readPage(unsigned number, void* data)
	{
		file.seekg(header.pageSize * number, ios::beg);
		file.read(static_cast<char*>(data), header.pageSize);
	}

public:
	enum RelationId
	{
		RELATION_ID_PAGES = 0,
		RELATION_ID_INDEX_SEGMENTS = 3,
		RELATION_ID_RELATIONS = 6
	};

	fstream file;
	HeaderPage header;
	map<RelationId, unsigned> relationPointer;
};


class FullScanStream
{
public:
	FullScanStream(Database* aDatabase, Database::RelationId aRelationId)
		: database(aDatabase),
		  relationId(aRelationId),
		  first(true),
		  pointerNum(0),
		  dataNum(0)
	{
		init();
	}

	FullScanStream(Database* aDatabase, const char* relationName)
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

private:
	void init()
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

public:
	bool fetch(void* recordBuffer)
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

private:
	void readPointer()
	{
		database->readPage(pointer->page[pointerNum], data);

		/***
		cout << "\tpage: " << pointer->page[pointerNum] << endl;
		cout << "\t\ttype: " << int(data->pageHeader.type) <<
			", sequence: " << data->sequence <<
			", count: " << data->count << endl;
		***/
	}

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


void run()
{
	Database database("t.fdb");

	/***
	{
		FullScanStream scan(&database, Database::RELATION_ID_PAGES);

		struct RdbPages
		{
			boost::uint32_t nullFlags[1];
			boost::int32_t pageNumber;
			boost::int16_t relationId;
			boost::int32_t pageSequence;
			boost::int16_t pageType;
		} rdbPages;

		while (scan.fetch(&rdbPages))
		{
			cout << "pageNumber: " << rdbPages.pageNumber <<
				", relationId: " << rdbPages.relationId <<
				", pageSequence: " << rdbPages.pageSequence <<
				", pageType: " << rdbPages.pageType <<
				endl;
		}
	}

	cout << endl;

	{
		FullScanStream scan(&database, Database::RELATION_ID_INDEX_SEGMENTS);

		struct RdbIndexSegments
		{
			boost::uint32_t nullFlags[1];
			char indexName[31];
			char fieldName[31];
			boost::int16_t fieldPosition;
			double statistics;
		} rdbIndexSegments;

		while (scan.fetch(&rdbIndexSegments))
		{
			cout << "fieldPosition: " << rdbIndexSegments.fieldPosition <<
				", indexName: " << string(rdbIndexSegments.indexName, sizeof(rdbIndexSegments.indexName)) <<
				", fieldName: " << string(rdbIndexSegments.fieldName, sizeof(rdbIndexSegments.fieldName)) <<
				", fieldPosition: " << rdbIndexSegments.fieldPosition <<
				", statistics: " << rdbIndexSegments.statistics <<
				endl;
		}
	}

	cout << endl;

	{
		FullScanStream scan(&database, Database::RELATION_ID_RELATIONS);

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
			cout << "relationId: " << rdbRelations.relationId <<
				", relationName: " << string(rdbRelations.relationName, sizeof(rdbRelations.relationName)) <<
				endl;
		}
	}

	cout << endl;
	***/

	{
		FullScanStream scan(&database, "T");

		struct T
		{
			boost::uint32_t nullFlags[1];
			boost::int32_t n;
		} t;

		unsigned count = 0;

		while (scan.fetch(&t))
		{
			++count;
			///cout << "n: " << t.n <<
			///	endl;
		}

		cout << "count: " << count << endl;
	}
}


int main()
{
	try
	{
		run();
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
	}

	return 0;
}
