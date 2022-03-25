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

set(SAPI_ROOT "" CACHE PATH "Path to the Sandboxed API source tree")

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

#  "${libraw_SOURCE_DIR}/internal/libraw_internal_funcs.h"
#  "${libraw_SOURCE_DIR}/internal/dcraw_defs.h"
#  "${libraw_SOURCE_DIR}/internal/dcraw_fileio_defs.h"
#  "${libraw_SOURCE_DIR}/internal/defines.h"
#  "${libraw_SOURCE_DIR}/internal/dmp_include.h"
#  "${libraw_SOURCE_DIR}/internal/libraw_cameraids.h"
#  "${libraw_SOURCE_DIR}/internal/libraw_cxx_defs.h"
#  "${libraw_SOURCE_DIR}/internal/var_defines.h"
#  "${libraw_SOURCE_DIR}/internal/x3f_tools.h"
  "${libraw_SOURCE_DIR}/libraw/libraw.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_alloc.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_const.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_datastream.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_internal.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_types.h"
  "${libraw_SOURCE_DIR}/libraw/libraw_version.h"
  "${libraw_SOURCE_DIR}/src/metadata/pentax.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/samsung.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/makernotes.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/p1.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/kodak.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/misc_parsers.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/hasselblad_model.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/leica.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/identify_tools.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/fuji.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/epson.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/minolta.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/mediumformat.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/adobepano.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/canon.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/nikon.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/olympus.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/tiff.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/normalize_model.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/exif_gps.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/cr3_parser.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/ciff.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/identify.cpp"
  "${libraw_SOURCE_DIR}/src/metadata/sony.cpp"
  "${libraw_SOURCE_DIR}/src/x3f/x3f_parse_process.cpp"
  "${libraw_SOURCE_DIR}/src/x3f/x3f_utils_patched.cpp"
  "${libraw_SOURCE_DIR}/src/write/apply_profile.cpp"
  "${libraw_SOURCE_DIR}/src/write/file_write.cpp"
#  "${libraw_SOURCE_DIR}/src/write/write_ph.cpp"
  "${libraw_SOURCE_DIR}/src/write/tiff_writer.cpp"
  "${libraw_SOURCE_DIR}/src/preprocessing/ext_preprocess.cpp"
#  "${libraw_SOURCE_DIR}/src/preprocessing/preprocessing_ph.cpp"
  "${libraw_SOURCE_DIR}/src/preprocessing/raw2image.cpp"
  "${libraw_SOURCE_DIR}/src/preprocessing/subtract_black.cpp"
  "${libraw_SOURCE_DIR}/src/demosaic/aahd_demosaic.cpp"
  "${libraw_SOURCE_DIR}/src/demosaic/xtrans_demosaic.cpp"
  "${libraw_SOURCE_DIR}/src/demosaic/misc_demosaic.cpp"
  "${libraw_SOURCE_DIR}/src/demosaic/dcb_demosaic.cpp"
  "${libraw_SOURCE_DIR}/src/demosaic/ahd_demosaic.cpp"
  "${libraw_SOURCE_DIR}/src/demosaic/dht_demosaic.cpp"
  "${libraw_SOURCE_DIR}/src/integration/dngsdk_glue.cpp"
  "${libraw_SOURCE_DIR}/src/integration/rawspeed_glue.cpp"
  "${libraw_SOURCE_DIR}/src/utils/curves.cpp"
  "${libraw_SOURCE_DIR}/src/utils/utils_dcraw.cpp"
  "${libraw_SOURCE_DIR}/src/utils/init_close_utils.cpp"
  "${libraw_SOURCE_DIR}/src/utils/read_utils.cpp"
  "${libraw_SOURCE_DIR}/src/utils/decoder_info.cpp"
  "${libraw_SOURCE_DIR}/src/utils/open.cpp"
  "${libraw_SOURCE_DIR}/src/utils/phaseone_processing.cpp"
  "${libraw_SOURCE_DIR}/src/utils/thumb_utils.cpp"
  "${libraw_SOURCE_DIR}/src/utils/utils_libraw.cpp"
  "${libraw_SOURCE_DIR}/src/postprocessing/dcraw_process.cpp"
#  "${libraw_SOURCE_DIR}/src/postprocessing/postprocessing_ph.cpp"
  "${libraw_SOURCE_DIR}/src/postprocessing/postprocessing_utils.cpp"
  "${libraw_SOURCE_DIR}/src/postprocessing/mem_image.cpp"
  "${libraw_SOURCE_DIR}/src/postprocessing/aspect_ratio.cpp"
  "${libraw_SOURCE_DIR}/src/postprocessing/postprocessing_utils_dcrdefs.cpp"
  "${libraw_SOURCE_DIR}/src/postprocessing/postprocessing_aux.cpp"
  "${libraw_SOURCE_DIR}/src/tables/cameralist.cpp"
  "${libraw_SOURCE_DIR}/src/tables/colordata.cpp"
  "${libraw_SOURCE_DIR}/src/tables/colorconst.cpp"
  "${libraw_SOURCE_DIR}/src/tables/wblists.cpp"
  "${libraw_SOURCE_DIR}/src/libraw_c_api.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/smal.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/decoders_dcraw.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/generic.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/fuji_compressed.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/decoders_libraw.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/unpack_thumb.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/kodak_decoders.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/dng.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/canon_600.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/decoders_libraw_dcrdefs.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/crx.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/fp_dng.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/unpack.cpp"
  "${libraw_SOURCE_DIR}/src/decoders/load_mfbacks.cpp"
  "${libraw_SOURCE_DIR}/src/libraw_datastream.cpp"
)

target_include_directories(
  libraw PUBLIC

  "${libraw_SOURCE_DIR}"
)

add_sapi_library(
  sapi_libraw

  FUNCTIONS
    libraw_strprogress
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
    libraw_processed_image_t
    libraw_dcraw_make_mem_image
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

target_include_directories(
  sapi_libraw INTERFACE

  "${PROJECT_BINARY_DIR}"
)

if(SAPI_ENABLE_EXAMPLES)
    add_subdirectory(example)
endif()

#if(SAPI_ENABLE_TESTS)
#    add_subdirectory(test)
#endif()
