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

#include "Ods.h"
#include "Database.h"
#include "FullScanStream.h"
#include <iostream>
#include <stdexcept>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;

namespace fbods
{

//------------------------------------------------------------------------------


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


//------------------------------------------------------------------------------

}	// fbods


int main()
{
	try
	{
		fbods::run();
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
	}

	return 0;
}
