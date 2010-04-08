// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <atlbase.h>
#include <atlcom.h>
#include "app/win_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gmock_mutant.h"

#include "chrome_frame/urlmon_url_request.h"
#include "chrome_frame/urlmon_url_request_private.h"
#include "chrome_frame/test/chrome_frame_test_utils.h"
#include "chrome_frame/test/http_server.h"
using testing::CreateFunctor;

const int kChromeFrameLongNavigationTimeoutInSeconds = 10;

static void AppendToStream(IStream* s, void* buffer, ULONG cb) {
  ULONG bytes_written;
  LARGE_INTEGER current_pos;
  LARGE_INTEGER zero = {0};
  // Remember current position.
  ASSERT_HRESULT_SUCCEEDED(s->Seek(zero, STREAM_SEEK_CUR,
      reinterpret_cast<ULARGE_INTEGER*>(&current_pos)));
  // Seek to the end.
  ASSERT_HRESULT_SUCCEEDED(s->Seek(zero, STREAM_SEEK_END, NULL));
  ASSERT_HRESULT_SUCCEEDED(s->Write(buffer, cb, &bytes_written));
  ASSERT_EQ(cb, bytes_written);
  // Seek to original position.
  ASSERT_HRESULT_SUCCEEDED(s->Seek(current_pos, STREAM_SEEK_SET, NULL));
}

TEST(UrlmonUrlRequestCache, ReadWrite) {
  UrlmonUrlRequest::Cache cache;
  ScopedComPtr<IStream> stream;
  ASSERT_HRESULT_SUCCEEDED(::CreateStreamOnHGlobal(0, TRUE, stream.Receive()));
  cache.Append(stream);
  ASSERT_EQ(0, cache.size());
  size_t bytes_read;
  const size_t BUF_SIZE = UrlmonUrlRequest::Cache::BUF_SIZE;
  scoped_array<uint8> buffer(new uint8[BUF_SIZE * 2]);

  AppendToStream(stream, "hello", 5u);
  ASSERT_TRUE(cache.Append(stream));
  ASSERT_HRESULT_SUCCEEDED(cache.Read(buffer.get(), 2u, &bytes_read));
  ASSERT_EQ(2, bytes_read);
  ASSERT_EQ('h', buffer[0]);
  ASSERT_EQ('e', buffer[1]);

  AppendToStream(stream, "world\0", 6u);
  ASSERT_TRUE(cache.Append(stream));
  ASSERT_HRESULT_SUCCEEDED(cache.Read(buffer.get(), 1u, &bytes_read));
  ASSERT_EQ(1, bytes_read);
  ASSERT_EQ('l', buffer[0]);
  ASSERT_HRESULT_SUCCEEDED(cache.Read(buffer.get(), 100u, &bytes_read));
  ASSERT_EQ(8, bytes_read);
  ASSERT_STREQ("loworld", (const char*)buffer.get());

  memset(buffer.get(), '1', BUF_SIZE / 2);
  AppendToStream(stream, buffer.get(), BUF_SIZE / 2);
  cache.Append(stream);
  memset(buffer.get(), '2', BUF_SIZE);
  AppendToStream(stream, buffer.get(), BUF_SIZE);
  memset(buffer.get(), '3', BUF_SIZE * 3 / 4);
  AppendToStream(stream, buffer.get(), BUF_SIZE * 3 / 4);
  cache.Append(stream);

  cache.Read(buffer.get(), BUF_SIZE / 2, &bytes_read);
  ASSERT_EQ(BUF_SIZE / 2, bytes_read);
  ASSERT_EQ('1', buffer[0]);
  ASSERT_EQ('1', buffer[BUF_SIZE / 4]);
  ASSERT_EQ('1', buffer[BUF_SIZE /2 - 1]);

  cache.Read(buffer.get(), BUF_SIZE, &bytes_read);
  ASSERT_EQ(BUF_SIZE, bytes_read);
  ASSERT_EQ('2', buffer[0]);
  ASSERT_EQ('2', buffer[BUF_SIZE /2]);
  ASSERT_EQ('2', buffer[BUF_SIZE - 1]);

  cache.Read(buffer.get(), BUF_SIZE * 3 / 4, &bytes_read);
  ASSERT_EQ(BUF_SIZE * 3 / 4, bytes_read);
  ASSERT_EQ('3', buffer[0]);
  ASSERT_EQ('3', buffer[BUF_SIZE / 2]);
  ASSERT_EQ('3', buffer[BUF_SIZE * 3 / 4 - 1]);
  cache.Read(buffer.get(), 11, &bytes_read);
  ASSERT_EQ(0, bytes_read);
}


class MockUrlDelegate : public PluginUrlRequestDelegate {
 public:
  MOCK_METHOD7(OnResponseStarted, void(int request_id, const char* mime_type,
      const char* headers, int size, base::Time last_modified,
      const std::string& redirect_url, int redirect_status));
  MOCK_METHOD2(OnReadComplete, void(int request_id, const std::string& data));
  MOCK_METHOD2(OnResponseEnd, void(int request_id,
                                   const URLRequestStatus& status));

  static bool ImplementsThreadSafeReferenceCounting() {
    return false;
  }
  void AddRef() {}
  void Release() {}

  void PostponeReadRequest(chrome_frame_test::TimedMsgLoop* loop,
                   UrlmonUrlRequest* request, int bytes_to_read) {
    loop->PostDelayedTask(FROM_HERE, NewRunnableMethod(this,
        &MockUrlDelegate::RequestRead, request, bytes_to_read), 0);
  }

 private:
  void RequestRead(UrlmonUrlRequest* request, int bytes_to_read) {
    request->Read(bytes_to_read);
  }
};

// Simplest UrlmonUrlRequest. Retrieve a file from local web server.
TEST(UrlmonUrlRequestTest, Simple1) {
  MockUrlDelegate mock;
  ChromeFrameHTTPServer server;
  chrome_frame_test::TimedMsgLoop loop;
  win_util::ScopedCOMInitializer init_com;
  CComObjectStackEx<UrlmonUrlRequest> request;

  server.SetUp();
  request.AddRef();
  request.Initialize(&mock, 1,  // request_id
      server.Resolve(L"files/chrome_frame_window_open.html").spec(),
      "get",
      "",      // referrer
      "",      // extra request
      NULL,    // upload data
      true);   // frame busting

  testing::InSequence s;
  EXPECT_CALL(mock, OnResponseStarted(1, testing::_, testing::_, testing::_,
                                      testing::_, testing::_, testing::_))
    .Times(1)
    .WillOnce(testing::IgnoreResult(testing::InvokeWithoutArgs(CreateFunctor(
        &request, &UrlmonUrlRequest::Read, 512))));


  EXPECT_CALL(mock, OnReadComplete(1, testing::Property(&std::string::size,
                                                        testing::Gt(0u))))
    .Times(testing::AtLeast(1))
    .WillRepeatedly(testing::InvokeWithoutArgs(CreateFunctor(&mock,
        &MockUrlDelegate::PostponeReadRequest, &loop, &request, 64)));

  EXPECT_CALL(mock, OnResponseEnd(1, testing::_))
    .Times(1)
    .WillOnce(QUIT_LOOP_SOON(loop, 2));

  request.Start();
  loop.RunFor(kChromeFrameLongNavigationTimeoutInSeconds);
  request.Release();
  server.TearDown();
}

// Same as Simple1 except we use the HEAD verb to fetch only the headers
// from the server.
TEST(UrlmonUrlRequestTest, Head) {
  MockUrlDelegate mock;
  ChromeFrameHTTPServer server;
  chrome_frame_test::TimedMsgLoop loop;
  win_util::ScopedCOMInitializer init_com;
  CComObjectStackEx<UrlmonUrlRequest> request;

  server.SetUp();
  request.AddRef();
  request.Initialize(&mock, 1,  // request_id
      server.Resolve(L"files/chrome_frame_window_open.html").spec(),
      "head",
      "",      // referrer
      "",      // extra request
      NULL,    // upload data
      true);   // frame busting

  testing::InSequence s;
  EXPECT_CALL(mock, OnResponseStarted(1, testing::_, testing::_, testing::_,
                                      testing::_, testing::_, testing::_))
    .Times(1)
    .WillOnce(testing::IgnoreResult(testing::InvokeWithoutArgs(CreateFunctor(
        &request, &UrlmonUrlRequest::Read, 512))));


  // For HEAD requests we don't expect content reads.
  EXPECT_CALL(mock, OnReadComplete(1, testing::_)).Times(0);

  EXPECT_CALL(mock, OnResponseEnd(1, testing::_))
    .Times(1)
    .WillOnce(QUIT_LOOP_SOON(loop, 2));

  request.Start();
  loop.RunFor(kChromeFrameLongNavigationTimeoutInSeconds);
  request.Release();
  server.TearDown();
}

TEST(UrlmonUrlRequestTest, UnreachableUrl) {
  MockUrlDelegate mock;
  chrome_frame_test::TimedMsgLoop loop;
  win_util::ScopedCOMInitializer init_com;
  CComObjectStackEx<UrlmonUrlRequest> request;
  ChromeFrameHTTPServer server;
  server.SetUp();
  GURL unreachable = server.Resolve(L"files/non_existing.html");
  server.TearDown();

  request.AddRef();
  request.Initialize(&mock, 1,  // request_id
      unreachable.spec(), "get",
      "",      // referrer
      "",      // extra request
      NULL,    // upload data
      true);   // frame busting

  EXPECT_CALL(mock, OnResponseEnd(1, testing::Property(
              &URLRequestStatus::os_error, net::ERR_TUNNEL_CONNECTION_FAILED)))
    .Times(1)
    .WillOnce(QUIT_LOOP_SOON(loop, 2));

  request.Start();
  loop.RunFor(kChromeFrameLongNavigationTimeoutInSeconds);
  request.Release();
}

TEST(UrlmonUrlRequestTest, ZeroLengthResponse) {
  MockUrlDelegate mock;
  ChromeFrameHTTPServer server;
  chrome_frame_test::TimedMsgLoop loop;
  win_util::ScopedCOMInitializer init_com;
  CComObjectStackEx<UrlmonUrlRequest> request;

  server.SetUp();
  request.AddRef();
  request.Initialize(&mock, 1,  // request_id
      server.Resolve(L"files/empty.html").spec(), "get",
      "",      // referrer
      "",      // extra request
      NULL,    // upload data
      true);   // frame busting

  // Expect headers
  EXPECT_CALL(mock, OnResponseStarted(1, testing::_, testing::_, testing::_,
                                      testing::_, testing::_, testing::_))
    .Times(1)
    .WillOnce(QUIT_LOOP(loop));

  request.Start();
  loop.RunFor(kChromeFrameLongNavigationTimeoutInSeconds);
  EXPECT_FALSE(loop.WasTimedOut());

  // Should stay quiet, since we do not ask for anything for awhile.
  EXPECT_CALL(mock, OnResponseEnd(1, testing::_)).Times(0);
  loop.RunFor(3);

  // Invoke read. Only now the response end ("server closed the connection")
  // is supposed to be delivered.
  EXPECT_CALL(mock, OnResponseEnd(1, testing::Property(
                                     &URLRequestStatus::is_success, true)))
      .Times(1);
  request.Read(512);
  request.Release();
  server.TearDown();
}

ACTION_P4(ManagerRead, loop, mgr, request_id, bytes_to_read) {
  loop->PostDelayedTask(FROM_HERE, NewRunnableMethod(mgr,
      &UrlmonUrlRequestManager::ReadUrlRequest, 0, request_id,
      bytes_to_read), 0);
}
ACTION_P3(ManagerEndRequest, loop, mgr, request_id) {
  loop->PostDelayedTask(FROM_HERE, NewRunnableMethod(mgr,
      &UrlmonUrlRequestManager::EndUrlRequest, 0, request_id,
      URLRequestStatus()), 0);
}

// Simplest test - retrieve file from local web server.
TEST(UrlmonUrlRequestManagerTest, Simple1) {
  MockUrlDelegate mock;
  ChromeFrameHTTPServer server;
  chrome_frame_test::TimedMsgLoop loop;
  server.SetUp();
  scoped_ptr<UrlmonUrlRequestManager> mgr(new UrlmonUrlRequestManager());
  mgr->set_delegate(&mock);
  IPC::AutomationURLRequest r1 = {
      server.Resolve(L"files/chrome_frame_window_open.html").spec(), "get" };

  EXPECT_CALL(mock, OnResponseStarted(1, testing::_, testing::_, testing::_,
                             testing::_, testing::_, testing::_))
      .Times(1)
      .WillOnce(ManagerRead(&loop, mgr.get(), 1, 512));

  EXPECT_CALL(mock, OnReadComplete(1, testing::Property(&std::string::size,
                                                        testing::Gt(0u))))
    .Times(testing::AtLeast(1))
    .WillRepeatedly(ManagerRead(&loop, mgr.get(), 1, 2));

  EXPECT_CALL(mock, OnResponseEnd(1, testing::_))
    .Times(1)
    .WillOnce(QUIT_LOOP_SOON(loop, 2));

  mgr->StartUrlRequest(0, 1, r1);
  loop.RunFor(kChromeFrameLongNavigationTimeoutInSeconds);
  mgr.reset();
  server.TearDown();
}

TEST(UrlmonUrlRequestManagerTest, Abort1) {
  MockUrlDelegate mock;
  ChromeFrameHTTPServer server;
  chrome_frame_test::TimedMsgLoop loop;
  server.SetUp();
  scoped_ptr<UrlmonUrlRequestManager> mgr(new UrlmonUrlRequestManager());
  mgr->set_delegate(&mock);
  IPC::AutomationURLRequest r1 = {
      server.Resolve(L"files/chrome_frame_window_open.html").spec(), "get" };

  EXPECT_CALL(mock, OnResponseStarted(1, testing::_, testing::_, testing::_,
                               testing::_, testing::_, testing::_))
    .Times(1)
    .WillOnce(testing::DoAll(
        ManagerEndRequest(&loop, mgr.get(), 1),
        QUIT_LOOP_SOON(loop, 3)));

  EXPECT_CALL(mock, OnReadComplete(1, testing::_))
    .Times(0);

  EXPECT_CALL(mock, OnResponseEnd(1, testing::_))
    .Times(0);

  mgr->StartUrlRequest(0, 1, r1);
  loop.RunFor(kChromeFrameLongNavigationTimeoutInSeconds);
  mgr.reset();
  server.TearDown();
}

