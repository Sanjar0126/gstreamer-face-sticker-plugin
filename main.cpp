#include <glib-object.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/gstmessage.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  GstElement *pipeline;
  GstElement *source;
  GstElement *videoconvert;
  GstElement *buffer_filter;
  GstElement *facedetect;
  GstElement *videoconvert2;
  GstElement *videosink;
  GMainLoop *loop;
} FaceDetectApp;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);

int main(int argc, char *argv[]) {
  FaceDetectApp app = {0};
  GstBus *bus;
  GstStateChangeReturn ret;

  gst_init(&argc, &argv);

  app.loop = g_main_loop_new(NULL, FALSE);

  app.pipeline = gst_pipeline_new("face-detection-pipeline");
  app.source = gst_element_factory_make("v4l2src", "camera-source");
  app.videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  app.buffer_filter = gst_element_factory_make("bufferfilter", "bufferfilter");
  app.facedetect = gst_element_factory_make("facedetect", "facedetect");
  app.videoconvert2 = gst_element_factory_make("videoconvert", "videoconvert2");
  app.videosink = gst_element_factory_make("xvimagesink", "videosink");

  if (!app.pipeline || !app.source || !app.videoconvert || !app.facedetect ||
      !app.videoconvert2 || !app.videosink || !app.buffer_filter) {
    g_printerr("Some elements could not be created. Exiting.\n");
    return -1;
  }

  g_object_set(app.buffer_filter, "silent", FALSE, NULL);
  g_object_set(app.buffer_filter, "analysis-interval", 30, NULL); 
  g_object_set(app.source, "device", "/dev/video0", NULL);
  g_object_set(app.facedetect, "display", TRUE, NULL);
  g_object_set(app.facedetect, "min-size-width", 30, NULL);
  g_object_set(app.facedetect, "min-size-height", 30, NULL);
  g_object_set(app.facedetect, "updates", 0, NULL);
  g_object_set(app.videosink, "sync", FALSE, NULL); 

  gst_bin_add_many(GST_BIN(app.pipeline), app.source, app.videoconvert,
                   app.buffer_filter, app.facedetect, 
                   app.videoconvert2, app.videosink, NULL);

  if (!gst_element_link_many(app.source, app.videoconvert, app.buffer_filter,
                             app.facedetect, app.videoconvert2, 
                             app.videosink, NULL)) {
    g_printerr("Elements could not be linked. Exiting.\n");
    return -1;
  }

  bus = gst_pipeline_get_bus(GST_PIPELINE(app.pipeline));
  gst_bus_add_watch(bus, bus_call, &app);
  gst_object_unref(bus);

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

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  FaceDetectApp *app = (FaceDetectApp *)data;
  
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError *err;
      gchar *debug;
      
      gst_message_parse_error(msg, &err, &debug);
      g_printerr("Error: %s\n", err->message);
      g_error_free(err);
      g_free(debug);
      
      g_main_loop_quit(app->loop);
      break;
    }
    
    case GST_MESSAGE_EOS:
      g_print("End of stream\n");
      g_main_loop_quit(app->loop);
      break;
      
    case GST_MESSAGE_APPLICATION: {
      const GstStructure *structure = gst_message_get_structure(msg);
      
      if (gst_structure_has_name(structure, "buffer-filter-caps")) {
        const gchar *caps_str;
        gint width, height;
        
        caps_str = gst_structure_get_string(structure, "caps");
        if (caps_str && 
            gst_structure_get_int(structure, "width", &width) &&
            gst_structure_get_int(structure, "height", &height)) {
          
          g_print("BufferFilter: New video format: %dx%d\n", width, height);
          g_print("  Caps: %s\n", caps_str);
        }
      }
      else if (gst_structure_has_name(structure, "buffer-filter-analysis")) {
        guint frame_number;
        guint64 timestamp;
        guint buffer_size;
        gdouble average_value;
        
        if (gst_structure_get_uint(structure, "frame-number", &frame_number) &&
            gst_structure_get_uint64(structure, "timestamp", &timestamp) &&
            gst_structure_get_uint(structure, "buffer-size", &buffer_size) &&
            gst_structure_get_double(structure, "average-value", &average_value)) {
          
          if (frame_number % 30 == 0) {
            g_print("BufferFilter: Frame #%u, Size: %u bytes, Avg value: %.2f\n", 
                    frame_number, buffer_size, average_value);
          }
        }
      }
      break;
    }
    
    default:
      break;
  }
  
  return TRUE;
}