# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13..3.22)

project(sapi_libraw CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT TARGET sapi::sapi)
  set(SAPI_ROOT "../.." CACHE PATH "Path to the Sandboxed API source tree")
  add_subdirectory(
    "${SAPI_ROOT}"
    "${CMAKE_BINARY_DIR}/sandboxed-api-build"
    EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  libraw

  GIT_REPOSITORY https://github.com/LibRaw/LibRaw.git
  GIT_TAG adcb898a00746c8aa886eb06cc9f5a1cb1834fca
)

FetchContent_GetProperties(libraw)
if(NOT libraw_POPULATED)
  FetchContent_Populate(libraw)
  set(libraw_STATUS_FILE "${libraw_SOURCE_DIR}/config.status")
  if(EXISTS "${libraw_STATUS_FILE}")
    file(SHA256 "${libraw_STATUS_FILE}" _sapi_CONFIG_STATUS)
  endif()
  if(NOT _sapi_CONFIG_STATUS STREQUAL "${libraw_CONFIG_STATUS}")
    message("-- Configuring libraw...")
    execute_process(
            COMMAND autoreconf -i
            WORKING_DIRECTORY "${libraw_SOURCE_DIR}"
            RESULT_VARIABLE _sapi_libraw_autoreconf_result
    )
    if(NOT _sapi_libraw_autoreconf_result EQUAL "0")
      message(FATAL_ERROR "Configuration for libraw failed: "
              "${_sapi_libraw_autoreconf_result}")
    endif()
    execute_process(
            COMMAND ./configure --disable-dependency-tracking
            --quiet
            WORKING_DIRECTORY "${libraw_SOURCE_DIR}"
            RESULT_VARIABLE _sapi_libraw_config_result
    )
    if(NOT _sapi_libraw_config_result EQUAL "0")
      message(FATAL_ERROR "Configuration for libraw failed: "
              "${_sapi_libraw_config_result}")
    endif()
    file(SHA256 "${libraw_SOURCE_DIR}/config.status" _sapi_CONFIG_STATUS)
    set(libraw_CONFIG_STATUS "${_sapi_CONFIG_STATUS}" CACHE INTERNAL "")
  endif()
endif()

add_library(
  libraw STATIC

  "${libraw_SOURCE_DIR}/internal/libraw_internal_funcs.h"
  "${libraw_SOURCE_DIR}/internal/dcraw_defs.h"
  "${libraw_SOURCE_DIR}/internal/dcraw_fileio_defs.h"
  "${libraw_SOURCE_DIR}/internal/defines.h"
  "${libraw_SOURCE_DIR}/internal/dmp_include.h"
  "${libraw_SOURCE_DIR}/internal/libraw_cameraids.h"
  "${libraw_SOURCE_DIR}/internal/libraw_cxx_defs.h"
  "${libraw_SOURCE_DIR}/internal/var_defines.h"
  "${libraw_SOURCE_DIR}/internal/x3f_tools.h"
  "${libraw_SOURCE_DIR}/libraw/libraw.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_alloc.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_const.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_datastream.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_internal.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_types.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_version.h"
  "${libraw_SOURCE_DIR}/src/libraw_datastream.cpp"
)

target_include_directories(
  libraw PUBLIC

  "${libraw_SOURCE_DIR}"
)

add_sapi_library(
  sapi_libraw

  FUNCTIONS
    libraw_init
    libraw_init
    libraw_open_file
    libraw_open_file_ex
    libraw_open_wfile
    libraw_open_wfile_ex
    libraw_open_buffer
    libraw_unpack
    libraw_unpack_thumb
    libraw_recycle_datastream
    libraw_recycle
    libraw_close
    libraw_subtract_black
    libraw_raw2image
    libraw_free_image
    libraw_version
    libraw_versionNumber
    libraw_cameraList
    libraw_cameraCount
    libraw_set_memerror_handler
    libraw_set_exifparser_handler
    exif_parser_callback cb,
    libraw_set_dataerror_handler
    libraw_set_progress_handler
    libraw_unpack_function_name
    libraw_get_decoder_info
    libraw_COLOR
    libraw_capabilities
    libraw_adjust_sizes_info_only
    libraw_dcraw_ppm_tiff_writer
    libraw_dcraw_thumb_writer
    libraw_dcraw_process
    libraw_processed_image_t *
    libraw_dcraw_make_mem_image
    libraw_processed_image_t *
    libraw_dcraw_make_mem_thumb
    libraw_dcraw_clear_mem
    libraw_set_demosaic
    libraw_set_output_color
    libraw_set_user_mul
    libraw_set_output_bps
    libraw_set_gamma
    libraw_set_no_auto_bright
    libraw_set_bright
    libraw_set_highlight
    libraw_set_fbdd_noiserd
    libraw_get_raw_height
    libraw_get_raw_width
    libraw_get_iheight
    libraw_get_iwidth
    libraw_get_cam_mul
    libraw_get_pre_mul
    libraw_get_rgb_cam
    libraw_get_color_maximum
    libraw_set_output_tif
    libraw_get_iparams
    libraw_get_lensinfo
    libraw_get_imgother

  INPUTS
    "${libraw_SOURCE_DIR}/libraw/libraw.h"

  LIBRARY libraw
  LIBRARY_NAME LibRaw
  NAMESPACE ""
)

add_library(sapi_contrib::libraw ALIAS sapi_libraw)

target_include_directories(
  sapi_libraw INTERFACE

  "${PROJECT_BINARY_DIR}"
  "${SAPI_SOURCE_DIR}"
)

#if(SAPI_ENABLE_EXAMPLES)
#    add_subdirectory(example)
#endif()

#if(SAPI_ENABLE_TESTS)
#    add_subdirectory(test)
#endif()
