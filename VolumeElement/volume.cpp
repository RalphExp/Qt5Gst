#include <stdio.h>
#include <gst/gst.h>

int main(int argc, char* argv[]) {
    GstElement *pipeline = nullptr;
    GstElement *volume = nullptr;
    GstMessage * msg = nullptr;
    GstBus* bus = nullptr;

    gst_init(&argc, &argv);

    pipeline = gst_parse_launch("osxaudiosrc device=79 do-timestamp=true name=source ! "
        "capsfilter name=source_caps caps=audio/x-raw,layout=interleaved,rate=48000,channels=1,format=S16LE ! "
        "audioconvert ! level name=_lvl interval=10000000 ! "
        "volume name=Play/audio/remote_vol volume=1 mute=false ! queue name=_queue ! "
        "speed name=_speed ! audiorate  ! audioresample ! audioconvert  ! "
        "capsfilter name=sink_caps caps=audio/x-raw ! osxaudiosink buffer-time=150000 name=sink sync=false", NULL);

    volume = gst_bin_get_by_name(GST_BIN(pipeline), "volume");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus (pipeline);
    msg = gst_bus_timed_pop_filtered (bus, 
        GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        g_error ("An error occurred! Re-run with the GST_DEBUG=*:WARN environment "
            "variable set for more details.");
    }

    /* Free resources */
    gst_message_unref (msg);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}


