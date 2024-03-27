#include <stdio.h>
#include <gst/gst.h>
#include <string.h>

#include <algorithm>
#include <string>

using namespace std;

GstElement* pipeline = nullptr;
GstElement* volume = nullptr;
GstMessage* msg = nullptr;
GstBus* bus = nullptr;
GIOChannel* io_stdin = nullptr;
GMainLoop* main_loop = nullptr;

const char* tpl = "filesrc location=../resource/%s name=source ! "
    "decodebin ! audioconvert ! level name=_lvl interval=10000000 ! "
    "volume name=test_vol volume=%f mute=false ! "
    "speed name=_speed ! audiorate ! audioresample ! audioconvert ! capsfilter name=sink_caps caps=audio/x-raw ! "
    "osxaudiosink buffer-time=150000 name=sink sync=false";

const char* file = nullptr;

int vol = 50;

static gboolean handle_keyboard(GIOChannel* source, GIOCondition cond, void*);

static gboolean handle_message(GstBus* bus, GstMessage * msg, GstElement* pipeline);

string build_pipeline() {
    char buff[1024] = {0};
    sprintf(buff, tpl, file, vol/100.0);
    return buff;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && strcmp(argv[1], "1") == 0) {
        file = "ring.wav";
    } else {
        file = "audio_test.wav";
    }

    gst_init(&argc, &argv);
    pipeline = gst_parse_launch(build_pipeline().c_str(), NULL);

    volume = gst_bin_get_by_name(GST_BIN(pipeline), "test_vol");
    bus = gst_element_get_bus (pipeline);
    gst_bus_add_watch(bus, (GstBusFunc) handle_message, pipeline);

    io_stdin = g_io_channel_unix_new(fileno (stdin));
    g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, NULL);

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    /* Free resources */
    g_main_loop_unref(main_loop);
    g_io_channel_unref(io_stdin);
    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}

/* Process messages from GStreamer */
static gboolean handle_message(GstBus* bus, GstMessage* msg, GstElement* pipeline)
{
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from element %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information: %s\n",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        g_main_loop_quit (main_loop);
        break;

    case GST_MESSAGE_EOS:
        g_print ("End-Of-Stream reached.\n");
        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref(volume);
        gst_object_unref(bus);
        gst_object_unref(pipeline);
        
        pipeline = gst_parse_launch(build_pipeline().c_str(), NULL);
        volume = gst_bin_get_by_name(GST_BIN(pipeline), "test_vol");
        gst_element_set_state (pipeline, GST_STATE_PLAYING);

        bus = gst_element_get_bus (pipeline);
        gst_bus_add_watch(bus, (GstBusFunc) handle_message, pipeline);
        break;
    default:
        break;
    }

    /* We want to keep receiving messages */
    return true;
}

static gboolean handle_keyboard(GIOChannel* source, GIOCondition cond, void*) {
    gchar *str = NULL;
    
    if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
        vol = g_ascii_strtoull(str, NULL, 0);

        vol = std::min(100, std::max(0, vol));

        // volume = gst_bin_get_by_name(GST_BIN(pipeline), "test_vol");
        if (vol < 0 || vol > 100) {
            g_printerr ("Index out of bounds\n");
        } else {
            /* If the input was a valid audio stream index, set the current audio stream */
            double f = vol/100.0;
            g_print ("Setting volume to %f\n", f);
            g_object_set(volume, "volume", f, NULL);

            double g = 0;
            g_object_get(volume, "volume", &g, NULL);
            g_printerr("Getting volume: %f\n", g);
        }
        // gst_object_unref(volume);
    }
    g_free (str);
    return TRUE;
}
