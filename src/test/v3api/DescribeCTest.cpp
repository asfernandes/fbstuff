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

#include <ibase.h>
#include "gen/FbCApi.c"
#include "../FbTest.h"
#include <string>
#include <boost/test/unit_test.hpp>

extern "C" struct Master* fb_get_master_interface();

using std::string;

//------------------------------------------------------------------------------

static bool checkStatus(Status* status)
{
	return !(Status_getState(status) & Status_STATE_ERRORS);
}

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


BOOST_AUTO_TEST_CASE(describeC)
{
	const unsigned char* testDpb[2] = {NULL, FbTest::ASCII_DPB};
	unsigned testDpbLength[2] = {0, sizeof(FbTest::ASCII_DPB)};
	unsigned testSubType[2] = {3, 2};
	unsigned testLength[2] = {31 * 3, 31};

	Master* master = fb_get_master_interface();
	Provider* dispatcher = Master_getDispatcher(master);

	for (unsigned test = 0; test < 2; ++test)
	{
		const string location = FbTest::getLocation("describe.fdb");

		Status* status = Master_getStatus(master);

		Attachment* attachment = Provider_createDatabase(dispatcher, status, location.c_str(),
			testDpbLength[test], testDpb[test]);
		BOOST_CHECK(checkStatus(status));
		BOOST_REQUIRE(attachment);

		Transaction* transaction = Attachment_startTransaction(attachment, status, 0, NULL);
		BOOST_CHECK(checkStatus(status));
		BOOST_REQUIRE(transaction);

		// Test with and without metadata prefetch.
		for (unsigned i = 0; i < 2; ++i)
		{
			bool prefetch = (i == 0);

			Statement* stmt = Attachment_prepare(attachment, status, transaction, 0,
				"select rdb$relation_id relid, rdb$character_set_name csname"
				"  from rdb$database"
				"  where rdb$relation_id < ?",
				FbTest::DIALECT, (prefetch ? Statement_PREPARE_PREFETCH_ALL : 0));
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			unsigned type = Statement_getType(stmt, status);
			BOOST_CHECK(checkStatus(status));

			string legacyPlan = Statement_getPlan(stmt, status, false);
			BOOST_CHECK(checkStatus(status));

			MessageMetadata* inputParams = Statement_getInputMetadata(stmt, status);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(inputParams);

			MessageMetadata* outputParams = Statement_getOutputMetadata(stmt, status);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(outputParams);

			BOOST_CHECK_EQUAL(type, isc_info_sql_stmt_select);

			BOOST_CHECK_EQUAL(legacyPlan,
				"\nPLAN (RDB$DATABASE NATURAL)");

			struct FieldInfo
			{
				string field;
				string relation;
				string owner;
				string alias;
				unsigned type;
				bool nullable;
				unsigned charSet;
				unsigned length;
				unsigned scale;

				static void test(Status* status, MessageMetadata* params,
					unsigned count, FieldInfo* fieldInfo)
				{
					BOOST_CHECK_EQUAL(MessageMetadata_getCount(params, status), count);
					BOOST_CHECK(checkStatus(status));

					for (unsigned i = 0; i < count; ++i)
					{
						BOOST_CHECK_EQUAL(MessageMetadata_getField(params, status, i), fieldInfo[i].field);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getRelation(params, status, i), fieldInfo[i].relation);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getOwner(params, status, i), fieldInfo[i].owner);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getAlias(params, status, i), fieldInfo[i].alias);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getType(params, status, i), fieldInfo[i].type);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_isNullable(params, status, i), fieldInfo[i].nullable);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getCharSet(params, status, i), fieldInfo[i].charSet);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getLength(params, status, i), fieldInfo[i].length);
						BOOST_CHECK(checkStatus(status));

						BOOST_CHECK_EQUAL(MessageMetadata_getScale(params, status, i), fieldInfo[i].scale);
						BOOST_CHECK(checkStatus(status));
					}
				}
			};

			FieldInfo inputInfo[] = {
				{"", "", "", "", SQL_SHORT, true, 0, sizeof(ISC_SHORT), 0}
			};

			FieldInfo::test(status, inputParams, sizeof(inputInfo) / sizeof(inputInfo[0]), inputInfo);

			FieldInfo outputInfo[] = {
				{"RDB$RELATION_ID",        "RDB$DATABASE", "SYSDBA", "RELID",  SQL_SHORT, true,
					0,                 sizeof(ISC_SHORT),   0},
				{"RDB$CHARACTER_SET_NAME", "RDB$DATABASE", "SYSDBA", "CSNAME", SQL_TEXT,  true,
					testSubType[test], testLength[test], 0}
			};

			FieldInfo::test(status, outputParams, sizeof(outputInfo) / sizeof(outputInfo[0]), outputInfo);

			MessageMetadata_release(outputParams);
			MessageMetadata_release(inputParams);

			Statement_free(stmt, status);
			BOOST_CHECK(checkStatus(status));
		}

		Transaction_commit(transaction, status);
		BOOST_CHECK(checkStatus(status));

		Attachment_dropDatabase(attachment, status);
		BOOST_CHECK(checkStatus(status));

		Status_dispose(status);
	}

	Provider_release(dispatcher);
}


BOOST_AUTO_TEST_SUITE_END()
