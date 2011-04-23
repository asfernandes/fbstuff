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


BOOST_AUTO_TEST_CASE(affectedRecords)
{
	const string location = FbTest::getLocation("affectedRecords.fdb");

	IStatus* status = master->getStatus();
	IAttachment* attachment = NULL;

	dispatcher->createDatabase(status, &attachment, 0, location.c_str(), 0, NULL);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL, 0);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(transaction);

	// Create some tables.
	{
		attachment->execute(status, transaction, 0,
			"create table t0 (n integer)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment->execute(status, transaction, 0,
			"create table t1 (n integer)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		attachment->execute(status, transaction, 0,
			"create generator g1", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		transaction->commitRetaining(status);
		BOOST_CHECK(status->isSuccess());
	}

	unsigned int counts[] = {7, 9};
	ISC_UINT64 rec;

	IStatement* stmt = attachment->allocateStatement(status);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(stmt);

	// Add some data.
	for (unsigned i = 0; i < 2; ++i)
	{
		stmt->prepare(status, transaction, 0,
			("insert into t" + lexical_cast<string>(i) + " values (1)").c_str(),
			FbTest::DIALECT, IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS);
		BOOST_CHECK(status->isSuccess());

		for (unsigned j = 0; j < counts[i]; ++j)
		{
			stmt->execute(status, transaction, 0, NULL, NULL);
			BOOST_CHECK(status->isSuccess());

			rec = stmt->getAffectedRecords(status);
			BOOST_CHECK(status->isSuccess());
			BOOST_CHECK_EQUAL(rec, 1);
		}
	}

	{
		stmt->prepare(status, transaction, 0, "insert into t0 select * from t1", FbTest::DIALECT,
			IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS);
		BOOST_CHECK(status->isSuccess());

		stmt->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		rec = stmt->getAffectedRecords(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK_EQUAL(rec, counts[1]);
	}

	{
		stmt->prepare(status, transaction, 0, "update t0 set n = n + 1", FbTest::DIALECT,
			IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS);
		BOOST_CHECK(status->isSuccess());

		stmt->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		rec = stmt->getAffectedRecords(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK_EQUAL(rec, counts[0] + counts[1]);
	}

	{
		// Lets try without IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS.
		stmt->prepare(status, transaction, 0, "delete from t0", FbTest::DIALECT, 0);
		BOOST_CHECK(status->isSuccess());

		stmt->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		rec = stmt->getAffectedRecords(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK_EQUAL(rec, counts[0] + counts[1]);
	}

	{
		attachment->execute(status, transaction, 0,
			"update t1 set n = gen_id(g1, 1)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());
	}

	{
		stmt->prepare(status, transaction, 0,
			"merge into t0"
			"  using (select n from t1) t1"
			"    on (t0.n = t1.n)"
			"  when matched then update set n = t1.n"
			"  when not matched then insert values (n)",
			FbTest::DIALECT, IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS);
		BOOST_CHECK(status->isSuccess());

		stmt->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		rec = stmt->getAffectedRecords(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK_EQUAL(rec, counts[1]);
	}

	{
		stmt->prepare(status, transaction, 0,
			"merge into t0"
			"  using (select n + 2 n from t1) t1"
			"    on (t0.n = t1.n)"
			"  when matched then update set n = t1.n"
			"  when not matched then insert values (n)",
			FbTest::DIALECT, IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS);
		BOOST_CHECK(status->isSuccess());

		stmt->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		rec = stmt->getAffectedRecords(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK_EQUAL(rec, counts[1]);
	}

	{
		stmt->prepare(status, transaction, 0,
			"merge into t0"
			"  using (select n + 2 + 4 n from t1) t1"
			"    on (t0.n = t1.n)"
			"  when not matched then insert values (n)",
			FbTest::DIALECT, IStatement::PREPARE_PREFETCH_AFFECTED_RECORDS);
		BOOST_CHECK(status->isSuccess());

		stmt->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		rec = stmt->getAffectedRecords(status);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK_EQUAL(rec, 4);
	}

	stmt->free(status, DSQL_drop);
	BOOST_CHECK(status->isSuccess());

	transaction->commit(status);
	BOOST_CHECK(status->isSuccess());

	attachment->drop(status);
	BOOST_CHECK(status->isSuccess());

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
