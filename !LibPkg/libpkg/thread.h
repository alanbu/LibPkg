// This file is part of LibPkg.
//
// Copyright 2003-2020 Graham Shaw
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIBPKG_THREAD
#define LIBPKG_THREAD

namespace pkg {

/** A mixin class to represent a cooperative thread. */
class thread
{
public:
	/** Construct thread. */
	thread();

	/** Destroy thread. */
	virtual ~thread();
protected:
	/** Poll this thread.
	 * This function will be called repeatedly until the thread ceases to
	 * exist.  The amount of work done per invokation should be kept small,
	 * in order that multithreading (within the application) and multitasking
	 * (across RISC OS as a whole) operate smoothly.
	 */
	virtual void poll();
public:
	/** Poll all threads.
	 * Call the poll() method for all threads currently in existance.
	 */
	static void poll_all();
};

}; /* namespace pkg */

#endif
