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

/* -*- C++ -*-
 * File: raw2text.cpp
 * Copyright 2008-2021 LibRaw LLC (info@libraw.org)
 * Created: Sun Sept 01, 2020
 *
 * LibRaw sample
 * Dumps (small) selection of RAW data to text file
 *

LibRaw is free software; you can redistribute it and/or modify
it under the terms of the one of two licenses as you choose:

1. GNU LESSER GENERAL PUBLIC LICENSE version 2.1
   (See file LICENSE.LGPL provided in LibRaw distribution archive for details).

2. COMMON DEVELOPMENT AND DISTRIBUTION LICENSE (CDDL) Version 1.0
   (See file LICENSE.CDDL provided in LibRaw distribution archive for details).


 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <fcntl.h>
#ifndef WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include "contrib/libraw/sandboxed.h"

void usage(const char *av)
{
  printf(
      "Dump (small) selecton of RAW file as tab-separated text file\n"
      "Usage: %s inputfile COL ROW [CHANNEL] [width] [height]\n"
      "  COL - start column\n"
      "  ROW - start row\n"
      "  CHANNEL - raw channel to dump, default is 0 (red for rggb)\n"
      "  width - area width to dump, default is 16\n"
      "  height - area height to dump, default is 4\n"
      , av);
}

unsigned subtract_bl(unsigned int val, int bl)
{
  return val > (unsigned)bl ? val - (unsigned)bl : 0;
}

//class LibRaw_bl : public LibRaw
//{
// public:
//  void adjust_blacklevel() { LibRaw::adjust_bl(); }
//};


class LibRaw {
 public:
  LibRaw(LibRawSapiSandbox* sandbox, const std::string& file_name)
      : sandbox_(CHECK_NOTNULL(sandbox)),
        api_(sandbox_),
        file_name_(file_name) {
    init_status_ = InitLibRaw();
  }

  absl::Status InitLibRaw() {

    SAPI_ASSIGN_OR_RETURN(libraw_data_t * lrd, api_.libraw_init(0));

    sapi_libraw_data_t_.SetRemote(lrd);
    SAPI_RETURN_IF_ERROR(sandbox_->TransferFromSandboxee(&sapi_libraw_data_t_));

    return absl::OkStatus();
  }

  absl::Status OpenFile() {
    SAPI_RETURN_IF_ERROR(CheckIsInit());

    sapi::v::CStr file_name(file_name_.c_str());

    SAPI_ASSIGN_OR_RETURN(
        int error_code,
        api_.libraw_open_file(sapi_libraw_data_t_.PtrAfter(),
                              file_name.PtrBefore()));

    if (error_code != 0) {
      return absl::UnavailableError(absl::string_view(std::to_string(error_code)));
    }

    return absl::OkStatus();
  }

  absl::Status Unpack() {
    SAPI_RETURN_IF_ERROR(CheckIsInit());

    sapi::v::CStr file_name(file_name_.c_str());

    SAPI_ASSIGN_OR_RETURN(
        int error_code,
        api_.libraw_unpack(sapi_libraw_data_t_.PtrAfter()));
    if (error_code != 0) {
      return absl::UnavailableError(absl::string_view("error"));
    }

    return absl::OkStatus();
  }

  absl::Status _COLOR(int row, int col) {
    SAPI_RETURN_IF_ERROR(CheckIsInit());

    SAPI_ASSIGN_OR_RETURN(
        color,
        api_.libraw_COLOR(sapi_libraw_data_t_.PtrBefore(), row, col));

    return absl::OkStatus();
  }

  int COLOR(int row, int col) {
    absl::Status status = _COLOR(row, col);
    return color;
  }

  absl::Status CheckIsInit() {
    return init_status_;
  }

  LibRawSapiSandbox* sandbox_;
  LibRawApi api_;
  absl::Status init_status_;

  std::string file_name_;

  sapi::v::Struct<libraw_data_t> sapi_libraw_data_t_;

  int color;
};

int main(int ac, char *av[])
{
//  google::InitGoogleLogging(av[0]);

  if (ac < 4) {
    usage(av[0]);
    exit(1);
  }
  int colstart = atoi(av[2]);
  int rowstart = atoi(av[3]);
  int channel = 0;
  if (ac > 4)
    channel = atoi(av[4]);
  int width = 16;
  if (ac > 5)
    width = atoi(av[5]);
  int height = 4;
  if (ac > 6)
    height = atoi(av[6]);
  if (width <1 || height<1) {
    usage(av[0]);
    exit(1);
  }

  sapi::v::ConstCStr file_name(av[1]);
  LibRawSapiSandbox sandbox(file_name.GetData());
  if (!sandbox.Init().ok()) {
    std::cerr << "Unable to start sandbox\n";
    return EXIT_FAILURE;
  }
  LibRaw lr(&sandbox, av[1]);

//  absl::StatusOr<libraw_data_t*> lr = api.libraw_init(0);
//  if (!lr.ok()) {
//    std::cerr << "Unable to init LibRaw\n";
//    return EXIT_FAILURE;
//  }
//  libraw_data_t* lr = status_or_lr.value();
//  LibRaw_bl lr;
//  sapi::v::RemotePtr plr(*lr);
//  sapi::v::Struct<libraw_data_t> var(&lr);

//  absl::StatusOr<char *> response = api.libraw_version();
//
//  if (response.ok()) {
//
////    printf("%i", *response);
//    char * version = response.value();
////    if (version == NULL)
//    std::cout << (void *) version;
//    std::cout << ":)";
////    std::cout << *response;
//  } else {
//    std::cout << ":(";
//  }
   absl::Status status;

   status = lr.OpenFile();
   if (!status.ok()) {
     fprintf(stderr, "Unable to open file %s\n", av[1]);
     std::cerr << status << "\n";
     return EXIT_FAILURE;
   }
//  {
//    absl::StatusOr<int> response =
//        api.libraw_open_file(&plr, file_name.PtrBefore());
//    //  absl::StatusOr<int> response = api.libraw_versionNumber();
//
//    if (!response.ok()) {
//      fprintf(stderr, "SAPI Unable to open file %s\n", av[1]);
//      exit(1);
//    }
//
//    if (*response != 0) {
//      fprintf(stderr, "LibRaw Unable to open file %s\n", av[1]);
//      exit(1);
//    }
//  }

//  sapi::v::Struct<libraw_data_t> lr_struct;

  if ((lr.sapi_libraw_data_t_.data().idata.colors == 1 && channel>0) || (channel >3))
  {
    fprintf(stderr, "Incorrect CHANNEL specified: %d\n", channel);
    exit(1);
  }
  lr.Unpack();
//  {
//    absl::StatusOr<int> response = api.libraw_unpack(&plr);
//    if (!response.ok() or *response != 0) {
//      fprintf(stderr, "Unable to unpack raw data from %s\n", av[1]);
//      exit(1);
//    }
//  }
//  lr.adjust_blacklevel();
  printf("%s\t%d-%d-%dx%d\tchannel: %d\n", av[1], colstart, rowstart, width, height, channel);

  printf("%6s", "R\\C");
  for (int col = colstart; col < colstart + width && col < lr.sapi_libraw_data_t_.data().sizes.raw_width; col++)
    printf("%6u", col);
  printf("\n");

  if (lr.sapi_libraw_data_t_.data().rawdata.raw_image) {
    std::cout << "A";
    for (int row = rowstart; row < rowstart + height && row < lr.sapi_libraw_data_t_.data().sizes.raw_height; row++)
    {
      unsigned rcolors[48];
      if (lr.sapi_libraw_data_t_.data().idata.colors > 1)
        for (int c = 0; c < 48; c++)
          rcolors[c] = lr.COLOR(row, c);
      else
        memset(rcolors, 0, sizeof(rcolors));
      unsigned short *rowdata = &lr.sapi_libraw_data_t_.data().rawdata.raw_image[row * lr.sapi_libraw_data_t_.data().sizes.raw_pitch / 2];
      printf("%6u", row);
      for (int col = colstart; col < colstart + width && col < lr.sapi_libraw_data_t_.data().sizes.raw_width; col++)
        if (rcolors[col % 48] == (unsigned)channel) printf("%6u", subtract_bl(rowdata[col],lr.sapi_libraw_data_t_.data().color.cblack[channel]));
        else printf("     -");
      printf("\n");
    }
  }
  else if (lr.sapi_libraw_data_t_.data().rawdata.color4_image && channel < 4) {
    std::cout << "B";
    for (int row = rowstart; row < rowstart + height && row < lr.sapi_libraw_data_t_.data().sizes.raw_height; row++) {
      unsigned short(*rowdata)[4] = &lr.sapi_libraw_data_t_.data().rawdata.color4_image[row * lr.sapi_libraw_data_t_.data().sizes.raw_pitch / 8];
      printf("%6u", row);
      for (int col = colstart; col < colstart + width && col < lr.sapi_libraw_data_t_.data().sizes.raw_width; col++)
        printf("%6u", subtract_bl(rowdata[col][channel],lr.sapi_libraw_data_t_.data().color.cblack[channel]));
      printf("\n");
    }
  }
  else if (lr.sapi_libraw_data_t_.data().rawdata.color3_image && channel < 3) {
    std::cout << "C";
    for (int row = rowstart; row < rowstart + height && row < lr.sapi_libraw_data_t_.data().sizes.raw_height; row++) {
      unsigned short(*rowdata)[3] = &lr.sapi_libraw_data_t_.data().rawdata.color3_image[row * lr.sapi_libraw_data_t_.data().sizes.raw_pitch / 6];
      printf("%6u", row);
      for (int col = colstart; col < colstart + width && col < lr.sapi_libraw_data_t_.data().sizes.raw_width; col++)
        printf("%6u", subtract_bl(rowdata[col][channel],lr.sapi_libraw_data_t_.data().color.cblack[channel]));
      printf("\n");
    }
  }
  else
    printf("Unsupported file data (e.g. floating point format), or incorrect channel specified\n");
//  api.libraw_close(&plr).IgnoreError();
  return EXIT_SUCCESS;
}