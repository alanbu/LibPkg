// This file is part of LibPkg.
// Copyright © 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef _LIBPKG_THREAD
#define _LIBPKG_THREAD

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
