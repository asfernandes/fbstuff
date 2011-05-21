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


BOOST_AUTO_TEST_CASE(multiDbTrans)
{
	const string location1 = FbTest::getLocation("multiDbTrans1.fdb");
	const string location2 = FbTest::getLocation("multiDbTrans2.fdb");

	IStatus* status = master->getStatus();

	IAttachment* attachment1 = dispatcher->createDatabase(status, location1.c_str(),
		sizeof(FbTest::ASCII_DPB), FbTest::ASCII_DPB);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment1);

	IAttachment* attachment2 = dispatcher->createDatabase(status, location2.c_str(),
		sizeof(FbTest::ASCII_DPB), FbTest::ASCII_DPB);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment2);

	IDtc* dtc = master->getDtc();

	for (unsigned i = 0; i < 2; ++i)
	{
		ITransaction* transaction;

		if (i == 0)
		{
			DtcStart transactionParameters[] = {
				{attachment1, 0, NULL},
				{attachment2, 0, NULL}
			};

			transaction = dtc->start(status,
				sizeof(transactionParameters) / sizeof(transactionParameters[0]), transactionParameters);
		}
		else
		{
			ITransaction* transaction1 = attachment1->startTransaction(status, 0, NULL);
			BOOST_CHECK(status->isSuccess());
			BOOST_REQUIRE(transaction1);

			ITransaction* transaction2 = attachment2->startTransaction(status, 0, NULL);
			BOOST_CHECK(status->isSuccess());
			BOOST_REQUIRE(transaction2);

			transaction = dtc->join(status, transaction1, transaction2);
		}

		BOOST_CHECK(status->isSuccess());
		BOOST_REQUIRE(transaction);

		// Lets try to create the same table in the two databases, using the same transaction.
		const char* const CMD = "create table employee (id integer)";

		attachment1->execute(status, transaction, 0, CMD, FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment2->execute(status, transaction, 0, CMD, FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		if (i == 0)
			transaction->rollback(status);
		else
			transaction->commit(status);

		BOOST_CHECK(status->isSuccess());
	}

	attachment2->drop(status);
	BOOST_CHECK(status->isSuccess());

	attachment1->drop(status);
	BOOST_CHECK(status->isSuccess());

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
