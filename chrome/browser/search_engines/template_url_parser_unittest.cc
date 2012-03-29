// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_parser.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

// ParamFilterImpl ------------------------------------------------------------

// Filters any param which as an occurrence of name_str_ in its name or an
// occurrence of value_str_ in its value.
class ParamFilterImpl : public TemplateURLParser::ParameterFilter {
 public:
  ParamFilterImpl(std::string name_str, std::string value_str);
  ~ParamFilterImpl();

  virtual bool KeepParameter(const std::string& key,
                             const std::string& value) OVERRIDE;

 private:
  std::string name_str_;
  std::string value_str_;

  DISALLOW_COPY_AND_ASSIGN(ParamFilterImpl);
};

ParamFilterImpl::ParamFilterImpl(std::string name_str, std::string value_str)
   : name_str_(name_str),
     value_str_(value_str) {
}

ParamFilterImpl::~ParamFilterImpl() {
}

bool ParamFilterImpl::KeepParameter(const std::string& key,
                                    const std::string& value) {
  return (name_str_.empty() || key.find(name_str_) == std::string::npos) &&
         (value_str_.empty() || value.find(value_str_) == std::string::npos);
}


// TemplateURLParserTest ------------------------------------------------------

class TemplateURLParserTest : public testing::Test {
 protected:
  TemplateURLParserTest();
  ~TemplateURLParserTest();

  virtual void SetUp() OVERRIDE;

  bool is_disabled() const;

  // Parses the OpenSearch description document at file_name (relative to the
  // data dir). The TemplateURL is placed in |template_url_|.
  void ParseFile(const std::string& file_name,
                 TemplateURLParser::ParameterFilter* filter);

  // ParseFile parses the results into this template_url.
  scoped_ptr<TemplateURL> template_url_;

 private:
  FilePath full_path_;
};

TemplateURLParserTest::TemplateURLParserTest() {
}

TemplateURLParserTest::~TemplateURLParserTest() {
}

void TemplateURLParserTest::SetUp() {
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &full_path_));
  full_path_ = full_path_.AppendASCII("osdd");
  if (!file_util::PathExists(full_path_)) {
    LOG(ERROR) <<
        "This test can't be run without some non-redistributable data";
    full_path_ = FilePath();
  }
}

bool TemplateURLParserTest::is_disabled() const {
  return full_path_.empty();
}

void TemplateURLParserTest::ParseFile(
    const std::string& file_name,
    TemplateURLParser::ParameterFilter* filter) {
  FilePath full_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &full_path));
  full_path = full_path.AppendASCII("osdd");
  full_path = full_path.AppendASCII(file_name);
  ASSERT_TRUE(file_util::PathExists(full_path));

  std::string contents;
  ASSERT_TRUE(file_util::ReadFileToString(full_path, &contents));
  template_url_.reset(TemplateURLParser::Parse(NULL, contents.data(),
                                               contents.length(), filter));
}


// Actual tests ---------------------------------------------------------------

TEST_F(TemplateURLParserTest, FailOnBogusURL) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("bogus.xml", NULL));
  EXPECT_FALSE(template_url_.get());
}

TEST_F(TemplateURLParserTest, PassOnHTTPS) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("https.xml", NULL));
  EXPECT_TRUE(template_url_.get());
}

TEST_F(TemplateURLParserTest, FailOnPost) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("post.xml", NULL));
  EXPECT_FALSE(template_url_.get());
}

TEST_F(TemplateURLParserTest, TestDictionary) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("dictionary.xml", NULL));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("Dictionary.com"), template_url_->short_name());
  EXPECT_EQ(GURL("http://cache.lexico.com/g/d/favicon.ico"),
            template_url_->favicon_url());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_EQ("http://dictionary.reference.com/browse/{searchTerms}?r=75",
            template_url_->url()->url());
}

TEST_F(TemplateURLParserTest, TestMSDN) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("msdn.xml", NULL));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("Search \" MSDN"), template_url_->short_name());
  EXPECT_EQ(GURL("http://search.msdn.microsoft.com/search/favicon.ico"),
            template_url_->favicon_url());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_EQ("http://search.msdn.microsoft.com/search/default.aspx?"
            "Query={searchTerms}&brand=msdn&locale=en-US",
            template_url_->url()->url());
}

TEST_F(TemplateURLParserTest, TestWikipedia) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("wikipedia.xml", NULL));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("Wikipedia (English)"), template_url_->short_name());
  EXPECT_EQ(GURL("http://en.wikipedia.org/favicon.ico"),
            template_url_->favicon_url());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_EQ("http://en.wikipedia.org/w/index.php?"
            "title=Special:Search&search={searchTerms}",
            template_url_->url()->url());
  ASSERT_FALSE(template_url_->suggestions_url() == NULL);
  EXPECT_TRUE(template_url_->suggestions_url()->SupportsReplacement());
  EXPECT_EQ("http://en.wikipedia.org/w/api.php?"
            "action=opensearch&search={searchTerms}",
            template_url_->suggestions_url()->url());
  ASSERT_EQ(2U, template_url_->input_encodings().size());
  EXPECT_EQ("UTF-8", template_url_->input_encodings()[0]);
  EXPECT_EQ("Shift_JIS", template_url_->input_encodings()[1]);
}

TEST_F(TemplateURLParserTest, NoCrashOnEmptyAttributes) {
  if (is_disabled())
    return;
  ASSERT_NO_FATAL_FAILURE(ParseFile("url_with_no_attributes.xml", NULL));
}

TEST_F(TemplateURLParserTest, TestFirefoxEbay) {
  if (is_disabled())
    return;
  // This file uses the Parameter extension
  // (see http://www.opensearch.org/Specifications/OpenSearch/Extensions/Parameter/1.0)
  ParamFilterImpl filter("ebay", "ebay");
  ASSERT_NO_FATAL_FAILURE(ParseFile("firefox_ebay.xml", &filter));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("eBay"), template_url_->short_name());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_EQ("http://search.ebay.com/search/search.dll?query={searchTerms}&"
            "MfcISAPICommand=GetResult&ht=1&srchdesc=n&maxRecordsReturned=300&"
            "maxRecordsPerPage=50&SortProperty=MetaEndSort",
            template_url_->url()->url());
  ASSERT_EQ(1U, template_url_->input_encodings().size());
  EXPECT_EQ("ISO-8859-1", template_url_->input_encodings()[0]);
  EXPECT_EQ(GURL("http://search.ebay.com/favicon.ico"),
            template_url_->favicon_url());
}

TEST_F(TemplateURLParserTest, TestFirefoxWebster) {
  if (is_disabled())
    return;
  // This XML file uses a namespace.
  ParamFilterImpl filter(std::string(), "Mozilla");
  ASSERT_NO_FATAL_FAILURE(ParseFile("firefox_webster.xml", &filter));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("Webster"), template_url_->short_name());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_EQ("http://www.webster.com/cgi-bin/dictionary?va={searchTerms}",
            template_url_->url()->url());
  ASSERT_EQ(1U, template_url_->input_encodings().size());
  EXPECT_EQ("ISO-8859-1", template_url_->input_encodings()[0]);
  EXPECT_EQ(GURL("http://www.webster.com/favicon.ico"),
            template_url_->favicon_url());
}

TEST_F(TemplateURLParserTest, TestFirefoxYahoo) {
  if (is_disabled())
    return;
  // This XML file uses a namespace.
  ParamFilterImpl filter(std::string(), "Mozilla");
  ASSERT_NO_FATAL_FAILURE(ParseFile("firefox_yahoo.xml", &filter));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("Yahoo"), template_url_->short_name());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_EQ("http://ff.search.yahoo.com/gossip?"
            "output=fxjson&command={searchTerms}",
            template_url_->suggestions_url()->url());
  EXPECT_EQ("http://search.yahoo.com/search?p={searchTerms}&ei=UTF-8",
            template_url_->url()->url());
  ASSERT_EQ(1U, template_url_->input_encodings().size());
  EXPECT_EQ("UTF-8", template_url_->input_encodings()[0]);
  EXPECT_EQ(GURL("http://search.yahoo.com/favicon.ico"),
            template_url_->favicon_url());
}

// Make sure we ignore POST suggestions (this is the same XML file as
// firefox_yahoo.xml, the suggestion method was just changed to POST).
TEST_F(TemplateURLParserTest, TestPostSuggestion) {
  if (is_disabled())
    return;
  // This XML file uses a namespace.
  ParamFilterImpl filter(std::string(), "Mozilla");
  ASSERT_NO_FATAL_FAILURE(ParseFile("post_suggestion.xml", &filter));
  ASSERT_TRUE(template_url_.get());
  EXPECT_EQ(ASCIIToUTF16("Yahoo"), template_url_->short_name());
  ASSERT_FALSE(template_url_->url() == NULL);
  EXPECT_TRUE(template_url_->url()->SupportsReplacement());
  EXPECT_TRUE(template_url_->suggestions_url() == NULL);
  EXPECT_EQ("http://search.yahoo.com/search?p={searchTerms}&ei=UTF-8",
            template_url_->url()->url());
  ASSERT_EQ(1U, template_url_->input_encodings().size());
  EXPECT_EQ("UTF-8", template_url_->input_encodings()[0]);
  EXPECT_EQ(GURL("http://search.yahoo.com/favicon.ico"),
            template_url_->favicon_url());
}
