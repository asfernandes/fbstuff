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

#include "V3Util.h"
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

using namespace V3Util;
using namespace Firebird;
using std::string;
using boost::lexical_cast;

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


BOOST_AUTO_TEST_CASE(staticMessage)
{
	const string location = FbTest::getLocation("staticMessage.fdb");

	IStatus* status = master->getStatus();

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(),
		sizeof(FbTest::ASCII_DPB), FbTest::ASCII_DPB);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(transaction);

	// Create some tables and comment them.
	{
		attachment->execute(status, transaction, 0,
			"create table employee (id integer)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment->execute(status, transaction, 0,
			"comment on table employee is 'Employees'", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment->execute(status, transaction, 0,
			"create table customer (id integer)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment->execute(status, transaction, 0,
			"comment on table customer is 'Customers'", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment->execute(status, transaction, 0,
			"create table sales (id integer)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		transaction->commitRetaining(status);
		BOOST_CHECK(status->isSuccess());
	}

	{
		IStatement* stmt = attachment->allocateStatement(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_REQUIRE(stmt);

		static const char* const MSGS[] = {
			"128 | EMPLOYEE                        | Employees",
			"129 | CUSTOMER                        | Customers",
			"130 | SALES                           | "
		};

		// Retrieve data with they datatype.
		{
			stmt->prepare(status, transaction, 0,
				"select rdb$relation_id, rdb$relation_name, rdb$description"
				"  from rdb$relations"
				"  where rdb$system_flag = ?"
				"  order by rdb$relation_id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(status->isSuccess());

			const IParametersMetadata* inParams = stmt->getInputParameters(status);
			BOOST_CHECK(status->isSuccess());

			MessageImpl inMessage(inParams->getCount(status));
			BOOST_CHECK(status->isSuccess());

			Offset<ISC_SHORT> systemFlagParam(inMessage);

			const IParametersMetadata* outParams = stmt->getOutputParameters(status);
			BOOST_CHECK(status->isSuccess());

			MessageImpl outMessage(outParams->getCount(status));
			BOOST_CHECK(status->isSuccess());

			Offset<ISC_SHORT> relationId(outMessage);
			Offset<FbString> relationName(outMessage, 31);
			Offset<ISC_QUAD> description(outMessage);

			inMessage[systemFlagParam] = 0;

			stmt->execute(status, transaction, 0, &inMessage, NULL);
			BOOST_CHECK(status->isSuccess());

			int pos = 0;
			int ret;

			while ((ret = stmt->fetch(status, &outMessage)) != 100)
			{
				BOOST_CHECK(status->isSuccess());

				string descriptionStr;

				if (!outMessage.isNull(description))
				{
					IBlob* blob = attachment->openBlob(status, transaction,
						&outMessage[description], 0, NULL);
					BOOST_CHECK(status->isSuccess());

					char blobBuffer[2];	// intentionaly test very small buffer
					unsigned blobLen;

					while ((blobLen = blob->getSegment(status, sizeof(blobBuffer), blobBuffer)) != 0)
						descriptionStr.append(blobBuffer, blobLen);

					blob->close(status);
					BOOST_CHECK(status->isSuccess());
				}

				string msg =
					lexical_cast<string>(outMessage[relationId]) + " | " +
					outMessage[relationName].asStdString() + " | " +
					descriptionStr;

				BOOST_CHECK_EQUAL(msg, MSGS[pos]);
				++pos;
			}

			BOOST_CHECK_EQUAL(pos, 3);

			stmt->free(status, DSQL_unprepare);
			BOOST_CHECK(status->isSuccess());
		}

		unsigned major, minor, revision;
		getEngineVersion(attachment, &major, &minor, &revision);
		unsigned version = major * 100u + minor * 10u + revision;

		if (version >= 251)
		{
			// Retrieve data with FB_MESSAGE.

			stmt->prepare(status, transaction, 0,
				"select rdb$relation_id, rdb$relation_name, rdb$description"
				"  from rdb$relations"
				"  where rdb$system_flag = ?"
				"  order by rdb$relation_id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(status->isSuccess());

			FB_MESSAGE_DESC(InputType,
				(FB_INTEGER, systemFlag)
			) input;

			FB_MESSAGE_DESC(OutputType,
				(FB_SMALLINT, relationId)
				(FB_VARCHAR(31), relationName)
				(FB_VARCHAR(100), description)
			) output;

			input.clear();
			input.systemFlag = 0;

			stmt->execute(status, transaction, 0, &input.desc, NULL);
			BOOST_CHECK(status->isSuccess());

			int pos = 0;
			int ret;

			while ((ret = stmt->fetch(status, &output.desc)) != 100)
			{
				BOOST_CHECK(status->isSuccess());

				string msg =
					lexical_cast<string>(output.relationId) + " | " +
					string(output.relationName.str, output.relationName.length) + " | " +
					string(output.description.str, output.description.length);

				BOOST_CHECK_EQUAL(msg, MSGS[pos]);
				++pos;
			}

			BOOST_CHECK_EQUAL(pos, 3);

			stmt->free(status, DSQL_unprepare);
			BOOST_CHECK(status->isSuccess());
		}

		if (version >= 251)
		{
			// Retrieve data as strings.

			// Also make the input parameter a blob.
			stmt->prepare(status, transaction, 0,
				"select rdb$relation_id, rdb$relation_name, rdb$description"
				"  from rdb$relations"
				"  where cast(rdb$system_flag as blob sub_type text) = ?"
				"  order by rdb$relation_id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(status->isSuccess());

			const IParametersMetadata* inParams = stmt->getInputParameters(status);
			BOOST_CHECK(status->isSuccess());

			MessageImpl inMessage(inParams->getCount(status));
			BOOST_CHECK(status->isSuccess());

			Offset<FbString> systemFlagParam(inMessage, 1);

			const IParametersMetadata* outParams = stmt->getOutputParameters(status);
			BOOST_CHECK(status->isSuccess());

			MessageImpl outMessage(outParams->getCount(status));
			BOOST_CHECK(status->isSuccess());

			Offset<FbString> relationId(outMessage, 10);
			Offset<FbString> relationName(outMessage, 31);
			Offset<FbString> description(outMessage, 100);

			strcpy(inMessage[systemFlagParam].str, "0");
			inMessage[systemFlagParam].length = 1;

			stmt->execute(status, transaction, 0, &inMessage, NULL);
			BOOST_CHECK(status->isSuccess());

			int pos = 0;
			int ret;

			while ((ret = stmt->fetch(status, &outMessage)) != 100)
			{
				BOOST_CHECK(status->isSuccess());

				string msg =
					outMessage[relationId].asStdString() + " | " +
					outMessage[relationName].asStdString() + " | " +
					outMessage[description].asStdString();

				BOOST_CHECK_EQUAL(msg, MSGS[pos]);
				++pos;
			}

			BOOST_CHECK_EQUAL(pos, 3);

			stmt->free(status, DSQL_unprepare);
			BOOST_CHECK(status->isSuccess());
		}

		if (version >= 251)
		{
			// Try to retrieve a number as a blob in execute.

			stmt->prepare(status, transaction, 0, "insert into employee values (11) returning id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(status->isSuccess());

			const IParametersMetadata* outParams = stmt->getOutputParameters(status);
			BOOST_CHECK(status->isSuccess());

			MessageImpl outMessage(outParams->getCount(status));
			BOOST_CHECK(status->isSuccess());

			Offset<ISC_QUAD> idAsBlob(outMessage);

			stmt->execute(status, transaction, 0, NULL, &outMessage);
			BOOST_CHECK(status->isSuccess());

			string msg;

			if (!outMessage.isNull(idAsBlob))
			{
				IBlob* blob = attachment->openBlob(status, transaction,
					&outMessage[idAsBlob], 0, NULL);
				BOOST_CHECK(status->isSuccess());

				char blobBuffer[2];	// intentionaly test very small buffer
				unsigned blobLen;

				while ((blobLen = blob->getSegment(status, sizeof(blobBuffer), blobBuffer)) != 0)
					msg.append(blobBuffer, blobLen);

				blob->close(status);
				BOOST_CHECK(status->isSuccess());
			}

			BOOST_CHECK_EQUAL(msg, "11");
		}

		stmt->free(status, DSQL_drop);
		BOOST_CHECK(status->isSuccess());
	}

	transaction->commit(status);
	BOOST_CHECK(status->isSuccess());

	attachment->dropDatabase(status);
	BOOST_CHECK(status->isSuccess());

	status->dispose();
}


BOOST_AUTO_TEST_CASE(staticMessage2)
{
	const string location = FbTest::getLocation("staticMessage2.fdb");

	IStatus* status = master->getStatus();

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(),
		sizeof(FbTest::UTF8_DPB), FbTest::UTF8_DPB);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(transaction);

	{
		IStatement* stmt = attachment->allocateStatement(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_REQUIRE(stmt);

		{
			stmt->prepare(status, transaction, 0,
				"execute block (c varchar(10) character set win1252 not null = ?) returns (n integer) "
				"as "
				"begin "
				"  n = octet_length(c); "
				"  suspend; "
				"end",
				FbTest::DIALECT, 0);
			BOOST_CHECK(status->isSuccess());

			FB_MESSAGE_DESC(InputType,
				(FB_VARCHAR(10 * 4), c)
			) input;

			FB_MESSAGE_DESC(OutputType,
				(FB_INTEGER, n)
			) output;

			input.clear();
			input.c.set("123áé456");

			stmt->execute(status, transaction, 0, &input.desc, NULL);
			BOOST_CHECK(status->isSuccess());

			int ret = stmt->fetch(status, &output.desc);
			BOOST_CHECK(status->isSuccess() && ret != 100);

			BOOST_CHECK_EQUAL(output.n, 8);	// CORE-3737

			stmt->free(status, DSQL_close);
			BOOST_CHECK(status->isSuccess());

			input.cNull = 1;
			stmt->execute(status, transaction, 0, &input.desc, NULL);
			BOOST_CHECK(!status->isSuccess() && status->get()[1] == isc_not_valid_for_var);
		}

		stmt->free(status, DSQL_drop);
		BOOST_CHECK(status->isSuccess());
	}

	transaction->commit(status);
	BOOST_CHECK(status->isSuccess());

	attachment->dropDatabase(status);
	BOOST_CHECK(status->isSuccess());

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
