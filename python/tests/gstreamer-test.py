#!/usr/bin/env python3
import sys
import gi
import logging


gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gst', '1.0')


from gi.repository import Gst, GObject, GLib


logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

# initialize GStreamer
Gst.init(sys.argv[1:])

# create Gst elements
source = Gst.ElementFactory.make("videotestsrc", "source")
sink = Gst.ElementFactory.make("autovideosink", "sink")
vertigotv = Gst.ElementFactory.make("vertigotv", "effects")
videoconvert = Gst.ElementFactory.make("videoconvert", "videoconvert0")

# create empty pipeline
pipeline = Gst.Pipeline.new("test-pipeline")

if not pipeline or not source or not sink or not vertigotv or not videoconvert:
    logger.error("Not all elements could be created")
    sys.exit(1)


# Build the pipeline
pipeline.add(source)
pipeline.add(sink)
pipeline.add(vertigotv)
pipeline.add(videoconvert)

if not source.link(vertigotv) or not vertigotv.link(videoconvert) or not videoconvert.link(sink):
    logger.error("Elements could not be linked.")
    sys.exit(1)

source.set_property('pattern', 0)
ret = pipeline.set_state(Gst.State.PLAYING)
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