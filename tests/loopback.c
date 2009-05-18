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

#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <getopt.h>
#include "config.h"
#if HAVE_LINUX_USB_SUBDIR
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif
#include <usg.h>

/* /dev/gadget/ep* doesn't support poll, we have to use an alternative
   approach. */
#include <pthread.h>		

#define STRING_MANUFACTURER             25
#define STRING_PRODUCT                  45
#define STRING_SERIAL                   101
#define STRING_LOOPBACK              250

static struct usg_string strings[] = {
  {STRING_MANUFACTURER, "The manufacturer",},
  {STRING_PRODUCT, "The product",},
  {STRING_SERIAL, "0123456789.0123456789.0123456789",},
  {STRING_LOOPBACK, "The loopback",},
};

static struct usg_strings loopback_strings = {
  .language = 0x0409,		/* en-us */
  .strings = strings
};

#define CONFIG_LOOPBACK         2

static struct usb_device_descriptor loopback_device_descriptor = {
  .bLength = sizeof(loopback_device_descriptor),
  .bDescriptorType = USB_DT_DEVICE,

  .bcdUSB = usg_cpu_to_le16(0x0200),
  .bDeviceClass = USB_CLASS_VENDOR_SPEC,

  .iManufacturer = usg_cpu_to_le16(STRING_MANUFACTURER),
  .iProduct = usg_cpu_to_le16(STRING_PRODUCT),
  .iSerialNumber = usg_cpu_to_le16(STRING_SERIAL),
  .bNumConfigurations = 1,
};

static struct usb_config_descriptor loopback_config_descriptor = {
  .bLength = sizeof(loopback_config_descriptor),
  .bDescriptorType = USB_DT_CONFIG,

  .bNumInterfaces = 1,
  .bConfigurationValue = CONFIG_LOOPBACK,
  .iConfiguration = STRING_LOOPBACK,
  .bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  .bMaxPower = 1,		/* self-powered */
};

static const struct usb_interface_descriptor loopback_interface_descriptor = {
  .bLength = sizeof(loopback_interface_descriptor),
  .bDescriptorType = USB_DT_INTERFACE,

  .bNumEndpoints = 2,
  .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
  .iInterface = STRING_LOOPBACK,
};

static struct usb_endpoint_descriptor loopback_ep_in_descriptor = {
  .bLength = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType = USB_DT_ENDPOINT,

  .bEndpointAddress = USB_DIR_IN | 7, /* number is mandatory for gadgetfs */
  .bmAttributes = USB_ENDPOINT_XFER_BULK,
  .wMaxPacketSize = usg_cpu_to_le16(64), /* mandatory for gadgetfs */
};

static struct usb_endpoint_descriptor loopback_ep_out_descriptor = {
  .bLength = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType = USB_DT_ENDPOINT,

  .bEndpointAddress = USB_DIR_OUT | 3, /* number is mandatory for gadgetfs */
  .bmAttributes = USB_ENDPOINT_XFER_BULK,
  .wMaxPacketSize = usg_cpu_to_le16(64), /* mandatory for gadgetfs */
};

static struct usb_endpoint_descriptor loopback_hs_ep_in_descriptor = {
  .bLength = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType = USB_DT_ENDPOINT,

  .bEndpointAddress = USB_DIR_IN | 7, /* number is mandatory for gadgetfs */
  .bmAttributes = USB_ENDPOINT_XFER_BULK,
  .wMaxPacketSize = usg_cpu_to_le16(512), /* mandatory for gadgetfs */
};

static struct usb_endpoint_descriptor loopback_hs_ep_out_descriptor = {
  .bLength = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType = USB_DT_ENDPOINT,

  .bEndpointAddress = USB_DIR_OUT | 3, /* number is mandatory for gadgetfs */
  .bmAttributes = USB_ENDPOINT_XFER_BULK,
  .wMaxPacketSize = usg_cpu_to_le16(512), /* mandatory for gadgetfs */
};

static struct usb_descriptor_header *loopback_config[] = {
  (struct usb_descriptor_header *)&loopback_config_descriptor,
  (struct usb_descriptor_header *)&loopback_interface_descriptor,
  (struct usb_descriptor_header *)&loopback_ep_in_descriptor,
  (struct usb_descriptor_header *)&loopback_ep_out_descriptor,
  NULL,
};

static struct usb_descriptor_header *loopback_hs_config[] = {
  (struct usb_descriptor_header *)&loopback_config_descriptor,
  (struct usb_descriptor_header *)&loopback_interface_descriptor,
  (struct usb_descriptor_header *)&loopback_hs_ep_in_descriptor,
  (struct usb_descriptor_header *)&loopback_hs_ep_out_descriptor,
  NULL,
};

static struct usg_endpoint *loopback_ep_in, *loopback_ep_out;
static pthread_t loopback_thread;

static void
loopback_stop_endpoints (void *data)
{
  usg_endpoint_close (loopback_ep_in);
  usg_endpoint_close (loopback_ep_out);
}

static void*
loopback_loop (void *data)
{
  char buf[BUFSIZ];
  int ret;

  pthread_cleanup_push (loopback_stop_endpoints, NULL);
  while (1)
    {
      int i;

      pthread_testcancel ();
      ret = usg_endpoint_read (loopback_ep_out, buf, 64, 100);
      if (ret < 0)
	{
	  perror ("usg_endpoint_read");
	  break;
	}
      for (i = 0; i < ret / 2; i++)
	{
	  char c;
	  c = buf[i];
	  buf[i] = buf[ret - i - 1];
	  buf[ret - i - 1] = c;
	}
      if (usg_endpoint_write (loopback_ep_in, buf, ret, 100) < 0)
	{
	  perror ("usg_endpoint_write");
	  break;
	}
    }
  pthread_cleanup_pop (1);
}

static void
loopback_event_cb (usg_dev_handle *handle, struct usg_event *event, void *arg)
{
  switch (event->type)
    {
    case USG_EVENT_ENDPOINT_ENABLE:
      if (event->u.number == (loopback_ep_in_descriptor.bEndpointAddress
			      & USB_ENDPOINT_NUMBER_MASK))
	loopback_ep_in = usg_endpoint (handle, event->u.number);
      else if (event->u.number == (loopback_ep_out_descriptor.bEndpointAddress
				   & USB_ENDPOINT_NUMBER_MASK))
	loopback_ep_out = usg_endpoint (handle, event->u.number);

      if (!loopback_ep_in || !loopback_ep_out)
	return;

      if (pthread_create (&loopback_thread, 0, loopback_loop, NULL) != 0)
	perror ("pthread_create");
      break;
    case USG_EVENT_ENDPOINT_DISABLE:
      if (event->u.number == (loopback_ep_in_descriptor.bEndpointAddress
			      & USB_ENDPOINT_NUMBER_MASK))
	loopback_ep_in = NULL;
      else if (event->u.number == (loopback_ep_out_descriptor.bEndpointAddress
				   & USB_ENDPOINT_NUMBER_MASK))
	loopback_ep_out = NULL;
    case USG_EVENT_DISCONNECT:	/* FALLTHROUGH */
      if (loopback_thread)
	pthread_cancel (loopback_thread);
      break;
    }
}

static char *program_name;

static void
usage (FILE *out)
{
  fprintf (out, "Usage: %s [OPTIONS] VEND:PROD\n"
	   "Options are:\n"
	   "\t--debug=LEVEL, -d\tSpecify debug level\n"
	   "\t--help, -h\tShow this help\n",
	   program_name);
}

int main (int argc, char **argv)
{
  struct usg_device device = {
    .device = &loopback_device_descriptor,
    .config = loopback_config,
    .hs_config = loopback_hs_config,
    .strings = &loopback_strings,
  };
  usg_dev_handle *handle;
  struct usg_endpoint *ep0;
  struct pollfd fds;
  int vendor_id, product_id, c, debug_level = 0;
  struct option long_options[] = {
    {"debug", 1, 0, 'd'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
  };
  program_name = argv[0];

  while (1)
    {
      int option_index = 0;

      c = getopt_long (argc, argv, "hd:", long_options, &option_index);
      if (c == -1)
	break;
      switch (c)
	{
	case 'd':
	  debug_level = atoi (optarg);
	  break;
	case 'h':
	  usage (stdout);
	  exit (0);
	default:
	  usage (stderr);
	  exit (1);
	}
    }

  if ((argc - optind) != 1
      || sscanf (argv[optind], "%X:%X", &vendor_id, &product_id) != 2)
    {
      usage (stderr);
      exit (1);
    }

  loopback_device_descriptor.idVendor = vendor_id;
  loopback_device_descriptor.idProduct = product_id;
  handle = usg_open (&device);
  if (!handle)
    {
      fprintf (stderr, "Couldn't open device.\n");
      exit (1);
    }
  usg_set_event_cb (handle, loopback_event_cb, NULL);
  usg_set_debug_level (handle, debug_level);
  ep0 = usg_endpoint (handle, 0);
  fds.fd = usg_control_fd (handle);
  fds.events = POLLIN;
  while (1)
    {
      if (poll (&fds, 1, -1) < 0)
	{
	  perror ("poll");
	  break;
	}
      if (fds.revents & POLLIN)
	usg_handle_control_event (handle);
    }
  usg_close (handle);

  return 0;
}
