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

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/scoped_array.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <vector>
#include <ibase.h>

namespace po = boost::program_options;
using boost::scoped_array;
using boost::thread;
using namespace std;


//--------------------------------------


static string database;
static string method;
static int requestMerge = 1;
static string table;


//--------------------------------------


static void check(const ISC_STATUS* status);
static void run();
static int start(int argc, char* argv[]);


//--------------------------------------


static void check(const ISC_STATUS* status)
{
	if (status[1] != 0)
	{
		string message;
		char buffer[1024];

		while (fb_interpret(buffer, sizeof(buffer), &status))
		{
			if (!message.empty())
			{
				message += "\n";
				message += "-";
			}

			message += buffer;
		}

		throw runtime_error(message);
	}
}

static void pushString(vector<unsigned char>& vec, const string& s)
{
	vec.push_back(s.length());

	for (const char* p = s.c_str(); *p; ++p)
		vec.push_back((unsigned char) *p);
}

static void run(int repeat)
{
	ISC_STATUS_ARRAY status = {0};

	FB_API_HANDLE attachment = 0;
	isc_attach_database(status, 0, database.c_str(), &attachment, 0, NULL);
	check(status);

	FB_API_HANDLE transaction = 0;
	isc_start_transaction(status, &transaction, 1, &attachment, 0, NULL);
	check(status);

	{
		FB_API_HANDLE statement = 0;
		isc_dsql_allocate_statement(status, &attachment, &statement);
		check(status);

		scoped_array<char> sqlBuffer(new char[XSQLDA_LENGTH(1)]);
		memset(sqlBuffer.get(), 0, sizeof(sqlBuffer));

		XSQLDA* sqlda = (XSQLDA*) sqlBuffer.get();
		sqlda->version = 1;

		string stmtText = "select * from \"" + table + "\"";
		isc_dsql_prepare(status, &transaction, &statement, 0, stmtText.c_str(), 3, sqlda);
		check(status);

		unsigned sqld = sqlda->sqld;

		sqlBuffer.reset(new char[XSQLDA_LENGTH(sqld)]);
		memset(sqlBuffer.get(), 0, sizeof(sqlBuffer));

		sqlda = (XSQLDA*) sqlBuffer.get();
		sqlda->version = 1;
		sqlda->sqln = sqlda->sqld = sqld;

		isc_dsql_describe(status, &statement, 3, sqlda);
		check(status);

		isc_dsql_free_statement(status, &statement, DSQL_unprepare);
		check(status);

		stmtText = "insert into \"" + table + "\" values (";

		for (unsigned i = 0; i < sqld; ++i)
		{
			if (i > 0)
				stmtText += ", ";

			stmtText += "?";
		}

		stmtText += ")";

		static const unsigned char BLR_HEADER[] = {
			blr_version5,
			blr_begin,
			blr_message, 0, 0, 0
		};

		if (method == "sqlda")
		{
			isc_dsql_prepare(status, &transaction, &statement, 0, stmtText.c_str(), 3, NULL);
			check(status);

			ISC_SHORT nullInd = 0;

			for (unsigned i = 0; i < sqlda->sqln; ++i)
			{
				sqlda->sqlvar[i].sqltype = SQL_SHORT;
				sqlda->sqlvar[i].sqldata = (char*) &nullInd;
				sqlda->sqlvar[i].sqllen = sizeof(nullInd);
				sqlda->sqlvar[i].sqlind = &nullInd;
			}

			for (int i = 0; i < repeat; ++i)
			{
				isc_dsql_execute(status, &transaction, &statement, 3, sqlda);
				check(status);
			}
		}
		else if (method == "message")
		{
			isc_dsql_prepare_m(status, &transaction, &statement, 0, stmtText.c_str(), 3,
				0, NULL, 0, NULL);
			check(status);

			vector<unsigned char> blr;
			vector<unsigned char> message;

			blr.insert(blr.end(), BLR_HEADER, BLR_HEADER + sizeof(BLR_HEADER));

			for (unsigned i = 0; i < sqld; ++i)
			{
				blr.push_back(blr_short);
				blr.push_back(0);
				blr.push_back(blr_short);
				blr.push_back(0);

				message.push_back(0);
				message.push_back(0);
				message.push_back(0);
				message.push_back(0);
			}

			blr.push_back(blr_end);
			blr.push_back(blr_eoc);
			blr[4] = (sqld * 2) & 0xFF;
			blr[5] = ((sqld * 2) >> 8) & 0xFF;

			for (int i = 0; i < repeat; ++i)
			{
				isc_dsql_execute_m(status, &transaction, &statement,
					blr.size(), (char*) &blr.front(), 0,
					message.size(), (char*) &message.front());
				check(status);
			}
		}
		else if (method == "request-execs" || method == "request-sends")
		{
			bool sends = (method == "request-sends");
			int merge = requestMerge;
			int left = repeat % merge;
			repeat /= merge;

			if (repeat == 0)
			{
				repeat = left;
				left = 0;
			}

			while (repeat > 0)
			{
				vector<unsigned char> blr;
				vector<unsigned char> message;

				blr.insert(blr.end(), BLR_HEADER, BLR_HEADER + sizeof(BLR_HEADER));

				for (unsigned i = 0; i < sqld; ++i)
				{
					for (unsigned j = 0; j < merge; ++j)
					{
						blr.push_back(blr_short);
						blr.push_back(0);
						blr.push_back(blr_short);
						blr.push_back(0);

						message.push_back(0);
						message.push_back(0);
						message.push_back(0);
						message.push_back(0);
					}
				}

				if (sends)
					blr.push_back(blr_loop);

				blr.push_back(blr_receive);
				blr.push_back(0);

				if (merge > 1)
					blr.push_back(blr_begin);

				for (unsigned j = 0; j < merge; ++j)
				{
					blr.push_back(blr_store);
					blr.push_back(blr_relation);
					pushString(blr, table);
					blr.push_back(j);	// context

					blr.push_back(blr_begin);

					for (unsigned i = 0; i < sqld; ++i)
					{
						blr.push_back(blr_assignment);

						unsigned n = (j * sqld * 2) + (i * 2);
						blr.push_back(blr_parameter2);
						blr.push_back(0);
						blr.push_back(n & 0xFF);
						blr.push_back((n >> 8) & 0xFF);
						blr.push_back((n + 1) & 0xFF);
						blr.push_back(((n + 1) >> 8) & 0xFF);

						blr.push_back(blr_field);
						blr.push_back(j);	// context

						pushString(blr, sqlda->sqlvar[i].sqlname);
					}

					blr.push_back(blr_end);
				}

				if (merge > 1)
					blr.push_back(blr_end);

				blr.push_back(blr_end);
				blr.push_back(blr_eoc);
				blr[4] = (sqld * merge * 2) & 0xFF;
				blr[5] = ((sqld * merge * 2) >> 8) & 0xFF;

				FB_API_HANDLE request = 0;

				isc_compile_request(status, &attachment, &request, blr.size(), (char*) &blr.front());
				check(status);

				if (sends)
				{
					isc_start_and_send(status, &request, &transaction, 0,
						message.size(), (char*) &message.front(), 0);
					check(status);

					for (int i = 1; i < repeat; ++i)
					{
						isc_send(status, &request, 0, message.size(), (char*) &message.front(), 0);
						check(status);
					}
				}
				else
				{
					for (int i = 0; i < repeat; ++i)
					{
						isc_start_and_send(status, &request, &transaction, 0,
							message.size(), (char*) &message.front(), 0);
						check(status);
					}
				}

				isc_release_request(status, &request);
				check(status);

				repeat = left;
				left = 0;
				merge = 1;
			}
		}

		isc_dsql_free_statement(status, &statement, DSQL_drop);
		check(status);
	}

	isc_commit_transaction(status, &transaction);
	check(status);

	isc_detach_database(status, &attachment);
	check(status);
}

static int start(int argc, char* argv[])
{
	int repeat = 1;
	int threads = 1;

	po::options_description options("Options");
	options.add_options()
		("help", "help")
		("database", po::value<string>(&database), "database")
		("method", po::value<string>(&method), "method")
		("repeat", po::value<int>(&repeat), "repeat count")
		("request-merge", po::value<int>(&requestMerge), "request merge")
		("table", po::value<string>(&table), "table name")
		("threads", po::value<int>(&threads), "number of threads")
	;

	po::variables_map optionsMap;
	po::store(po::parse_command_line(argc, argv, options), optionsMap);
	po::notify(optionsMap);    

	if (optionsMap.count("help"))
	{
		cout << options << "\n";
		return 1;
	}

	if (threads == 1)
		run(repeat);
	else
	{
		int left = repeat % threads;
		repeat /= threads;

		vector<thread*> vecThreads;

		for (int i = 0; i < threads; ++i)
		{
			int n = repeat + (i == 0 ? left : 0);
			vecThreads.push_back(new thread(boost::bind(&run, n)));
		}

		for (int i = 0; i < threads; ++i)
		{
			vecThreads[i]->join();
			delete vecThreads[i];
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{
	try
	{
		return start(argc, argv);
	}
	catch (const exception& e)
	{
		cerr << e.what() << endl;
		return 1;
	}
}
