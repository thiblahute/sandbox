#include <gst/gst.h>

static gboolean add_videosink(GstElement *pipeline, GstElement *tee)
{
    GstElement* queue1 = gst_element_factory_make("queue", NULL);
    GstElement* sink = gst_element_factory_make("autovideosink", NULL);

    gst_bin_add_many (GST_BIN (pipeline), queue1, sink, NULL);
    gst_element_sync_state_with_parent(queue1);
    gst_element_sync_state_with_parent(sink);
    gst_element_link_many (queue1, sink, NULL);
    gst_element_link (tee, queue1);
    return TRUE;
}

gboolean sink_added = FALSE;
GstElement *pipeline = NULL;
GstElement *tee = NULL;

static 
GstBusSyncReply sync_handler (GstBus *bus, GstMessage *msg, gpointer user_data)
{
    GST_ERROR ("%" GST_PTR_FORMAT, msg);
    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_STATE_CHANGED && GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
        GstState old, _new, next;
        gst_message_parse_state_changed(msg, &old, &_new, &next);
        if (_new == GST_STATE_READY && !sink_added) {
            GST_ERROR ("Addfing");
            sink_added = add_videosink(pipeline, tee);
        }
    }

    return GST_BUS_PASS;
}

int main (int argc, char ** argv) {
    gst_init (&argc, &argv);
    GstElement* src = gst_element_factory_make("v4l2src", NULL);
    GstElement* queue1 = gst_element_factory_make("queue", NULL);
    GstElement* appsink = gst_element_factory_make("appsink", NULL);

    tee = gst_element_factory_make("tee", NULL);
    pipeline = gst_element_factory_make("pipeline", NULL);
    gst_bin_add_many (GST_BIN (pipeline), src, tee, queue1, appsink, NULL);
    gst_element_link_many (src, tee, queue1, appsink, NULL);

    GstBus* bus = gst_element_get_bus(pipeline);
    g_assert (bus);
    gst_bus_set_sync_handler (bus, sync_handler, NULL, NULL);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    while (TRUE) {
        GstMessage* msg = gst_bus_timed_pop(bus, GST_SECOND);

        if (!msg)
            continue;

        if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
            gst_debug_bin_to_dot_file_with_ts(
                GST_BIN (pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "error");

            return 1;
        }

        if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_STATE_CHANGED && GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
                GstState old, _new, next;

                gst_message_parse_state_changed(msg, &old, &_new, &next);
                if (_new == GST_STATE_PLAYING && sink_added) {
                    return 0;
                }
                GST_ERROR ("%s", gst_element_state_get_name (_new));
                gst_debug_bin_to_dot_file_with_ts(
                    GST_BIN (pipeline), GST_DEBUG_GRAPH_SHOW_ALL, gst_element_state_get_name (_new));
        }
    }

    return 0;
}