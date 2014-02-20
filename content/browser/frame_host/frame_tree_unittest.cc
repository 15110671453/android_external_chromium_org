// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/frame_tree.h"

#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "content/browser/frame_host/navigator_impl.h"
#include "content/browser/frame_host/render_frame_host_factory.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class FrameTreeTest : public RenderViewHostTestHarness {
 protected:
  // Prints a FrameTree, for easy assertions of the tree hierarchy.
  std::string GetTreeState(FrameTree* frame_tree) {
    std::string result;
    AppendTreeNodeState(frame_tree->root(), &result);
    return result;
  }

 private:
  void AppendTreeNodeState(FrameTreeNode* node, std::string* result) {
    result->append(base::Int64ToString(
        node->current_frame_host()->GetRoutingID()));
    if (!node->frame_name().empty()) {
      result->append(" '");
      result->append(node->frame_name());
      result->append("'");
    }
    result->append(": [");
    const char* separator = "";
    for (size_t i = 0; i < node->child_count(); i++) {
      result->append(separator);
      AppendTreeNodeState(node->child_at(i), result);
      separator = ", ";
    }
    result->append("]");
  }
};

// Exercise tree manipulation routines.
//  - Add a series of nodes and verify tree structure.
//  - Remove a series of nodes and verify tree structure.
TEST_F(FrameTreeTest, Shape) {
  // Use the FrameTree of the WebContents so that it has all the delegates it
  // needs.  We may want to consider a test version of this.
  FrameTree* frame_tree =
      static_cast<WebContentsImpl*>(web_contents())->GetFrameTree();
  FrameTreeNode* root = frame_tree->root();

  std::string no_children_node("no children node");
  std::string deep_subtree("node with deep subtree");

  ASSERT_EQ("1: []", GetTreeState(frame_tree));

  // Simulate attaching a series of frames to build the frame tree.
  frame_tree->AddFrame(root, 14, std::string());
  frame_tree->AddFrame(root, 15, std::string());
  frame_tree->AddFrame(root, 16, std::string());

  frame_tree->AddFrame(root->child_at(0), 244, std::string());
  frame_tree->AddFrame(root->child_at(1), 255, no_children_node);
  frame_tree->AddFrame(root->child_at(0), 245, std::string());

  ASSERT_EQ("1: [14: [244: [], 245: []], "
                "15: [255 'no children node': []], "
                "16: []]",
            GetTreeState(frame_tree));

  FrameTreeNode* child_16 = root->child_at(2);
  frame_tree->AddFrame(child_16, 264, std::string());
  frame_tree->AddFrame(child_16, 265, std::string());
  frame_tree->AddFrame(child_16, 266, std::string());
  frame_tree->AddFrame(child_16, 267, deep_subtree);
  frame_tree->AddFrame(child_16, 268, std::string());

  FrameTreeNode* child_267 = child_16->child_at(3);
  frame_tree->AddFrame(child_267, 365, std::string());
  frame_tree->AddFrame(child_267->child_at(0), 455, std::string());
  frame_tree->AddFrame(child_267->child_at(0)->child_at(0), 555, std::string());
  frame_tree->AddFrame(child_267->child_at(0)->child_at(0)->child_at(0), 655,
                       std::string());

  // Now that's it's fully built, verify the tree structure is as expected.
  ASSERT_EQ("1: [14: [244: [], 245: []], "
                "15: [255 'no children node': []], "
                "16: [264: [], 265: [], 266: [], "
                     "267 'node with deep subtree': "
                         "[365: [455: [555: [655: []]]]], 268: []]]",
            GetTreeState(frame_tree));

  FrameTreeNode* child_555 = child_267->child_at(0)->child_at(0)->child_at(0);
  frame_tree->RemoveFrame(child_555);
  ASSERT_EQ("1: [14: [244: [], 245: []], "
                "15: [255 'no children node': []], "
                "16: [264: [], 265: [], 266: [], "
                     "267 'node with deep subtree': "
                         "[365: [455: []]], 268: []]]",
            GetTreeState(frame_tree));

  frame_tree->RemoveFrame(child_16->child_at(1));
  ASSERT_EQ("1: [14: [244: [], 245: []], "
                "15: [255 'no children node': []], "
                "16: [264: [], 266: [], "
                     "267 'node with deep subtree': "
                         "[365: [455: []]], 268: []]]",
            GetTreeState(frame_tree));

  frame_tree->RemoveFrame(root->child_at(1));
  ASSERT_EQ("1: [14: [244: [], 245: []], "
                "16: [264: [], 266: [], "
                     "267 'node with deep subtree': "
                         "[365: [455: []]], 268: []]]",
            GetTreeState(frame_tree));
}

}  // namespace
}  // namespace content
