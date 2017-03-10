//
//  main.c
//  MediaFormatPadcapabilities
//
//  Created by shuyj on 2017/3/9.
//  Copyright © 2017年 shuyj. All rights reserved.
//

#include <stdio.h>
#include <gst/gst.h>
gsize ntotalkb = 0;
GstClockTime ntotalTime = 0;
static GstPadProbeReturn on_rtmp_sink_data_flow (GstPad * pad, GstPadProbeInfo * info, gpointer user_data){
//    PushEngineImpl *self = (PushEngineImpl*)(user_data);
    GstBuffer *buffer;
    
    buffer = GST_PAD_PROBE_INFO_BUFFER (info);
    
    /* Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == NULL)
        return GST_PAD_PROBE_OK;
    
    GST_BUFFER_DURATION(buffer);
    GST_BUFFER_PTS(buffer);
    gsize nsize = gst_buffer_get_size(buffer);
    ntotalkb += nsize;
    ntotalTime += GST_BUFFER_DURATION(buffer);
    if( ntotalTime/1000000 >= 1000 ){
        printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-===============:::::    size:%luKB %luKbps dur:%lu\n", ntotalkb/1024,ntotalkb/1024*8, ntotalTime/1000000);
        ntotalkb = 0;
        ntotalTime = 0;
    }
    
//    self->ProbeRtmpDataSize(gst_buffer_get_size(buffer));
    
    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *vconvert, *encoder, *capfilter, *sink;
    
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    gst_debug_set_threshold_for_name("openh264enc", GST_LEVEL_DEBUG);
    
    /* Ask the factories to instantiate actual elements */
    source = gst_element_factory_make ("autovideosrc", "source");
    vconvert = gst_element_factory_make ("videoconvert", "vconvert");
    encoder = gst_element_factory_make ("openh264enc", "encoder");
    capfilter = gst_element_factory_make ("capsfilter", "capfilter");
    sink = gst_element_factory_make ("fakesink", "sink");
    
//    g_object_set(source, "is-live", 1, NULL);
    
    GstCaps* gcap = gst_caps_new_simple("video/x-h264",
//                        "bitrate", G_TYPE_UINT, 800000,
                                        NULL);
    
    g_object_set(capfilter, "caps", gcap, NULL);
    
    g_object_set(encoder, "rate-control", 1, NULL);
    g_object_set(encoder, "bitrate", 160000, NULL);
    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");
    
    if (!pipeline || !encoder || !capfilter || !source || !vconvert || !sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, vconvert, encoder, capfilter, sink, NULL);
    if (gst_element_link_many (source, vconvert, encoder, capfilter, sink, NULL) != TRUE) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (pipeline);
        return -1;
    }
    
    GstPad *sink_pad;
    gulong data_probe_id = -1;
    
    sink_pad = gst_element_get_static_pad (sink, "sink");
    data_probe_id = gst_pad_add_probe (sink_pad, GST_PAD_PROBE_TYPE_BUFFER, on_rtmp_sink_data_flow, NULL, NULL);
    gst_object_unref (sink_pad);
    
    /* Print initial negotiated caps (in NULL state) */
    g_print ("In NULL state:\n");
    
    /* Start playing */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state (check the bus for error messages).\n");
    }
    int delay_caps = 20;
    /* Wait until error, EOS or State Change */
    bus = gst_element_get_bus (pipeline);
    do {
        msg = gst_bus_timed_pop_filtered (bus, GST_SECOND, GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
                                          GST_MESSAGE_STATE_CHANGED);
        /* Parse message */
        if (msg != NULL) {
            GError *err;
            gchar *debug_info;
            
            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    /* We are only interested in state-changed messages from the pipeline */
                    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                        g_print ("\nPipeline state changed from %s to %s:\n",
                                 gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                        /* Print the current capabilities of the sink element */
                    }
                    break;
                default:
                    /* We should not reach here because we only asked for ERRORs, EOS and STATE_CHANGED */
                    g_printerr ("Unexpected message received.\n");
                    break;
            }
            gst_message_unref (msg);
        }
        else{
            delay_caps--;
            if( delay_caps == 20 ){
//                GstCaps* gcap = gst_caps_new_simple("video/x-h264",
//                                                    "bitrate", G_TYPE_UINT, 800000, NULL);
//                g_object_set(capfilter, "caps", gcap, NULL);
                g_object_set(encoder, "bitrate", 400000, NULL);
                printf("capsfilter set caps bitrate  800000 \n");
            }
            if( delay_caps == 10 ){
                GstCaps* cap1 = gst_caps_from_string("video/x-raw, width=(int)1280, height=(int)720, framerate=(fraction)25/1, format=(string)I420");
                GstCaps* caporig = gst_caps_copy( gst_pad_get_current_caps(gst_element_get_static_pad(vconvert, "src")) );
                printf("caporig-1:%s \n", gst_caps_to_string(caporig));
                //gst_caps_make_writable(caporig);
                gst_caps_set_simple (caporig, "framerate", GST_TYPE_FRACTION, 25, 1, NULL);
                printf("caporig-2:%s \n", gst_caps_to_string(caporig));
                printf("vconvert src caps-1:%s \n", gst_caps_to_string(gst_pad_get_current_caps(gst_element_get_static_pad(vconvert, "src")) ) );
                printf("encoder sink caps-1:%s \n", gst_caps_to_string(gst_pad_get_current_caps(gst_element_get_static_pad(encoder, "sink")) ) );
                printf("encoder src caps-1:%s \n", gst_caps_to_string(gst_pad_get_current_caps(gst_element_get_static_pad(encoder, "src")) ) );
                gst_pad_set_caps(gst_element_get_static_pad(vconvert, "src"), caporig);
                printf("vconvert src caps:%s \n", gst_caps_to_string(gst_pad_get_current_caps(gst_element_get_static_pad(vconvert, "src")) ) );
                printf("encoder sink caps:%s \n", gst_caps_to_string(gst_pad_get_current_caps(gst_element_get_static_pad(encoder, "sink")) ) );
                printf("encoder src caps:%s \n", gst_caps_to_string(gst_pad_get_current_caps(gst_element_get_static_pad(encoder, "src")) ) );
                
//                g_object_set(encoder, "bitrate", 800000, NULL);
//                GstEvent* capev = gst_event_new_caps(cap1);
//                gst_element_send_event(encoder, capev);
                printf("send event caps test \n");
                
            }
            if( delay_caps == 0 ){
//                g_object_set(encoder, "bitrate", 400000, NULL);
////                gst_event_new_navigation(gst_structure_new_empty("sempty"));
//                gst_element_send_event(encoder, gst_event_new_reconfigure() );
//                printf("send event reconfigure test \n");
            }
        }
    } while (!terminate);
    
    /* Free resources */
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}

