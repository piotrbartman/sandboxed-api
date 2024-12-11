// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <arpa/inet.h>

#include <fstream>

#include "contrib/uriparser/sandboxed.h"
#include "sandboxed_api/util/path.h"
#include "sandboxed_api/util/status_matchers.h"
#include "sandboxed_api/util/temp_file.h"

#include "absl/cleanup/cleanup.h"

namespace {

using ::sapi::IsOk;

const struct TestVariant {
  std::string test;
  std::string uri;
  std::string uriescaped;
  std::string scheme;
  std::string userinfo;
  std::string hosttext;
  std::string hostip;
  std::string porttext;
  std::string query;
  std::string fragment;
  std::string normalized;
  std::string add_base_example;
  std::string remove_base_example;
  std::vector<std::string> path_elements;
  std::map<std::string, std::string> query_elements;
} TestData[] = {
    {
        .test = "http://www.example.com/",
        .uri = "http://www.example.com/",
        .uriescaped = "http%3A%2F%2Fwww.example.com%2F",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "www.example.com",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://www.example.com/",
        .add_base_example = "http://www.example.com/",
        .remove_base_example = "./",
    },
    {
        .test = "https://github.com/google/sandboxed-api/",
        .uri = "https://github.com/google/sandboxed-api/",
        .uriescaped = "https%3A%2F%2Fgithub.com%2Fgoogle%2Fsandboxed-api%2F",
        .scheme = "https",
        .userinfo = "",
        .hosttext = "github.com",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "https://github.com/google/sandboxed-api/",
        .add_base_example = "https://github.com/google/sandboxed-api/",
        .remove_base_example = "https://github.com/google/sandboxed-api/",
        .path_elements = {"google", "sandboxed-api"},
    },
    {
        .test = "mailto:test@example.com",
        .uri = "mailto:test@example.com",
        .uriescaped = "mailto%3Atest%40example.com",
        .scheme = "mailto",
        .userinfo = "",
        .hosttext = "",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "mailto:test@example.com",
        .add_base_example = "mailto:test@example.com",
        .remove_base_example = "mailto:test@example.com",
        .path_elements = {"test@example.com"},
    },
    {
        .test = "file:///bin/bash",
        .uri = "file:///bin/bash",
        .uriescaped = "file%3A%2F%2F%2Fbin%2Fbash",
        .scheme = "file",
        .userinfo = "",
        .hosttext = "",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "file:///bin/bash",
        .add_base_example = "file:///bin/bash",
        .remove_base_example = "file:///bin/bash",
        .path_elements =
            {
                "bin",
                "bash",
            },
    },
    {
        .test = "http://www.example.com/name%20with%20spaces/",
        .uri = "http://www.example.com/name%20with%20spaces/",
        .uriescaped =
            "http%3A%2F%2Fwww.example.com%2Fname%2520with%2520spaces%2F",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "www.example.com",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://www.example.com/name%20with%20spaces/",
        .add_base_example = "http://www.example.com/name%20with%20spaces/",
        .remove_base_example = "name%20with%20spaces/",
        .path_elements =
            {
                "name%20with%20spaces",
            },
    },
    {
        .test = "http://abcdefg@localhost/",
        .uri = "http://abcdefg@localhost/",
        .uriescaped = "http%3A%2F%2Fabcdefg%40localhost%2F",
        .scheme = "http",
        .userinfo = "abcdefg",
        .hosttext = "localhost",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://abcdefg@localhost/",
        .add_base_example = "http://abcdefg@localhost/",
        .remove_base_example = "//abcdefg@localhost/",
    },
    {
        .test = "https://localhost:123/",
        .uri = "https://localhost:123/",
        .uriescaped = "https%3A%2F%2Flocalhost%3A123%2F",
        .scheme = "https",
        .userinfo = "",
        .hosttext = "localhost",
        .hostip = "",
        .porttext = "123",
        .query = "",
        .fragment = "",
        .normalized = "https://localhost:123/",
        .add_base_example = "https://localhost:123/",
        .remove_base_example = "https://localhost:123/",
    },
    {
        .test = "http://[::1]/",
        .uri = "http://[0000:0000:0000:0000:0000:0000:0000:0001]/",
        .uriescaped = "http%3A%2F%2F%5B0000%3A0000%3A0000%3A0000%3A0000%3A0000%"
                      "3A0000%3A0001%5D%2F",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "::1",
        .hostip = "::1",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://[0000:0000:0000:0000:0000:0000:0000:0001]/",
        .add_base_example = "http://[0000:0000:0000:0000:0000:0000:0000:0001]/",
        .remove_base_example = "//[0000:0000:0000:0000:0000:0000:0000:0001]/",
    },
    {
        .test = "http://a/b/c/d;p?q",
        .uri = "http://a/b/c/d;p?q",
        .uriescaped = "http%3A%2F%2Fa%2Fb%2Fc%2Fd%3Bp%3Fq",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "a",
        .hostip = "",
        .porttext = "",
        .query = "q",
        .fragment = "",
        .normalized = "http://a/b/c/d;p?q",
        .add_base_example = "http://a/b/c/d;p?q",
        .remove_base_example = "//a/b/c/d;p?q",
        .path_elements = {"b", "c", "d;p"},
        .query_elements = {{"q", ""}},
    },
    {.test = "http://a/b/c/../d;p?q",
     .uri = "http://a/b/c/../d;p?q",
     .uriescaped = "http%3A%2F%2Fa%2Fb%2Fc%2F..%2Fd%3Bp%3Fq",
     .scheme = "http",
     .userinfo = "",
     .hosttext = "a",
     .hostip = "",
     .porttext = "",
     .query = "q",
     .fragment = "",
     .normalized = "http://a/b/d;p?q",
     .add_base_example = "http://a/b/d;p?q",
     .remove_base_example = "//a/b/c/../d;p?q",
     .path_elements = {"b", "c", "..", "d;p"},
     .query_elements = {{"q", ""}}},
    {
        .test = "http://example.com/abc/def/",
        .uri = "http://example.com/abc/def/",
        .uriescaped = "http%3A%2F%2Fexample.com%2Fabc%2Fdef%2F",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "example.com",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://example.com/abc/def/",
        .add_base_example = "http://example.com/abc/def/",
        .remove_base_example = "//example.com/abc/def/",
        .path_elements =
            {
                "abc",
                "def",
            },
    },
    {.test = "http://example.com/?abc",
     .uri = "http://example.com/?abc",
     .uriescaped = "http%3A%2F%2Fexample.com%2F%3Fabc",
     .scheme = "http",
     .userinfo = "",
     .hosttext = "example.com",
     .hostip = "",
     .porttext = "",
     .query = "abc",
     .fragment = "",
     .normalized = "http://example.com/?abc",
     .add_base_example = "http://example.com/?abc",
     .remove_base_example = "//example.com/?abc",
     .query_elements = {{"abc", ""}}},
    {
        .test = "http://[vA.123456]/",
        .uri = "http://[vA.123456]/",
        .uriescaped = "http%3A%2F%2F%5BvA.123456%5D%2F",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "vA.123456",
        .hostip = "",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://[va.123456]/",
        .add_base_example = "http://[vA.123456]/",
        .remove_base_example = "//[vA.123456]/",
    },
    {
        .test = "http://8.8.8.8/",
        .uri = "http://8.8.8.8/",
        .uriescaped = "http%3A%2F%2F8.8.8.8%2F",
        .scheme = "http",
        .userinfo = "",
        .hosttext = "8.8.8.8",
        .hostip = "8.8.8.8",
        .porttext = "",
        .query = "",
        .fragment = "",
        .normalized = "http://8.8.8.8/",
        .add_base_example = "http://8.8.8.8/",
        .remove_base_example = "//8.8.8.8/",
    },
    {.test = "http://www.example.com/?abc",
     .uri = "http://www.example.com/?abc",
     .uriescaped = "http%3A%2F%2Fwww.example.com%2F%3Fabc",
     .scheme = "http",
     .userinfo = "",
     .hosttext = "www.example.com",
     .hostip = "",
     .porttext = "",
     .query = "abc",
     .fragment = "",
     .normalized = "http://www.example.com/?abc",
     .add_base_example = "http://www.example.com/?abc",
     .remove_base_example = "./?abc",
     .query_elements = {{"abc", ""}}},
    {.test = "https://google.com?q=asd&x=y&zxc=asd",
     .uri = "https://google.com?q=asd&x=y&zxc=asd",
     .uriescaped = "https%3A%2F%2Fgoogle.com%3Fq%3Dasd%26x%3Dy%26zxc%3Dasd",
     .scheme = "https",
     .userinfo = "",
     .hosttext = "google.com",
     .hostip = "",
     .porttext = "",
     .query = "q=asd&x=y&zxc=asd",
     .fragment = "",
     .normalized = "https://google.com?q=asd&x=y&zxc=asd",
     .add_base_example = "https://google.com?q=asd&x=y&zxc=asd",
     .remove_base_example = "https://google.com?q=asd&x=y&zxc=asd",
     .query_elements = {{"q", "asd"}, {"x", "y"}, {"zxc", "asd"}}},
    {.test = "https://google.com?q=asd#newplace",
     .uri = "https://google.com?q=asd#newplace",
     .uriescaped = "https%3A%2F%2Fgoogle.com%3Fq%3Dasd%23newplace",
     .scheme = "https",
     .userinfo = "",
     .hosttext = "google.com",
     .hostip = "",
     .porttext = "",
     .query = "q=asd",
     .fragment = "newplace",
     .normalized = "https://google.com?q=asd#newplace",
     .add_base_example = "https://google.com?q=asd#newplace",
     .remove_base_example = "https://google.com?q=asd#newplace",
     .query_elements = {{"q", "asd"}}},
};

class UriParserBase : public testing::Test {
 protected:
  void SetUp() override;
  void ParseUri(sapi::v::ConstCStr&, sapi::v::Struct<UriParserStateA>&,
                sapi::v::Struct<UriUriA>*);
  void GetUriString(std::string&, sapi::v::Struct<UriUriA>*);
  std::unique_ptr<UriparserSapiSandbox> sandbox_;
  std::unique_ptr<UriparserApi> api_;
};

class UriParserTestData : public UriParserBase,
                          public testing::WithParamInterface<TestVariant> {};

void UriParserBase::SetUp() {
  sandbox_ = std::make_unique<UriparserSapiSandbox>();
  ASSERT_THAT(sandbox_->Init(), IsOk());
  api_ = std::make_unique<UriparserApi>(sandbox_.get());
}

void UriParserBase::ParseUri(
    sapi::v::ConstCStr& c_uri,
    sapi::v::Struct<UriParserStateA>& state,
    sapi::v::Struct<UriUriA>* uri_) {
  SAPI_ASSERT_OK(sandbox_->Allocate(uri_, true));
  state.mutable_data()->uri = reinterpret_cast<UriUriA*>(uri_->GetRemote());
  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret, api_->uriParseUriA(state.PtrBefore(), c_uri.PtrBefore()));
  ASSERT_EQ(ret, 0);
  SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(uri_));
}

void UriParserBase::GetUriString(
    std::string& actual, sapi::v::Struct<UriUriA>* uri_) {
  sapi::v::Int size;
  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret,
      api_->uriToStringCharsRequiredA(uri_->PtrNone(), size.PtrAfter()));
  ASSERT_EQ(ret, 0);

  sapi::v::Array<char> buf(size.GetValue() + 1);

  SAPI_ASSERT_OK_AND_ASSIGN(
      ret, api_->uriToStringA(
               buf.PtrAfter(), uri_->PtrNone(), buf.GetSize(), nullptr));
  ASSERT_EQ(ret, 0);

  actual = std::string(buf.GetData());
}

TEST_P(UriParserTestData, TestUri) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  std::string actual;
  GetUriString(actual, &uri);
  ASSERT_EQ(actual, tv.uri);
}

TEST_P(UriParserTestData, TestUriEscaped) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  std::string uri_str;
  GetUriString(uri_str, &uri);

  int space = uri_str.length() * 6 + 1;

  sapi::v::Array<char> bufout(space);
  sapi::v::ConstCStr bufin(uri_str.c_str());

  SAPI_ASSERT_OK(
      api_->uriEscapeA(bufin.PtrBefore(), bufout.PtrAfter(), true, true));

  std::string actual(bufout.GetData());
  ASSERT_EQ(actual, tv.uriescaped);
}

TEST_P(UriParserTestData, TestScheme) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  UriTextRangeA part = uri.mutable_data()->scheme;

  size_t size = part.afterLast - part.first;
  SAPI_ASSERT_OK_AND_ASSIGN(
      std::string uri_str,
      sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part.first))));
  std::string actual(uri_str.substr(0, size));
  ASSERT_EQ(actual, tv.scheme);
}

TEST_P(UriParserTestData, TestUserInfo) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  UriTextRangeA* part = &uri.mutable_data()->userInfo;
  if (part != nullptr and part->first != nullptr) {
  	size_t size = part->afterLast - part->first;
  	SAPI_ASSERT_OK_AND_ASSIGN(
    	std::string uri_str,
      	sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
  	std::string actual(uri_str.substr(0, size));
    ASSERT_EQ(actual, tv.userinfo);
  } else {
    ASSERT_EQ("", tv.userinfo);
  }
}


TEST_P(UriParserTestData, TestHostText) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  UriTextRangeA* part = &uri.mutable_data()->hostText;
  if (part != nullptr and part->first != nullptr) {
  	size_t size = part->afterLast - part->first;
  	SAPI_ASSERT_OK_AND_ASSIGN(
    	std::string uri_str,
      	sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
  	std::string actual(uri_str.substr(0, size));
    ASSERT_EQ(actual, tv.hosttext);
  } else {
    ASSERT_EQ("", tv.hosttext);
  }
}

TEST_P(UriParserTestData, TestHostIP) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  char ipstr[INET6_ADDRSTRLEN] = "";

  if (uri.mutable_data()->hostData.ip4) {
    sapi::v::Struct<UriIp4> ip4;
    ip4.SetRemote(uri.mutable_data()->hostData.ip4);
    SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(&ip4));
    inet_ntop(AF_INET, ip4.mutable_data()->data, ipstr, sizeof(ipstr));
  } else if (uri.mutable_data()->hostData.ip6) {
    sapi::v::Struct<UriIp6> ip6;
    ip6.SetRemote(uri.mutable_data()->hostData.ip6);
    SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(&ip6));
    inet_ntop(AF_INET6, ip6.mutable_data()->data, ipstr, sizeof(ipstr));
  }
  ASSERT_EQ(ipstr, tv.hostip);
}

TEST_P(UriParserTestData, TestPortText) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  UriTextRangeA* part = &uri.mutable_data()->portText;
  if (part != nullptr and part->first != nullptr) {
  	size_t size = part->afterLast - part->first;
  	SAPI_ASSERT_OK_AND_ASSIGN(
    	std::string uri_str,
      	sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
  	std::string actual(uri_str.substr(0, size));
    ASSERT_EQ(actual, tv.porttext);
  } else {
    ASSERT_EQ("", tv.porttext);
  }
}

TEST_P(UriParserTestData, TestQuery) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  UriTextRangeA* part = &uri.mutable_data()->query;
  if (part != nullptr and part->first != nullptr) {
  	size_t size = part->afterLast - part->first;
  	SAPI_ASSERT_OK_AND_ASSIGN(
    	std::string uri_str,
      	sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
  	std::string actual(uri_str.substr(0, size));
    ASSERT_EQ(actual, tv.query);
  } else {
    ASSERT_EQ("", tv.query);
  }
}

TEST_P(UriParserTestData, TestFragment) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  UriTextRangeA* part = &uri.mutable_data()->fragment;
  if (part != nullptr and part->first != nullptr) {
  	size_t size = part->afterLast - part->first;
  	SAPI_ASSERT_OK_AND_ASSIGN(
    	std::string uri_str,
      	sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
  	std::string actual(uri_str.substr(0, size));
    ASSERT_EQ(actual, tv.fragment);
  } else {
    ASSERT_EQ("", tv.fragment);
  }
}

TEST_P(UriParserTestData, TestNormalize) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  SAPI_ASSERT_OK_AND_ASSIGN(
      int norm_mask, api_->uriNormalizeSyntaxMaskRequiredA(uri.PtrNone()));
  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret, api_->uriNormalizeSyntaxExA(uri.PtrAfter(), norm_mask));
  ASSERT_EQ(ret, 0);

  sapi::v::Int size;
  SAPI_ASSERT_OK_AND_ASSIGN(
      ret, api_->uriToStringCharsRequiredA(uri.PtrNone(), size.PtrAfter()));
  ASSERT_EQ(ret, 0);

  sapi::v::Array<char> buf(size.GetValue() + 1);

  SAPI_ASSERT_OK_AND_ASSIGN(
      ret,
      api_->uriToStringA(buf.PtrAfter(), uri.PtrNone(), buf.GetSize(), nullptr)
  );
  ASSERT_EQ(ret, 0);

  std::string actual(buf.GetData());
  ASSERT_EQ(actual, tv.normalized);
}

TEST_P(UriParserTestData, TestMultiple) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  // get query
  UriTextRangeA* part = &uri.mutable_data()->query;
  if (part != nullptr and part->first != nullptr) {
    size_t size = part->afterLast - part->first;
    SAPI_ASSERT_OK_AND_ASSIGN(
      std::string uri_str,
        sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
    std::string actual(uri_str.substr(0, size));
    ASSERT_EQ(actual, tv.query);
  } else {
    ASSERT_EQ("", tv.query);
  }

  // get host IP
  char ipstr[INET6_ADDRSTRLEN] = "";

  if (uri.mutable_data()->hostData.ip4) {
    sapi::v::Struct<UriIp4> ip4;
    ip4.SetRemote(uri.mutable_data()->hostData.ip4);
    SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(&ip4));
    inet_ntop(AF_INET, ip4.mutable_data()->data, ipstr, sizeof(ipstr));
  } else if (uri.mutable_data()->hostData.ip6) {
    sapi::v::Struct<UriIp6> ip6;
    ip6.SetRemote(uri.mutable_data()->hostData.ip6);
    SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(&ip6));
    inet_ntop(AF_INET6, ip6.mutable_data()->data, ipstr, sizeof(ipstr));
  }
  ASSERT_EQ(ipstr, tv.hostip);

  // normalize syntax
  SAPI_ASSERT_OK_AND_ASSIGN(
        int norm_mask, api_->uriNormalizeSyntaxMaskRequiredA(uri.PtrNone()));
  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret, api_->uriNormalizeSyntaxExA(uri.PtrAfter(), norm_mask));
  ASSERT_EQ(ret, 0);

  // GetUri
  sapi::v::Int size;
  SAPI_ASSERT_OK_AND_ASSIGN(
      ret, api_->uriToStringCharsRequiredA(uri.PtrNone(), size.PtrAfter()));
  ASSERT_EQ(ret, 0);

  sapi::v::Array<char> buf(size.GetValue() + 1);

  SAPI_ASSERT_OK_AND_ASSIGN(
      ret,
      api_->uriToStringA(buf.PtrAfter(), uri.PtrNone(), buf.GetSize(), nullptr)
  );
  ASSERT_EQ(ret, 0);

  std::string actual(buf.GetData());
  ASSERT_EQ(actual, tv.normalized);
}

TEST_P(UriParserTestData, TestAddBaseExample) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  // add base
  sapi::v::ConstCStr c_base_uri("http://www.example.com");
  sapi::v::Struct<UriUriA> base_uri;
  sapi::v::Struct<UriParserStateA> base_state;
  ParseUri(c_base_uri, base_state, &base_uri);
  absl::Cleanup base_uri_cleanup = [this, &base_uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(base_uri.PtrNone()));
  };

  sapi::v::Struct<UriUriA> newuri;
  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret,
      api_->uriAddBaseUriA(
          newuri.PtrAfter(), uri.PtrNone(), base_uri.PtrBefore()));
  ASSERT_EQ(ret, 0);
  absl::Cleanup newuri_cleanup = [this, &newuri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(newuri.PtrNone()));
  };

  // GetUri
  sapi::v::Int size;
  SAPI_ASSERT_OK_AND_ASSIGN(
      ret, api_->uriToStringCharsRequiredA(newuri.PtrNone(), size.PtrAfter()));
  ASSERT_EQ(ret, 0);

  sapi::v::Array<char> buf(size.GetValue() + 1);

  SAPI_ASSERT_OK_AND_ASSIGN(
      ret,
      api_->uriToStringA(buf.PtrAfter(), newuri.PtrNone(), buf.GetSize(), nullptr)
  );
  ASSERT_EQ(ret, 0);

  std::string actual(buf.GetData());
  ASSERT_EQ(actual, tv.add_base_example);

//  SAPI_ASSERT_OK(api_->uriFreeUriMembersA(newuri.PtrNone()));  // TODO
}

TEST_P(UriParserTestData, TestRemoveBaseExample) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  // remove base
  sapi::v::ConstCStr c_base_uri("http://www.example.com");
  sapi::v::Struct<UriUriA> base_uri;
  sapi::v::Struct<UriParserStateA> base_state;
  ParseUri(c_base_uri, base_state, &base_uri);
  absl::Cleanup base_uri_cleanup = [this, &base_uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(base_uri.PtrNone()));
  };

  sapi::v::Struct<UriUriA> newuri;
  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret,
      api_->uriRemoveBaseUriA(newuri.PtrAfter(), uri.PtrNone(),
                             base_uri.PtrBefore(), false));
  ASSERT_EQ(ret, 0);
  absl::Cleanup newuri_cleanup = [this, &newuri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(newuri.PtrNone()));
  };

  // GetUri
  sapi::v::Int size;
  SAPI_ASSERT_OK_AND_ASSIGN(
      ret, api_->uriToStringCharsRequiredA(newuri.PtrNone(), size.PtrAfter()));
  ASSERT_EQ(ret, 0);

  sapi::v::Array<char> buf(size.GetValue() + 1);

  SAPI_ASSERT_OK_AND_ASSIGN(
      ret,
      api_->uriToStringA(buf.PtrAfter(), newuri.PtrNone(), buf.GetSize(), nullptr)
  );
  ASSERT_EQ(ret, 0);

  std::string actual(buf.GetData());
  ASSERT_EQ(actual, tv.remove_base_example);

}

TEST_P(UriParserTestData, TestPath) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  // get path
  std::vector<std::string> actual;

  UriPathSegmentA* pathHead = uri.mutable_data()->pathHead;
  if (pathHead != nullptr) {
    sapi::v::Struct<UriPathSegmentA> path_segment;
    path_segment.SetRemote(pathHead);

    while (path_segment.GetRemote() != nullptr) {
      SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(&path_segment));

      UriTextRangeA* part = &path_segment.mutable_data()->text;
      if (part != nullptr and part->first != nullptr) {
        size_t size = part->afterLast - part->first;
        SAPI_ASSERT_OK_AND_ASSIGN(
          std::string uri_str,
            sandbox_->GetCString(sapi::v::RemotePtr(const_cast<char*>(part->first))));
        std::string seg = (uri_str.substr(0, size));
        if (!seg.empty()) {
          actual.push_back(seg);
        }
      }

      path_segment.SetRemote(path_segment.mutable_data()->next);
    }
  }

  ASSERT_EQ(actual.size(), tv.path_elements.size());
  for (int i = 0; i < actual.size(); ++i) {
    ASSERT_EQ(actual[i], tv.path_elements[i]);
  }
}

TEST_P(UriParserTestData, TestQueryElements) {
  const TestVariant& tv = GetParam();
  sapi::v::ConstCStr c_uri(tv.test.c_str());
  sapi::v::Struct<UriParserStateA> state;
  sapi::v::Struct<UriUriA> uri;
  ParseUri(c_uri, state, &uri);
  absl::Cleanup uri_cleanup = [this, &uri] {
    SAPI_ASSERT_OK(api_->uriFreeUriMembersA(uri.PtrNone()));
  };

  // get query elements
  //SAPI_ASSERT_OK_AND_ASSIGN(auto ret, uri.GetQueryElements());
  absl::btree_map<std::string, std::string> actual;

  if (uri.mutable_data()->query.first == nullptr) {
    return;
  }

  sapi::v::Array<void*> query_ptr(1);
  sapi::v::Int query_count;
  sapi::v::RemotePtr first(const_cast<char*>(uri.mutable_data()->query.first));
  sapi::v::RemotePtr afterLast(
      const_cast<char*>(uri.mutable_data()->query.afterLast));

  SAPI_ASSERT_OK_AND_ASSIGN(
      int ret,
      api_->uriDissectQueryMallocA(query_ptr.PtrAfter(), query_count.PtrAfter(),
                                  &first, &afterLast));
  ASSERT_EQ(ret, 0);
  absl::Cleanup query_list_cleanup = [this, &query_ptr] {
    sapi::v::RemotePtr rptr(query_ptr[0]);
    api_->uriFreeQueryListA(&rptr).IgnoreError();
  };

  sapi::v::Struct<UriQueryListA> obj;
  obj.SetRemote(query_ptr[0]);

  for (int i = 0; i < query_count.GetValue(); ++i) {
    SAPI_ASSERT_OK(sandbox_->TransferFromSandboxee(&obj));
    obj.SetRemote(nullptr);

    void* key_p = const_cast<char*>(obj.mutable_data()->key);
    void* value_p = const_cast<char*>(obj.mutable_data()->value);

    SAPI_ASSERT_OK_AND_ASSIGN(std::string key_s,
                          sandbox_->GetCString(sapi::v::RemotePtr(key_p)));
    std::string value_s;
    if (value_p != nullptr) {
      SAPI_ASSERT_OK_AND_ASSIGN(value_s,
                            sandbox_->GetCString(sapi::v::RemotePtr(value_p)));
    } else {
      value_s = "";
    }
    actual[key_s] = value_s;
    obj.SetRemote(obj.mutable_data()->next);
  }
  obj.SetRemote(nullptr);

  ASSERT_EQ(actual.size(), tv.query_elements.size());
  for (auto orig : tv.query_elements) {
    ASSERT_NE(actual.find(orig.first), actual.end());
    ASSERT_EQ(actual[orig.first], orig.second);
  }
}

INSTANTIATE_TEST_SUITE_P(UriParserBase, UriParserTestData,
                         testing::ValuesIn(TestData));

}  // namespace
