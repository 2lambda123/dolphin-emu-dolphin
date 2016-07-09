// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <vector>

#include "VideoBackends/Vulkan/CommandBufferManager.h"
#include "VideoBackends/Vulkan/ObjectCache.h"
#include "VideoBackends/Vulkan/RasterFont.h"
#include "VideoBackends/Vulkan/ShaderCache.h"
#include "VideoBackends/Vulkan/Texture2D.h"
#include "VideoBackends/Vulkan/Util.h"

// Based on OGL RasterFont
// TODO: We should move this to common.

namespace Vulkan
{
static const int CHAR_WIDTH = 8;
static const int CHAR_HEIGHT = 13;
static const int CHAR_OFFSET = 32;
static const int CHAR_COUNT = 95;

static const u8 rasters[CHAR_COUNT][CHAR_HEIGHT] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36},
    {0x00, 0x00, 0x00, 0x66, 0x66, 0xff, 0x66, 0x66, 0xff, 0x66, 0x66, 0x00, 0x00},
    {0x00, 0x00, 0x18, 0x7e, 0xff, 0x1b, 0x1f, 0x7e, 0xf8, 0xd8, 0xff, 0x7e, 0x18},
    {0x00, 0x00, 0x0e, 0x1b, 0xdb, 0x6e, 0x30, 0x18, 0x0c, 0x76, 0xdb, 0xd8, 0x70},
    {0x00, 0x00, 0x7f, 0xc6, 0xcf, 0xd8, 0x70, 0x70, 0xd8, 0xcc, 0xcc, 0x6c, 0x38},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1c, 0x0c, 0x0e},
    {0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c},
    {0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30},
    {0x00, 0x00, 0x00, 0x00, 0x99, 0x5a, 0x3c, 0xff, 0x3c, 0x5a, 0x99, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03},
    {0x00, 0x00, 0x3c, 0x66, 0xc3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc3, 0x66, 0x3c},
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x38, 0x18},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0xe7, 0x7e},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0x07, 0x03, 0x03, 0xe7, 0x7e},
    {0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xff, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x03, 0x03, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x03, 0x7f, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
    {0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x1c, 0x1c, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06},
    {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60},
    {0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x0c, 0x06, 0x03, 0xc3, 0xc3, 0x7e},
    {0x00, 0x00, 0x3f, 0x60, 0xcf, 0xdb, 0xd3, 0xdd, 0xc3, 0x7e, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18},
    {0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
    {0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e},
    {0x00, 0x00, 0x7c, 0xee, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06},
    {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3},
    {0x00, 0x00, 0xc7, 0xc7, 0xcf, 0xcf, 0xdf, 0xdb, 0xfb, 0xf3, 0xf3, 0xe3, 0xe3},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e},
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
    {0x00, 0x00, 0x3f, 0x6e, 0xdf, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c},
    {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e},
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff},
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
    {0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff},
    {0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c},
    {0x00, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60},
    {0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18},
    {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x38, 0x30, 0x70},
    {0x00, 0x00, 0x7f, 0xc3, 0xc3, 0x7f, 0x03, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0x7e, 0xc3, 0xc0, 0xc0, 0xc0, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x03},
    {0x00, 0x00, 0x7f, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x33, 0x1e},
    {0x7e, 0xc3, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
    {0x38, 0x6c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x00},
    {0x00, 0x00, 0xc6, 0xcc, 0xf8, 0xf0, 0xd8, 0xcc, 0xc6, 0xc0, 0xc0, 0xc0, 0xc0},
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78},
    {0x00, 0x00, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xfe, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfc, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00},
    {0xc0, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0x00, 0x00, 0x00, 0x00},
    {0x03, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xfe, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xfe, 0x03, 0x03, 0x7e, 0xc0, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x1c, 0x36, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x00},
    {0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xdb, 0xc3, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0xc0, 0x60, 0x60, 0x30, 0x18, 0x3c, 0x66, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0xff, 0x60, 0x30, 0x18, 0x0c, 0x06, 0xff, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x0f, 0x18, 0x18, 0x18, 0x38, 0xf0, 0x38, 0x18, 0x18, 0x18, 0x0f},
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    {0x00, 0x00, 0xf0, 0x18, 0x18, 0x18, 0x1c, 0x0f, 0x1c, 0x18, 0x18, 0x18, 0xf0},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x8f, 0xf1, 0x60, 0x00, 0x00, 0x00}};

static const char VERTEX_SHADER_SOURCE[] = R"(

layout(std140, set = 0, binding = 0) uniform VSBlock {
  vec2 char_size;
  vec2 offset;
};

layout(location = 0) in vec4 ipos;
layout(location = 5) in vec4 icol0;
layout(location = 8) in vec3 itex0;

layout(location = 0) out vec2 uv0;

void main()
{
  gl_Position = vec4(ipos.xy + offset, 0.0f, 1.0f);
  gl_Position.y = -gl_Position.y;
  uv0 = itex0.xy * char_size;
}

)";

static const char FRAGMENT_SHADER_SOURCE[] = R"(

layout(std140, set = 0, binding = 2) uniform PSBlock {
  vec4 color;
};

layout(set = 1, binding = 0) uniform sampler2D samp0;

layout(location = 0) in vec2 uv0;

layout(location = 0) out vec4 ocol0;

void main()
{
  ocol0 = texture(samp0, uv0) * color;
  //ocol0 = color + vec4(0.2f, 0.2f, 0.2f, 1.0f);
  //ocol0 = float4(uv0.xy, 0.1f, 1.0f);
}

)";

RasterFont::RasterFont()
{
}

RasterFont::~RasterFont()
{
  if (m_vertex_shader != VK_NULL_HANDLE)
    g_command_buffer_mgr->DeferResourceDestruction(m_vertex_shader);
  if (m_fragment_shader != VK_NULL_HANDLE)
    g_command_buffer_mgr->DeferResourceDestruction(m_fragment_shader);
}

bool RasterFont::Initialize()
{
  // Create shaders and texture
  if (!CreateShaders() || !CreateTexture())
    return false;

  return true;
}

bool RasterFont::CreateTexture()
{
  // generate the texture
  std::vector<u32> texture_data(CHAR_WIDTH * CHAR_COUNT * CHAR_HEIGHT);
  for (int y = 0; y < CHAR_HEIGHT; y++)
  {
    for (int c = 0; c < CHAR_COUNT; c++)
    {
      for (int x = 0; x < CHAR_WIDTH; x++)
      {
        bool pixel = (0 != (rasters[c][y] & (1 << (CHAR_WIDTH - x - 1))));
        texture_data[CHAR_WIDTH * CHAR_COUNT * y + CHAR_WIDTH * c + x] = pixel ? -1 : 0;
      }
    }
  }

  // create the actual texture object
  m_texture = Texture2D::Create(CHAR_WIDTH * CHAR_COUNT, CHAR_HEIGHT, 1, 1,
                                VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_VIEW_TYPE_2D,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  if (!m_texture)
    return false;

  // create temporary buffer for uploading texture
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                    nullptr,
                                    0,
                                    static_cast<VkDeviceSize>(texture_data.size() * sizeof(u32)),
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_SHARING_MODE_EXCLUSIVE,
                                    0,
                                    nullptr};
  VkBuffer temp_buffer;
  VkResult res = vkCreateBuffer(g_object_cache->GetDevice(), &buffer_info, nullptr, &temp_buffer);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkCreateBuffer failed: ");
    return false;
  }

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(g_object_cache->GetDevice(), temp_buffer, &memory_requirements);
  uint32_t memory_type_index = g_object_cache->GetMemoryType(memory_requirements.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr,
                                               memory_requirements.size, memory_type_index};
  VkDeviceMemory temp_buffer_memory;
  res = vkAllocateMemory(g_object_cache->GetDevice(), &memory_allocate_info, nullptr,
                         &temp_buffer_memory);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkAllocateMemory failed: ");
    vkDestroyBuffer(g_object_cache->GetDevice(), temp_buffer, nullptr);
    return false;
  }

  // Bind buffer to memory
  res = vkBindBufferMemory(g_object_cache->GetDevice(), temp_buffer, temp_buffer_memory, 0);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkBindBufferMemory failed: ");
    vkDestroyBuffer(g_object_cache->GetDevice(), temp_buffer, nullptr);
    vkFreeMemory(g_object_cache->GetDevice(), temp_buffer_memory, nullptr);
    return false;
  }

  // Copy into buffer
  void* mapped_ptr;
  res = vkMapMemory(g_object_cache->GetDevice(), temp_buffer_memory, 0, buffer_info.size, 0,
                    &mapped_ptr);
  if (res != VK_SUCCESS)
  {
    LOG_VULKAN_ERROR(res, "vkMapMemory failed: ");
    vkDestroyBuffer(g_object_cache->GetDevice(), temp_buffer, nullptr);
    vkFreeMemory(g_object_cache->GetDevice(), temp_buffer_memory, nullptr);
    return false;
  }

  // Copy texture data into staging buffer
  memcpy(mapped_ptr, texture_data.data(), texture_data.size() * sizeof(u32));
  vkUnmapMemory(g_object_cache->GetDevice(), temp_buffer_memory);

  // Copy from staging buffer to the final texture
  VkBufferImageCopy region = {0,           CHAR_WIDTH * CHAR_COUNT,
                              CHAR_HEIGHT, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                              {0, 0, 0},   {CHAR_WIDTH * CHAR_COUNT, CHAR_HEIGHT, 1}};
  m_texture->TransitionToLayout(g_command_buffer_mgr->GetCurrentCommandBuffer(),
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vkCmdCopyBufferToImage(g_command_buffer_mgr->GetCurrentCommandBuffer(), temp_buffer,
                         m_texture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  // Free temp buffers after command buffer executes
  m_texture->TransitionToLayout(g_command_buffer_mgr->GetCurrentCommandBuffer(),
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  g_command_buffer_mgr->DeferResourceDestruction(temp_buffer);
  g_command_buffer_mgr->DeferResourceDestruction(temp_buffer_memory);
  return true;
}

bool RasterFont::CreateShaders()
{
  m_vertex_shader =
      g_object_cache->GetVertexShaderCache().CompileAndCreateShader(VERTEX_SHADER_SOURCE);
  m_fragment_shader =
      g_object_cache->GetPixelShaderCache().CompileAndCreateShader(FRAGMENT_SHADER_SOURCE);
  if (m_vertex_shader == VK_NULL_HANDLE || m_fragment_shader == VK_NULL_HANDLE)
    return false;

  return true;
}

void RasterFont::PrintMultiLineText(VkRenderPass render_pass, const std::string& text,
                                    float start_x, float start_y, u32 bbWidth, u32 bbHeight,
                                    u32 color)
{
  // skip empty strings
  if (text.empty())
    return;

  UtilityShaderDraw draw(g_object_cache->GetStandardPipelineLayout(),
                         render_pass,
                         m_vertex_shader,
                         VK_NULL_HANDLE,
                         m_fragment_shader);

  UtilityShaderVertex* vertices =
      draw.ReserveVertices(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, text.length() * 6);
  size_t num_vertices = 0;
  if (!vertices)
    return;

  float delta_x = float(2 * CHAR_WIDTH) / float(bbWidth);
  float delta_y = float(2 * CHAR_HEIGHT) / float(bbHeight);
  float border_x = 2.0f / float(bbWidth);
  float border_y = 4.0f / float(bbHeight);

  float x = float(start_x);
  float y = float(start_y);

  for (const char& c : text)
  {
    if (c == '\n')
    {
      x = float(start_x);
      y -= delta_y + border_y;
      continue;
    }

    // do not print spaces, they can be skipped easily
    if (c == ' ')
    {
      x += delta_x + border_x;
      continue;
    }

    if (c < CHAR_OFFSET || c >= CHAR_COUNT + CHAR_OFFSET)
      continue;

    vertices[num_vertices].SetPosition(x, y);
    vertices[num_vertices].SetTextureCoordinates(static_cast<float>(c - CHAR_OFFSET), 0.0f);
    num_vertices++;

    vertices[num_vertices].SetPosition(x + delta_x, y);
    vertices[num_vertices].SetTextureCoordinates(static_cast<float>(c - CHAR_OFFSET + 1), 0.0f);
    num_vertices++;

    vertices[num_vertices].SetPosition(x + delta_x, y + delta_y);
    vertices[num_vertices].SetTextureCoordinates(static_cast<float>(c - CHAR_OFFSET + 1), 1.0f);
    num_vertices++;

    vertices[num_vertices].SetPosition(x, y);
    vertices[num_vertices].SetTextureCoordinates(static_cast<float>(c - CHAR_OFFSET), 0.0f);
    num_vertices++;

    vertices[num_vertices].SetPosition(x + delta_x, y + delta_y);
    vertices[num_vertices].SetTextureCoordinates(static_cast<float>(c - CHAR_OFFSET + 1), 1.0f);
    num_vertices++;

    vertices[num_vertices].SetPosition(x, y + delta_y);
    vertices[num_vertices].SetTextureCoordinates(static_cast<float>(c - CHAR_OFFSET), 1.0f);
    num_vertices++;

    x += delta_x + border_x;
  }

  // skip all whitespace strings
  if (num_vertices == 0)
    return;

  draw.CommitVertices(num_vertices);

  struct VSBlock
  {
    float char_size[2];
    float offset[2];
    int pad[2];
  } vs_block = {};

  struct PSBlock
  {
    float color[4];
  } ps_block = {};

  vs_block.char_size[0] = 1.0f / static_cast<float>(CHAR_COUNT);
  vs_block.char_size[1] = 1.0f;

  // shadows
  vs_block.offset[0] = 2.0f / bbWidth;
  vs_block.offset[1] = -2.0f / bbHeight;
  ps_block.color[3] = (color >> 24) / 255.0f;

  u8* vs_uniforms_ptr = draw.AllocateVSUniforms(sizeof(vs_block));
  memcpy(vs_uniforms_ptr, &vs_block, sizeof(vs_block));
  draw.CommitVSUniforms(sizeof(vs_block));

  u8* ps_uniforms_ptr = draw.AllocatePSUniforms(sizeof(ps_block));
  memcpy(ps_uniforms_ptr, &ps_block, sizeof(ps_block));
  draw.CommitPSUniforms(sizeof(ps_block));

  draw.SetPSSampler(0, m_texture->GetView(), g_object_cache->GetLinearSampler());

  // Setup alpha blending
  BlendState blend_state = Util::GetNoBlendingBlendState();
  blend_state.blend_enable = VK_TRUE;
  blend_state.src_blend = VK_BLEND_FACTOR_SRC_ALPHA;
  blend_state.blend_op = VK_BLEND_OP_ADD;
  blend_state.dst_blend = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  draw.SetBlendState(blend_state);

  draw.Draw();

  // non-shadowed part
  vs_block.offset[0] = 0.0f;
  vs_block.offset[1] = 0.0f;
  ps_block.color[0] = ((color >> 16) & 0xFF) / 255.0f;
  ps_block.color[1] = ((color >> 8) & 0xFF) / 255.0f;
  ps_block.color[2] = (color & 0xFF) / 255.0f;
  vs_uniforms_ptr = draw.AllocateVSUniforms(sizeof(vs_block));
  memcpy(vs_uniforms_ptr, &vs_block, sizeof(vs_block));
  draw.CommitVSUniforms(sizeof(vs_block));

  ps_uniforms_ptr = draw.AllocatePSUniforms(sizeof(ps_block));
  memcpy(ps_uniforms_ptr, &ps_block, sizeof(ps_block));
  draw.CommitPSUniforms(sizeof(ps_block));

  draw.Draw();
}

}  // namespace Vulkan
