// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_MOCK_OBJECTS_H_
#define REMOTING_BASE_MOCK_OBJECTS_H_

#include "remoting/base/protocol_decoder.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace remoting {

class MockProtocolDecoder : public ProtocolDecoder {
 public:
  MockProtocolDecoder() {}

  MOCK_METHOD2(ParseClientMessages,
               void(scoped_refptr<media::DataBuffer> data,
                    ClientMessageList* messages));
  MOCK_METHOD2(ParseHostMessages,
               void(scoped_refptr<media::DataBuffer> data,
                    HostMessageList* messages));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProtocolDecoder);
};

}  // namespace remoting

#endif  // REMOTING_BASE_MOCK_OBJECTS_H_
