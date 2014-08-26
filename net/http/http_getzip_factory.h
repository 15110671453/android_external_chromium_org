/**
 * Copyright (c) 2012, 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other *materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef HTTP_GETZIP_FACTORY_H_
#define HTTP_GETZIP_FACTORY_H_

#include "net/socket/stream_socket.h"
#include <sys/types.h>
#include "base/basictypes.h"

namespace net
{

//forward declarations
class HttpRequestHeaders;
class HttpResponseHeaders;

typedef enum
{
  GETZIP_OK                  = 1,
  REQUEST_RETRY_NEEDED       = 2, //GETzip failure that requires last request retry
  NULL_ARGUMENT              = 3, //One of the passed argument was NULL
  NO_GETZIP_CONNECTION       = 4  //No GETzip connection was found
} GETZipDecompressionStatus;

//Main GetZip interface
class __attribute__((visibility("default"))) IGetZipManager
{
public:
  IGetZipManager() {};
  virtual void CompressRequestHeaders(HttpRequestHeaders&, StreamSocket*) = 0;
  virtual GETZipDecompressionStatus
      DecompressResponseHeaders(HttpResponseHeaders*, StreamSocket*) = 0;
  virtual void StopGetZipConnection(StreamSocket*) = 0;
  virtual void OpenGetZipConnection(StreamSocket*) = 0;
  virtual ~IGetZipManager() {};

private:
  DISALLOW_COPY_AND_ASSIGN(IGetZipManager);
};

//Simple, non private GetZip manager implementation
class GetZipManager: public IGetZipManager
{
public:
  GetZipManager();
  virtual ~GetZipManager() {}
  virtual void CompressRequestHeaders(HttpRequestHeaders&, StreamSocket*) OVERRIDE {}
  virtual GETZipDecompressionStatus DecompressResponseHeaders(HttpResponseHeaders*, StreamSocket*) OVERRIDE;
  virtual void StopGetZipConnection(StreamSocket*) OVERRIDE {}
  virtual void OpenGetZipConnection(StreamSocket*) OVERRIDE {}

private:
  DISALLOW_COPY_AND_ASSIGN(GetZipManager);
};

//This class is used to initialize GetZip manager
//First tries to initialize GetZipManager from proprietary library,
//if the library does not exist, creates GetZipManager
//Note: In the current implementation of network stack all the actions
//related to GetZipManager and GetZipFactory are carried out via IOThread
//hence, the implementation is not synchronized (this might change in the future).
class HttpGetZipFactory
{

public:

  //GetZipManager is kept within HttpGetZipFactory,
  //which is singleton.
  //This method is used to access the GetZipManager
  //Don't use it before initializing HttpGetZipFactory object.
  static IGetZipManager* GetGETZipManager();

  //Initialization must be called before accessing
  //GetZipManager for the first time.
  static void InitGETZipManager();

  void StopGETZipManager();

private:
  IGetZipManager* m_pMngr;
  static HttpGetZipFactory* s_pFactory;

  void* libHandle;

  HttpGetZipFactory();
  ~HttpGetZipFactory();

  //DISALLOW_COPY_AND_ASSIGN(HttpGetZipFactory);
};

}; //end network
#endif /* HTTP_GETZIP_FACTORY_H_ */
