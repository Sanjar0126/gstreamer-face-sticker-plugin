/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2020 Niels De Graef <niels.degraef@gmail.com>
 * Copyright (C) 2025 sanjar <<user@hostname.org>>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_FACESTICKER_H__
#define __GST_FACESTICKER_H__

#include <glib.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <opencv2/core/mat.hpp>

#define GST_API_VERSION "1.0"
#define GST_LICENSE "LGPL"
#define GST_PACKAGE_NAME "GStreamer template Plug-ins"
#define GST_PACKAGE_ORIGIN "https://gstreamer.freedesktop.org"
#define PACKAGE "facesticker"
#define PACKAGE_VERSION "1.19.0.1"

#define DETECT_BUFFER_SIZE 0x9000

G_BEGIN_DECLS

#define GST_TYPE_FACESTICKER (gst_face_sticker_get_type())
G_DECLARE_FINAL_TYPE(GstFaceSticker, gst_face_sticker, GST, FACESTICKER,
                     GstBaseTransform)

struct _GstFaceSticker {
  GstBaseTransform element;

  gboolean silent;
  gchar* eye_img_path;
  gfloat eye_img_scale;
  cv::Mat eye_img;

  GstVideoInfo in_info;
  GstVideoInfo out_info;

  unsigned char *face_detection_buffer;
};

G_END_DECLS

#endif /* __GST_FACESTICKER_H__ */

typedef struct {
  int confidence;
  int x;
  int y;
  int width;
  int height;
  cv::Point leftEye;
  cv::Point rightEye;
  cv::Point nose;
  cv::Point leftMouth;
  cv::Point rightMouth;
} FacialData;