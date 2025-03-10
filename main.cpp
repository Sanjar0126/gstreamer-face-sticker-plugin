#include <glib-object.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/gstmessage.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>

typedef struct {
  GstElement *pipeline;
  GstElement *source;
  GstElement *videoconvert;
  GstElement *facefilter_sticker;
  GstElement *videoconvert2;
  GstElement *videosink;
  GMainLoop *loop;
} FaceDetectApp;

typedef struct {
  gboolean SILENT;
  gchar* EYEIMG_PATH;
  gfloat EYEIMG_SCALE;
  gint MIN_CONFIDENCE;
} Arguments;

static Arguments parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  Arguments args = parse_args(argc, argv);

  FaceDetectApp app = {0};
  GstBus *bus;
  GstStateChangeReturn ret;

  gst_init(&argc, &argv);

  app.loop = g_main_loop_new(NULL, FALSE);

  app.pipeline = gst_pipeline_new("face-detection-pipeline");
  app.source = gst_element_factory_make("v4l2src", "camera-source");
  app.videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  app.facefilter_sticker =
      gst_element_factory_make("face_sticker", "face_sticker");
  app.videoconvert2 = gst_element_factory_make("videoconvert", "videoconvert2");
  app.videosink = gst_element_factory_make("xvimagesink", "videosink");

  if (!app.pipeline || !app.source || !app.videoconvert || !app.videoconvert2 ||
      !app.videosink || !app.facefilter_sticker) {
    g_printerr("Some elements could not be created. Exiting.\n");
    return -1;
  }

  g_object_set(app.facefilter_sticker, "silent", args.SILENT, NULL);
  g_object_set(app.facefilter_sticker, "eye_img_path", args.EYEIMG_PATH, NULL);
  g_object_set(app.facefilter_sticker, "eye_img_scale", args.EYEIMG_SCALE, NULL);
  g_object_set(app.facefilter_sticker, "min_confidence", args.MIN_CONFIDENCE, NULL);
  g_object_set(app.source, "device", "/dev/video0", NULL);
  g_object_set(app.videosink, "sync", FALSE, NULL);

  gst_bin_add_many(GST_BIN(app.pipeline), app.source, app.videoconvert,
                   app.facefilter_sticker, app.videoconvert2, app.videosink,
                   NULL);

  if (!gst_element_link_many(app.source, app.videoconvert,
                             app.facefilter_sticker, app.videoconvert2,
                             app.videosink, NULL)) {
    g_printerr("Elements could not be linked. Exiting.\n");
    return -1;
  }

  g_print("Starting pipeline...\n");
  ret = gst_element_set_state(app.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    return -1;
  }

  g_print("Running...\n");
  g_main_loop_run(app.loop);

  g_print("Stopping pipeline...\n");
  gst_element_set_state(app.pipeline, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT(app.pipeline));
  g_main_loop_unref(app.loop);

  return 0;
}

static Arguments parse_args(int argc, char *argv[]) {
  Arguments args;
  std::map<std::string, std::string> args_map;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    size_t pos = arg.find('=');

    if (pos != std::string::npos) {
      std::string key = arg.substr(0, pos);
      std::string value = arg.substr(pos + 1);
      args_map[key] = value;
    }
  }

  if (args_map.find("silent") != args_map.end()) {
    args.SILENT = args_map["silent"] == "TRUE";
  }
  if (args_map.find("eye_img_path") != args_map.end()) {
    args.EYEIMG_PATH = g_strdup(args_map["eye_img_path"].c_str());
  } else {
    args.EYEIMG_PATH = g_strdup("");
  }
  if (args_map.find("eye_img_scale") != args_map.end()) {
    args.EYEIMG_SCALE = std::stof(args_map["eye_img_scale"]);
  } else {
    args.EYEIMG_SCALE = 1.0;
  }
  if (args_map.find("min_confidence") != args_map.end()) {
    args.MIN_CONFIDENCE = std::stoi(args_map["min_confidence"]);
  } else {
    args.MIN_CONFIDENCE = 50;
  }

  return args;
}