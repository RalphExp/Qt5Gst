#include <stdio.h>
#include <gst/gst.h>

GstElement* pipeline = nullptr;
GstElement* volume = nullptr;
GstMessage* msg = nullptr;
GstBus* bus = nullptr;
GIOChannel* io_stdin = nullptr;
GMainLoop* main_loop = nullptr;

static gboolean handle_keyboard(GIOChannel* source, GIOCondition cond, GstElement* volume);

static gboolean handle_message(GstBus* bus, GstMessage * msg, GstElement* pipeline);

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    pipeline = gst_parse_launch("osxaudiosrc device=79 do-timestamp=true name=source ! "
        "capsfilter name=source_caps caps=audio/x-raw,layout=interleaved,rate=48000,channels=1,format=S16LE ! "
        "audioconvert ! level name=_lvl interval=10000000 ! "
        "volume name=my_vol volume=1 mute=false ! queue name=_queue ! "
        "speed name=_speed ! audiorate  ! audioresample ! audioconvert ! "
        "capsfilter name=sink_caps caps=audio/x-raw ! osxaudiosink buffer-time=150000 name=sink sync=false", NULL);

    volume = gst_bin_get_by_name(GST_BIN(pipeline), "my_vol");
    bus = gst_element_get_bus (pipeline);
    gst_bus_add_watch(bus, (GstBusFunc) handle_message, pipeline);

    io_stdin = g_io_channel_unix_new(fileno (stdin));
    g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, volume);

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
        g_main_loop_quit (main_loop);
        break;
    default:
        break;
    }

    /* We want to keep receiving messages */
    return true;
}

static gboolean handle_keyboard(GIOChannel* source, GIOCondition cond, GstElement* volume) {
    gchar *str = NULL;
    int vol = -1;

    if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
        vol = g_ascii_strtoull(str, NULL, 0);

        if (vol < 0 || vol > 100) {
            g_printerr ("Index out of bounds\n");
        } else {
            /* If the input was a valid audio stream index, set the current audio stream */
            double f = vol/100.0;
            g_print ("Setting volume to %f\n", f);
            g_object_set(volume, "volume", &f, NULL);
        }
    }
    g_free (str);
    return TRUE;
}
