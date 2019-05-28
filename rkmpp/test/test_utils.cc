/*
 * Copyright (C) 2017 Hertz Wang wangh@rock-chips.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses
 *
 * Any non-GPL usage of this software or parts of this software is strictly
 * forbidden.
 *
 * Commercial non-GPL licensing of this software is possible.
 * For more info contact Rockchip Electronics Co., Ltd.
 */

#include "test_utils.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef LIBION
#include <ion/ion.h>
class IonBuffer {
public:
  IonBuffer(int param_client, ion_user_handle_t param_handle, int param_fd,
            void *param_map_ptr, size_t param_len)
      : client(param_client), handle(param_handle), fd(param_fd),
        map_ptr(param_map_ptr), len(param_len) {}
  ~IonBuffer();

private:
  int client;
  ion_user_handle_t handle;
  int fd;
  void *map_ptr;
  size_t len;
};

IonBuffer::~IonBuffer() {
  if (map_ptr)
    munmap(map_ptr, len);
  if (fd >= 0)
    close(fd);
  if (client < 0)
    return;
  if (handle) {
    int ret = ion_free(client, handle);
    if (ret)
      fprintf(stderr, "ion_free() failed <handle: %d>: %m!\n", handle);
  }
  ion_close(client);
}
#endif
#ifdef LIBDRM
#error(__FILE__:__LINE__): drm TODO
#endif

static int free_hw_memory(void *buffer) {
  assert(buffer);
#ifdef LIBION
  IonBuffer *ion_buffer = static_cast<IonBuffer *>(buffer);
  delete ion_buffer;
#endif
  return 0;
}

std::shared_ptr<rkmedia::MediaBuffer> alloc_hw_memory(ImageInfo &info, int num,
                                                      int den) {
  size_t size = info.vir_width * info.vir_height * num / den;
#ifdef LIBION
  int client = ion_open();
  if (client < 0) {
    fprintf(stderr, "ion_open() failed: %m\n");
    return nullptr;
  }
  ion_user_handle_t handle;
  int ret = ion_alloc(client, size, 0, ION_HEAP_TYPE_DMA_MASK, 0, &handle);
  if (ret) {
    fprintf(stderr, "ion_alloc() failed: %m\n");
    ion_close(client);
    return nullptr;
  }
  int fd;
  ret = ion_share(client, handle, &fd);
  if (ret < 0) {
    fprintf(stderr, "ion_share() failed: %m\n");
    ion_free(client, handle);
    ion_close(client);
    return nullptr;
  }
  void *ptr =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
  if (!ptr) {
    fprintf(stderr, "ion mmap() failed: %m\n");
    ion_free(client, handle);
    ion_close(client);
    close(fd);
    return nullptr;
  }
  IonBuffer *buffer = new IonBuffer(client, handle, fd, ptr, size);
  if (!buffer) {
    ion_free(client, handle);
    ion_close(client);
    close(fd);
    munmap(ptr, size);
    return nullptr;
  }

  return std::make_shared<rkmedia::ImageBuffer>(
      rkmedia::MediaBuffer(ptr, size, fd, buffer, free_hw_memory), info);
#endif
}
