/*
 * HEIF codec.
 * Copyright (c) 2017-2025 Dirk Farin <dirk.farin@gmail.com>
 *
 * This file is part of libheif.
 *
 * libheif is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libheif is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libheif.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBHEIF_HEIF_IMAGE_HANDLE_H
#define LIBHEIF_HEIF_IMAGE_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <libheif/heif_library.h>
#include <libheif/heif_image.h>


// ========================= heif_image_handle =========================

// An heif_image_handle is a handle to a logical image in the HEIF file.
// To get the actual pixel data, you have to decode the handle to an heif_image.
// An heif_image_handle also gives you access to the thumbnails and Exif data
// associated with an image.

// Once you obtained an heif_image_handle, you can already release the heif_context,
// since it is internally ref-counted.

// Release image handle.
LIBHEIF_API
void heif_image_handle_release(const heif_image_handle*);

// Check whether the given image_handle is the primary image of the file.
LIBHEIF_API
int heif_image_handle_is_primary_image(const heif_image_handle* handle);

LIBHEIF_API
heif_item_id heif_image_handle_get_item_id(const heif_image_handle* handle);

/** Get the image width.
 *
 * If 'handle' is invalid (NULL) or if the image size exceeds the range of `int`, 0 is returned.
 */
LIBHEIF_API
int heif_image_handle_get_width(const heif_image_handle* handle);

/** Get the image height.
 *
 * If 'handle' is invalid (NULL) or if the image size exceeds the range of `int`, 0 is returned.
 */
LIBHEIF_API
int heif_image_handle_get_height(const heif_image_handle* handle);

LIBHEIF_API
int heif_image_handle_has_alpha_channel(const heif_image_handle*);

LIBHEIF_API
int heif_image_handle_is_premultiplied_alpha(const heif_image_handle*);

// Returns -1 on error, e.g. if this information is not present in the image.
// Only defined for images coded in the YCbCr or monochrome colorspace.
LIBHEIF_API
int heif_image_handle_get_luma_bits_per_pixel(const heif_image_handle*);

// Returns -1 on error, e.g. if this information is not present in the image.
// Only defined for images coded in the YCbCr colorspace.
LIBHEIF_API
int heif_image_handle_get_chroma_bits_per_pixel(const heif_image_handle*);

// Return the colorspace that libheif proposes to use for decoding.
// Usually, these will be either YCbCr or Monochrome, but it may also propose RGB for images
// encoded with matrix_coefficients=0 or for images coded natively in RGB.
// It may also return *_undefined if the file misses relevant information to determine this without decoding.
// These are only proposed values that avoid colorspace conversions as much as possible.
// You can still request the output in your preferred colorspace, but this may involve an internal conversion.
//
// Note on matrix_coefficients=0 (H.273 identity, i.e. RGB carried in YCbCr planes):
// for such images this function proposes RGB. However, heif_decode_image() called with
// heif_colorspace_undefined currently returns the image still tagged as YCbCr (with
// matrix_coefficients=0). That tagging is self-consistent. Converting it to RGB applies
// the identity mapping and yields correct colors, so request heif_colorspace_RGB
// explicitly if you need RGB output. This exact behavior is not specified and may change.
// A later version may return RGB directly when decoding with an undefined colorspace.
// See heif_decode_image().
LIBHEIF_API
heif_error heif_image_handle_get_preferred_decoding_colorspace(const heif_image_handle* image_handle,
                                                               heif_colorspace* out_colorspace,
                                                               heif_chroma* out_chroma);

// Get the image width from the 'ispe' box. This is the original image size without
// any transformations applied to it. Do not use this unless you know exactly what
// you are doing.
LIBHEIF_API
int heif_image_handle_get_ispe_width(const heif_image_handle* handle);

LIBHEIF_API
int heif_image_handle_get_ispe_height(const heif_image_handle* handle);

// Returns whether the image has 'pixel aspect ratio information' information. If 0 is returned, the output is filled with the 1:1 default.
LIBHEIF_API
int heif_image_handle_get_pixel_aspect_ratio(const heif_image_handle*, uint32_t* aspect_h, uint32_t* aspect_v);


// This gets the context associated with the image handle.
// Note that you have to release the returned context with heif_context_free() in any case.
//
// This means: when you have several image-handles that originate from the same file and you get the
// context of each of them, the returned pointer may be different even though it refers to the same
// logical context. You have to call heif_context_free() on all those context pointers.
// After you freed a context pointer, you can still use the context through a different pointer that you
// might have acquired from elsewhere.
LIBHEIF_API
heif_context* heif_image_handle_get_context(const heif_image_handle* handle);

LIBHEIF_API
const char* heif_image_handle_get_gimi_content_id(const heif_image_handle* handle);

LIBHEIF_API
void heif_image_handle_set_gimi_content_id(heif_image_handle* handle, const char* content_id);


// --- cmpd component queries

// Returns the number of components in the cmpd box, or 0 if no cmpd property exists.
LIBHEIF_API
uint32_t heif_image_handle_get_number_of_cmpd_components(const heif_image_handle*);

// Returns the component_type for the given cmpd component index.
// Returns 0 if out of range or no cmpd property.
LIBHEIF_API
uint16_t heif_image_handle_get_cmpd_component_type(const heif_image_handle*, uint32_t component_idx);

// Returns the component_type_uri for the given cmpd component index (component_type >= 0x8000).
// Returns NULL if the component does not have a URI.
// The returned string must be freed with heif_string_release().
LIBHEIF_API
const char* heif_image_handle_get_cmpd_component_type_uri(const heif_image_handle*, uint32_t component_idx);


// --- GIMI component content IDs (handle-level)

// Returns non-zero (count of content IDs) if an ItemComponentContentIDProperty is set, 0 otherwise.
LIBHEIF_API
int heif_image_handle_has_gimi_component_content_ids(const heif_image_handle*);

// Returns the GIMI component content ID for the given component index.
// Returns NULL if no ItemComponentContentIDProperty is set or index is out of range.
// The returned string must be freed with heif_string_release().
LIBHEIF_API
const char* heif_image_handle_get_gimi_component_content_id(const heif_image_handle*, uint32_t component_idx);

// Set a GIMI component content ID for a single component.
// If an ItemComponentContentIDProperty does not yet exist, one will be created.
// The content IDs array is resized as needed (new entries default to empty).
LIBHEIF_API
void heif_image_handle_set_gimi_component_content_id(heif_image_handle*,
                                                     uint32_t component_idx,
                                                     const char* content_id);

#if WITH_EXPERIMENTAL_GAIN_MAP
#define HAS_GAIN_MAPS

// ------------------------- gain map images -------------------------

// Get the gain map image associated with the main image. If no gain map image is available, this
// method will return error.
LIBHEIF_API
struct heif_error heif_image_handle_get_gain_map_image_handle(
    const struct heif_image_handle* handle, struct heif_image_handle** gain_map_handle);

// Get the gain map metadata size associated with the main image. If no gain map image is available,
// this method will return 0
LIBHEIF_API
struct heif_error heif_image_handle_get_gain_map_metadata_item_id(const struct heif_image_handle* handle, heif_item_id* item_id);

// Get the gain map metadata size associated with the main image. If no gain map image is available,
// this method will return 0
LIBHEIF_API
size_t heif_image_handle_get_gain_map_metadata_size(const struct heif_image_handle* handle);

// Get the gain map metadata associated with the main image. if no gain map image is available, this
// method will return error
LIBHEIF_API
struct heif_error heif_image_handle_get_gain_map_metadata(const struct heif_image_handle* handle,
                                                          void* out_data);

// Compress the gain map image and write metadata.
// Returns a handle to the coded image in 'out_image_handle' unless out_image_handle = NULL.
LIBHEIF_API
struct heif_error heif_context_encode_gain_map_image(
    struct heif_context* ctx, const struct heif_image_handle* base_image_handle,
    struct heif_encoder* encoder, const struct heif_image* gain_map_image,
    const struct heif_encoding_options* input_options, const uint8_t* gain_map_metadata,
    int gain_map_metadata_len, const struct heif_color_profile_nclx* derived_image_nclx,
    struct heif_image_handle** out_image_handle);

#endif  // WITH_EXPERIMENTAL_GAIN_MAP

#ifdef __cplusplus
}
#endif

#endif
