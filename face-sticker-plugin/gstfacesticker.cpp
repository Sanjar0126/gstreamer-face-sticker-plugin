/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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

/**
 * SECTION:element-facesticker
 *
 * A GStreamer element that detects faces in video frames and applies stickers
 * over eye regions.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! facesticker ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/base/base.h>
#include <gst/controller/controller.h>
#include <gst/gst.h>
#include <gst/video/video-frame.h>
#include <opencv2/opencv.hpp>

#include "facedetectcnn.h"
#include "gstfacesticker.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_face_sticker_debug);
#define GST_CAT_DEFAULT gst_face_sticker_debug

// Default values for properties
#define DEFAULT_EYE_IMG_SCALE 1.0f

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SILENT,
  PROP_EYEIMG_PATH,
  PROP_EYEIMG_SCALE,
};

/* the capabilities of the inputs and outputs.
 *
 * FIXME:describe the real formats here.
 */
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ BGR }")));

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ BGR }")));

#define gst_face_sticker_parent_class parent_class
G_DEFINE_TYPE(GstFaceSticker, gst_face_sticker, GST_TYPE_BASE_TRANSFORM);
GST_ELEMENT_REGISTER_DEFINE(face_sticker, "face_sticker", GST_RANK_NONE,
                            GST_TYPE_FACESTICKER);

static void gst_face_sticker_set_property(GObject *object, guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec);
static void gst_face_sticker_get_property(GObject *object, guint prop_id,
                                          GValue *value, GParamSpec *pspec);
static void gst_face_sticker_finalize(GObject *object);
static gboolean gst_face_sticker_set_caps(GstBaseTransform *trans,
                                          GstCaps *incaps, GstCaps *outcaps);
static GstFlowReturn gst_face_sticker_transform_ip(GstBaseTransform *base,
                                                   GstBuffer *outbuf);
static void process_face_detection(GstFaceSticker *filter, cv::Mat &frame_mat);
static void apply_eye_stickers(GstFaceSticker *filter, cv::Mat &frame_mat,
                               short *facial_landmarks, int x, int y, int w,
                               int h);

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void gst_face_sticker_class_init(GstFaceStickerClass *klass) {
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseTransformClass *base_transform_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gstelement_class = GST_ELEMENT_CLASS(klass);
  base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

  gobject_class->set_property = gst_face_sticker_set_property;
  gobject_class->get_property = gst_face_sticker_get_property;
  gobject_class->finalize = gst_face_sticker_finalize;

  base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_face_sticker_set_caps);
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR(gst_face_sticker_transform_ip);

  g_object_class_install_property(
      gobject_class, PROP_SILENT,
      g_param_spec_boolean(
          "silent", "Silent", "Produce verbose output?", FALSE,
          (GParamFlags)(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  g_object_class_install_property(
      gobject_class, PROP_EYEIMG_PATH,
      g_param_spec_string("eye_img_path", "Eye image path",
                          "Path to the image file used as eye sticker", "",
                          (GParamFlags)(G_PARAM_READWRITE)));

  g_object_class_install_property(
      gobject_class, PROP_EYEIMG_SCALE,
      g_param_spec_float("eye_img_scale", "Eye image scale",
                         "Scale factor for the eye sticker image", 0,
                         G_MAXFLOAT, DEFAULT_EYE_IMG_SCALE,
                         (GParamFlags)(G_PARAM_READWRITE)));

  gst_element_class_set_details_simple(
      gstelement_class, "FaceSticker", "Filter/Effect/Video",
      "Detects faces and applies stickers over eye regions",
      "sanjar <<user@hostname.org>>");

  gst_element_class_add_pad_template(
      gstelement_class, gst_static_pad_template_get(&src_template));
  gst_element_class_add_pad_template(
      gstelement_class, gst_static_pad_template_get(&sink_template));

  /* debug category for fltering log messages
   *
   * FIXME:exchange the string 'Template plugin' with your description
   */
  GST_DEBUG_CATEGORY_INIT(gst_face_sticker_debug, "facesticker", 0,
                          "Face detection and sticker application element");
}

static gboolean gst_face_sticker_set_caps(GstBaseTransform *trans,
                                          GstCaps *incaps, GstCaps *outcaps) {
  GstFaceSticker *filter = GST_FACESTICKER(trans);

  if (!gst_video_info_from_caps(&filter->in_info, incaps)) {
    GST_ERROR_OBJECT(filter, "Failed to get video info from input caps");
    return FALSE;
  }

  if (!gst_video_info_from_caps(&filter->out_info, outcaps)) {
    GST_ERROR_OBJECT(filter, "Failed to get video info from output caps");
    return FALSE;
  }

  GST_DEBUG_OBJECT(filter, "Input video format: %dx%d", filter->in_info.width,
                   filter->in_info.height);
  GST_DEBUG_OBJECT(filter, "Output video format: %dx%d", filter->out_info.width,
                   filter->out_info.height);

  return TRUE;
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_face_sticker_init(GstFaceSticker *filter) {
  filter->silent = FALSE;
  filter->eye_img_path = NULL;
  filter->eye_img_scale = DEFAULT_EYE_IMG_SCALE;

  filter->face_detection_buffer = (unsigned char *)malloc(DETECT_BUFFER_SIZE);
  if (!filter->face_detection_buffer) {
    GST_ERROR_OBJECT(filter, "Failed to allocate face detection buffer");
  }
}

static void gst_face_sticker_finalize(GObject *object) {
  GstFaceSticker *filter = GST_FACESTICKER(object);

  if (filter->face_detection_buffer) {
    free(filter->face_detection_buffer);
    filter->face_detection_buffer = NULL;
  }

  if (filter->eye_img_path) {
    g_free(filter->eye_img_path);
    filter->eye_img_path = NULL;
  }

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_face_sticker_set_property(GObject *object, guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec) {
  GstFaceSticker *filter = GST_FACESTICKER(object);

  switch (prop_id) {
  case PROP_SILENT:
    filter->silent = g_value_get_boolean(value);
    break;

  case PROP_EYEIMG_PATH: {
    const gchar *path = g_value_get_string(value);

    if (filter->eye_img_path) {
      g_free(filter->eye_img_path);
    }

    filter->eye_img_path = g_strdup(path);

    if (path && *path) {
      filter->eye_img = cv::imread(path, cv::IMREAD_COLOR);
      if (filter->eye_img.empty()) {
        GST_WARNING_OBJECT(filter, "Failed to load eye image from %s", path);
      } else {
        GST_DEBUG_OBJECT(filter, "Successfully loaded eye image from %s", path);
      }
    }
    break;
  }

  case PROP_EYEIMG_SCALE:
    filter->eye_img_scale = g_value_get_float(value);
    GST_DEBUG_OBJECT(filter, "Eye image scale set to %f",
                     filter->eye_img_scale);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void gst_face_sticker_get_property(GObject *object, guint prop_id,
                                          GValue *value, GParamSpec *pspec) {
  GstFaceSticker *filter = GST_FACESTICKER(object);

  switch (prop_id) {
  case PROP_SILENT:
    g_value_set_boolean(value, filter->silent);
    break;

  case PROP_EYEIMG_PATH:
    g_value_set_string(value, filter->eye_img_path);
    break;

  case PROP_EYEIMG_SCALE:
    g_value_set_float(value, filter->eye_img_scale);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void apply_eye_stickers(GstFaceSticker *filter, cv::Mat &frame_mat,
                               short *facial_landmarks, int x, int y, int w,
                               int h) {
  if (filter->eye_img.empty()) {
    cv::circle(frame_mat, cv::Point(facial_landmarks[0], facial_landmarks[1]),
               1, cv::Scalar(255, 0, 0), 2); // left eye
    cv::circle(frame_mat, cv::Point(facial_landmarks[2], facial_landmarks[3]),
               1, cv::Scalar(0, 0, 255), 2); // right eye
    return;
  }

  cv::Mat eye_img_resized;
  cv::Mat eye_mask;
  cv::Rect roi_left_eye, roi_right_eye;

  cv::resize(filter->eye_img, eye_img_resized,
             cv::Size(w * filter->eye_img_scale, h * filter->eye_img_scale));

  cvtColor(eye_img_resized, eye_mask, cv::COLOR_BGR2GRAY);

  int left_eye_x = facial_landmarks[0] - (eye_img_resized.cols / 2);
  int left_eye_y = facial_landmarks[1] - (eye_img_resized.rows / 2);
  int right_eye_x = facial_landmarks[2] - (eye_img_resized.cols / 2);
  int right_eye_y = facial_landmarks[3] - (eye_img_resized.rows / 2);

  left_eye_x =
      std::max(0, std::min(left_eye_x, frame_mat.cols - eye_img_resized.cols));
  left_eye_y =
      std::max(0, std::min(left_eye_y, frame_mat.rows - eye_img_resized.rows));
  right_eye_x =
      std::max(0, std::min(right_eye_x, frame_mat.cols - eye_img_resized.cols));
  right_eye_y =
      std::max(0, std::min(right_eye_y, frame_mat.rows - eye_img_resized.rows));

  roi_left_eye = cv::Rect(left_eye_x, left_eye_y, eye_img_resized.cols,
                          eye_img_resized.rows);
  roi_right_eye = cv::Rect(right_eye_x, right_eye_y, eye_img_resized.cols,
                           eye_img_resized.rows);

  if (roi_left_eye.width > 0 && roi_left_eye.height > 0 &&
      roi_left_eye.x >= 0 && roi_left_eye.y >= 0 &&
      roi_left_eye.x + roi_left_eye.width <= frame_mat.cols &&
      roi_left_eye.y + roi_left_eye.height <= frame_mat.rows) {
    eye_img_resized.copyTo(frame_mat(roi_left_eye), eye_mask != 255);
  }

  if (roi_right_eye.width > 0 && roi_right_eye.height > 0 &&
      roi_right_eye.x >= 0 && roi_right_eye.y >= 0 &&
      roi_right_eye.x + roi_right_eye.width <= frame_mat.cols &&
      roi_right_eye.y + roi_right_eye.height <= frame_mat.rows) {
    eye_img_resized.copyTo(frame_mat(roi_right_eye), eye_mask != 255);
  }
}

static void process_face_detection(GstFaceSticker *filter, cv::Mat &frame_mat) {
  int *p_results = NULL;

  p_results = facedetect_cnn(
      filter->face_detection_buffer, (unsigned char *)(frame_mat.ptr(0)),
      frame_mat.cols, frame_mat.rows, (int)frame_mat.step);

  int num_faces = p_results ? *p_results : 0;

  for (int i = 0; i < num_faces; i++) {
    short *facial_data = ((short *)(p_results + 1)) + 16 * i;

    int confidence = facial_data[0];
    int x = facial_data[1];
    int y = facial_data[2];
    int w = facial_data[3];
    int h = facial_data[4];

    cv::putText(frame_mat, std::to_string(confidence), cv::Point(x, y - 3),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);

    rectangle(frame_mat, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0), 2);

    cv::circle(frame_mat, cv::Point(facial_data[5 + 4], facial_data[5 + 5]), 1,
               cv::Scalar(0, 255, 0), 2); // nose
    cv::circle(frame_mat, cv::Point(facial_data[5 + 6], facial_data[5 + 7]), 1,
               cv::Scalar(255, 0, 255), 2); // left mouth
    cv::circle(frame_mat, cv::Point(facial_data[5 + 8], facial_data[5 + 9]), 1,
               cv::Scalar(0, 255, 255), 2); // right mouth

    apply_eye_stickers(filter, frame_mat, &facial_data[5], x, y, w, h);

    if (!filter->silent) {
      GST_LOG_OBJECT(filter,
                     "Face %d: confidence=%d, [%d, %d, %d, %d] (%d,%d) (%d,%d) "
                     "(%d,%d) (%d,%d) (%d,%d)",
                     i, confidence, x, y, w, h, facial_data[5], facial_data[6],
                     facial_data[7], facial_data[8], facial_data[9],
                     facial_data[10], facial_data[11], facial_data[12],
                     facial_data[13], facial_data[14]);
    }
  }
}

/* GstBaseTransform vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn gst_face_sticker_transform_ip(GstBaseTransform *base,
                                                   GstBuffer *outbuf) {
  GstFaceSticker *filter = GST_FACESTICKER(base);
  GstVideoFrame frame;
  int map_flags;

  if (GST_CLOCK_TIME_IS_VALID(GST_BUFFER_TIMESTAMP(outbuf))) {
    gst_object_sync_values(GST_OBJECT(filter), GST_BUFFER_TIMESTAMP(outbuf));
  }

  map_flags = GST_MAP_READ | GST_VIDEO_FRAME_MAP_FLAG_NO_REF;
  if (!gst_base_transform_is_passthrough(base)) {
    map_flags |= GST_MAP_WRITE;
  }

  if (!gst_video_frame_map(&frame, &filter->in_info, outbuf,
                           (GstMapFlags)map_flags)) {
    GST_ERROR_OBJECT(filter, "Failed to map video frame");
    return GST_FLOW_ERROR;
  }

  cv::Mat frame_mat(filter->in_info.height, filter->in_info.width, CV_8UC3,
                    frame.data[0]);

  process_face_detection(filter, frame_mat);

  gst_video_frame_unmap(&frame);

  return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin *facesticker) {
  return GST_ELEMENT_REGISTER(face_sticker, facesticker);
}

/* gstreamer looks for this structure to register plugins
 *
 * FIXME:exchange the string 'Template plugin' with you plugin description
 */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, facesticker,
                  "Face detection and sticker application plugin", plugin_init,
                  "0.1", "LGPL", PACKAGE,
                  "https://github.com/Sanjar0126/gstreamer-face-sticker-plugin")