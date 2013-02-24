// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/synchronization/waitable_event.h"
#include "media/base/test_data_util.h"
#include "media/ffmpeg/ffmpeg_common.h"
#include "media/filters/blocking_url_protocol.h"
#include "media/filters/file_data_source.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class BlockingUrlProtocolTest : public testing::Test {
 public:
  BlockingUrlProtocolTest() {
    data_source_ = new FileDataSource();
    CHECK(data_source_->Initialize(GetTestDataFilePath("bear-320x240.webm")));

    url_protocol_.reset(new BlockingUrlProtocol(data_source_, base::Bind(
        &BlockingUrlProtocolTest::OnDataSourceError, base::Unretained(this))));
  }

  virtual ~BlockingUrlProtocolTest() {
    base::WaitableEvent stop_event(false, false);
    data_source_->Stop(base::Bind(
        &base::WaitableEvent::Signal, base::Unretained(&stop_event)));
    stop_event.Wait();
  }

  MOCK_METHOD0(OnDataSourceError, void());

  scoped_refptr<FileDataSource> data_source_;
  scoped_ptr<BlockingUrlProtocol> url_protocol_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BlockingUrlProtocolTest);
};


TEST_F(BlockingUrlProtocolTest, Read) {
  // Set read head to zero as Initialize() will have parsed a bit of the file.
  int64 position = 0;
  EXPECT_TRUE(url_protocol_->SetPosition(0));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(0, position);

  // Read 32 bytes from offset zero and verify position.
  uint8 buffer[32];
  EXPECT_EQ(32, url_protocol_->Read(32, buffer));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(32, position);

  // Read an additional 32 bytes and verify position.
  EXPECT_EQ(32, url_protocol_->Read(32, buffer));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(64, position);

  // Seek to end and read until EOF.
  int64 size = 0;
  EXPECT_TRUE(url_protocol_->GetSize(&size));
  EXPECT_TRUE(url_protocol_->SetPosition(size - 48));
  EXPECT_EQ(32, url_protocol_->Read(32, buffer));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(size - 16, position);

  EXPECT_EQ(16, url_protocol_->Read(32, buffer));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(size, position);

  EXPECT_EQ(0, url_protocol_->Read(32, buffer));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(size, position);
}

TEST_F(BlockingUrlProtocolTest, ReadError) {
  data_source_->force_read_errors_for_testing();

  uint8 buffer[32];
  EXPECT_CALL(*this, OnDataSourceError());
  EXPECT_EQ(AVERROR(EIO), url_protocol_->Read(32, buffer));
}

TEST_F(BlockingUrlProtocolTest, GetSetPosition) {
  int64 size;
  int64 position;
  EXPECT_TRUE(url_protocol_->GetSize(&size));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));

  EXPECT_TRUE(url_protocol_->SetPosition(512));
  EXPECT_FALSE(url_protocol_->SetPosition(size));
  EXPECT_FALSE(url_protocol_->SetPosition(size + 1));
  EXPECT_FALSE(url_protocol_->SetPosition(-1));
  EXPECT_TRUE(url_protocol_->GetPosition(&position));
  EXPECT_EQ(512, position);
}

TEST_F(BlockingUrlProtocolTest, GetSize) {
  int64 data_source_size = 0;
  int64 url_protocol_size = 0;
  EXPECT_TRUE(data_source_->GetSize(&data_source_size));
  EXPECT_TRUE(url_protocol_->GetSize(&url_protocol_size));
  EXPECT_NE(0, data_source_size);
  EXPECT_EQ(data_source_size, url_protocol_size);
}

TEST_F(BlockingUrlProtocolTest, IsStreaming) {
  EXPECT_FALSE(data_source_->IsStreaming());
  EXPECT_FALSE(url_protocol_->IsStreaming());

  data_source_->force_streaming_for_testing();
  EXPECT_TRUE(data_source_->IsStreaming());
  EXPECT_TRUE(url_protocol_->IsStreaming());
}

}  // namespace media
