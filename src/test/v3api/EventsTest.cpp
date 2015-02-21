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
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace V3Util;
using namespace Firebird;
using std::string;
using boost::atomic;
using boost::thread;
using boost::mutex;
using boost::lock_guard;

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


namespace
{
	class PostThread
	{
	public:
		const static int COUNT = 5;

		PostThread(IAttachment* aAttachment)
			: attachment(aAttachment)
		{
		}

		PostThread(const PostThread& o)
			: attachment(o.attachment)
		{
		}

		void operator ()()
		{
			const char cmdTra[] = "set transaction";
			const char cmdBlock[] = "execute block as begin post_event 'EVENT1'; end";

			CheckStatusWrapper statusWrapper(master->getStatus());
			CheckStatusWrapper* status = &statusWrapper;

			ITransaction* transaction = attachment->execute(status, NULL, strlen(cmdTra), cmdTra,
				3, NULL, NULL, NULL, NULL);
			BOOST_CHECK(checkStatus(status));
			BOOST_REQUIRE(transaction);

			for (int i = 0; i < COUNT; ++i)
			{
				attachment->execute(status, transaction, strlen(cmdBlock), cmdBlock, 3,
					NULL, NULL, NULL, NULL);
				BOOST_CHECK(checkStatus(status));
			}

			transaction->commit(status);
			BOOST_CHECK(checkStatus(status));

			status->dispose();
		}

		IAttachment* attachment;
	};

	class Event : public IEventCallbackImpl<Event, CheckStatusWrapper>
	{
	public:
		Event(IAttachment* aAttachment, volatile int* aCounter)
			: refCounter(1),
			  attachment(aAttachment),
			  counter(aCounter),
			  statusWrapper(master->getStatus()),
			  status(&statusWrapper)
		{
			eveLen = isc_event_block(&eveBuffer, &eveResult, 1, "EVENT1");

			// Make queEvents wait instead of return counters before no event was happened.
			eveBuffer[eveLen - 4] = 1;

			mut.lock();

			events = attachment->queEvents(status, this, eveLen, eveBuffer);
			BOOST_CHECK(checkStatus(status));
		}

		~Event()
		{
			if (events)
			{
				events->cancel(status);
				BOOST_CHECK(checkStatus(status));
			}

			status->dispose();

			isc_free((char*) eveBuffer);
			isc_free((char*) eveResult);
		}

		virtual IPluginModule* getModule()
		{
			return NULL;
		}

		virtual void addRef()
		{
			++refCounter;
		}

		virtual int release()
		{
			if (--refCounter == 0)
			{
				delete this;
				return 0;
			}
			else
				return 1;
		}

		virtual void eventCallbackFunction(unsigned int length, const ISC_UCHAR* events)
		{
			ISC_ULONG increment = 0;
			isc_event_counts(&increment, eveLen, eveBuffer, events);

			*counter += increment;
			mut.unlock();
		}

		atomic<int> refCounter;

		IAttachment* attachment;
		volatile int* counter;

		CheckStatusWrapper statusWrapper;
		CheckStatusWrapper* status;
		IEvents* events;
		unsigned char* eveBuffer;
		unsigned char* eveResult;
		unsigned eveLen;

		mutex mut;
	};
}


BOOST_AUTO_TEST_CASE(events)
{
	const string location = FbTest::getLocation("events.fdb");

	CheckStatusWrapper statusWrapper(master->getStatus());
	CheckStatusWrapper* status = &statusWrapper;

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(),
		sizeof(FbTest::ASCII_DPB), FbTest::ASCII_DPB);
	BOOST_CHECK(checkStatus(status));
	BOOST_REQUIRE(attachment);

	volatile int counter = 1;

	{	// scope
		Event event(attachment, &counter);

		BOOST_CHECK(counter == 1);

		thread thd = thread(PostThread(attachment));
		lock_guard<mutex> guard(event.mut);
		thd.join();

		BOOST_CHECK(counter == PostThread::COUNT + 1);
	}

	attachment->dropDatabase(status);
	BOOST_CHECK(checkStatus(status));

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
