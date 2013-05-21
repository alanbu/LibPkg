// This file is part of LibPkg.
// Copyright © 2003-2005 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#ifndef LIBPKG_DOWNLOAD
#define LIBPKG_DOWNLOAD

#include <string>
#include <fstream>

#include "curl/curl.h"

namespace pkg {

using std::string;

/** A class for downloading a file from a URL. */
class download
{
public:
	/** A type for representing byte counts. */
	typedef unsigned long long size_type;

	/** A null value for use in place of a byte count. */
	static const size_type npos=static_cast<size_type>(-1);

	// An enumeration for describing the state of the download. */
	enum state_type
	{
		/** The state in which the download is in progress. */
		state_download,
		/** The state in which the download has been successfully
		 * completed. */
		state_done,
		/** The state in which the download has failed. */
		state_fail
	};
private:
	/** The current state of the download. */
	state_type _state;

	/** The libcurl easy handle. */
	CURL* _ceasy;

	/** The libcurl result code. */
	CURLcode _result;

	/** The libcurl error buffer. */
	char* _error_buffer;

	/** The URL from which to download. */
	string _url;

	/** The stream to which the file is to be written. */
	std::ofstream _out;

	/** The number of bytes downloaded. */
	size_type _bytes_done;

	/** The total number of bytes to download, or npos if not known. */
	size_type _bytes_total;
public:
	/** Construct download action.
	 * @param url the URL from which to download
	 * @param pathname the pathname to which the file is to be written
	 */
	download(const string& url,const string& pathname);

	/** Destroy download action. */
	~download();

	/** Get current state of the download.
	 * @return the current state
	 */
	state_type state() const
		{ return _state; }

	/** Get libcurl result code.
	 * @return the result code
	 */
	CURLcode result() const
		{ return _result; }

	/** Get libcurl error message.
	 * @return the message
	 */
	string message() const
		{ return _error_buffer; }

	/** Get number of bytes downloaded.
	 * @return the number of bytes downloaded
	 */
	size_type bytes_done()
		{ return _bytes_done; }

	/** Get total number of bytes to download.
	 * @return the total number of bytes, or npos if not known
	 */
	size_type bytes_total()
		{ return _bytes_total; }

	/** Handler for CURLOPT_WRITEFUNCTION callbacks.
	 * @param buffer the data buffer
	 * @param size the size of each data item
	 * @param nitems the number of data items
	 * @return the number of bytes written (size*nitems)
	 */
	size_t write_callback(char* buffer,size_t size,size_t nitems);

	/** Handler for CURLOPT_PROGRESSFUNCTION callbacks.
	 * @param dltotal the total number of bytes to download, or 0 if not known
	 * @param dlnow the number of bytes downloaded
	 * @return zero
	 */
	int progress_callback(double dltotal,double dlnow);

	/** Handler for Curl messages.
	 * @param msg the message
	 */
	void message_callback(CURLMsg* msg);
private:
	/** The libcurl multi handle.
	 * This is shared between all downloads.  A multi handle is created
	 * whenever there is at least one easy handle to attach to it.  When
	 * all easy handles have been removed, the multi handle is deleted
	 * (and the pointer set to null).
	 */
	static CURLM* _cmulti;

	/** The number of libcurl easy handles attached to the shared
	 * multi handle. */
	static unsigned int _cmulti_refcount;
public:
	/** Poll all download operations. */
	static void poll_all();
};

}; /* namespace pkg */

#endif
