// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_condition_attribute.h"

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_vector.h"
#include "base/message_loop.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_condition.h"
#include "chrome/browser/extensions/api/declarative_webrequest/webrequest_constants.h"
#include "content/public/browser/resource_request_info.h"
#include "net/test/test_server.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::DictionaryValue;
using base::FundamentalValue;
using base::ListValue;
using base::StringValue;
using base::Value;

namespace {
const char kUnknownConditionName[] = "unknownType";
}  // namespace

namespace extensions {

namespace keys = declarative_webrequest_constants;

TEST(WebRequestConditionAttributeTest, CreateConditionAttribute) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  std::string error;
  scoped_ptr<WebRequestConditionAttribute> result;
  StringValue string_value("main_frame");
  ListValue resource_types;
  resource_types.Append(Value::CreateStringValue("main_frame"));

  // Test wrong condition name passed.
  error.clear();
  result = WebRequestConditionAttribute::Create(
      kUnknownConditionName, &resource_types, &error);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(result.get());

  // Test wrong data type passed
  error.clear();
  result = WebRequestConditionAttribute::Create(
      keys::kResourceTypeKey, &string_value, &error);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(result.get());

  error.clear();
  result = WebRequestConditionAttribute::Create(
      keys::kContentTypeKey, &string_value, &error);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(result.get());

  // Test success
  error.clear();
  result = WebRequestConditionAttribute::Create(
      keys::kResourceTypeKey, &resource_types, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(result.get());
  EXPECT_EQ(WebRequestConditionAttribute::CONDITION_RESOURCE_TYPE,
            result->GetType());
}

TEST(WebRequestConditionAttributeTest, ResourceType) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  std::string error;
  ListValue resource_types;
  resource_types.Append(Value::CreateStringValue("main_frame"));

  scoped_ptr<WebRequestConditionAttribute> attribute =
      WebRequestConditionAttribute::Create(
          keys::kResourceTypeKey, &resource_types, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(attribute.get());

  net::TestURLRequestContext context;
  net::TestURLRequest url_request_ok(
      GURL("http://www.example.com"), NULL, &context);
  content::ResourceRequestInfo::AllocateForTesting(&url_request_ok,
      ResourceType::MAIN_FRAME, NULL, -1, -1);
  EXPECT_TRUE(attribute->IsFulfilled(WebRequestData(&url_request_ok,
                                                    ON_BEFORE_REQUEST)));

  net::TestURLRequest url_request_fail(
      GURL("http://www.example.com"), NULL, &context);
  content::ResourceRequestInfo::AllocateForTesting(&url_request_ok,
      ResourceType::SUB_FRAME, NULL, -1, -1);
  EXPECT_FALSE(attribute->IsFulfilled(WebRequestData(&url_request_fail,
                                                     ON_BEFORE_REQUEST)));
}

TEST(WebRequestConditionAttributeTest, ContentType) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  std::string error;
  scoped_ptr<WebRequestConditionAttribute> result;

  net::TestServer test_server(
      net::TestServer::TYPE_HTTP,
      net::TestServer::kLocalhost,
      base::FilePath(FILE_PATH_LITERAL(
          "chrome/test/data/extensions/api_test/webrequest/declarative")));
  ASSERT_TRUE(test_server.Start());

  net::TestURLRequestContext context;
  net::TestDelegate delegate;
  net::TestURLRequest url_request(test_server.GetURL("files/headers.html"),
                                                     &delegate, &context);
  url_request.Start();
  MessageLoop::current()->Run();

  ListValue content_types;
  content_types.Append(Value::CreateStringValue("text/plain"));
  scoped_ptr<WebRequestConditionAttribute> attribute_include =
      WebRequestConditionAttribute::Create(
          keys::kContentTypeKey, &content_types, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(attribute_include.get());
  EXPECT_FALSE(attribute_include->IsFulfilled(
      WebRequestData(&url_request, ON_BEFORE_REQUEST,
                     url_request.response_headers())));
  EXPECT_TRUE(attribute_include->IsFulfilled(
      WebRequestData(&url_request, ON_HEADERS_RECEIVED,
                     url_request.response_headers())));

  scoped_ptr<WebRequestConditionAttribute> attribute_exclude =
      WebRequestConditionAttribute::Create(
          keys::kExcludeContentTypeKey, &content_types, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(attribute_exclude.get());
  EXPECT_FALSE(attribute_exclude->IsFulfilled(
      WebRequestData(&url_request, ON_HEADERS_RECEIVED,
                     url_request.response_headers())));

  content_types.Clear();
  content_types.Append(Value::CreateStringValue("something/invalid"));
  scoped_ptr<WebRequestConditionAttribute> attribute_unincluded =
      WebRequestConditionAttribute::Create(
          keys::kContentTypeKey, &content_types, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(attribute_unincluded.get());
  EXPECT_FALSE(attribute_unincluded->IsFulfilled(
      WebRequestData(&url_request, ON_HEADERS_RECEIVED,
                     url_request.response_headers())));

  scoped_ptr<WebRequestConditionAttribute> attribute_unexcluded =
      WebRequestConditionAttribute::Create(
          keys::kExcludeContentTypeKey, &content_types, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(attribute_unexcluded.get());
  EXPECT_TRUE(attribute_unexcluded->IsFulfilled(
      WebRequestData(&url_request, ON_HEADERS_RECEIVED,
                     url_request.response_headers())));
}

// Testing WebRequestConditionAttributeThirdParty.
TEST(WebRequestConditionAttributeTest, ThirdParty) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  std::string error;
  const FundamentalValue value_true(true);
  // This attribute matches only third party requests.
  scoped_ptr<WebRequestConditionAttribute> third_party_attribute =
      WebRequestConditionAttribute::Create(keys::kThirdPartyKey,
                                           &value_true,
                                           &error);
  ASSERT_EQ("", error);
  ASSERT_TRUE(third_party_attribute.get());
  const FundamentalValue value_false(false);
  // This attribute matches only first party requests.
  scoped_ptr<WebRequestConditionAttribute> first_party_attribute =
      WebRequestConditionAttribute::Create(keys::kThirdPartyKey,
                                           &value_false,
                                           &error);
  ASSERT_EQ("", error);
  ASSERT_TRUE(first_party_attribute.get());

  const GURL url_empty;
  const GURL url_a("http://a.com");
  const GURL url_b("http://b.com");
  net::TestURLRequestContext context;
  net::TestDelegate delegate;
  net::TestURLRequest url_request(url_a, &delegate, &context);

  for (unsigned int i = 1; i <= kLastActiveStage; i <<= 1) {
    if (!(kActiveStages & i))
      continue;
    const RequestStage stage = static_cast<RequestStage>(i);
    url_request.set_first_party_for_cookies(url_empty);
    EXPECT_FALSE(third_party_attribute->IsFulfilled(WebRequestData(&url_request,
                                                                   stage)));
    EXPECT_TRUE(first_party_attribute->IsFulfilled(WebRequestData(&url_request,
                                                                  stage)));

    url_request.set_first_party_for_cookies(url_b);
    EXPECT_TRUE(third_party_attribute->IsFulfilled(WebRequestData(&url_request,
                                                                  stage)));
    EXPECT_FALSE(first_party_attribute->IsFulfilled(WebRequestData(&url_request,
                                                                   stage)));

    url_request.set_first_party_for_cookies(url_a);
    EXPECT_FALSE(third_party_attribute->IsFulfilled(WebRequestData(&url_request,
                                                                   stage)));
    EXPECT_TRUE(first_party_attribute->IsFulfilled(WebRequestData(&url_request,
                                                                  stage)));
  }
}

// Testing WebRequestConditionAttributeStages. This iterates over all stages,
// and tests a couple of "stage" attributes -- one created with an empty set of
// applicable stages, one for each stage applicable for that stage, and one
// applicable in all stages.
TEST(WebRequestConditionAttributeTest, Stages) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  typedef std::pair<RequestStage, const char*> StageNamePair;
  static const StageNamePair active_stages[] = {
    StageNamePair(ON_BEFORE_REQUEST, keys::kOnBeforeRequestEnum),
    StageNamePair(ON_BEFORE_SEND_HEADERS, keys::kOnBeforeSendHeadersEnum),
    StageNamePair(ON_HEADERS_RECEIVED, keys::kOnHeadersReceivedEnum),
    StageNamePair(ON_AUTH_REQUIRED, keys::kOnAuthRequiredEnum)
  };

  // Check that exactly all active stages are considered in this test.
  unsigned int covered_stages = 0;
  for (size_t i = 0; i < arraysize(active_stages); ++i)
    covered_stages |= active_stages[i].first;
  EXPECT_EQ(kActiveStages, covered_stages);

  std::string error;

  // Create an attribute with an empty set of applicable stages.
  ListValue empty_list;
  scoped_ptr<WebRequestConditionAttribute> empty_attribute =
      WebRequestConditionAttribute::Create(keys::kStagesKey,
                                           &empty_list,
                                           &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(empty_attribute.get());

  // Create an attribute with all possible applicable stages.
  ListValue all_stages;
  for (size_t i = 0; i < arraysize(active_stages); ++i)
    all_stages.AppendString(active_stages[i].second);
  scoped_ptr<WebRequestConditionAttribute> attribute_with_all =
      WebRequestConditionAttribute::Create(keys::kStagesKey,
                                           &all_stages,
                                           &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(attribute_with_all.get());

  // Create one attribute for each single stage, to be applicable in that stage.
  ScopedVector<WebRequestConditionAttribute> one_stage_attributes;

  for (size_t i = 0; i < arraysize(active_stages); ++i) {
    ListValue single_stage_list;
    single_stage_list.AppendString(active_stages[i].second);
    one_stage_attributes.push_back(
        WebRequestConditionAttribute::Create(keys::kStagesKey,
                                             &single_stage_list,
                                             &error).release());
    EXPECT_EQ("", error);
    ASSERT_TRUE(one_stage_attributes.back() != NULL);
  }

  const GURL url_empty;
  net::TestURLRequestContext context;
  net::TestDelegate delegate;
  net::TestURLRequest url_request(url_empty, &delegate, &context);

  for (size_t i = 0; i < arraysize(active_stages); ++i) {
    EXPECT_FALSE(empty_attribute->IsFulfilled(
        WebRequestData(&url_request, active_stages[i].first)));

    for (size_t j = 0; j < one_stage_attributes.size(); ++j) {
      EXPECT_EQ(i == j,
                one_stage_attributes[j]->IsFulfilled(
                    WebRequestData(&url_request, active_stages[i].first)));
    }

    EXPECT_TRUE(attribute_with_all->IsFulfilled(
        WebRequestData(&url_request, active_stages[i].first)));
  }
}

namespace {

// Builds a vector of vectors of string pointers from an array of strings.
// |array| is in fact a sequence of arrays. The array |sizes| captures the sizes
// of all parts of |array|, and |size| is the length of |sizes| itself.
// Example (this is pseudo-code, not C++):
// array = { "a", "b", "c", "d", "e", "f" }
// sizes = { 2, 0, 4 }
// size = 3
// results in out == { {&"a", &"b"}, {}, {&"c", &"d", &"e", &"f"} }
void GetArrayAsVector(const std::string array[],
                      const size_t sizes[],
                      const size_t size,
                      std::vector< std::vector<const std::string*> >* out) {
  out->clear();
  size_t next = 0;
  for (size_t i = 0; i < size; ++i) {
    out->push_back(std::vector<const std::string*>());
    for (size_t j = next; j < next + sizes[i]; ++j) {
      out->back().push_back(&(array[j]));
    }
    next += sizes[i];
  }
}

// Builds a DictionaryValue from an array of the form {name1, value1, name2,
// value2, ...}. Values for the same key are grouped in a ListValue.
scoped_ptr<DictionaryValue> GetDictionaryFromArray(
    const std::vector<const std::string*>& array) {
  const size_t length = array.size();
  CHECK(length % 2 == 0);

  scoped_ptr<DictionaryValue> dictionary(new DictionaryValue);
  for (size_t i = 0; i < length; i += 2) {
    const std::string* name = array[i];
    const std::string* value = array[i+1];
    if (dictionary->HasKey(*name)) {
      Value* entry = NULL;
      ListValue* list = NULL;
      if (!dictionary->GetWithoutPathExpansion(*name, &entry))
        return scoped_ptr<DictionaryValue>(NULL);
      switch (entry->GetType()) {
        case Value::TYPE_STRING:  // Replace the present string with a list.
          list = new ListValue;
          // Ignoring return value, we already verified the entry is there.
          dictionary->RemoveWithoutPathExpansion(*name, &entry);
          list->Append(entry);
          list->Append(Value::CreateStringValue(*value));
          dictionary->SetWithoutPathExpansion(*name, list);
          break;
        case Value::TYPE_LIST:  // Just append to the list.
          CHECK(entry->GetAsList(&list));
          list->Append(Value::CreateStringValue(*value));
          break;
        default:
          NOTREACHED();  // We never put other Values here.
          return scoped_ptr<DictionaryValue>(NULL);
      }
    } else {
      dictionary->SetString(*name, *value);
    }
  }
  return dictionary.Pass();
}

// Returns whether the response headers from |url_request| satisfy the match
// criteria given in |tests|. For at least one |i| all tests from |tests[i]|
// must pass. If |positive_test| is true, the dictionary is interpreted as the
// containsHeaders property of a RequestMatcher, otherwise as
// doesNotContainHeaders.
void MatchAndCheck(const std::vector< std::vector<const std::string*> >& tests,
                   const std::string& key,
                   RequestStage stage,
                   net::URLRequest* url_request,
                   bool* result) {
  ListValue contains_headers;
  for (size_t i = 0; i < tests.size(); ++i) {
    scoped_ptr<DictionaryValue> temp(GetDictionaryFromArray(tests[i]));
    ASSERT_TRUE(temp.get());
    contains_headers.Append(temp.release());
  }

  std::string error;
  scoped_ptr<WebRequestConditionAttribute> attribute =
      WebRequestConditionAttribute::Create(key, &contains_headers, &error);
  ASSERT_EQ("", error);
  ASSERT_TRUE(attribute.get());

  *result = attribute->IsFulfilled(WebRequestData(
      url_request, stage, url_request->response_headers()));
}

}  // namespace

// Here we test WebRequestConditionAttributeRequestHeaders for matching
// correctly against request headers. This test is not as extensive as
// "ResponseHeaders" (below), because the header-matching code is shared
// by both types of condition attributes, so it is enough to test it once.
TEST(WebRequestConditionAttributeTest, RequestHeaders) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  net::TestURLRequestContext context;
  net::TestDelegate delegate;
  net::TestURLRequest url_request(GURL("http://example.com"),  // Dummy URL.
                                  &delegate, &context);
  url_request.SetExtraRequestHeaderByName(
      "Custom-header", "custom/value", true /* overwrite */);
  url_request.Start();
  MessageLoop::current()->Run();

  std::vector<std::vector<const std::string*> > tests;
  bool result = false;

  const RequestStage stage = ON_BEFORE_SEND_HEADERS;

  // First set of test data -- passing conjunction.
  const std::string kPassingCondition[] = {
    keys::kNameContainsKey, "CuStOm",  // Header names are case insensitive.
    keys::kNameEqualsKey, "custom-header",
    keys::kValueSuffixKey, "alue",
    keys::kValuePrefixKey, "custom/value"
  };
  const size_t kPassingConditionSizes[] = { arraysize(kPassingCondition) };
  GetArrayAsVector(kPassingCondition, kPassingConditionSizes, 1u, &tests);
  // Positive filter, passing (conjunction of tests).
  MatchAndCheck(tests, keys::kRequestHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);
  // Negative filter, failing (conjunction of tests).
  MatchAndCheck(
      tests, keys::kExcludeRequestHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);

  // Second set of test data -- failing disjunction.
  const std::string kFailCondition[] = {
    keys::kNameSuffixKey, "Custom",      // Test 1.
    keys::kNameEqualsKey, "ustom-valu",  // Test 2.
    keys::kValuePrefixKey, "custom ",    // Test 3.
    keys::kValueContainsKey, " value"    // Test 4.
  };
  const size_t kFailConditionSizes[] = { 2u, 2u, 2u, 2u };
  GetArrayAsVector(kFailCondition, kFailConditionSizes, 4u, &tests);
  // Positive filter, failing (disjunction of tests).
  MatchAndCheck(tests, keys::kRequestHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);
  // Negative filter, passing (disjunction of tests).
  MatchAndCheck(
      tests, keys::kExcludeRequestHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // Third set of test data, corner case -- empty disjunction.
  GetArrayAsVector(NULL, NULL, 0u, &tests);
  // Positive filter, failing (no test to pass).
  MatchAndCheck(tests, keys::kRequestHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);
  // Negative filter, passing (no test to fail).
  MatchAndCheck(
      tests, keys::kExcludeRequestHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // Fourth set of test data, corner case -- empty conjunction.
  const size_t kEmptyConjunctionSizes[] = { 0u };
  GetArrayAsVector(NULL, kEmptyConjunctionSizes, 1u, &tests);
  // Positive filter, passing (trivial test).
  MatchAndCheck(tests, keys::kRequestHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);
  // Negative filter, failing.
  MatchAndCheck(
      tests, keys::kExcludeRequestHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);
}

// Here we test WebRequestConditionAttributeResponseHeaders for:
// 1. Correct implementation of prefix/suffix/contains/equals matching.
// 2. Performing logical disjunction (||) between multiple specifications.
// 3. Negating the match in case of 'doesNotContainHeaders'.
TEST(WebRequestConditionAttributeTest, ResponseHeaders) {
  // Necessary for TestURLRequest.
  MessageLoop message_loop(MessageLoop::TYPE_IO);

  net::TestServer test_server(
      net::TestServer::TYPE_HTTP,
      net::TestServer::kLocalhost,
      base::FilePath(FILE_PATH_LITERAL(
          "chrome/test/data/extensions/api_test/webrequest/declarative")));
  ASSERT_TRUE(test_server.Start());

  net::TestURLRequestContext context;
  net::TestDelegate delegate;
  net::TestURLRequest url_request(test_server.GetURL("files/headers.html"),
                                  &delegate, &context);
  url_request.Start();
  MessageLoop::current()->Run();

  // In all the tests below we assume that the server includes the headers
  // Custom-Header: custom/value
  // Custom-Header-B: valueA
  // Custom-Header-B: valueB
  // Custom-Header-C: valueC, valueD
  // Custom-Header-D:
  // in the response, but does not include "Non-existing: void".

  std::vector< std::vector<const std::string*> > tests;
  bool result;

  const RequestStage stage = ON_HEADERS_RECEIVED;

  // 1.a. -- All these tests should pass.
  const std::string kPassingCondition[] = {
    keys::kNamePrefixKey, "Custom",
    keys::kNameSuffixKey, "m-header",  // Header names are case insensitive.
    keys::kValueContainsKey, "alu",
    keys::kValueEqualsKey, "custom/value"
  };
  const size_t kPassingConditionSizes[] = { arraysize(kPassingCondition) };
  GetArrayAsVector(kPassingCondition, kPassingConditionSizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 1.b. -- None of the following tests in the discjunction should pass.
  const std::string kFailCondition[] = {
    keys::kNamePrefixKey, " Custom",  // Test 1.
    keys::kNameContainsKey, " -",     // Test 2.
    keys::kValueSuffixKey, "alu",     // Test 3.
    keys::kValueEqualsKey, "custom"   // Test 4.
  };
  const size_t kFailConditionSizes[] = { 2u, 2u, 2u, 2u };
  GetArrayAsVector(kFailCondition, kFailConditionSizes, 4u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);

  // 1.c. -- This should fail (mixing name and value from different headers)
  const std::string kMixingCondition[] = {
    keys::kNameSuffixKey, "Header-B",
    keys::kValueEqualsKey, "custom/value"
  };
  const size_t kMixingConditionSizes[] = { arraysize(kMixingCondition) };
  GetArrayAsVector(kMixingCondition, kMixingConditionSizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);

  // 1.d. -- Test handling multiple values for one header (both should pass).
  const std::string kMoreValues1[] = {
    keys::kNameEqualsKey, "Custom-header-b",
    keys::kValueEqualsKey, "valueA"
  };
  const size_t kMoreValues1Sizes[] = { arraysize(kMoreValues1) };
  GetArrayAsVector(kMoreValues1, kMoreValues1Sizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);
  const std::string kMoreValues2[] = {
    keys::kNameEqualsKey, "Custom-header-b",
    keys::kValueEqualsKey, "valueB"
  };
  const size_t kMoreValues2Sizes[] = { arraysize(kMoreValues2) };
  GetArrayAsVector(kMoreValues2, kMoreValues2Sizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 1.e. -- This should fail as conjunction but pass as disjunction.
  const std::string kConflict[] = {
    keys::kNameSuffixKey, "Header",      // True for some header.
    keys::kNameContainsKey, "Header-B"   // True for a different header.
  };
  // First disjunction, no conflict.
  const size_t kNoConflictSizes[] = { 2u, 2u };
  GetArrayAsVector(kConflict, kNoConflictSizes, 2u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);
  // Then conjunction, conflict.
  const size_t kConflictSizes[] = { arraysize(kConflict) };
  GetArrayAsVector(kConflict, kConflictSizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);

  // 1.f. -- This should pass, checking for correct treatment of ',' in values.
  const std::string kComma[] = {
    keys::kNameSuffixKey, "Header-C",
    keys::kValueEqualsKey, "valueC, valueD"
  };
  const size_t kCommaSizes[] = { arraysize(kComma) };
  GetArrayAsVector(kComma, kCommaSizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 1.g. -- This should pass, empty values are values as well.
  const std::string kEmpty[] = {
    keys::kNameEqualsKey, "custom-header-d",
    keys::kValueEqualsKey, ""
  };
  const size_t kEmptySizes[] = { arraysize(kEmpty) };
  GetArrayAsVector(kEmpty, kEmptySizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 1.h. -- Values are case-sensitive, this should fail.
  const std::string kLowercase[] = {
    keys::kNameEqualsKey, "Custom-header-b",
    keys::kValuePrefixKey, "valueb",  // valueb != valueB
    keys::kNameEqualsKey, "Custom-header-b",
    keys::kValueSuffixKey, "valueb",
    keys::kNameEqualsKey, "Custom-header-b",
    keys::kValueContainsKey, "valueb",
    keys::kNameEqualsKey, "Custom-header-b",
    keys::kValueEqualsKey, "valueb"
  };
  const size_t kLowercaseSizes[] = { 4u, 4u, 4u, 4u };  // As disjunction.
  GetArrayAsVector(kLowercase, kLowercaseSizes, 4u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);

  // 1.i. -- Names are case-insensitive, this should pass.
  const std::string kUppercase[] = {
    keys::kNamePrefixKey, "CUSTOM-HEADER-B",
    keys::kNameSuffixKey, "CUSTOM-HEADER-B",
    keys::kNameEqualsKey, "CUSTOM-HEADER-B",
    keys::kNameContainsKey, "CUSTOM-HEADER-B"
  };
  const size_t kUppercaseSizes[] = { arraysize(kUppercase) };  // Conjunction.
  GetArrayAsVector(kUppercase, kUppercaseSizes, 1u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 2.a. -- This should pass as disjunction, because one of the tests passes.
  const std::string kDisjunction[] = {
    keys::kNamePrefixKey, "Non-existing",  // This one fails.
    keys::kNameSuffixKey, "Non-existing",  // This one fails.
    keys::kValueEqualsKey, "void",         // This one fails.
    keys::kValueContainsKey, "alu"         // This passes.
  };
  const size_t kDisjunctionSizes[] = { 2u, 2u, 2u, 2u };
  GetArrayAsVector(kDisjunction, kDisjunctionSizes, 4u, &tests);
  MatchAndCheck(tests, keys::kResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 3.a. -- This should pass.
  const std::string kNonExistent[] = {
    keys::kNameEqualsKey, "Non-existing",
    keys::kValueEqualsKey, "void"
  };
  const size_t kNonExistentSizes[] = { arraysize(kNonExistent) };
  GetArrayAsVector(kNonExistent, kNonExistentSizes, 1u, &tests);
  MatchAndCheck(
      tests, keys::kExcludeResponseHeadersKey, stage, &url_request, &result);
  EXPECT_TRUE(result);

  // 3.b. -- This should fail.
  const std::string kExisting[] = {
    keys::kNameEqualsKey, "custom-header-b",
    keys::kValueEqualsKey, "valueB"
  };
  const size_t kExistingSize[] = { arraysize(kExisting) };
  GetArrayAsVector(kExisting, kExistingSize, 1u, &tests);
  MatchAndCheck(
      tests, keys::kExcludeResponseHeadersKey, stage, &url_request, &result);
  EXPECT_FALSE(result);
}

}  // namespace extensions
