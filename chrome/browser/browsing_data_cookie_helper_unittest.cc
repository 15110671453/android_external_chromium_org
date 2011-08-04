// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data_cookie_helper.h"


#include "base/bind.h"
#include "base/message_loop.h"
#include "chrome/test/base/testing_profile.h"
#include "content/browser/browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class BrowsingDataCookieHelperTest : public testing::Test {
 public:
  virtual void SetUp() {
    ui_thread_.reset(new BrowserThread(BrowserThread::UI, &message_loop_));
    // Note: we're starting a real IO thread because parts of the
    // BrowsingDataCookieHelper expect to run on that thread.
    io_thread_.reset(new BrowserThread(BrowserThread::IO));
    ASSERT_TRUE(io_thread_->Start());
  }

  virtual void TearDown() {
    message_loop_.RunAllPending();
    io_thread_.reset();
    ui_thread_.reset();
  }

  void CreateCookiesForTest() {
    testing_profile_.CreateRequestContext();
    scoped_refptr<net::CookieMonster> cookie_monster =
        testing_profile_.GetCookieMonster();
    cookie_monster->SetCookieWithOptionsAsync(
        GURL("http://www.google.com"), "A=1", net::CookieOptions(),
        net::CookieMonster::SetCookiesCallback());
    cookie_monster->SetCookieWithOptionsAsync(
        GURL("http://www.gmail.google.com"), "B=1", net::CookieOptions(),
        net::CookieMonster::SetCookiesCallback());
  }

  void FetchCallback(const net::CookieList& cookies) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    ASSERT_EQ(2UL, cookies.size());
    cookie_list_ = cookies;
    net::CookieList::const_iterator it = cookies.begin();

    // Correct because fetching cookies will get a sorted cookie list.
    ASSERT_TRUE(it != cookies.end());
    EXPECT_EQ("www.google.com", it->Domain());
    EXPECT_EQ("A", it->Name());

    ASSERT_TRUE(++it != cookies.end());
    EXPECT_EQ("www.gmail.google.com", it->Domain());
    EXPECT_EQ("B", it->Name());

    ASSERT_TRUE(++it == cookies.end());
    MessageLoop::current()->Quit();
  }

  void DeleteCallback(const net::CookieList& cookies) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    ASSERT_EQ(1UL, cookies.size());
    net::CookieList::const_iterator it = cookies.begin();

    ASSERT_TRUE(it != cookies.end());
    EXPECT_EQ("www.gmail.google.com", it->Domain());
    EXPECT_EQ("B", it->Name());

    ASSERT_TRUE(++it == cookies.end());
    MessageLoop::current()->Quit();
  }

  void CannedUniqueCallback(const net::CookieList& cookies) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    ASSERT_EQ(1UL, cookies.size());
    cookie_list_ = cookies;
    net::CookieList::const_iterator it = cookies.begin();

    ASSERT_TRUE(it != cookies.end());
    EXPECT_EQ("http://www.google.com/", it->Source());
    EXPECT_EQ("A", it->Name());

    ASSERT_TRUE(++it == cookies.end());
  }

 protected:
  MessageLoop message_loop_;
  scoped_ptr<BrowserThread> ui_thread_;
  scoped_ptr<BrowserThread> io_thread_;
  TestingProfile testing_profile_;

  net::CookieList cookie_list_;
};

TEST_F(BrowsingDataCookieHelperTest, FetchData) {
  CreateCookiesForTest();
  scoped_refptr<BrowsingDataCookieHelper> cookie_helper(
      new BrowsingDataCookieHelper(&testing_profile_));

  cookie_helper->StartFetching(
      base::Bind(&BrowsingDataCookieHelperTest::FetchCallback,
                 base::Unretained(this)));

  // Blocks until BrowsingDataCookieHelperTest::FetchCallback is notified.
  MessageLoop::current()->Run();
}

TEST_F(BrowsingDataCookieHelperTest, DeleteCookie) {
  CreateCookiesForTest();
  scoped_refptr<BrowsingDataCookieHelper> cookie_helper(
      new BrowsingDataCookieHelper(&testing_profile_));

  cookie_helper->StartFetching(
      base::Bind(&BrowsingDataCookieHelperTest::FetchCallback,
                 base::Unretained(this)));

  // Blocks until BrowsingDataCookieHelperTest::FetchCallback is notified.
  MessageLoop::current()->Run();

  net::CookieMonster::CanonicalCookie cookie = cookie_list_[0];
  cookie_helper->DeleteCookie(cookie);

  cookie_helper->StartFetching(
      base::Bind(&BrowsingDataCookieHelperTest::DeleteCallback,
                 base::Unretained(this)));
  MessageLoop::current()->Run();
}

TEST_F(BrowsingDataCookieHelperTest, CannedUnique) {
  const GURL origin("http://www.google.com");
  net::CookieList cookie;

  scoped_refptr<CannedBrowsingDataCookieHelper> helper(
      new CannedBrowsingDataCookieHelper(&testing_profile_));

  ASSERT_TRUE(helper->empty());
  helper->AddChangedCookie(origin, "A=1", net::CookieOptions());
  helper->AddChangedCookie(origin, "A=1", net::CookieOptions());
  helper->StartFetching(
      base::Bind(&BrowsingDataCookieHelperTest::CannedUniqueCallback,
                 base::Unretained(this)));
  cookie = cookie_list_;
  helper->Reset();
  ASSERT_TRUE(helper->empty());

  helper->AddReadCookies(origin, cookie);
  helper->AddReadCookies(origin, cookie);
  helper->StartFetching(
      base::Bind(&BrowsingDataCookieHelperTest::CannedUniqueCallback,
                 base::Unretained(this)));
}

TEST_F(BrowsingDataCookieHelperTest, CannedEmpty) {
  const GURL url_google("http://www.google.com");

  scoped_refptr<CannedBrowsingDataCookieHelper> helper(
      new CannedBrowsingDataCookieHelper(&testing_profile_));

  ASSERT_TRUE(helper->empty());
  helper->AddChangedCookie(url_google, "a=1",
                          net::CookieOptions());
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());

  net::CookieList cookies;
  net::CookieMonster::ParsedCookie pc("a=1");
  scoped_ptr<net::CookieMonster::CanonicalCookie> cookie(
      new net::CookieMonster::CanonicalCookie(url_google, pc));
  cookies.push_back(*cookie);

  helper->AddReadCookies(url_google, cookies);
  ASSERT_FALSE(helper->empty());
  helper->Reset();
  ASSERT_TRUE(helper->empty());
}

}  // namespace
