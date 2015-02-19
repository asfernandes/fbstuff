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
#include <vector>
#include <boost/test/unit_test.hpp>

using namespace V3Util;
using namespace Firebird;
using std::string;
using std::vector;

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


BOOST_AUTO_TEST_CASE(dynamicMessage)
{
	const string location = FbTest::getLocation("dynamicMessage.fdb");

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
		IStatement* stmt = attachment->prepare(status, transaction, 0,
			"select rdb$relation_id, rdb$relation_name, rdb$description"
			"  from rdb$relations"
			"  where rdb$system_flag = ?"
			"  order by rdb$relation_id",
			FbTest::DIALECT, IStatement::PREPARE_PREFETCH_METADATA);
		BOOST_CHECK(checkStatus(status));
		BOOST_REQUIRE(stmt);

		IMessageMetadata* inParams = stmt->getInputMetadata(status);
		BOOST_CHECK(checkStatus(status));

		MessageImpl inMessage(status, inParams);
		Offset<void*> systemFlagParam(status, inMessage);

		IMessageMetadata* outParams = stmt->getOutputMetadata(status);
		BOOST_CHECK(checkStatus(status));

		unsigned outParamsCount = outParams->getCount(status);
		BOOST_CHECK(checkStatus(status));

		MessageImpl outMessage(status, outParams);
		vector<Offset<void*> > outFields;

		for (unsigned i = 0; i < outParamsCount; ++i)
			outFields.push_back(Offset<void*>(status, outMessage));

		BOOST_CHECK(inParams->getType(status, systemFlagParam.index) == SQL_SHORT);
		BOOST_CHECK(checkStatus(status));
		*static_cast<ISC_SHORT*>(inMessage[systemFlagParam]) = 0;

		IResultSet* rs = stmt->openCursor(status, transaction,
			inMessage.getMetadata(), inMessage.getBuffer(), outMessage.getMetadata(), 0);
		BOOST_CHECK(checkStatus(status));
		BOOST_REQUIRE(rs);

		static const char* const MSGS[] = {
			"128 | EMPLOYEE                        | Employees",
			"129 | CUSTOMER                        | Customers",
			"130 | SALES                           | "
		};
		int pos = 0;
		int ret;

		while ((ret = rs->fetchNext(status, outMessage.getBuffer())))
		{
			BOOST_CHECK(checkStatus(status));

			string msg;

			for (unsigned i = 0; i < outParamsCount; ++i)
			{
				if (!msg.empty())
					msg += " | ";

				msg += valueToString(attachment, transaction, outMessage, outFields[i]);
			}

			BOOST_CHECK_EQUAL(msg, MSGS[pos]);
			++pos;
		}

		outParams->release();
		inParams->release();

		rs->close(status);
		BOOST_CHECK(checkStatus(status));

		stmt->free(status);
		BOOST_CHECK(checkStatus(status));
	}

	transaction->commit(status);
	BOOST_CHECK(checkStatus(status));

	attachment->dropDatabase(status);
	BOOST_CHECK(checkStatus(status));

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
