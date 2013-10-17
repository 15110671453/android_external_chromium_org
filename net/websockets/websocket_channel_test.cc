// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_channel.h"

#include <string.h>

#include <iostream>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/message_loop/message_loop.h"
#include "base/safe_numerics.h"
#include "base/strings/string_piece.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_context.h"
#include "net/websockets/websocket_errors.h"
#include "net/websockets/websocket_event_interface.h"
#include "net/websockets/websocket_mux.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// Hacky macros to construct the body of a Close message from a code and a
// string, while ensuring the result is a compile-time constant string.
// Use like CLOSE_DATA(NORMAL_CLOSURE, "Explanation String")
#define CLOSE_DATA(code, string) WEBSOCKET_CLOSE_CODE_AS_STRING_##code string
#define WEBSOCKET_CLOSE_CODE_AS_STRING_NORMAL_CLOSURE "\x03\xe8"
#define WEBSOCKET_CLOSE_CODE_AS_STRING_GOING_AWAY "\x03\xe9"
#define WEBSOCKET_CLOSE_CODE_AS_STRING_PROTOCOL_ERROR "\x03\xea"
#define WEBSOCKET_CLOSE_CODE_AS_STRING_SERVER_ERROR "\x03\xf3"

namespace net {

// Printing helpers to allow GoogleMock to print frames. These are explicitly
// designed to look like the static initialisation format we use in these
// tests. They have to live in the net namespace in order to be found by
// GoogleMock; a nested anonymous namespace will not work.

std::ostream& operator<<(std::ostream& os, const WebSocketFrameHeader& header) {
  return os << (header.final ? "FINAL_FRAME" : "NOT_FINAL_FRAME") << ", "
            << header.opcode << ", "
            << (header.masked ? "MASKED" : "NOT_MASKED");
}

std::ostream& operator<<(std::ostream& os, const WebSocketFrame& frame) {
  os << "{" << frame.header << ", ";
  if (frame.data) {
    return os << "\"" << base::StringPiece(frame.data->data(),
                                           frame.header.payload_length)
              << "\"}";
  }
  return os << "NULL}";
}

std::ostream& operator<<(std::ostream& os,
                         const ScopedVector<WebSocketFrame>& vector) {
  os << "{";
  bool first = true;
  for (ScopedVector<WebSocketFrame>::const_iterator it = vector.begin();
       it != vector.end();
       ++it) {
    if (!first) {
      os << ",\n";
    } else {
      first = false;
    }
    os << **it;
  }
  return os << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ScopedVector<WebSocketFrame>* vector) {
  return os << '&' << *vector;
}

namespace {

using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::MockFunction;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;
using ::testing::_;

// A selection of characters that have traditionally been mangled in some
// environment or other, for testing 8-bit cleanliness.
const char kBinaryBlob[] = {'\n',   '\r',    // BACKWARDS CRNL
                            '\0',            // nul
                            '\x7F',          // DEL
                            '\x80', '\xFF',  // NOT VALID UTF-8
                            '\x1A',          // Control-Z, EOF on DOS
                            '\x03',          // Control-C
                            '\x04',          // EOT, special for Unix terms
                            '\x1B',          // ESC, often special
                            '\b',            // backspace
                            '\'',            // single-quote, special in PHP
};
const size_t kBinaryBlobSize = arraysize(kBinaryBlob);

// The amount of quota a new connection gets by default.
// TODO(ricea): If kDefaultSendQuotaHighWaterMark changes, then this value will
// need to be updated.
const size_t kDefaultInitialQuota = 1 << 17;
// The amount of bytes we need to send after the initial connection to trigger a
// quota refresh. TODO(ricea): Change this if kDefaultSendQuotaHighWaterMark or
// kDefaultSendQuotaLowWaterMark change.
const size_t kDefaultQuotaRefreshTrigger = (1 << 16) + 1;

// This mock is for testing expectations about how the EventInterface is used.
class MockWebSocketEventInterface : public WebSocketEventInterface {
 public:
  MOCK_METHOD2(OnAddChannelResponse, void(bool, const std::string&));
  MOCK_METHOD3(OnDataFrame,
               void(bool, WebSocketMessageType, const std::vector<char>&));
  MOCK_METHOD1(OnFlowControl, void(int64));
  MOCK_METHOD0(OnClosingHandshake, void(void));
  MOCK_METHOD2(OnDropChannel, void(uint16, const std::string&));
};

// This fake EventInterface is for tests which need a WebSocketEventInterface
// implementation but are not verifying how it is used.
class FakeWebSocketEventInterface : public WebSocketEventInterface {
  virtual void OnAddChannelResponse(
      bool fail,
      const std::string& selected_protocol) OVERRIDE {}
  virtual void OnDataFrame(bool fin,
                           WebSocketMessageType type,
                           const std::vector<char>& data) OVERRIDE {}
  virtual void OnFlowControl(int64 quota) OVERRIDE {}
  virtual void OnClosingHandshake() OVERRIDE {}
  virtual void OnDropChannel(uint16 code, const std::string& reason) OVERRIDE {}
};

// This fake WebSocketStream is for tests that require a WebSocketStream but are
// not testing the way it is used. It has minimal functionality to return
// the |protocol| and |extensions| that it was constructed with.
class FakeWebSocketStream : public WebSocketStream {
 public:
  // Constructs with empty protocol and extensions.
  FakeWebSocketStream() {}

  // Constructs with specified protocol and extensions.
  FakeWebSocketStream(const std::string& protocol,
                      const std::string& extensions)
      : protocol_(protocol), extensions_(extensions) {}

  virtual int SendHandshakeRequest(
      const GURL& url,
      const HttpRequestHeaders& headers,
      HttpResponseInfo* response_info,
      const CompletionCallback& callback) OVERRIDE {
    return ERR_IO_PENDING;
  }

  virtual int ReadHandshakeResponse(
      const CompletionCallback& callback) OVERRIDE {
    return ERR_IO_PENDING;
  }

  virtual int ReadFrames(ScopedVector<WebSocketFrame>* frames,
                         const CompletionCallback& callback) OVERRIDE {
    return ERR_IO_PENDING;
  }

  virtual int WriteFrames(ScopedVector<WebSocketFrame>* frames,
                          const CompletionCallback& callback) OVERRIDE {
    return ERR_IO_PENDING;
  }

  virtual void Close() OVERRIDE {}

  // Returns the string passed to the constructor.
  virtual std::string GetSubProtocol() const OVERRIDE { return protocol_; }

  // Returns the string passed to the constructor.
  virtual std::string GetExtensions() const OVERRIDE { return extensions_; }

 private:
  // The string to return from GetSubProtocol().
  std::string protocol_;

  // The string to return from GetExtensions().
  std::string extensions_;
};

// To make the static initialisers easier to read, we use enums rather than
// bools.
enum IsFinal {
  NOT_FINAL_FRAME,
  FINAL_FRAME
};

enum IsMasked {
  NOT_MASKED,
  MASKED
};

// This is used to initialise a WebSocketFrame but is statically initialisable.
struct InitFrame {
  IsFinal final;
  // Reserved fields omitted for now. Add them if you need them.
  WebSocketFrameHeader::OpCode opcode;
  IsMasked masked;

  // Will be used to create the IOBuffer member. Can be NULL for NULL data. Is a
  // nul-terminated string for ease-of-use. |header.payload_length| is
  // initialised from |strlen(data)|. This means it is not 8-bit clean, but this
  // is not an issue for test data.
  const char* const data;
};

// For GoogleMock
std::ostream& operator<<(std::ostream& os, const InitFrame& frame) {
  os << "{" << (frame.final == FINAL_FRAME ? "FINAL_FRAME" : "NOT_FINAL_FRAME")
     << ", " << frame.opcode << ", "
     << (frame.masked == MASKED ? "MASKED" : "NOT_MASKED") << ", ";
  if (frame.data) {
    return os << "\"" << frame.data << "\"}";
  }
  return os << "NULL}";
}

template <size_t N>
std::ostream& operator<<(std::ostream& os, const InitFrame (&frames)[N]) {
  os << "{";
  bool first = true;
  for (size_t i = 0; i < N; ++i) {
    if (!first) {
      os << ",\n";
    } else {
      first = false;
    }
    os << frames[i];
  }
  return os << "}";
}

// Convert a const array of InitFrame structs to the format used at
// runtime. Templated on the size of the array to save typing.
template <size_t N>
ScopedVector<WebSocketFrame> CreateFrameVector(
    const InitFrame (&source_frames)[N]) {
  ScopedVector<WebSocketFrame> result_frames;
  result_frames.reserve(N);
  for (size_t i = 0; i < N; ++i) {
    const InitFrame& source_frame = source_frames[i];
    scoped_ptr<WebSocketFrame> result_frame(
        new WebSocketFrame(source_frame.opcode));
    size_t frame_length = source_frame.data ? strlen(source_frame.data) : 0;
    WebSocketFrameHeader& result_header = result_frame->header;
    result_header.final = (source_frame.final == FINAL_FRAME);
    result_header.masked = (source_frame.masked == MASKED);
    result_header.payload_length = frame_length;
    if (source_frame.data) {
      result_frame->data = new IOBuffer(frame_length);
      memcpy(result_frame->data->data(), source_frame.data, frame_length);
    }
    result_frames.push_back(result_frame.release());
  }
  return result_frames.Pass();
}

// A GoogleMock action which can be used to respond to call to ReadFrames with
// some frames. Use like ReadFrames(_, _).WillOnce(ReturnFrames(&frames));
// |frames| is an array of InitFrame. |frames| needs to be passed by pointer
// because otherwise it will be treated as a pointer and the array size
// information will be lost.
ACTION_P(ReturnFrames, source_frames) {
  *arg0 = CreateFrameVector(*source_frames);
  return OK;
}

// The implementation of a GoogleMock matcher which can be used to compare a
// ScopedVector<WebSocketFrame>* against an expectation defined as an array of
// InitFrame objects. Although it is possible to compose built-in GoogleMock
// matchers to check the contents of a WebSocketFrame, the results are so
// unreadable that it is better to use this matcher.
template <size_t N>
class EqualsFramesMatcher
    : public ::testing::MatcherInterface<ScopedVector<WebSocketFrame>*> {
 public:
  EqualsFramesMatcher(const InitFrame (*expect_frames)[N])
      : expect_frames_(expect_frames) {}

  virtual bool MatchAndExplain(ScopedVector<WebSocketFrame>* actual_frames,
                               ::testing::MatchResultListener* listener) const {
    if (actual_frames->size() != N) {
      *listener << "the vector size is " << actual_frames->size();
      return false;
    }
    for (size_t i = 0; i < N; ++i) {
      const WebSocketFrame& actual_frame = *(*actual_frames)[i];
      const InitFrame& expected_frame = (*expect_frames_)[i];
      if (actual_frame.header.final != (expected_frame.final == FINAL_FRAME)) {
        *listener << "the frame is marked as "
                  << (actual_frame.header.final ? "" : "not ") << "final";
        return false;
      }
      if (actual_frame.header.opcode != expected_frame.opcode) {
        *listener << "the opcode is " << actual_frame.header.opcode;
        return false;
      }
      if (actual_frame.header.masked != (expected_frame.masked == MASKED)) {
        *listener << "the frame is "
                  << (actual_frame.header.masked ? "masked" : "not masked");
        return false;
      }
      const size_t expected_length =
          expected_frame.data ? strlen(expected_frame.data) : 0;
      if (actual_frame.header.payload_length != expected_length) {
        *listener << "the payload length is "
                  << actual_frame.header.payload_length;
        return false;
      }
      if (expected_length != 0 &&
          memcmp(actual_frame.data->data(),
                 expected_frame.data,
                 actual_frame.header.payload_length) != 0) {
        *listener << "the data content differs";
        return false;
      }
    }
    return true;
  }

  virtual void DescribeTo(std::ostream* os) const {
    *os << "matches " << *expect_frames_;
  }

  virtual void DescribeNegationTo(std::ostream* os) const {
    *os << "does not match " << *expect_frames_;
  }

 private:
  const InitFrame (*expect_frames_)[N];
};

// The definition of EqualsFrames GoogleMock matcher. Unlike the ReturnFrames
// action, this can take the array by reference.
template <size_t N>
::testing::Matcher<ScopedVector<WebSocketFrame>*> EqualsFrames(
    const InitFrame (&frames)[N]) {
  return ::testing::MakeMatcher(new EqualsFramesMatcher<N>(&frames));
}

// A FakeWebSocketStream whose ReadFrames() function returns data.
class ReadableFakeWebSocketStream : public FakeWebSocketStream {
 public:
  enum IsSync {
    SYNC,
    ASYNC
  };

  // After constructing the object, call PrepareReadFrames() once for each
  // time you wish it to return from the test.
  ReadableFakeWebSocketStream() : index_(0), read_frames_pending_(false) {}

  // Check that all the prepared responses have been consumed.
  virtual ~ReadableFakeWebSocketStream() {
    CHECK(index_ >= responses_.size());
    CHECK(!read_frames_pending_);
  }

  // Prepares a fake response. Fake responses will be returned from ReadFrames()
  // in the same order they were prepared with PrepareReadFrames() and
  // PrepareReadFramesError(). If |async| is ASYNC, then ReadFrames() will
  // return ERR_IO_PENDING and the callback will be scheduled to run on the
  // message loop. This requires the test case to run the message loop. If
  // |async| is SYNC, the response will be returned synchronously. |error| is
  // returned directly from ReadFrames() in the synchronous case, or passed to
  // the callback in the asynchronous case. |frames| will be converted to a
  // ScopedVector<WebSocketFrame> and copied to the pointer that was passed to
  // ReadFrames().
  template <size_t N>
  void PrepareReadFrames(IsSync async,
                         int error,
                         const InitFrame (&frames)[N]) {
    responses_.push_back(new Response(async, error, CreateFrameVector(frames)));
  }

  // An alternate version of PrepareReadFrames for when we need to construct
  // the frames manually.
  void PrepareRawReadFrames(IsSync async,
                            int error,
                            ScopedVector<WebSocketFrame> frames) {
    responses_.push_back(new Response(async, error, frames.Pass()));
  }

  // Prepares a fake error response (ie. there is no data).
  void PrepareReadFramesError(IsSync async, int error) {
    responses_.push_back(
        new Response(async, error, ScopedVector<WebSocketFrame>()));
  }

  virtual int ReadFrames(ScopedVector<WebSocketFrame>* frames,
                         const CompletionCallback& callback) OVERRIDE {
    CHECK(!read_frames_pending_);
    if (index_ >= responses_.size())
      return ERR_IO_PENDING;
    if (responses_[index_]->async == ASYNC) {
      read_frames_pending_ = true;
      base::MessageLoop::current()->PostTask(
          FROM_HERE,
          base::Bind(&ReadableFakeWebSocketStream::DoCallback,
                     base::Unretained(this),
                     frames,
                     callback));
      return ERR_IO_PENDING;
    } else {
      frames->swap(responses_[index_]->frames);
      return responses_[index_++]->error;
    }
  }

 private:
  void DoCallback(ScopedVector<WebSocketFrame>* frames,
                  const CompletionCallback& callback) {
    read_frames_pending_ = false;
    frames->swap(responses_[index_]->frames);
    callback.Run(responses_[index_++]->error);
    return;
  }

  struct Response {
    Response(IsSync async, int error, ScopedVector<WebSocketFrame> frames)
        : async(async), error(error), frames(frames.Pass()) {}

    IsSync async;
    int error;
    ScopedVector<WebSocketFrame> frames;

   private:
    // Bad things will happen if we attempt to copy or assign |frames|.
    DISALLOW_COPY_AND_ASSIGN(Response);
  };
  ScopedVector<Response> responses_;

  // The index into the responses_ array of the next response to be returned.
  size_t index_;

  // True when an async response from ReadFrames() is pending. This only applies
  // to "real" async responses. Once all the prepared responses have been
  // returned, ReadFrames() returns ERR_IO_PENDING but read_frames_pending_ is
  // not set to true.
  bool read_frames_pending_;
};

// A FakeWebSocketStream where writes always complete successfully and
// synchronously.
class WriteableFakeWebSocketStream : public FakeWebSocketStream {
 public:
  virtual int WriteFrames(ScopedVector<WebSocketFrame>* frames,
                          const CompletionCallback& callback) OVERRIDE {
    return OK;
  }
};

// A FakeWebSocketStream where writes always fail.
class UnWriteableFakeWebSocketStream : public FakeWebSocketStream {
 public:
  virtual int WriteFrames(ScopedVector<WebSocketFrame>* frames,
                          const CompletionCallback& callback) OVERRIDE {
    return ERR_CONNECTION_RESET;
  }
};

// A FakeWebSocketStream which echoes any frames written back. Clears the
// "masked" header bit, but makes no other checks for validity. Tests using this
// must run the MessageLoop to receive the callback(s). If a message with opcode
// Close is echoed, then an ERR_CONNECTION_CLOSED is returned in the next
// callback. The test must do something to cause WriteFrames() to be called,
// otherwise the ReadFrames() callback will never be called.
class EchoeyFakeWebSocketStream : public FakeWebSocketStream {
 public:
  EchoeyFakeWebSocketStream() : read_frames_(NULL), done_(false) {}

  virtual int WriteFrames(ScopedVector<WebSocketFrame>* frames,
                          const CompletionCallback& callback) OVERRIDE {
    // Users of WebSocketStream will not expect the ReadFrames() callback to be
    // called from within WriteFrames(), so post it to the message loop instead.
    stored_frames_.insert(stored_frames_.end(), frames->begin(), frames->end());
    frames->weak_clear();
    PostCallback();
    return OK;
  }

  virtual int ReadFrames(ScopedVector<WebSocketFrame>* frames,
                         const CompletionCallback& callback) OVERRIDE {
    read_callback_ = callback;
    read_frames_ = frames;
    if (done_)
      PostCallback();
    return ERR_IO_PENDING;
  }

 private:
  void PostCallback() {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&EchoeyFakeWebSocketStream::DoCallback,
                   base::Unretained(this)));
  }

  void DoCallback() {
    if (done_) {
      read_callback_.Run(ERR_CONNECTION_CLOSED);
    } else if (!stored_frames_.empty()) {
      done_ = MoveFrames(read_frames_);
      read_frames_ = NULL;
      read_callback_.Run(OK);
    }
  }

  // Copy the frames stored in stored_frames_ to |out|, while clearing the
  // "masked" header bit. Returns true if a Close Frame was seen, false
  // otherwise.
  bool MoveFrames(ScopedVector<WebSocketFrame>* out) {
    bool seen_close = false;
    *out = stored_frames_.Pass();
    for (ScopedVector<WebSocketFrame>::iterator it = out->begin();
         it != out->end();
         ++it) {
      WebSocketFrameHeader& header = (*it)->header;
      header.masked = false;
      if (header.opcode == WebSocketFrameHeader::kOpCodeClose)
        seen_close = true;
    }
    return seen_close;
  }

  ScopedVector<WebSocketFrame> stored_frames_;
  CompletionCallback read_callback_;
  // Owned by the caller of ReadFrames().
  ScopedVector<WebSocketFrame>* read_frames_;
  // True if we should close the connection.
  bool done_;
};

// A FakeWebSocketStream where writes trigger a connection reset.
// This differs from UnWriteableFakeWebSocketStream in that it is asynchronous
// and triggers ReadFrames to return a reset as well. Tests using this need to
// run the message loop.
class ResetOnWriteFakeWebSocketStream : public FakeWebSocketStream {
 public:
  virtual int WriteFrames(ScopedVector<WebSocketFrame>* frames,
                          const CompletionCallback& callback) OVERRIDE {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, ERR_CONNECTION_RESET));
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(read_callback_, ERR_CONNECTION_RESET));
    return ERR_IO_PENDING;
  }

  virtual int ReadFrames(ScopedVector<WebSocketFrame>* frames,
                         const CompletionCallback& callback) OVERRIDE {
    read_callback_ = callback;
    return ERR_IO_PENDING;
  }

 private:
  CompletionCallback read_callback_;
};

// This mock is for verifying that WebSocket protocol semantics are obeyed (to
// the extent that they are implemented in WebSocketCommon).
class MockWebSocketStream : public WebSocketStream {
 public:
  MOCK_METHOD2(ReadFrames,
               int(ScopedVector<WebSocketFrame>* frames,
                   const CompletionCallback& callback));
  MOCK_METHOD2(WriteFrames,
               int(ScopedVector<WebSocketFrame>* frames,
                   const CompletionCallback& callback));
  MOCK_METHOD0(Close, void());
  MOCK_CONST_METHOD0(GetSubProtocol, std::string());
  MOCK_CONST_METHOD0(GetExtensions, std::string());
  MOCK_METHOD0(AsWebSocketStream, WebSocketStream*());
  MOCK_METHOD4(SendHandshakeRequest,
               int(const GURL& url,
                   const HttpRequestHeaders& headers,
                   HttpResponseInfo* response_info,
                   const CompletionCallback& callback));
  MOCK_METHOD1(ReadHandshakeResponse, int(const CompletionCallback& callback));
};

struct ArgumentCopyingWebSocketStreamFactory {
  scoped_ptr<WebSocketStreamRequest> Factory(
      const GURL& socket_url,
      const std::vector<std::string>& requested_subprotocols,
      const GURL& origin,
      URLRequestContext* url_request_context,
      const BoundNetLog& net_log,
      scoped_ptr<WebSocketStream::ConnectDelegate> connect_delegate) {
    this->socket_url = socket_url;
    this->requested_subprotocols = requested_subprotocols;
    this->origin = origin;
    this->url_request_context = url_request_context;
    this->net_log = net_log;
    this->connect_delegate = connect_delegate.Pass();
    return make_scoped_ptr(new WebSocketStreamRequest);
  }

  GURL socket_url;
  GURL origin;
  std::vector<std::string> requested_subprotocols;
  URLRequestContext* url_request_context;
  BoundNetLog net_log;
  scoped_ptr<WebSocketStream::ConnectDelegate> connect_delegate;
};

// Converts a std::string to a std::vector<char>. For test purposes, it is
// convenient to be able to specify data as a string, but the
// WebSocketEventInterface requires the vector<char> type.
std::vector<char> AsVector(const std::string& s) {
  return std::vector<char>(s.begin(), s.end());
}

// Base class for all test fixtures.
class WebSocketChannelTest : public ::testing::Test {
 protected:
  WebSocketChannelTest() : stream_(new FakeWebSocketStream) {}

  // Creates a new WebSocketChannel and connects it, using the settings stored
  // in |connect_data_|.
  void CreateChannelAndConnect() {
    channel_.reset(new WebSocketChannel(CreateEventInterface(),
                                        &connect_data_.url_request_context));
    channel_->SendAddChannelRequestForTesting(
        connect_data_.socket_url,
        connect_data_.requested_subprotocols,
        connect_data_.origin,
        base::Bind(&ArgumentCopyingWebSocketStreamFactory::Factory,
                   base::Unretained(&connect_data_.factory)));
  }

  // Same as CreateChannelAndConnect(), but calls the on_success callback as
  // well. This method is virtual so that subclasses can also set the stream.
  virtual void CreateChannelAndConnectSuccessfully() {
    CreateChannelAndConnect();
    connect_data_.factory.connect_delegate->OnSuccess(stream_.Pass());
  }

  // Returns a WebSocketEventInterface to be passed to the WebSocketChannel.
  // This implementation returns a newly-created fake. Subclasses may return a
  // mock instead.
  virtual scoped_ptr<WebSocketEventInterface> CreateEventInterface() {
    return scoped_ptr<WebSocketEventInterface>(new FakeWebSocketEventInterface);
  }

  // This method serves no other purpose than to provide a nice syntax for
  // assigning to stream_. class T must be a subclass of WebSocketStream or you
  // will have unpleasant compile errors.
  template <class T>
  void set_stream(scoped_ptr<T> stream) {
    // Since the definition of "PassAs" depends on the type T, the C++ standard
    // requires the "template" keyword to indicate that "PassAs" should be
    // parsed as a template method.
    stream_ = stream.template PassAs<WebSocketStream>();
  }

  // A struct containing the data that will be used to connect the channel.
  // Grouped for readability.
  struct ConnectData {
    // URLRequestContext object.
    URLRequestContext url_request_context;

    // URL to (pretend to) connect to.
    GURL socket_url;
    // Requested protocols for the request.
    std::vector<std::string> requested_subprotocols;
    // Origin of the request
    GURL origin;

    // A fake WebSocketStreamFactory that just records its arguments.
    ArgumentCopyingWebSocketStreamFactory factory;
  };
  ConnectData connect_data_;

  // The channel we are testing. Not initialised until SetChannel() is called.
  scoped_ptr<WebSocketChannel> channel_;

  // A mock or fake stream for tests that need one.
  scoped_ptr<WebSocketStream> stream_;
};

class WebSocketChannelDeletingTest : public WebSocketChannelTest {
 public:
  void ResetChannel() { channel_.reset(); }

 protected:
  // Create a ChannelDeletingFakeWebSocketEventInterface. Defined out-of-line to
  // avoid circular dependency.
  virtual scoped_ptr<WebSocketEventInterface> CreateEventInterface() OVERRIDE;
};

// A FakeWebSocketEventInterface that deletes the WebSocketChannel on failure to
// connect.
class ChannelDeletingFakeWebSocketEventInterface
    : public FakeWebSocketEventInterface {
 public:
  ChannelDeletingFakeWebSocketEventInterface(
      WebSocketChannelDeletingTest* fixture)
      : fixture_(fixture) {}

  virtual void OnAddChannelResponse(
      bool fail,
      const std::string& selected_protocol) OVERRIDE {
    if (fail) {
      fixture_->ResetChannel();
    }
  }

 private:
  // A pointer to the test fixture. Owned by the test harness; this object will
  // be deleted before it is.
  WebSocketChannelDeletingTest* fixture_;
};

scoped_ptr<WebSocketEventInterface>
WebSocketChannelDeletingTest::CreateEventInterface() {
  return scoped_ptr<WebSocketEventInterface>(
      new ChannelDeletingFakeWebSocketEventInterface(this));
}

// Base class for tests which verify that EventInterface methods are called
// appropriately.
class WebSocketChannelEventInterfaceTest : public WebSocketChannelTest {
 protected:
  WebSocketChannelEventInterfaceTest()
      : event_interface_(new StrictMock<MockWebSocketEventInterface>) {}

  // Tests using this fixture must set expectations on the event_interface_ mock
  // object before calling CreateChannelAndConnect() or
  // CreateChannelAndConnectSuccessfully(). This will only work once per test
  // case, but once should be enough.
  virtual scoped_ptr<WebSocketEventInterface> CreateEventInterface() OVERRIDE {
    return scoped_ptr<WebSocketEventInterface>(event_interface_.release());
  }

  scoped_ptr<MockWebSocketEventInterface> event_interface_;
};

// Base class for tests which verify that WebSocketStream methods are called
// appropriately by using a MockWebSocketStream.
class WebSocketChannelStreamTest : public WebSocketChannelTest {
 protected:
  WebSocketChannelStreamTest()
      : mock_stream_(new StrictMock<MockWebSocketStream>) {}

  virtual void CreateChannelAndConnectSuccessfully() OVERRIDE {
    set_stream(mock_stream_.Pass());
    WebSocketChannelTest::CreateChannelAndConnectSuccessfully();
  }

  scoped_ptr<MockWebSocketStream> mock_stream_;
};

// Simple test that everything that should be passed to the factory function is
// passed to the factory function.
TEST_F(WebSocketChannelTest, EverythingIsPassedToTheFactoryFunction) {
  connect_data_.socket_url = GURL("ws://example.com/test");
  connect_data_.origin = GURL("http://example.com/test");
  connect_data_.requested_subprotocols.push_back("Sinbad");

  CreateChannelAndConnect();

  const ArgumentCopyingWebSocketStreamFactory& actual =
      connect_data_.factory;

  EXPECT_EQ(&connect_data_.url_request_context, actual.url_request_context);

  EXPECT_EQ(connect_data_.socket_url, actual.socket_url);
  EXPECT_EQ(connect_data_.requested_subprotocols,
            actual.requested_subprotocols);
  EXPECT_EQ(connect_data_.origin, actual.origin);
}

// The documentation for WebSocketEventInterface::OnAddChannelResponse() says
// that if the first argument is true, ie. the connection failed, then we can
// safely synchronously delete the WebSocketChannel. This test will only
// reliably find problems if run with a memory debugger such as
// AddressSanitizer.
TEST_F(WebSocketChannelDeletingTest, DeletingFromOnAddChannelResponseWorks) {
  CreateChannelAndConnect();
  connect_data_.factory.connect_delegate->OnFailure(
      kWebSocketErrorNoStatusReceived);
  EXPECT_EQ(NULL, channel_.get());
}

TEST_F(WebSocketChannelEventInterfaceTest, ConnectSuccessReported) {
  // false means success.
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, ""));
  // OnFlowControl is always called immediately after connect to provide initial
  // quota to the renderer.
  EXPECT_CALL(*event_interface_, OnFlowControl(_));

  CreateChannelAndConnect();

  connect_data_.factory.connect_delegate->OnSuccess(stream_.Pass());
}

TEST_F(WebSocketChannelEventInterfaceTest, ConnectFailureReported) {
  // true means failure.
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(true, ""));

  CreateChannelAndConnect();

  connect_data_.factory.connect_delegate->OnFailure(
      kWebSocketErrorNoStatusReceived);
}

TEST_F(WebSocketChannelEventInterfaceTest, ProtocolPassed) {
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, "Bob"));
  EXPECT_CALL(*event_interface_, OnFlowControl(_));

  CreateChannelAndConnect();

  connect_data_.factory.connect_delegate->OnSuccess(
      scoped_ptr<WebSocketStream>(new FakeWebSocketStream("Bob", "")));
}

// The first frames from the server can arrive together with the handshake, in
// which case they will be available as soon as ReadFrames() is called the first
// time.
TEST_F(WebSocketChannelEventInterfaceTest, DataLeftFromHandshake) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, NOT_MASKED, "HELLO"}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::SYNC, OK, frames);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            true, WebSocketFrameHeader::kOpCodeText, AsVector("HELLO")));
  }

  CreateChannelAndConnectSuccessfully();
}

// A remote server could accept the handshake, but then immediately send a
// Close frame.
TEST_F(WebSocketChannelEventInterfaceTest, CloseAfterHandshake) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       NOT_MASKED,  CLOSE_DATA(SERVER_ERROR, "Internal Server Error")}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::SYNC, OK, frames);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::SYNC,
                                 ERR_CONNECTION_CLOSED);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_, OnClosingHandshake());
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorInternalServerError,
                              "Internal Server Error"));
  }

  CreateChannelAndConnectSuccessfully();
}

// A remote server could close the connection immediately after sending the
// handshake response (most likely a bug in the server).
TEST_F(WebSocketChannelEventInterfaceTest, ConnectionCloseAfterHandshake) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::SYNC,
                                 ERR_CONNECTION_CLOSED);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorAbnormalClosure, _));
  }

  CreateChannelAndConnectSuccessfully();
}

TEST_F(WebSocketChannelEventInterfaceTest, NormalAsyncRead) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, NOT_MASKED, "HELLO"}};
  // We use this checkpoint object to verify that the callback isn't called
  // until we expect it to be.
  MockFunction<void(int)> checkpoint;
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(1));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            true, WebSocketFrameHeader::kOpCodeText, AsVector("HELLO")));
    EXPECT_CALL(checkpoint, Call(2));
  }

  CreateChannelAndConnectSuccessfully();
  checkpoint.Call(1);
  base::MessageLoop::current()->RunUntilIdle();
  checkpoint.Call(2);
}

// Extra data can arrive while a read is being processed, resulting in the next
// read completing synchronously.
TEST_F(WebSocketChannelEventInterfaceTest, AsyncThenSyncRead) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames1[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, NOT_MASKED, "HELLO"}};
  static const InitFrame frames2[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, NOT_MASKED, "WORLD"}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames1);
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::SYNC, OK, frames2);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            true, WebSocketFrameHeader::kOpCodeText, AsVector("HELLO")));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            true, WebSocketFrameHeader::kOpCodeText, AsVector("WORLD")));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// Data frames are delivered the same regardless of how many reads they arrive
// as.
TEST_F(WebSocketChannelEventInterfaceTest, FragmentedMessage) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  // Here we have one message which arrived in five frames split across three
  // reads. It may have been reframed on arrival, but this class doesn't care
  // about that.
  static const InitFrame frames1[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, NOT_MASKED, "THREE"},
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       NOT_MASKED,      " "}};
  static const InitFrame frames2[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       NOT_MASKED,      "SMALL"}};
  static const InitFrame frames3[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       NOT_MASKED,      " "},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       NOT_MASKED,  "FRAMES"}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames1);
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames2);
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames3);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            false, WebSocketFrameHeader::kOpCodeText, AsVector("THREE")));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            false, WebSocketFrameHeader::kOpCodeContinuation, AsVector(" ")));
    EXPECT_CALL(*event_interface_,
                OnDataFrame(false,
                            WebSocketFrameHeader::kOpCodeContinuation,
                            AsVector("SMALL")));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            false, WebSocketFrameHeader::kOpCodeContinuation, AsVector(" ")));
    EXPECT_CALL(*event_interface_,
                OnDataFrame(true,
                            WebSocketFrameHeader::kOpCodeContinuation,
                            AsVector("FRAMES")));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// A control frame is not permitted to be split into multiple frames. RFC6455
// 5.5 "All control frames ... MUST NOT be fragmented."
TEST_F(WebSocketChannelEventInterfaceTest, MultiFrameControlMessageIsRejected) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodePing, NOT_MASKED, "Pi"},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       NOT_MASKED,  "ng"}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorProtocolError, _));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// Connection closed by the remote host without a closing handshake.
TEST_F(WebSocketChannelEventInterfaceTest, AsyncAbnormalClosure) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::ASYNC,
                                 ERR_CONNECTION_CLOSED);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorAbnormalClosure, _));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// A connection reset should produce the same event as an unexpected closure.
TEST_F(WebSocketChannelEventInterfaceTest, ConnectionReset) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::ASYNC,
                                 ERR_CONNECTION_RESET);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorAbnormalClosure, _));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// RFC6455 5.1 "A client MUST close a connection if it detects a masked frame."
TEST_F(WebSocketChannelEventInterfaceTest, MaskedFramesAreRejected) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "HELLO"}};

  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorProtocolError, _));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// RFC6455 5.2 "If an unknown opcode is received, the receiving endpoint MUST
// _Fail the WebSocket Connection_."
TEST_F(WebSocketChannelEventInterfaceTest, UnknownOpCodeIsRejected) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {{FINAL_FRAME, 4, NOT_MASKED, "HELLO"}};

  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorProtocolError, _));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// RFC6455 5.4 "Control frames ... MAY be injected in the middle of a
// fragmented message."
TEST_F(WebSocketChannelEventInterfaceTest, ControlFrameInDataMessage) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  // We have one message of type Text split into two frames. In the middle is a
  // control message of type Pong.
  static const InitFrame frames1[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeText,
       NOT_MASKED,      "SPLIT "}};
  static const InitFrame frames2[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodePong, NOT_MASKED, ""}};
  static const InitFrame frames3[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       NOT_MASKED,  "MESSAGE"}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames1);
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames2);
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames3);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(
        *event_interface_,
        OnDataFrame(
            false, WebSocketFrameHeader::kOpCodeText, AsVector("SPLIT ")));
    EXPECT_CALL(*event_interface_,
                OnDataFrame(true,
                            WebSocketFrameHeader::kOpCodeContinuation,
                            AsVector("MESSAGE")));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// If a frame has an invalid header, then the connection is closed and
// subsequent frames must not trigger events.
TEST_F(WebSocketChannelEventInterfaceTest, FrameAfterInvalidFrame) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "HELLO"},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, NOT_MASKED, " WORLD"}};

  stream->PrepareReadFrames(ReadableFakeWebSocketStream::ASYNC, OK, frames);
  set_stream(stream.Pass());
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorProtocolError, _));
  }

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// If the renderer sends lots of small writes, we don't want to update the quota
// for each one.
TEST_F(WebSocketChannelEventInterfaceTest, SmallWriteDoesntUpdateQuota) {
  set_stream(make_scoped_ptr(new WriteableFakeWebSocketStream));
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
  }

  CreateChannelAndConnectSuccessfully();
  channel_->SendFrame(true, WebSocketFrameHeader::kOpCodeText, AsVector("B"));
}

// If we send enough to go below send_quota_low_water_mask_ we should get our
// quota refreshed.
TEST_F(WebSocketChannelEventInterfaceTest, LargeWriteUpdatesQuota) {
  set_stream(make_scoped_ptr(new WriteableFakeWebSocketStream));
  // We use this checkpoint object to verify that the quota update comes after
  // the write.
  MockFunction<void(int)> checkpoint;
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(1));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(2));
  }

  CreateChannelAndConnectSuccessfully();
  checkpoint.Call(1);
  channel_->SendFrame(true,
                      WebSocketFrameHeader::kOpCodeText,
                      std::vector<char>(kDefaultInitialQuota, 'B'));
  checkpoint.Call(2);
}

// Verify that our quota actually is refreshed when we are told it is.
TEST_F(WebSocketChannelEventInterfaceTest, QuotaReallyIsRefreshed) {
  set_stream(make_scoped_ptr(new WriteableFakeWebSocketStream));
  MockFunction<void(int)> checkpoint;
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(1));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(2));
    // If quota was not really refreshed, we would get an OnDropChannel()
    // message.
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(3));
  }

  CreateChannelAndConnectSuccessfully();
  checkpoint.Call(1);
  channel_->SendFrame(true,
                      WebSocketFrameHeader::kOpCodeText,
                      std::vector<char>(kDefaultQuotaRefreshTrigger, 'D'));
  checkpoint.Call(2);
  // We should have received more quota at this point.
  channel_->SendFrame(true,
                      WebSocketFrameHeader::kOpCodeText,
                      std::vector<char>(kDefaultQuotaRefreshTrigger, 'E'));
  checkpoint.Call(3);
}

// If we send more than the available quota then the connection will be closed
// with an error.
TEST_F(WebSocketChannelEventInterfaceTest, WriteOverQuotaIsRejected) {
  set_stream(make_scoped_ptr(new WriteableFakeWebSocketStream));
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(kDefaultInitialQuota));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketMuxErrorSendQuotaViolation, _));
  }

  CreateChannelAndConnectSuccessfully();
  channel_->SendFrame(true,
                      WebSocketFrameHeader::kOpCodeText,
                      std::vector<char>(kDefaultInitialQuota + 1, 'C'));
}

// If a write fails, the channel is dropped.
TEST_F(WebSocketChannelEventInterfaceTest, FailedWrite) {
  set_stream(make_scoped_ptr(new UnWriteableFakeWebSocketStream));
  MockFunction<void(int)> checkpoint;
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(checkpoint, Call(1));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketErrorAbnormalClosure, _));
    EXPECT_CALL(checkpoint, Call(2));
  }

  CreateChannelAndConnectSuccessfully();
  checkpoint.Call(1);

  channel_->SendFrame(true, WebSocketFrameHeader::kOpCodeText, AsVector("H"));
  checkpoint.Call(2);
}

// OnDropChannel() is called exactly once when StartClosingHandshake() is used.
TEST_F(WebSocketChannelEventInterfaceTest, SendCloseDropsChannel) {
  set_stream(make_scoped_ptr(new EchoeyFakeWebSocketStream));
  {
    InSequence s;
    EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
    EXPECT_CALL(*event_interface_, OnFlowControl(_));
    EXPECT_CALL(*event_interface_,
                OnDropChannel(kWebSocketNormalClosure, "Fred"));
  }

  CreateChannelAndConnectSuccessfully();

  channel_->StartClosingHandshake(kWebSocketNormalClosure, "Fred");
  base::MessageLoop::current()->RunUntilIdle();
}

// OnDropChannel() is only called once when a write() on the socket triggers a
// connection reset.
TEST_F(WebSocketChannelEventInterfaceTest, OnDropChannelCalledOnce) {
  set_stream(make_scoped_ptr(new ResetOnWriteFakeWebSocketStream));
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
  EXPECT_CALL(*event_interface_, OnFlowControl(_));

  EXPECT_CALL(*event_interface_,
              OnDropChannel(kWebSocketErrorAbnormalClosure, "Abnormal Closure"))
      .Times(1);

  CreateChannelAndConnectSuccessfully();

  channel_->SendFrame(true, WebSocketFrameHeader::kOpCodeText, AsVector("yt?"));
  base::MessageLoop::current()->RunUntilIdle();
}

// When the remote server sends a Close frame with an empty payload,
// WebSocketChannel should report code 1005, kWebSocketErrorNoStatusReceived.
TEST_F(WebSocketChannelEventInterfaceTest, CloseWithNoPayloadGivesStatus1005) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose, NOT_MASKED, ""}};
  stream->PrepareReadFrames(ReadableFakeWebSocketStream::SYNC, OK, frames);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::SYNC,
                                 ERR_CONNECTION_CLOSED);
  set_stream(stream.Pass());
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
  EXPECT_CALL(*event_interface_, OnFlowControl(_));
  EXPECT_CALL(*event_interface_, OnClosingHandshake());
  EXPECT_CALL(*event_interface_,
              OnDropChannel(kWebSocketErrorNoStatusReceived, _));

  CreateChannelAndConnectSuccessfully();
}

// If ReadFrames() returns ERR_WS_PROTOCOL_ERROR, then
// kWebSocketErrorProtocolError must be sent to the renderer.
TEST_F(WebSocketChannelEventInterfaceTest, SyncProtocolErrorGivesStatus1002) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::SYNC,
                                 ERR_WS_PROTOCOL_ERROR);
  set_stream(stream.Pass());
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
  EXPECT_CALL(*event_interface_, OnFlowControl(_));

  EXPECT_CALL(*event_interface_,
              OnDropChannel(kWebSocketErrorProtocolError, _));

  CreateChannelAndConnectSuccessfully();
}

// Async version of above test.
TEST_F(WebSocketChannelEventInterfaceTest, AsyncProtocolErrorGivesStatus1002) {
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  stream->PrepareReadFramesError(ReadableFakeWebSocketStream::ASYNC,
                                 ERR_WS_PROTOCOL_ERROR);
  set_stream(stream.Pass());
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
  EXPECT_CALL(*event_interface_, OnFlowControl(_));

  EXPECT_CALL(*event_interface_,
              OnDropChannel(kWebSocketErrorProtocolError, _));

  CreateChannelAndConnectSuccessfully();
  base::MessageLoop::current()->RunUntilIdle();
}

// RFC6455 5.1 "a client MUST mask all frames that it sends to the server".
// WebSocketChannel actually only sets the mask bit in the header, it doesn't
// perform masking itself (not all transports actually use masking).
TEST_F(WebSocketChannelStreamTest, SentFramesAreMasked) {
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText,
       MASKED,      "NEEDS MASKING"}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _)).WillOnce(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));

  CreateChannelAndConnectSuccessfully();
  channel_->SendFrame(
      true, WebSocketFrameHeader::kOpCodeText, AsVector("NEEDS MASKING"));
}

// RFC6455 5.5.1 "The application MUST NOT send any more data frames after
// sending a Close frame."
TEST_F(WebSocketChannelStreamTest, NothingIsSentAfterClose) {
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       MASKED,      CLOSE_DATA(NORMAL_CLOSURE, "Success")}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _)).WillOnce(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));

  CreateChannelAndConnectSuccessfully();
  channel_->StartClosingHandshake(1000, "Success");
  channel_->SendFrame(
      true, WebSocketFrameHeader::kOpCodeText, AsVector("SHOULD  BE IGNORED"));
}

// RFC6455 5.5.1 "If an endpoint receives a Close frame and did not previously
// send a Close frame, the endpoint MUST send a Close frame in response."
TEST_F(WebSocketChannelStreamTest, CloseIsEchoedBack) {
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       NOT_MASKED,  CLOSE_DATA(NORMAL_CLOSURE, "Close")}};
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       MASKED,      CLOSE_DATA(NORMAL_CLOSURE, "Close")}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
      .WillOnce(ReturnFrames(&frames))
      .WillRepeatedly(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));

  CreateChannelAndConnectSuccessfully();
}

// The converse of the above case; after sending a Close frame, we should not
// send another one.
TEST_F(WebSocketChannelStreamTest, CloseOnlySentOnce) {
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       MASKED,      CLOSE_DATA(NORMAL_CLOSURE, "Close")}};
  static const InitFrame frames_init[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       NOT_MASKED,  CLOSE_DATA(NORMAL_CLOSURE, "Close")}};

  // We store the parameters that were passed to ReadFrames() so that we can
  // call them explicitly later.
  CompletionCallback read_callback;
  ScopedVector<WebSocketFrame>* frames = NULL;

  // Use a checkpoint to make the ordering of events clearer.
  MockFunction<void(int)> checkpoint;
  {
    InSequence s;
    EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
    EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
        .WillOnce(DoAll(SaveArg<0>(&frames),
                        SaveArg<1>(&read_callback),
                        Return(ERR_IO_PENDING)));
    EXPECT_CALL(checkpoint, Call(1));
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
        .WillOnce(Return(OK));
    EXPECT_CALL(checkpoint, Call(2));
    EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
        .WillOnce(Return(ERR_IO_PENDING));
    EXPECT_CALL(checkpoint, Call(3));
    // WriteFrames() must not be called again. GoogleMock will ensure that the
    // test fails if it is.
  }

  CreateChannelAndConnectSuccessfully();
  checkpoint.Call(1);
  channel_->StartClosingHandshake(kWebSocketNormalClosure, "Close");
  checkpoint.Call(2);

  *frames = CreateFrameVector(frames_init);
  read_callback.Run(OK);
  checkpoint.Call(3);
}

// We generate code 1005, kWebSocketErrorNoStatusReceived, when there is no
// status in the Close message from the other side. Code 1005 is not allowed to
// appear on the wire, so we should not echo it back. See test
// CloseWithNoPayloadGivesStatus1005, above, for confirmation that code 1005 is
// correctly generated internally.
TEST_F(WebSocketChannelStreamTest, Code1005IsNotEchoed) {
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose, NOT_MASKED, ""}};
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose, MASKED, ""}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
      .WillOnce(ReturnFrames(&frames))
      .WillRepeatedly(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));

  CreateChannelAndConnectSuccessfully();
}

// RFC6455 5.5.2 "Upon receipt of a Ping frame, an endpoint MUST send a Pong
// frame in response"
// 5.5.3 "A Pong frame sent in response to a Ping frame must have identical
// "Application data" as found in the message body of the Ping frame being
// replied to."
TEST_F(WebSocketChannelStreamTest, PingRepliedWithPong) {
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodePing,
       NOT_MASKED,  "Application data"}};
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodePong,
       MASKED,      "Application data"}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
      .WillOnce(ReturnFrames(&frames))
      .WillRepeatedly(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));

  CreateChannelAndConnectSuccessfully();
}

TEST_F(WebSocketChannelStreamTest, PongInTheMiddleOfDataMessage) {
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodePing,
       NOT_MASKED,  "Application data"}};
  static const InitFrame expected1[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "Hello "}};
  static const InitFrame expected2[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodePong,
       MASKED,      "Application data"}};
  static const InitFrame expected3[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeContinuation,
       MASKED,      "World"}};
  ScopedVector<WebSocketFrame>* read_frames;
  CompletionCallback read_callback;
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
      .WillOnce(DoAll(SaveArg<0>(&read_frames),
                      SaveArg<1>(&read_callback),
                      Return(ERR_IO_PENDING)))
      .WillRepeatedly(Return(ERR_IO_PENDING));
  {
    InSequence s;

    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected1), _))
        .WillOnce(Return(OK));
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected2), _))
        .WillOnce(Return(OK));
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected3), _))
        .WillOnce(Return(OK));
  }

  CreateChannelAndConnectSuccessfully();
  channel_->SendFrame(
      false, WebSocketFrameHeader::kOpCodeText, AsVector("Hello "));
  *read_frames = CreateFrameVector(frames);
  read_callback.Run(OK);
  channel_->SendFrame(
      true, WebSocketFrameHeader::kOpCodeContinuation, AsVector("World"));
}

// WriteFrames() may not be called until the previous write has completed.
// WebSocketChannel must buffer writes that happen in the meantime.
TEST_F(WebSocketChannelStreamTest, WriteFramesOneAtATime) {
  static const InitFrame expected1[] = {
      {NOT_FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "Hello "}};
  static const InitFrame expected2[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "World"}};
  CompletionCallback write_callback;
  MockFunction<void(int)> checkpoint;

  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _)).WillOnce(Return(ERR_IO_PENDING));
  {
    InSequence s;
    EXPECT_CALL(checkpoint, Call(1));
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected1), _))
        .WillOnce(DoAll(SaveArg<1>(&write_callback), Return(ERR_IO_PENDING)));
    EXPECT_CALL(checkpoint, Call(2));
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected2), _))
        .WillOnce(Return(ERR_IO_PENDING));
    EXPECT_CALL(checkpoint, Call(3));
  }

  CreateChannelAndConnectSuccessfully();
  checkpoint.Call(1);
  channel_->SendFrame(
      false, WebSocketFrameHeader::kOpCodeText, AsVector("Hello "));
  channel_->SendFrame(
      true, WebSocketFrameHeader::kOpCodeText, AsVector("World"));
  checkpoint.Call(2);
  write_callback.Run(OK);
  checkpoint.Call(3);
}

// WebSocketChannel must buffer frames while it is waiting for a write to
// complete, and then send them in a single batch. The batching behaviour is
// important to get good throughput in the "many small messages" case.
TEST_F(WebSocketChannelStreamTest, WaitingMessagesAreBatched) {
  static const char input_letters[] = "Hello";
  static const InitFrame expected1[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "H"}};
  static const InitFrame expected2[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "e"},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "l"},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "l"},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeText, MASKED, "o"}};
  CompletionCallback write_callback;

  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _)).WillOnce(Return(ERR_IO_PENDING));
  {
    InSequence s;
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected1), _))
        .WillOnce(DoAll(SaveArg<1>(&write_callback), Return(ERR_IO_PENDING)));
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected2), _))
        .WillOnce(Return(ERR_IO_PENDING));
  }

  CreateChannelAndConnectSuccessfully();
  for (size_t i = 0; i < strlen(input_letters); ++i) {
    channel_->SendFrame(true,
                        WebSocketFrameHeader::kOpCodeText,
                        std::vector<char>(1, input_letters[i]));
  }
  write_callback.Run(OK);
}

// When the renderer sends more on a channel than it has quota for, then we send
// a kWebSocketMuxErrorSendQuotaViolation status code (from the draft websocket
// mux specification) back to the renderer. This should not be sent to the
// remote server, which may not even implement the mux specification, and could
// even be using a different extension which uses that code to mean something
// else.
TEST_F(WebSocketChannelStreamTest, MuxErrorIsNotSentToStream) {
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       MASKED,      CLOSE_DATA(GOING_AWAY, "Internal Error")}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _)).WillOnce(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));
  EXPECT_CALL(*mock_stream_, Close());

  CreateChannelAndConnectSuccessfully();
  channel_->SendFrame(true,
                      WebSocketFrameHeader::kOpCodeText,
                      std::vector<char>(kDefaultInitialQuota + 1, 'C'));
}

// For convenience, most of these tests use Text frames. However, the WebSocket
// protocol also has Binary frames and those need to be 8-bit clean. For the
// sake of completeness, this test verifies that they are.
TEST_F(WebSocketChannelStreamTest, WrittenBinaryFramesAre8BitClean) {
  ScopedVector<WebSocketFrame>* frames = NULL;

  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _)).WillOnce(Return(ERR_IO_PENDING));
  EXPECT_CALL(*mock_stream_, WriteFrames(_, _))
      .WillOnce(DoAll(SaveArg<0>(&frames), Return(ERR_IO_PENDING)));

  CreateChannelAndConnectSuccessfully();
  channel_->SendFrame(
      true,
      WebSocketFrameHeader::kOpCodeBinary,
      std::vector<char>(kBinaryBlob, kBinaryBlob + kBinaryBlobSize));
  ASSERT_TRUE(frames != NULL);
  ASSERT_EQ(1U, frames->size());
  const WebSocketFrame* out_frame = (*frames)[0];
  EXPECT_EQ(kBinaryBlobSize, out_frame->header.payload_length);
  ASSERT_TRUE(out_frame->data);
  EXPECT_EQ(0, memcmp(kBinaryBlob, out_frame->data->data(), kBinaryBlobSize));
}

// Test the read path for 8-bit cleanliness as well.
TEST_F(WebSocketChannelEventInterfaceTest, ReadBinaryFramesAre8BitClean) {
  scoped_ptr<WebSocketFrame> frame(
      new WebSocketFrame(WebSocketFrameHeader::kOpCodeBinary));
  WebSocketFrameHeader& frame_header = frame->header;
  frame_header.final = true;
  frame_header.payload_length = kBinaryBlobSize;
  frame->data = new IOBuffer(kBinaryBlobSize);
  memcpy(frame->data->data(), kBinaryBlob, kBinaryBlobSize);
  ScopedVector<WebSocketFrame> frames;
  frames.push_back(frame.release());
  scoped_ptr<ReadableFakeWebSocketStream> stream(
      new ReadableFakeWebSocketStream);
  stream->PrepareRawReadFrames(
      ReadableFakeWebSocketStream::SYNC, OK, frames.Pass());
  set_stream(stream.Pass());
  EXPECT_CALL(*event_interface_, OnAddChannelResponse(false, _));
  EXPECT_CALL(*event_interface_, OnFlowControl(_));
  EXPECT_CALL(*event_interface_,
              OnDataFrame(true,
                          WebSocketFrameHeader::kOpCodeBinary,
                          std::vector<char>(kBinaryBlob,
                                            kBinaryBlob + kBinaryBlobSize)));

  CreateChannelAndConnectSuccessfully();
}

// If we receive another frame after Close, it is not valid. It is not
// completely clear what behaviour is required from the standard in this case,
// but the current implementation fails the connection. Since a Close has
// already been sent, this just means closing the connection.
TEST_F(WebSocketChannelStreamTest, PingAfterCloseIsRejected) {
  static const InitFrame frames[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       NOT_MASKED,  CLOSE_DATA(NORMAL_CLOSURE, "OK")},
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodePing,
       NOT_MASKED,  "Ping body"}};
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       MASKED,      CLOSE_DATA(NORMAL_CLOSURE, "OK")}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
      .WillOnce(ReturnFrames(&frames))
      .WillRepeatedly(Return(ERR_IO_PENDING));
  {
    // We only need to verify the relative order of WriteFrames() and
    // Close(). The current implementation calls WriteFrames() for the Close
    // frame before calling ReadFrames() again, but that is an implementation
    // detail and better not to consider required behaviour.
    InSequence s;
    EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
        .WillOnce(Return(OK));
    EXPECT_CALL(*mock_stream_, Close()).Times(1);
  }

  CreateChannelAndConnectSuccessfully();
}

// A protocol error from the remote server should result in a close frame with
// status 1002, followed by the connection closing.
TEST_F(WebSocketChannelStreamTest, ProtocolError) {
  static const InitFrame expected[] = {
      {FINAL_FRAME, WebSocketFrameHeader::kOpCodeClose,
       MASKED,      CLOSE_DATA(PROTOCOL_ERROR, "WebSocket Protocol Error")}};
  EXPECT_CALL(*mock_stream_, GetSubProtocol()).Times(AnyNumber());
  EXPECT_CALL(*mock_stream_, ReadFrames(_, _))
      .WillOnce(Return(ERR_WS_PROTOCOL_ERROR));
  EXPECT_CALL(*mock_stream_, WriteFrames(EqualsFrames(expected), _))
      .WillOnce(Return(OK));
  EXPECT_CALL(*mock_stream_, Close());

  CreateChannelAndConnectSuccessfully();
}

}  // namespace
}  // namespace net
