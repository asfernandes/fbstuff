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
#include <boost/test/unit_test.hpp>

using namespace V3Util;
using namespace Firebird;
using std::string;

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


BOOST_AUTO_TEST_CASE(staticMessage)
{
	const string location = FbTest::getLocation("staticMessage.fdb");

	IStatus* status = master->getStatus();
	IAttachment* attachment = NULL;

	dispatcher->createDatabase(status, &attachment, 0, location.c_str(), 0, NULL);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL, 0);
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

		stmt->prepare(status, transaction, 0,
			"select rdb$relation_id, rdb$relation_name, rdb$description"
			"  from rdb$relations"
			"  where rdb$system_flag = ?"
			"  order by rdb$relation_id",
			FbTest::DIALECT, 0);
		BOOST_CHECK(status->isSuccess());

		MessageImpl inMessage;
		Offset<ISC_SHORT> systemFlagParam(inMessage);
		inMessage.finish();

		MessageImpl outMessage;
		Offset<ISC_SHORT> relationId(outMessage);
		Offset<FbString> relationName(outMessage, 31 * 3);
		Offset<ISC_QUAD> description(outMessage);
		outMessage.finish();

		inMessage[systemFlagParam] = 0;

		stmt->execute(status, transaction, 0, &inMessage, NULL);
		BOOST_CHECK(status->isSuccess());

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

			BOOST_TEST_MESSAGE(outMessage[relationId] << " | " <<
				outMessage[relationName].asStdString() << " | " <<
				descriptionStr);
		}

		stmt->free(status, DSQL_drop);
		BOOST_CHECK(status->isSuccess());
	}

	transaction->commit(status);
	BOOST_CHECK(status->isSuccess());

	attachment->drop(status);
	BOOST_CHECK(status->isSuccess());

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
