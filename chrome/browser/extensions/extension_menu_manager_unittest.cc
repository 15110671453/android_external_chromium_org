// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/scoped_temp_dir.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_menu_manager.h"
#include "chrome/browser/extensions/extension_message_service.h"
#include "chrome/browser/extensions/test_extension_prefs.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/notification_service.h"
#include "chrome/test/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "webkit/glue/context_menu.h"

using testing::_;
using testing::AtLeast;
using testing::Return;
using testing::SaveArg;

// Base class for tests.
class ExtensionMenuManagerTest : public testing::Test {
 public:
  ExtensionMenuManagerTest() {}
  ~ExtensionMenuManagerTest() {}

  // Returns a test item.
  static ExtensionMenuItem* CreateTestItem(Extension* extension) {
    ExtensionMenuItem::Type type = ExtensionMenuItem::NORMAL;
    ExtensionMenuItem::ContextList contexts(ExtensionMenuItem::ALL);
    ExtensionMenuItem::ContextList enabled_contexts = contexts;
    return new ExtensionMenuItem(extension->id(), "test", false, type, contexts,
                                 enabled_contexts);
  }

  // Creates and returns a test Extension. The caller does *not* own the return
  // value.
  Extension* AddExtension(std::string name) {
    return prefs_.AddExtension(name);
  }

 protected:
  ExtensionMenuManager manager_;
  TestExtensionPrefs prefs_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionMenuManagerTest);
};

// Tests adding, getting, and removing items.
TEST_F(ExtensionMenuManagerTest, AddGetRemoveItems) {
  Extension* extension = AddExtension("test");

  // Add a new item, make sure you can get it back.
  ExtensionMenuItem* item1 = CreateTestItem(extension);
  ASSERT_TRUE(item1 != NULL);
  int id1 = manager_.AddContextItem(extension, item1);
  ASSERT_GT(id1, 0);
  ASSERT_EQ(item1, manager_.GetItemById(id1));
  const ExtensionMenuItem::List* items =
      manager_.MenuItems(item1->extension_id());
  ASSERT_EQ(1u, items->size());
  ASSERT_EQ(item1, items->at(0));

  // Add a second item, make sure it comes back too.
  ExtensionMenuItem* item2 = CreateTestItem(extension);
  int id2 = manager_.AddContextItem(extension, item2);
  ASSERT_GT(id2, 0);
  ASSERT_NE(id1, id2);
  ASSERT_EQ(item2, manager_.GetItemById(id2));
  items = manager_.MenuItems(item2->extension_id());
  ASSERT_EQ(2u, items->size());
  ASSERT_EQ(item1, items->at(0));
  ASSERT_EQ(item2, items->at(1));

  // Try adding item 3, then removing it.
  ExtensionMenuItem* item3 = CreateTestItem(extension);
  std::string extension_id = item3->extension_id();
  int id3 = manager_.AddContextItem(extension, item3);
  ASSERT_GT(id3, 0);
  ASSERT_EQ(item3, manager_.GetItemById(id3));
  ASSERT_EQ(3u, manager_.MenuItems(extension_id)->size());
  ASSERT_TRUE(manager_.RemoveContextMenuItem(id3));
  ASSERT_EQ(NULL, manager_.GetItemById(id3));
  ASSERT_EQ(2u, manager_.MenuItems(extension_id)->size());

  // Make sure removing a non-existent item returns false.
  ASSERT_FALSE(manager_.RemoveContextMenuItem(5));
}

// Test adding/removing child items.
TEST_F(ExtensionMenuManagerTest, ChildFunctions) {
  Extension* extension1 = AddExtension("1111");
  Extension* extension2 = AddExtension("2222");
  Extension* extension3 = AddExtension("3333");

  ExtensionMenuItem* item1 = CreateTestItem(extension1);
  ExtensionMenuItem* item2 = CreateTestItem(extension2);
  ExtensionMenuItem* item2_child = CreateTestItem(extension2);
  ExtensionMenuItem* item2_grandchild = CreateTestItem(extension2);

  // This third item we expect to fail inserting, so we use a scoped_ptr to make
  // sure it gets deleted.
  scoped_ptr<ExtensionMenuItem> item3(CreateTestItem(extension3));

  // Add in the first two items.
  int id1 = manager_.AddContextItem(extension1, item1);
  int id2 = manager_.AddContextItem(extension2, item2);

  ASSERT_NE(id1, id2);

  // Try adding item3 as a child of item2 - this should fail because item3 has
  // a different extension id.
  ASSERT_EQ(0, manager_.AddChildItem(id2, item3.get()));

  // Add item2_child as a child of item2.
  int id2_child = manager_.AddChildItem(id2, item2_child);
  ASSERT_GT(id2_child, 0);
  ASSERT_EQ(1, item2->child_count());
  ASSERT_EQ(0, item1->child_count());
  ASSERT_EQ(item2_child, manager_.GetItemById(id2_child));

  ASSERT_EQ(1u, manager_.MenuItems(item1->extension_id())->size());
  ASSERT_EQ(item1, manager_.MenuItems(item1->extension_id())->at(0));

  // Add item2_grandchild as a child of item2_child, then remove it.
  int id2_grandchild = manager_.AddChildItem(id2_child, item2_grandchild);
  ASSERT_GT(id2_grandchild, 0);
  ASSERT_EQ(1, item2->child_count());
  ASSERT_EQ(1, item2_child->child_count());
  ASSERT_TRUE(manager_.RemoveContextMenuItem(id2_grandchild));

  // We should only get 1 thing back when asking for item2's extension id, since
  // it has a child item.
  ASSERT_EQ(1u, manager_.MenuItems(item2->extension_id())->size());
  ASSERT_EQ(item2, manager_.MenuItems(item2->extension_id())->at(0));

  // Remove child2_item.
  ASSERT_TRUE(manager_.RemoveContextMenuItem(id2_child));
  ASSERT_EQ(1u, manager_.MenuItems(item2->extension_id())->size());
  ASSERT_EQ(item2, manager_.MenuItems(item2->extension_id())->at(0));
  ASSERT_EQ(0, item2->child_count());
}

// Tests changing parents.
TEST_F(ExtensionMenuManagerTest, ChangeParent) {
  Extension* extension1 = AddExtension("1111");

  // First create two items and add them both to the manager.
  ExtensionMenuItem* item1 = CreateTestItem(extension1);
  ExtensionMenuItem* item2 = CreateTestItem(extension1);

  int id1 = manager_.AddContextItem(extension1, item1);
  ASSERT_GT(id1, 0);
  int id2 = manager_.AddContextItem(extension1, item2);
  ASSERT_GT(id2, 0);

  const ExtensionMenuItem::List* items =
      manager_.MenuItems(item1->extension_id());
  ASSERT_EQ(2u, items->size());
  ASSERT_EQ(item1, items->at(0));
  ASSERT_EQ(item2, items->at(1));

  // Now create a third item, initially add it as a child of item1, then move
  // it to be a child of item2.
  ExtensionMenuItem* item3 = CreateTestItem(extension1);

  int id3 = manager_.AddChildItem(id1, item3);
  ASSERT_GT(id3, 0);
  ASSERT_EQ(1, item1->child_count());
  ASSERT_EQ(item3, item1->children()[0]);

  ASSERT_TRUE(manager_.ChangeParent(id3, id2));
  ASSERT_EQ(0, item1->child_count());
  ASSERT_EQ(1, item2->child_count());
  ASSERT_EQ(item3, item2->children()[0]);

  // Move item2 to be a child of item1.
  ASSERT_TRUE(manager_.ChangeParent(id2, id1));
  ASSERT_EQ(1, item1->child_count());
  ASSERT_EQ(item2, item1->children()[0]);
  ASSERT_EQ(1, item2->child_count());
  ASSERT_EQ(item3, item2->children()[0]);

  // Since item2 was a top-level item but is no longer, we should only have 1
  // top-level item.
  items = manager_.MenuItems(item1->extension_id());
  ASSERT_EQ(1u, items->size());
  ASSERT_EQ(item1, items->at(0));

  // Move item3 back to being a child of item1, so it's now a sibling of item2.
  ASSERT_TRUE(manager_.ChangeParent(id3, id1));
  ASSERT_EQ(2, item1->child_count());
  ASSERT_EQ(item2, item1->children()[0]);
  ASSERT_EQ(item3, item1->children()[1]);

  // Try switching item3 to be the parent of item1 - this should fail.
  ASSERT_FALSE(manager_.ChangeParent(id1, id3));
  ASSERT_EQ(0, item3->child_count());
  ASSERT_EQ(2, item1->child_count());
  ASSERT_EQ(item2, item1->children()[0]);
  ASSERT_EQ(item3, item1->children()[1]);
  items = manager_.MenuItems(item1->extension_id());
  ASSERT_EQ(1u, items->size());
  ASSERT_EQ(item1, items->at(0));

  // Move item2 to be a top-level item.
  ASSERT_TRUE(manager_.ChangeParent(id2, 0));
  items = manager_.MenuItems(item1->extension_id());
  ASSERT_EQ(2u, items->size());
  ASSERT_EQ(item1, items->at(0));
  ASSERT_EQ(item2, items->at(1));
  ASSERT_EQ(1, item1->child_count());
  ASSERT_EQ(item3, item1->children()[0]);

  // Make sure you can't move a node to be a child of another extension's item.
  Extension* extension2 = AddExtension("2222");
  ExtensionMenuItem* item4 = CreateTestItem(extension2);
  int id4 = manager_.AddContextItem(extension2, item4);
  ASSERT_GT(id4, 0);
  ASSERT_FALSE(manager_.ChangeParent(id4, id1));
  ASSERT_FALSE(manager_.ChangeParent(id1, id4));

  // Make sure you can't make an item be it's own parent.
  ASSERT_FALSE(manager_.ChangeParent(id1, id1));
}

// Tests that we properly remove an extension's menu item when that extension is
// unloaded.
TEST_F(ExtensionMenuManagerTest, ExtensionUnloadRemovesMenuItems) {
  NotificationService* notifier = NotificationService::current();
  ASSERT_TRUE(notifier != NULL);

  // Create a test extension.
  Extension* extension1 = AddExtension("1111");

  // Create an ExtensionMenuItem and put it into the manager.
  ExtensionMenuItem* item1 = CreateTestItem(extension1);
  ASSERT_EQ(extension1->id(), item1->extension_id());
  int id1 = manager_.AddContextItem(extension1, item1);
  ASSERT_GT(id1, 0);
  ASSERT_EQ(1u, manager_.MenuItems(extension1->id())->size());

  // Create a menu item with a different extension id and add it to the manager.
  Extension* extension2 = AddExtension("2222");
  ExtensionMenuItem* item2 = CreateTestItem(extension2);
  ASSERT_NE(item1->extension_id(), item2->extension_id());
  int id2 = manager_.AddContextItem(extension2, item2);
  ASSERT_GT(id2, 0);

  // Notify that the extension was unloaded, and make sure the right item is
  // gone.
  notifier->Notify(NotificationType::EXTENSION_UNLOADED,
                   Source<Profile>(NULL),
                   Details<Extension>(extension1));
  ASSERT_EQ(NULL, manager_.MenuItems(extension1->id()));
  ASSERT_EQ(1u, manager_.MenuItems(extension2->id())->size());
  ASSERT_TRUE(manager_.GetItemById(id1) == NULL);
  ASSERT_TRUE(manager_.GetItemById(id2) != NULL);
}

// A mock message service for tests of ExtensionMenuManager::ExecuteCommand.
class MockExtensionMessageService : public ExtensionMessageService {
 public:
  explicit MockExtensionMessageService(Profile* profile) :
      ExtensionMessageService(profile) {}

  MOCK_METHOD4(DispatchEventToRenderers, void(const std::string& event_name,
                                              const std::string& event_args,
                                              bool has_incognito_data,
                                              const GURL& event_url));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockExtensionMessageService);
};

// A mock profile for tests of ExtensionMenuManager::ExecuteCommand.
class MockTestingProfile : public TestingProfile {
 public:
  MockTestingProfile() {}
  MOCK_METHOD0(GetExtensionMessageService, ExtensionMessageService*());
  MOCK_METHOD0(IsOffTheRecord, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTestingProfile);
};

// Tests the RemoveAll functionality.
TEST_F(ExtensionMenuManagerTest, RemoveAll) {
  // Try removing all items for an extension id that doesn't have any items.
  manager_.RemoveAllContextItems("CCCC");

  // Add 2 top-level and one child item for extension 1.
  Extension* extension1 = AddExtension("1111");
  ExtensionMenuItem* item1 = CreateTestItem(extension1);
  ExtensionMenuItem* item2 = CreateTestItem(extension1);
  ExtensionMenuItem* item3 = CreateTestItem(extension1);
  int id1 = manager_.AddContextItem(extension1, item1);
  int id2 = manager_.AddContextItem(extension1, item2);
  EXPECT_GT(id1, 0);
  EXPECT_GT(id2, 0);
  int id3 = manager_.AddChildItem(id1, item3);
  EXPECT_GT(id3, 0);

  // Add one top-level item for extension 2.
  Extension* extension2 = AddExtension("2222");
  ExtensionMenuItem* item4 = CreateTestItem(extension2);
  manager_.AddContextItem(extension2, item4);

  EXPECT_EQ(2u, manager_.MenuItems(extension1->id())->size());
  EXPECT_EQ(1u, manager_.MenuItems(extension2->id())->size());

  // Remove extension2's item.
  manager_.RemoveAllContextItems(extension2->id());
  EXPECT_EQ(2u, manager_.MenuItems(extension1->id())->size());
  EXPECT_EQ(NULL, manager_.MenuItems(extension2->id()));

  // Remove extension1's items.
  manager_.RemoveAllContextItems(extension1->id());
  EXPECT_EQ(NULL, manager_.MenuItems(extension1->id()));
}

TEST_F(ExtensionMenuManagerTest, ExecuteCommand) {
  MessageLoopForUI message_loop;
  ChromeThread ui_thread(ChromeThread::UI, &message_loop);

  MockTestingProfile profile;

  scoped_refptr<MockExtensionMessageService> mock_message_service =
      new MockExtensionMessageService(&profile);

  ContextMenuParams params;
  params.media_type = WebKit::WebContextMenuData::MediaTypeImage;
  params.src_url = GURL("http://foo.bar/image.png");
  params.page_url = GURL("http://foo.bar");
  params.selection_text = L"Hello World";
  params.is_editable = false;

  Extension* extension = AddExtension("test");
  ExtensionMenuItem* item = CreateTestItem(extension);
  int id = manager_.AddContextItem(extension, item);
  ASSERT_GT(id, 0);

  EXPECT_CALL(profile, GetExtensionMessageService())
      .Times(1)
      .WillOnce(Return(mock_message_service.get()));

  EXPECT_CALL(profile, IsOffTheRecord())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));

  // Use the magic of googlemock to save a parameter to our mock's
  // DispatchEventToRenderers method into event_args.
  std::string event_args;
  std::string expected_event_name = "contextMenu/" + item->extension_id();
  EXPECT_CALL(*mock_message_service.get(),
              DispatchEventToRenderers(expected_event_name, _,
                                       profile.IsOffTheRecord(),
                                       GURL()))
      .Times(1)
      .WillOnce(SaveArg<1>(&event_args));

  manager_.ExecuteCommand(&profile, NULL /* tab_contents */, params, id);

  // Parse the json event_args, which should turn into a 2-element list where
  // the first element is a dictionary we want to inspect for the correct
  // values.
  scoped_ptr<Value> result(base::JSONReader::Read(event_args, true));
  Value* value = result.get();
  ASSERT_TRUE(result.get() != NULL);
  ASSERT_EQ(Value::TYPE_LIST, value->GetType());
  ListValue* list = static_cast<ListValue*>(value);
  ASSERT_EQ(2u, list->GetSize());

  DictionaryValue* info;
  ASSERT_TRUE(list->GetDictionary(0, &info));

  int tmp_id = 0;
  ASSERT_TRUE(info->GetInteger(L"menuItemId", &tmp_id));
  ASSERT_EQ(id, tmp_id);

  std::string tmp;
  ASSERT_TRUE(info->GetString(L"mediaType", &tmp));
  ASSERT_EQ("IMAGE", tmp);
  ASSERT_TRUE(info->GetString(L"srcUrl", &tmp));
  ASSERT_EQ(params.src_url.spec(), tmp);
  ASSERT_TRUE(info->GetString(L"mainFrameUrl", &tmp));
  ASSERT_EQ(params.page_url.spec(), tmp);

  std::wstring wide_tmp;
  ASSERT_TRUE(info->GetString(L"selectionText", &wide_tmp));
  ASSERT_EQ(params.selection_text, wide_tmp);

  bool bool_tmp = true;
  ASSERT_TRUE(info->GetBoolean(L"editable", &bool_tmp));
  ASSERT_EQ(params.is_editable, bool_tmp);
}
