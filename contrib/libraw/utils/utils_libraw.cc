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

#include "contrib/libraw/utils/utils_libraw.h"
#include "contrib/libraw/sandboxed.h"


absl::Status LibRaw::InitLibRaw() {
  SAPI_ASSIGN_OR_RETURN(libraw_data_t * lr_data, api_.libraw_init(0));

  sapi_libraw_data_t_.SetRemote(lr_data);
  SAPI_RETURN_IF_ERROR(sandbox_->TransferFromSandboxee(&sapi_libraw_data_t_));

  return absl::OkStatus();
}

LibRaw::~LibRaw() {
  if (sapi_libraw_data_t_.GetRemote() != nullptr) {
    api_.libraw_close(sapi_libraw_data_t_.PtrNone()).IgnoreError();
  }
}

absl::Status LibRaw::CheckIsInit() { return init_status_; }

absl::Status LibRaw::OpenFile() {
  SAPI_RETURN_IF_ERROR(CheckIsInit());

  sapi::v::CStr file_name(file_name_.c_str());

  SAPI_ASSIGN_OR_RETURN(int error_code,
                        api_.libraw_open_file(sapi_libraw_data_t_.PtrAfter(),
                                              file_name.PtrBefore()));

  if (error_code != LIBRAW_SUCCESS) {
    return absl::UnavailableError(
        absl::string_view(std::to_string(error_code)));
  }

  size_ = sapi_libraw_data_t_.data().sizes.raw_height *
          sapi_libraw_data_t_.data().sizes.raw_width;

  return absl::OkStatus();
}

absl::Status LibRaw::Unpack() {
  SAPI_RETURN_IF_ERROR(CheckIsInit());

  sapi::v::CStr file_name(file_name_.c_str());

  SAPI_ASSIGN_OR_RETURN(int error_code,
                        api_.libraw_unpack(sapi_libraw_data_t_.PtrBoth()));
  if (error_code != LIBRAW_SUCCESS) {
    return absl::UnavailableError(
        absl::string_view(std::to_string(error_code)));
  }

  return absl::OkStatus();
}

//absl::Status LibRaw::COLOR_(int row, int col) {
//  SAPI_RETURN_IF_ERROR(CheckIsInit());
//
//  SAPI_ASSIGN_OR_RETURN(
//      color, api_.libraw_COLOR(sapi_libraw_data_t_.PtrBefore(), row, col));
//
//  return absl::OkStatus();
//}

absl::StatusOr<int> LibRaw::COLOR(int row, int col) {
  SAPI_RETURN_IF_ERROR(CheckIsInit());

  int color;

  SAPI_ASSIGN_OR_RETURN(
      color, api_.libraw_COLOR(sapi_libraw_data_t_.PtrBefore(), row, col));

  return color;
//  absl::Status status = COLOR_(row, col);
//  return color;
}

absl::StatusOr<std::vector<uint16_t>> LibRaw::RawData() {
  SAPI_RETURN_IF_ERROR(CheckIsInit());

  std::vector<uint16_t> buf(size_);
  sapi::v::Array<uint16_t> rawdata(buf.data(), buf.size());

  rawdata.SetRemote(sapi_libraw_data_t_.data().rawdata.raw_image);
  SAPI_RETURN_IF_ERROR(api_.GetSandbox()->TransferFromSandboxee(&rawdata));

  return buf;
}
