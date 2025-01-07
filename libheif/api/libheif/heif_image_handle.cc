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

#include "heif_image_handle.h"
#include "api_structs.h"
#include "box.h"
#if WITH_UNCOMPRESSED_CODEC
#include "codecs/uncompressed/unc_boxes.h"
#endif
#include <climits>
#include <cstring>
#include <string>
#include <memory>
#include <vector>


void heif_image_handle_release(const heif_image_handle* handle)
{
  delete handle;
}


int heif_image_handle_is_primary_image(const heif_image_handle* handle)
{
  return handle->image->is_primary();
}


heif_item_id heif_image_handle_get_item_id(const heif_image_handle* handle)
{
  return handle->image->get_id();
}


int heif_image_handle_get_width(const heif_image_handle* handle)
{
  if (handle && handle->image) {
    uint32_t w = handle->image->get_width();
    if (w > INT_MAX) {
      return 0;
    }
    else {
      return static_cast<int>(w);
    }
  }
  else {
    return 0;
  }
}


int heif_image_handle_get_height(const heif_image_handle* handle)
{
  if (handle && handle->image) {
    uint32_t h = handle->image->get_height();
    if (h > INT_MAX) {
      return 0;
    }
    else {
      return static_cast<int>(h);
    }
  }
  else {
    return 0;
  }
}


int heif_image_handle_has_alpha_channel(const heif_image_handle* handle)
{
  // TODO: for now, also scan the grid tiles for alpha information (issue #708), but depending about
  // how the discussion about this structure goes forward, we might remove this again.

  return handle->context->has_alpha(handle->image->get_id()); // handle case in issue #708
  //return handle->image->get_alpha_channel() != nullptr;       // old alpha check that fails on alpha in grid tiles
}


int heif_image_handle_is_premultiplied_alpha(const heif_image_handle* handle)
{
  // TODO: what about images that have the alpha in the grid tiles (issue #708) ?
  return handle->image->is_premultiplied_alpha();
}


int heif_image_handle_get_luma_bits_per_pixel(const heif_image_handle* handle)
{
  return handle->image->get_luma_bits_per_pixel();
}


int heif_image_handle_get_chroma_bits_per_pixel(const heif_image_handle* handle)
{
  return handle->image->get_chroma_bits_per_pixel();
}


heif_error heif_image_handle_get_preferred_decoding_colorspace(const heif_image_handle* image_handle,
                                                               heif_colorspace* out_colorspace,
                                                               heif_chroma* out_chroma)
{
  Error err = image_handle->image->get_coded_image_colorspace(out_colorspace, out_chroma);
  if (err) {
    return err.error_struct(image_handle->image.get());
  }

  return heif_error_success;
}


int heif_image_handle_get_ispe_width(const heif_image_handle* handle)
{
  if (handle && handle->image) {
    return handle->image->get_ispe_width();
  }
  else {
    return 0;
  }
}


int heif_image_handle_get_ispe_height(const heif_image_handle* handle)
{
  if (handle && handle->image) {
    return handle->image->get_ispe_height();
  }
  else {
    return 0;
  }
}


int heif_image_handle_get_pixel_aspect_ratio(const heif_image_handle* handle, uint32_t* aspect_h, uint32_t* aspect_v)
{
  auto pasp = handle->image->get_property<Box_pasp>();
  if (pasp) {
    *aspect_h = pasp->hSpacing;
    *aspect_v = pasp->vSpacing;
    return 1;
  }
  else {
    *aspect_h = 1;
    *aspect_v = 1;
    return 0;
  }
}


heif_context* heif_image_handle_get_context(const heif_image_handle* handle)
{
  auto ctx = new heif_context();
  ctx->context = handle->context;
  return ctx;
}


const char* heif_image_handle_get_gimi_content_id(const heif_image_handle* handle)
{
  if (!handle->image->has_gimi_sample_content_id()) {
    return nullptr;
  }

  std::string id = handle->image->get_gimi_sample_content_id();
  char* idstring = new char[id.size() + 1];
  strcpy(idstring, id.c_str());
  return idstring;
}


void heif_image_handle_set_gimi_content_id(heif_image_handle* handle, const char* content_id)
{
  auto gimi_box = std::make_shared<Box_gimi_content_id>();
  gimi_box->set_content_id(content_id);
  handle->context->add_property(handle->image->get_id(), gimi_box, false);
  handle->image->set_gimi_sample_content_id(content_id);
}


#if WITH_UNCOMPRESSED_CODEC
static std::shared_ptr<Box_cmpd> get_effective_cmpd(const heif_image_handle* handle)
{
  // Try explicit cmpd property first.
  auto cmpd = handle->image->get_property<Box_cmpd>();
  if (cmpd) {
    return cmpd;
  }

  // For profile-based uncC (version 1), the cmpd is synthesized inside the uncC box.
  auto uncC = handle->image->get_property<Box_uncC>();
  if (uncC) {
    fill_uncC_and_cmpd_from_profile(uncC, cmpd);
  }
  return cmpd;
}

#endif


uint32_t heif_image_handle_get_number_of_cmpd_components(const heif_image_handle* handle)
{
#if WITH_UNCOMPRESSED_CODEC
  if (!handle) {
    return 0;
  }
  auto cmpd = get_effective_cmpd(handle);
  if (!cmpd) {
    return 0;
  }
  return static_cast<uint32_t>(cmpd->get_components().size());
#else
  return 0;
#endif
}


uint16_t heif_image_handle_get_cmpd_component_type(const heif_image_handle* handle, uint32_t component_idx)
{
#if WITH_UNCOMPRESSED_CODEC
  if (!handle) {
    return 0;
  }
  auto cmpd = get_effective_cmpd(handle);
  if (!cmpd) {
    return 0;
  }
  const auto& components = cmpd->get_components();
  if (component_idx >= components.size()) {
    return 0;
  }
  return components[component_idx].component_type;
#else
  return 0;
#endif
}


const char* heif_image_handle_get_cmpd_component_type_uri(const heif_image_handle* handle, uint32_t component_idx)
{
#if WITH_UNCOMPRESSED_CODEC
  if (!handle) {
    return nullptr;
  }
  auto cmpd = get_effective_cmpd(handle);
  if (!cmpd) {
    return nullptr;
  }
  const auto& components = cmpd->get_components();
  if (component_idx >= components.size()) {
    return nullptr;
  }
  const auto& uri = components[component_idx].component_type_uri;
  if (uri.empty()) {
    return nullptr;
  }
  char* uristring = new char[uri.size() + 1];
  strcpy(uristring, uri.c_str());
  return uristring;
#else
  return nullptr;
#endif
}


// heif_image_handle_get_number_of_components, _get_used_component_ids,
// _get_component_type, _get_component_bits_per_pixel and _get_component_datatype
// live in heif_components.cc.


int heif_image_handle_has_gimi_component_content_ids(const heif_image_handle* handle)
{
#if WITH_UNCOMPRESSED_CODEC
  if (!handle) {
    return 0;
  }
  auto box = handle->image->get_property<Box_gimi_component_content_ids>();
  if (!box) {
    return 0;
  }
  return static_cast<int>(box->get_content_ids().size());
#else
  return 0;
#endif
}


const char* heif_image_handle_get_gimi_component_content_id(const heif_image_handle* handle, uint32_t component_idx)
{
#if WITH_UNCOMPRESSED_CODEC
  if (!handle) {
    return nullptr;
  }
  auto box = handle->image->get_property<Box_gimi_component_content_ids>();
  if (!box) {
    return nullptr;
  }
  const auto& ids = box->get_content_ids();
  if (component_idx >= ids.size()) {
    return nullptr;
  }
  char* idstring = new char[ids[component_idx].size() + 1];
  strcpy(idstring, ids[component_idx].c_str());
  return idstring;
#else
  return nullptr;
#endif
}


void heif_image_handle_set_gimi_component_content_id(heif_image_handle* handle,
                                                     uint32_t component_idx,
                                                     const char* content_id)
{
#if WITH_UNCOMPRESSED_CODEC
  if (!handle || !content_id) {
    return;
  }

  auto box = handle->image->get_property<Box_gimi_component_content_ids>();
  if (!box) {
    // Create a new box and add it as property.
    auto new_box = std::make_shared<Box_gimi_component_content_ids>();
    std::vector<std::string> ids(component_idx + 1);
    ids[component_idx] = content_id;
    new_box->set_content_ids(ids);
    handle->context->add_property(handle->image->get_id(), new_box, false);
  }
  else {
    // Mutate the existing box in-place.
    auto ids = box->get_content_ids();
    if (component_idx >= ids.size()) {
      ids.resize(component_idx + 1);
    }
    ids[component_idx] = content_id;
    box->set_content_ids(ids);
  }
#endif
}

#if WITH_EXPERIMENTAL_GAIN_MAP

heif_error heif_image_handle_get_gain_map_metadata_item_id(
  const struct heif_image_handle* handle, heif_item_id* item_id) {
  if (!item_id) {
    return {heif_error_Usage_error, heif_suberror_Null_pointer_argument,
            "NULL item_id passed to heif_image_handle_get_gain_map_metadata_id()"};
  }
  std::shared_ptr<ImageMetadata> metadata = handle->image->get_gain_map_metadata();
  if (!metadata) {
    return {heif_error_Invalid_input, heif_suberror_No_item_data,
              "base image handle is not associated with a gain map image"};
  }
  *item_id = metadata->item_id;
  return Error::Ok.error_struct(handle->image.get());
}

heif_error heif_image_handle_get_gain_map_image_handle(
    const heif_image_handle* handle, heif_image_handle** gain_map_handle) {
  if (!gain_map_handle) {
    return {heif_error_Usage_error, heif_suberror_Null_pointer_argument,
            "NULL gain_map_handle passed to heif_image_handle_get_gain_map_image_handle()"};
  }

  std::shared_ptr<ImageItem> gain_map_image = handle->image->get_gain_map();
  if (!gain_map_image) {
    Error err(heif_error_Usage_error, heif_suberror_Nonexisting_item_referenced,
              "base image handle is not associated with a gain map image");
    return err.error_struct(handle->image.get());
  }

  *gain_map_handle = new heif_image_handle();
  (*gain_map_handle)->image = gain_map_image;
  (*gain_map_handle)->context = handle->context;

  return Error::Ok.error_struct(handle->image.get());
}

size_t heif_image_handle_get_gain_map_metadata_size(const heif_image_handle* handle) {
  std::shared_ptr<ImageMetadata> metadata = handle->image->get_gain_map_metadata();

  if (metadata) {
    // Ignore unsigned int(8) version = 0; field of ToneMapImage syntax
    size_t sz = metadata->m_data.size();
    return sz > 0 ? (sz - 1) : 0;
  }

  return 0;
}

heif_error heif_image_handle_get_gain_map_metadata(const heif_image_handle* handle,
                                                          void* out_data) {
  if (!out_data) {
    return {heif_error_Usage_error, heif_suberror_Null_pointer_argument,
            "NULL out_data passed to heif_image_handle_get_gain_map_metadata()"};
  }

  std::shared_ptr<ImageMetadata> metadata = handle->image->get_gain_map_metadata();
  if (!metadata) {
    Error err(heif_error_Invalid_input, heif_suberror_No_item_data,
              "base image handle is not associated with a gain map image");
    return err.error_struct(handle->image.get());
  }

  uint8_t version = 0xff;
  size_t pos = 0;
  std::vector<uint8_t>& buffer = metadata->m_data;

  if (pos >= buffer.size()) {
    Error err(heif_error_Invalid_input, heif_suberror_End_of_data);
    return err.error_struct(handle->image.get());
  }
  version = buffer[pos++];
  if (version != 0) {
    Error err(heif_error_Invalid_input, heif_suberror_Unsupported_data_version,
              "Box[tmap] has unsupported version");
    return err.error_struct(handle->image.get());
  }

  memcpy(out_data, buffer.data() + pos, buffer.size() - pos);

  return heif_error_success;
}

struct heif_error heif_context_encode_gain_map_image(
    struct heif_context* ctx, const struct heif_image_handle* base_image_handle,
    struct heif_encoder* encoder, const struct heif_image* gain_map_image,
    const struct heif_encoding_options* input_options, const uint8_t* gain_map_metadata,
    int gain_map_metadata_len, const struct heif_color_profile_nclx* derived_image_nclx,
    struct heif_image_handle** out_image_handle) {
  if (!encoder) {
    return Error(heif_error_Usage_error, heif_suberror_Null_pointer_argument)
        .error_struct(ctx->context.get());
  }

  if (gain_map_metadata_len <= 0) {
    return Error(heif_error_Invalid_input, heif_suberror_Invalid_parameter_value)
        .error_struct(ctx->context.get());
  }

  if (out_image_handle) {
    *out_image_handle = nullptr;
  }

  // --- write tmap item
  std::vector<uint8_t> metadata;
  metadata.push_back(0);  // version = 0
  for (int i = 0; i < gain_map_metadata_len; i++) {
    metadata.push_back(gain_map_metadata[i]);
  }
  heif_item_id tmap_item_id = -1;
  ctx->context->add_tmap_item(metadata, tmap_item_id);

  std::vector<std::shared_ptr<Box>> properties;

  // --- write ISPE property for tmap item
  std::shared_ptr<Box_ispe> ispe = std::make_shared<Box_ispe>();
  ispe->set_size(base_image_handle->image->get_ispe_width(),
                 base_image_handle->image->get_ispe_height());

  properties.push_back(ispe);

  // --- write PIXI property for tmap item
  // TODO: this assumes the base image is 8 bit and derived image is 10 bit. Have to handle the
  // other way where the base image is hdr and derived image is sdr
  std::shared_ptr<Box_pixi> pixi = std::make_shared<Box_pixi>();
  pixi->add_channel_bits(10);
  pixi->add_channel_bits(10);
  pixi->add_channel_bits(10);

  properties.push_back(pixi);

  // --- write COLR property for tmap item
  if (derived_image_nclx != nullptr) {
    std::shared_ptr<Box_colr> colr = std::make_shared<Box_colr>();
    auto derived_img_nclx_profile = std::make_shared<color_profile_nclx>();
    derived_img_nclx_profile->set_from_heif_color_profile_nclx(derived_image_nclx);
    colr->set_color_profile(derived_img_nclx_profile);

    properties.push_back(colr);
  }
  // set tmap item properties
  for (auto& propertyBox : properties) {
    int index =
        ctx->context->get_heif_file()->get_ipco_box()->find_or_append_child_box(propertyBox);
    ctx->context->get_heif_file()->get_ipma_box()->add_property_for_item_ID(
        tmap_item_id,
        Box_ipma::PropertyAssociation{propertyBox->is_essential(), uint16_t(index + 1)});
  }

  heif_encoding_options *options = heif_encoding_options_alloc();
  if (input_options != nullptr) {
    heif_encoding_options_copy(options, input_options);
  }

  auto gainmap_encoding_result = ctx->context->encode_image(gain_map_image->image, encoder, *options,
                                                            heif_image_input_class_gain_map);

  heif_encoding_options_free(options);

  if (gainmap_encoding_result.error()) {
    return gainmap_encoding_result.error().error_struct(ctx->context.get());
  }

  std::shared_ptr<ImageItem> gain_map_image_item = *gainmap_encoding_result;
  Error error =
      ctx->context->link_gain_map(base_image_handle->image, gain_map_image_item, tmap_item_id);
  if (error != Error::Ok) {
    return error.error_struct(ctx->context.get());
  }

  if (out_image_handle) {
    *out_image_handle = new heif_image_handle;
    (*out_image_handle)->image = std::move(gain_map_image_item);
    (*out_image_handle)->context = ctx->context;
  }

  // --- generate altr box
  auto altr_box = std::make_shared<Box_EntityToGroup>();
  altr_box->set_short_type(fourcc("altr"));
  altr_box->set_group_id(ctx->context->get_heif_file()->get_unused_item_id());

  std::vector<heif_item_id> ids;
  ids.push_back(tmap_item_id);
  ids.push_back(base_image_handle->image->get_id());

  altr_box->set_item_ids(ids);
  ctx->context->get_heif_file()->add_entity_group_box(altr_box);

  return heif_error_success;
}

#endif  // WITH_EXPERIMENTAL_GAIN_MAP
