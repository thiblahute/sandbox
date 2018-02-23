#!/usr/bin/python3
import gi
import os
import sys

gi.require_version('Gst', '1.0')
gi.require_version('GstApp', '1.0')

from gi.repository import GLib
from gi.repository import Gst
from gi.repository import GstApp

Gst.init(None)


def decodebin_pad_added_cb(decodebin, srcpad, sinkpad):
    Gst.error("Yay linking %s and %s" %
              (srcpad.props.name, sinkpad.props.name))
    srcpad.link(sinkpad)


def bus_call(bus, message, loop):
    t = message.type
    if t == Gst.MessageType.EOS:
        print("End-of-stream\n")
        loop.quit()
    elif t == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        print("Error: %s: %s\n" % (err, debug))
        loop.quit()
    return True


fnum = 0
pipeline = Gst.ElementFactory.make("pipeline", None)


def push_buffer(src, ml):
    global fnum
    global pipeline
    fname = "H264/%05d" % fnum
    try:
        with open(fname, "rb") as f:
            buf = Gst.Buffer.new_wrapped(list(f.read()))
    except FileNotFoundError:
        ml.quit()
        return

    fnum += 1
    print("Pushed %s: %s" % (fname, src.push_sample(
        Gst.Sample.new(buf, Gst.Caps("video/x-h264"), None, None))))

    Gst.debug_bin_to_dot_file(
        pipeline, Gst.DebugGraphDetails.ALL, os.path.basename(fname))

    return True


src = Gst.ElementFactory.make("appsrc", None)
decodebin = Gst.ElementFactory.make("decodebin", None)
sink = Gst.ElementFactory.make("autovideosink", None)

pipeline.add(src, decodebin, sink)
src.link(decodebin)

sinkpad = sink.get_static_pad("sink")
decodebin.connect("pad-added", decodebin_pad_added_cb, sinkpad)

ml = GLib.MainLoop.new(None, False)
bus = pipeline.get_bus()
bus.add_signal_watch()
bus.connect("message", bus_call, ml)
GLib.timeout_add(20, push_buffer, src, ml)

pipeline.set_state(Gst.State.PLAYING)
ml.run()
