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

#include "libpkg/download.h"

#include "unixlib/local.h"

extern int __riscosify_control;

extern "C" {

/** A callback function for CURLOPT_WRITEFUNCTION.
 * @param buffer the data buffer
 * @param size the size of each data item
 * @param nitems the number of data items
 * @param dl the private data (a pointer to a download object)
 * @return the number of bytes written (size*nitems)
 */
static size_t download_write(char* buffer,size_t size,size_t nitems,void* dl)
{
	return static_cast<pkg::download*>(dl)->write_callback(buffer,size,nitems);
}

/** A callback function for CURLOPT_PROGRESSFUNCTION.
 * @param dl the private data (a pointer to a download object)
 * @param dltotal the total number of bytes to download, or 0 if not known
 * @param dlnow the number of bytes downloaded
 * @param ultotal the total number of bytes to upload, or 0 if not known
 * @param ulnow the number of bytes uploaded
 * @return zero
 */
static int download_progress(void* dl,double dltotal,double dlnow,
	double ultotal,double ulnow)
{
	return static_cast<pkg::download*>(dl)->progress_callback(dltotal,dlnow);
}

#ifdef LOG_DOWNLOAD
static int debug_function(CURL *handle,curl_infotype type,char *data, size_t size, void *userptr)
{
	(void *)handle;
	return static_cast<pkg::download*>(userptr)->debug_callback(type, data, size);
}
#endif


}; /* extern "C" */

namespace pkg {

download::download(const string& url,const string& pathname, download::options *opts /*= nullptr*/) :
	_state(state_download),
	_ceasy(curl_easy_init()),
	_result(CURLE_OK),
	_error_buffer(new char[CURL_ERROR_SIZE]),
	_url(url),
	_out(pathname.c_str()),
	_bytes_done(0),
	_bytes_total(npos)
{
	_error_buffer[0]=0;
	#ifdef LOG_DOWNLOAD
	_log = nullptr;
	#endif

	if (!_cmulti) _cmulti=curl_multi_init();
	++_cmulti_refcount;

	int riscosify_control=__riscosify_control;
	__riscosify_control=0;

	curl_easy_setopt(_ceasy,CURLOPT_PRIVATE,this);
	curl_easy_setopt(_ceasy,CURLOPT_URL,_url.c_str());
	curl_easy_setopt(_ceasy,CURLOPT_WRITEFUNCTION,&download_write);
	curl_easy_setopt(_ceasy,CURLOPT_WRITEDATA,this);
	curl_easy_setopt(_ceasy,CURLOPT_PROGRESSFUNCTION,&download_progress);
	curl_easy_setopt(_ceasy,CURLOPT_PROGRESSDATA,this);
	curl_easy_setopt(_ceasy,CURLOPT_NOPROGRESS,false);
	curl_easy_setopt(_ceasy,CURLOPT_FAILONERROR,true);
	curl_easy_setopt(_ceasy,CURLOPT_ERRORBUFFER,_error_buffer);
	curl_easy_setopt(_ceasy, CURLOPT_CONNECTTIMEOUT, 120L);
	curl_easy_setopt(_ceasy, CURLOPT_CAINFO, "/<CaCertificates$Dir>/ca-certificates.crt");
	#ifdef LOG_DOWNLOAD
		curl_easy_setopt(_ceasy, CURLOPT_DEBUGFUNCTION, &debug_function);
		curl_easy_setopt(_ceasy, CURLOPT_DEBUGDATA, this);
		curl_easy_setopt(_ceasy, CURLOPT_VERBOSE, 1L);
	#endif
	// follow 301 Moved Permanently and similar redirects
	curl_easy_setopt(_ceasy,CURLOPT_FOLLOWLOCATION,1);

	if (opts)
	{
		if (opts->use_proxy)
		{
			if (!opts->proxy.empty())
			{
				curl_easy_setopt(_ceasy, CURLOPT_PROXY, opts->proxy.c_str());
				if (!opts->do_not_proxy.empty())
				{
					curl_easy_setopt(_ceasy, CURLOPT_NOPROXY, opts->do_not_proxy.c_str());
				}
			}
		}
	}
	curl_multi_add_handle(_cmulti,_ceasy);

	__riscosify_control=riscosify_control;
}

#ifdef LOG_DOWNLOAD
void download::log_to(log *use_log)
{
	_log = use_log;
	if (_log)
	{
		_log->message(LOG_INFO_DOWNLOAD_INFO, "Download logging on xxx");
	}
}
#endif


download::~download()
{
	int riscosify_control=__riscosify_control;
	__riscosify_control=0;

	curl_multi_remove_handle(_cmulti,_ceasy);
	curl_easy_cleanup(_ceasy);
	if (!--_cmulti_refcount)
	{
		curl_multi_cleanup(_cmulti);
		_cmulti=0;
	}
	delete[] _error_buffer;

	__riscosify_control=riscosify_control;
}

size_t download::write_callback(char* buffer,size_t size,size_t nitems)
{
	_out.write(buffer,nitems*size);
	return nitems*size;
}

int download::progress_callback(double dltotal,double dlnow)
{
	_bytes_done=static_cast<unsigned long long>(dlnow);
	_bytes_total=static_cast<unsigned long long>(dltotal);
	if ((_bytes_total==0)&&(_state==state_download)) _bytes_total=npos;
	return 0;
}

void download::message_callback(CURLMsg* msg)
{
	if (msg->msg==CURLMSG_DONE)
	{
		if (msg->data.result==CURLE_OK) _state=state_done;
		else _state=state_fail;
		_result=msg->data.result;
	}
}


#ifdef LOG_DOWNLOAD
int download::debug_callback(curl_infotype type,char *data, size_t size)
{
	if (!_log) return 0;
	char size_text[16];
	sprintf(size_text,"%d",(int)size);

	switch(type)
	{
		case CURLINFO_TEXT:
			_log->message(LOG_INFO_DOWNLOAD_INFO, data);
			break; 
  		case CURLINFO_HEADER_OUT:
			_log->message(LOG_INFO_DOWNLOAD_HEADER, "sent", size_text);
			break;
		case CURLINFO_DATA_OUT:
			_log->message(LOG_INFO_DOWNLOAD_DATA, "sent", size_text);
			break;
		case CURLINFO_SSL_DATA_OUT:
			_log->message(LOG_INFO_DOWNLOAD_DATA, "SSL sent", size_text);
			break;
		case CURLINFO_HEADER_IN:
			_log->message(LOG_INFO_DOWNLOAD_HEADER, "received", size_text);
			break;
		case CURLINFO_DATA_IN:
			_log->message(LOG_INFO_DOWNLOAD_DATA, "received", size_text);
			break;
		case CURLINFO_SSL_DATA_IN:
			_log->message(LOG_INFO_DOWNLOAD_DATA, "SSL received", size_text);
			break;

		case CURLINFO_END:
			// Stop compiler warning
			break;
	}
	return 0;
}
#endif

CURLM* download::_cmulti=0;
unsigned int download::_cmulti_refcount=0;

void download::poll_all()
{
	if (_cmulti)
	{
		int riscosify_control=__riscosify_control;
		__riscosify_control=0;

		int running_handles=0;
		CURLMcode mcode = curl_multi_perform(_cmulti,&running_handles);

		int msgs_in_queue=0;
		CURLMsg* msg=curl_multi_info_read(_cmulti,&msgs_in_queue);
		while (msg)
		{
			char* private_data;
			curl_easy_getinfo(msg->easy_handle,CURLINFO_PRIVATE,&private_data);
			reinterpret_cast<download*>(private_data)->message_callback(msg);
			msg=curl_multi_info_read(_cmulti,&msgs_in_queue);
		}

		__riscosify_control=riscosify_control;
	}
}

}; /* namespace pkg */
