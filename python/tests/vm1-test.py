#!/usr/bin/env python3
import sys
import gi
import logging
import os


gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gst', '1.0')


from gi.repository import Gst, GObject, GLib


logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

def on_new_decoded_pad(dbin, pad):
    decode = pad.get_parent()
    pipeline = decode.get_parent()
    sink = pipeline.get_by_name('sink')
    decode.link(sink)
    pipeline.set_state(Gst.State.PLAYING)
    

def main():
    # get and print the current working directory
    current_working_directory = os.getcwd()
    print(current_working_directory)

    # initialize GStreamer
    Gst.init(sys.argv[1:])

    # create Gst elements
    source = Gst.ElementFactory.make("filesrc", "source")
    source.set_property('location', './videos/BlenderReel_1080p.mp4')
    decode = Gst.ElementFactory.make("decodebin", "decode")
    sink = Gst.ElementFactory.make("autovideosink", "sink")

    fd = None
    
    # create empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    if not pipeline or not source or not sink or not decode :
        logger.error("Not all elements could be created")
        sys.exit(1)


    # Build the pipeline
    pipeline.add(source)
    pipeline.add(decode)
    pipeline.add(sink)

    if not source.link(decode):
        logger.error("Link Error: source -> decode")
        sys.exit(1)

    decode.connect("pad-added", on_new_decoded_pad)
    ret = pipeline.set_state(Gst.State.PAUSED)

    if ret == Gst.StateChangeReturn.FAILURE:
        logger.error("Unable to set the pipeline to the playing state.")
        sys.exit(1)


    bus = pipeline.get_bus()
    msg = bus.timed_pop_filtered(Gst.CLOCK_TIME_NONE, Gst.MessageType.ERROR | Gst.MessageType.EOS)

    # Parse message
    if msg:
        if msg.type == Gst.MessageType.ERROR:
            err, debug_info = msg.parse_error()
            logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
            logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
        elif msg.type == Gst.MessageType.EOS:
            logger.info("End-Of-Stream reached.")
        else:
            # This should not happen as we only asked for ERRORs and EOS
            logger.error("Unexpected message received.")

    pipeline.set_state(Gst.State.NULL)

main()