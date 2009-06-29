/*
 * Copyright (C) 2009 Daiki Ueno <ueno@unixuser.org>
 * This file is part of libusg.
 *
 * libusg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libusg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __USG_H
#define __USG_H

#include <endian.h>
#include <stdint.h>
#include <unistd.h>

/* borrowed from libusb/libusb.h */
#define usg_bswap16(x) (((x & 0xFF) << 8) | (x >> 8))
#define usg_bswap32(x) ((usg_bswap16(x & 0xFFFF) << 16) | usg_bswap16(x >> 16))
/* borrowed from libusb/libusb.h */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define usg_cpu_to_le16(x) (x)
#define usg_le16_to_cpu(x) (x)
#define usg_cpu_to_le32(x) (x)
#define usg_le32_to_cpu(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define usg_cpu_to_le16(x) usg_bswap16(x)
#define usg_le16_to_cpu(x) usg_bswap16(x)
#define usg_cpu_to_le32(x) usg_bswap32(x)
#define usg_le32_to_cpu(x) usg_bswap32(x)
#else
#error "Unrecognized endianness"
#endif

struct usg_string
{
  uint8_t id;
  const char *s;
};

struct usg_strings
{
  uint16_t language;	/* 0x0409 for en-us */
  struct usg_string *strings;
};

struct usg_endpoint
{
  char *name;
};

enum usg_event_type {
  USG_EVENT_ENDPOINT_ENABLE,
  USG_EVENT_ENDPOINT_DISABLE,
  USG_EVENT_CONNECT,
  USG_EVENT_DISCONNECT,
  USG_EVENT_SUSPEND
};

struct usg_event
{
  enum usg_event_type type;
  union
  {
    int number;
  } u;
};

struct usg_dev_handle;
typedef struct usg_dev_handle usg_dev_handle;

struct usg_device
{
  struct usb_device_descriptor *device;
  struct usb_descriptor_header **config, **hs_config;
  struct usg_strings *strings;
};

struct usg_endpoint *usg_endpoint (usg_dev_handle *, int);
int usg_endpoint_close (struct usg_endpoint *);
ssize_t usg_endpoint_write (struct usg_endpoint *, const void *, size_t, int);
ssize_t usg_endpoint_read (struct usg_endpoint *, void *, size_t, int);

usg_dev_handle *usg_open (struct usg_device *);
int usg_close (usg_dev_handle *);

typedef void (*usg_event_cb) (usg_dev_handle *, struct usg_event *, void *);
void usg_set_event_cb (usg_dev_handle *, usg_event_cb, void *);
void usg_set_debug_level (usg_dev_handle *, int);
int usg_handle_control_event (usg_dev_handle *);
int usg_control_fd (usg_dev_handle *);

#endif
