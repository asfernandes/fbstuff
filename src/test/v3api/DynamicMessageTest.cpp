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

		stmt->prepare(status, transaction, 0,
			"select rdb$relation_id, rdb$relation_name, rdb$description"
			"  from rdb$relations"
			"  where rdb$system_flag = ?"
			"  order by rdb$relation_id",
			FbTest::DIALECT, IStatement::PREPARE_PREFETCH_METADATA);
		BOOST_CHECK(status->isSuccess());

		MessageImpl inMessage;
		Offset<void*> systemFlagParam(inMessage, stmt->getInputParameters(status));
		BOOST_CHECK(status->isSuccess());
		inMessage.finish();

		const IParametersMetadata* outParams = stmt->getOutputParameters(status);
		BOOST_CHECK(status->isSuccess());

		unsigned outParamsCount = outParams->getCount(status);
		BOOST_CHECK(status->isSuccess());

		MessageImpl outMessage;
		vector<Offset<void*> > outFields;

		for (unsigned i = 0; i < outParamsCount; ++i)
			outFields.push_back(Offset<void*>(outMessage, outParams));

		outMessage.finish();

		BOOST_CHECK(systemFlagParam.getType() == SQL_SHORT);
		*static_cast<ISC_SHORT*>(inMessage[systemFlagParam]) = 0;

		stmt->execute(status, transaction, 0, &inMessage, NULL);
		BOOST_CHECK(status->isSuccess());

		static const char* const MSGS[] = {
			"128 | EMPLOYEE                        | Employees",
			"129 | CUSTOMER                        | Customers",
			"130 | SALES                           | "
		};
		int pos = 0;
		int ret;

		while ((ret = stmt->fetch(status, &outMessage)) != 100)
		{
			BOOST_CHECK(status->isSuccess());

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
