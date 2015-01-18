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

	CheckStatusWrapper statusWrapper(master->getStatus());
	CheckStatusWrapper* status = &statusWrapper;

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(),
		sizeof(FbTest::ASCII_DPB), FbTest::ASCII_DPB);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(transaction);

	// Create some tables and comment them.
	{
		attachment->execute(status, transaction, 0,
			"create table employee (id integer)", FbTest::DIALECT, NULL, NULL, NULL, NULL);
		BOOST_CHECK(checkStatus(status));

		attachment->execute(status, transaction, 0,
			"comment on table employee is 'Employees'", FbTest::DIALECT, NULL, NULL, NULL, NULL);
		BOOST_CHECK(checkStatus(status));

		attachment->execute(status, transaction, 0,
			"create table customer (id integer)", FbTest::DIALECT, NULL, NULL, NULL, NULL);
		BOOST_CHECK(checkStatus(status));

		attachment->execute(status, transaction, 0,
			"comment on table customer is 'Customers'", FbTest::DIALECT, NULL, NULL, NULL, NULL);
		BOOST_CHECK(checkStatus(status));

		attachment->execute(status, transaction, 0,
			"create table sales (id integer)", FbTest::DIALECT, NULL, NULL, NULL, NULL);
		BOOST_CHECK(checkStatus(status));

		transaction->commitRetaining(status);
		BOOST_CHECK(checkStatus(status));
	}

	{
		static const char* const MSGS[] = {
			"128 | EMPLOYEE                        | Employees",
			"129 | CUSTOMER                        | Customers",
			"130 | SALES                           | "
		};

		// Retrieve data with they datatype.
		{
			IStatement* stmt = attachment->prepare(status, transaction, 0,
				"select rdb$relation_id, rdb$relation_name, rdb$description"
				"  from rdb$relations"
				"  where rdb$system_flag = ?"
				"  order by rdb$relation_id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			IMessageMetadata* inParams = stmt->getInputMetadata(status);
			BOOST_CHECK(checkStatus(status));

			MessageImpl inMessage(status, inParams);
			BOOST_CHECK(checkStatus(status));

			Offset<ISC_SHORT> systemFlagParam(status, inMessage);

			IMessageMetadata* outParams = stmt->getOutputMetadata(status);
			BOOST_CHECK(checkStatus(status));

			MessageImpl outMessage(status, outParams);
			BOOST_CHECK(checkStatus(status));

			Offset<ISC_SHORT> relationId(status, outMessage);
			Offset<FbString> relationName(status, outMessage, 31);
			Offset<ISC_QUAD> description(status, outMessage);

			inMessage[systemFlagParam] = 0;

			IResultSet* rs = stmt->openCursor(status, transaction,
				inMessage.getMetadata(), inMessage.getBuffer(), outMessage.getMetadata());
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(rs);

			int pos = 0;
			int ret;

			while ((ret = rs->fetchNext(status, outMessage.getBuffer())) == IStatus::FB_OK)
			{
				BOOST_CHECK(checkStatus(status));

				string descriptionStr;

				if (!outMessage.isNull(description))
				{
					IBlob* blob = attachment->openBlob(status, transaction,
						&outMessage[description], 0, NULL);
					BOOST_CHECK(checkStatus(status));

					char blobBuffer[2];	// intentionaly test very small buffer
					int blobStatus;
					unsigned blobLen;

					while ((blobStatus = blob->getSegment(status, sizeof(blobBuffer),
											blobBuffer, &blobLen)) == IStatus::FB_OK ||
						   blobStatus == IStatus::FB_SEGMENT)
					{
						descriptionStr.append(blobBuffer, blobLen);
					}

					blob->close(status);
					BOOST_CHECK(checkStatus(status));
				}

				string msg =
					lexical_cast<string>(outMessage[relationId]) + " | " +
					outMessage[relationName].asStdString() + " | " +
					descriptionStr;

				BOOST_CHECK_EQUAL(msg, MSGS[pos]);
				++pos;
			}

			BOOST_CHECK_EQUAL(pos, 3);

			rs->close(status);
			BOOST_CHECK(checkStatus(status));

			stmt->free(status);
			BOOST_CHECK(checkStatus(status));
		}

		unsigned major, minor, revision;
		getEngineVersion(attachment, &major, &minor, &revision);
		unsigned version = major * 100u + minor * 10u + revision;

		if (version >= 251)
		{
			// Retrieve data with FB_MESSAGE.

			IStatement* stmt = attachment->prepare(status, transaction, 0,
				"select rdb$relation_id, rdb$relation_name, rdb$description"
				"  from rdb$relations"
				"  where rdb$system_flag = ?"
				"  order by rdb$relation_id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			FB_MESSAGE(Input, CheckStatusWrapper,
				(FB_INTEGER, systemFlag)
			) input(status, master);

			FB_MESSAGE(Output, CheckStatusWrapper,
				(FB_SMALLINT, relationId)
				(FB_VARCHAR(31), relationName)
				(FB_VARCHAR(100), description)
			) output(status, master);

			input.clear();
			input->systemFlag = 0;

			IResultSet* rs = stmt->openCursor(status, transaction,
				input.getMetadata(), input.getData(), output.getMetadata());
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(rs);

			int pos = 0;
			int ret;

			while ((ret = rs->fetchNext(status, output.getData())) == IStatus::FB_OK)
			{
				BOOST_CHECK(checkStatus(status));

				string msg =
					lexical_cast<string>(output->relationId) + " | " +
					string(output->relationName.str,
						(output->relationNameNull ? 0 : output->relationName.length)) + " | " +
					string(output->description.str,
						(output->descriptionNull ? 0 : output->description.length));

				BOOST_CHECK_EQUAL(msg, MSGS[pos]);
				++pos;
			}

			BOOST_CHECK_EQUAL(pos, 3);

			rs->close(status);
			BOOST_CHECK(checkStatus(status));

			stmt->free(status);
			BOOST_CHECK(checkStatus(status));
		}

		if (version >= 251)
		{
			// Retrieve data as strings.

			// Also make the input parameter a blob.
			IStatement* stmt = attachment->prepare(status, transaction, 0,
				"select rdb$relation_id, rdb$relation_name, rdb$description"
				"  from rdb$relations"
				"  where cast(rdb$system_flag as blob sub_type text) = ?"
				"  order by rdb$relation_id",
				FbTest::DIALECT, 0);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			IMessageMetadata* inParams = stmt->getInputMetadata(status);
			BOOST_CHECK(checkStatus(status));

			MessageImpl inMessage(status, inParams);
			BOOST_CHECK(checkStatus(status));

			Offset<FbString> systemFlagParam(status, inMessage, 1);

			IMessageMetadata* outParams = stmt->getOutputMetadata(status);
			BOOST_CHECK(checkStatus(status));

			MessageImpl outMessage(status, outParams);
			BOOST_CHECK(checkStatus(status));

			Offset<FbString> relationId(status, outMessage, 10);
			Offset<FbString> relationName(status, outMessage, 31);
			Offset<FbString> description(status, outMessage, 100);

			strcpy(inMessage[systemFlagParam].str, "0");
			inMessage[systemFlagParam].length = 1;

			IResultSet* rs = stmt->openCursor(status, transaction,
				inMessage.getMetadata(), inMessage.getBuffer(), outMessage.getMetadata());
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(rs);

			int pos = 0;
			int ret;

			while ((ret = rs->fetchNext(status, outMessage.getBuffer())) == IStatus::FB_OK)
			{
				BOOST_CHECK(checkStatus(status));

				string msg =
					outMessage[relationId].asStdString() + " | " +
					(outMessage.isNull(relationName) ? "" : outMessage[relationName].asStdString()) +
					" | " +
					(outMessage.isNull(description) ? "" : outMessage[description].asStdString());

				BOOST_CHECK_EQUAL(msg, MSGS[pos]);
				++pos;
			}

			BOOST_CHECK_EQUAL(pos, 3);

			rs->close(status);
			BOOST_CHECK(checkStatus(status));

			stmt->free(status);
			BOOST_CHECK(checkStatus(status));
		}

		if (version >= 251)
		{
			// Try to retrieve a number as a blob in execute.

			IStatement* stmt = attachment->prepare(status, transaction, 0,
				"insert into employee values (11) returning id", FbTest::DIALECT, 0);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			IMessageMetadata* outParams = stmt->getOutputMetadata(status);
			BOOST_CHECK(checkStatus(status));

			MessageImpl outMessage(status, outParams);
			BOOST_CHECK(checkStatus(status));

			Offset<ISC_QUAD> idAsBlob(status, outMessage);

			stmt->execute(status, transaction, NULL, NULL,
				outMessage.getMetadata(), outMessage.getBuffer());
			BOOST_CHECK(checkStatus(status));

			string msg;

			if (!outMessage.isNull(idAsBlob))
			{
				IBlob* blob = attachment->openBlob(status, transaction,
					&outMessage[idAsBlob], 0, NULL);
				BOOST_CHECK(checkStatus(status));

				char blobBuffer[2];	// intentionaly test very small buffer
				int blobStatus;
				unsigned blobLen;

				while ((blobStatus = blob->getSegment(status, sizeof(blobBuffer),
										blobBuffer, &blobLen)) == IStatus::FB_OK ||
					   blobStatus == IStatus::FB_SEGMENT)
				{
					msg.append(blobBuffer, blobLen);
				}

				blob->close(status);
				BOOST_CHECK(checkStatus(status));
			}

			BOOST_CHECK_EQUAL(msg, "11");

			stmt->free(status);
			BOOST_CHECK(checkStatus(status));
		}
	}

	transaction->commit(status);
	BOOST_CHECK(checkStatus(status));

	attachment->dropDatabase(status);
	BOOST_CHECK(checkStatus(status));

	status->dispose();
}


BOOST_AUTO_TEST_CASE(staticMessage2)
{
	const string location = FbTest::getLocation("staticMessage2.fdb");

	CheckStatusWrapper statusWrapper(master->getStatus());
	CheckStatusWrapper* status = &statusWrapper;

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(),
		sizeof(FbTest::UTF8_DPB), FbTest::UTF8_DPB);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(transaction);

	{
		IStatement* stmt = attachment->prepare(status, transaction, 0,
			"execute block (c varchar(10) character set win1252 not null = ?) returns (n integer) "
			"as "
			"begin "
			"  n = octet_length(c); "
			"  suspend; "
			"end",
			FbTest::DIALECT, 0);
		BOOST_CHECK(checkStatus(status));
		BOOST_REQUIRE(stmt);

		FB_MESSAGE(Input, CheckStatusWrapper,
			(FB_VARCHAR(10 * 4), c)
		) input(status, master);

		FB_MESSAGE(Output, CheckStatusWrapper,
			(FB_INTEGER, n)
		) output(status, master);

		input.clear();
		input->c.set("123áé456");

		IResultSet* rs = stmt->openCursor(status, transaction,
			input.getMetadata(), input.getData(), output.getMetadata());
		BOOST_CHECK(checkStatus(status));
		BOOST_REQUIRE(rs);

		bool ret = rs->fetchNext(status, output.getData()) == IStatus::FB_OK;
		BOOST_CHECK(checkStatus(status) && ret);

		BOOST_CHECK_EQUAL(output->n, 8);	// CORE-3737

		rs->close(status);
		BOOST_CHECK(checkStatus(status));

		input->cNull = 1;
		rs = stmt->openCursor(status, transaction,
			input.getMetadata(), input.getData(), output.getMetadata());

		if (rs)	// remote
			BOOST_CHECK(rs->fetchNext(status, output.getData()) == IStatus::FB_ERROR);

		BOOST_CHECK(!checkStatus(status) && status->getErrors()[1] == isc_not_valid_for_var);

		if (rs)	// remote
			rs->close(status);

		stmt->free(status);
		BOOST_CHECK(checkStatus(status));
	}

	transaction->commit(status);
	BOOST_CHECK(checkStatus(status));

	attachment->dropDatabase(status);
	BOOST_CHECK(checkStatus(status));

	status->dispose();
}


BOOST_AUTO_TEST_CASE(staticMessage3)	// test for CORE-4184
{
	const string location = FbTest::getLocation("staticMessage3.fdb");

	CheckStatusWrapper statusWrapper(master->getStatus());
	CheckStatusWrapper* status = &statusWrapper;

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(),
		sizeof(FbTest::UTF8_DPB), FbTest::UTF8_DPB);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(attachment);

	unsigned major, minor;
	getEngineVersion(attachment, &major, &minor, NULL);
	unsigned version = major * 100u + minor * 10u;

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(transaction);

	{
		attachment->execute(status, transaction, 0,
			"create procedure p1 (i integer) returns (o integer not null) "
			"as "
			"begin "
			"  if (i is not null) then "
			"      o = i; "
			"  if (1 = 0) then "
			"      suspend; "
			"end",
			FbTest::DIALECT, NULL, NULL, NULL, NULL);
		BOOST_CHECK(checkStatus(status));

		transaction->commitRetaining(status);
		BOOST_CHECK(checkStatus(status));
	}

	FB_MESSAGE(Message, CheckStatusWrapper,
		(FB_INTEGER, n)
	) input(status, master), output(status, master);

	for (int i = 0; i < 2; ++i)
	{
		input.clear();

		if (i == 0)
		{
			input->n = 11;
			input->nNull = FB_FALSE;
		}
		else
			input->nNull = FB_TRUE;

		{
			IStatement* stmt = attachment->prepare(status, transaction, 0,
				"execute block (i integer = ?) returns (o integer not null) "
				"as "
				"begin "
				"  if (i is not null) then "
				"      o = i; "
				"  suspend; "
				"end",
				FbTest::DIALECT, 0);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			output.clear();

			IResultSet* rs = stmt->openCursor(status, transaction,
				input.getMetadata(), input.getData(), output.getMetadata());
			BOOST_CHECK(checkStatus(status));

			if (i == 0)
			{
				BOOST_CHECK(rs->fetchNext(status, output.getData()) == IStatus::FB_OK);
				BOOST_CHECK(checkStatus(status));
				BOOST_CHECK_EQUAL(output->n, input->n);
			}
			else
			{
				BOOST_CHECK(!rs->fetchNext(status, output.getData()) == IStatus::FB_OK);
				BOOST_CHECK(!checkStatus(status));
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_not_valid_for_var);
			}

			rs->close(status);
			BOOST_CHECK(checkStatus(status));

			stmt->free(status);
			BOOST_CHECK(checkStatus(status));
		}

		{
			IStatement* stmt = attachment->prepare(status, transaction, 0,
				"execute block (i integer = ?) returns (o integer not null) "
				"as "
				"begin "
				"  if (i is not null) then "
				"      o = i; "
				"  if (1 = 0) then "
				"      suspend; "
				"end",
				FbTest::DIALECT, 0);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(stmt);

			output.clear();

			IResultSet* rs = stmt->openCursor(status, transaction,
				input.getMetadata(), input.getData(), output.getMetadata());
			BOOST_CHECK(checkStatus(status));

			BOOST_CHECK(rs->fetchNext(status, output.getData()) != IStatus::FB_OK);

			if (i == 0 || version == 300)
				BOOST_CHECK(checkStatus(status));
			else
			{
				BOOST_CHECK(!checkStatus(status));
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_not_valid_for_var);
			}

			rs->close(status);
			BOOST_CHECK(checkStatus(status));

			stmt->free(status);
			BOOST_CHECK(checkStatus(status));
		}

		{
			output.clear();

			attachment->execute(status, transaction, 0,
				"execute block (i integer = ?) returns (o integer not null) "
				"as "
				"begin "
				"  if (i is not null) then "
				"      o = i; "
				"end",
				FbTest::DIALECT,
				input.getMetadata(), input.getData(), output.getMetadata(), output.getData());

			BOOST_CHECK(!checkStatus(status));

			if (i == 0 || version == 300)
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_stream_eof);
			else
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_not_valid_for_var);
		}

		{
			output.clear();

			attachment->execute(status, transaction, 0, "execute procedure p1(?)",
				FbTest::DIALECT,
				input.getMetadata(), input.getData(), output.getMetadata(), output.getData());

			if (i == 0)
			{
				BOOST_CHECK(checkStatus(status));
				BOOST_CHECK_EQUAL(output->n, input->n);
			}
			else
			{
				BOOST_CHECK(!checkStatus(status));
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_not_valid_for_var);
			}
		}

		{
			output.clear();

			attachment->execute(status, transaction, 0, "select * from p1(?)",
				FbTest::DIALECT,
				input.getMetadata(), input.getData(), output.getMetadata(), output.getData());
			BOOST_CHECK(!checkStatus(status));

			if (i == 0 || version == 300)
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_stream_eof);
			else
				BOOST_CHECK_EQUAL(status->getErrors()[1] , isc_not_valid_for_var);
		}
	}

	transaction->commit(status);
	BOOST_CHECK(checkStatus(status));

	attachment->dropDatabase(status);
	BOOST_CHECK(checkStatus(status));

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
