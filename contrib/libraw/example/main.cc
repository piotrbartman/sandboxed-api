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


#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#ifndef WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include "contrib/libraw/sandboxed.h"
#include "contrib/libraw/utils/utils_libraw.h"

void usage(const char* av) {
  printf(
      "Dump (small) selecton of RAW file as tab-separated text file\n"
      "Usage: %s inputfile COL ROW [CHANNEL] [width] [height]\n"
      "  COL - start column\n"
      "  ROW - start row\n"
      "  CHANNEL - raw channel to dump, default is 0 (red for rggb)\n"
      "  width - area width to dump, default is 16\n"
      "  height - area height to dump, default is 4\n",
      av);
}

unsigned subtract_bl(unsigned int val, int bl) {
  return val > (unsigned)bl ? val - (unsigned)bl : 0;
}

int main(int ac, char* av[]) {
  google::InitGoogleLogging(av[0]);

  if (ac < 4) {
    usage(av[0]);
    exit(1);
  }
  int colstart = atoi(av[2]);
  int rowstart = atoi(av[3]);
  int channel = 0;
  if (ac > 4) channel = atoi(av[4]);
  int width = 16;
  if (ac > 5) width = atoi(av[5]);
  int height = 4;
  if (ac > 6) height = atoi(av[6]);
  if (width < 1 || height < 1) {
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

  absl::Status status;

  status = lr.OpenFile();
  if (!status.ok()) {
    fprintf(stderr, "Unable to open file %s\n", av[1]);
    std::cerr << status << "\n";
    return EXIT_FAILURE;
  }

  if ((lr.sapi_libraw_data_t_.data().idata.colors == 1 && channel > 0) ||
      (channel > 3)) {
    fprintf(stderr, "Incorrect CHANNEL specified: %d\n", channel);
    exit(1);
  }
  lr.Unpack();

  printf("%s\t%d-%d-%dx%d\tchannel: %d\n", av[1], colstart, rowstart, width,
         height, channel);

  printf("%6s", "R\\C");
  for (int col = colstart; col < colstart + width &&
                           col < lr.sapi_libraw_data_t_.data().sizes.raw_width;
       col++)
    printf("%6u", col);
  printf("\n");

  absl::StatusOr<std::vector<uint16_t>> rawdata = lr.RawData();
  if (not rawdata.ok()) {
    std::cerr << "Unable to get raw data\n";
    return EXIT_FAILURE;
  }

  if (lr.sapi_libraw_data_t_.data().rawdata.raw_image) {
    for (int row = rowstart;
         row < rowstart + height &&
         row < lr.sapi_libraw_data_t_.data().sizes.raw_height;
         row++)
    {
      unsigned rcolors[48];
      if (lr.sapi_libraw_data_t_.data().idata.colors > 1)
      {
        absl::StatusOr<uint16_t> color;
        for (int c = 0; c < 48; c++)
        {
          color = lr.COLOR(row, c);
          if (color.ok())
            rcolors[c] = *color;
        }
      }
      else
      {
        memset(rcolors, 0, sizeof(rcolors));
      }
      printf("%6u", row);
      for (int col = colstart;
           col < colstart + width &&
           col < lr.sapi_libraw_data_t_.data().sizes.raw_width;
           col++)
      {
        int idx = row * lr.sapi_libraw_data_t_.data().sizes.raw_pitch / 2 + col;
        if (rcolors[col % 48] == (unsigned)channel)
          printf("%6u",
                 subtract_bl((*rawdata)[idx],
                 lr.sapi_libraw_data_t_.data().color.cblack[channel]));
        else
          printf("     -");
      }
      printf("\n");
    }
  } else
    printf(
        "Unsupported file data (e.g. floating point format), or incorrect "
        "channel specified\n");
  return EXIT_SUCCESS;
}