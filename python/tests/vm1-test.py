#!/usr/bin/env python3
import sys
import gi
import logging
import os
import platform


gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gst', '1.0')


from gi.repository import Gst, GObject, GLib


logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

class VMOneContainer:
    def __init__(self):
        Gst.init(sys.argv[1:])
        self.pipeline = Gst.Pipeline.new("test-pipeline")
        self.mainloop = GLib.MainLoop()
        # Create bus and connect several handlers
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message::eos', self.on_eos)
        #self.bus.connect('message::tag', self.on_tag)
        self.bus.connect('message::error', self.on_error)
        self.elements = []
    
    def on_eos(self, bus, msg):
        print ('on_eos')
        self.mainloop.quit()
        self.pipeline.set_state(Gst.State.NULL)
			
    def on_tag(self, bus, msg):
        pass
        taglist = msg.parse_tag()
        print ('on_tag:')
        for key in taglist.keys():
            print ('\t%s = %s' % (key, taglist[key]))
					
    def on_error(self, bus, msg):
        error = msg.parse_error()
        print ('on_error:', error[1])
        self.mainloop.quit()
        self.pipeline.set_state(Gst.State.NULL)

    def addVideoElement(self, videoElement):
        videoElement.addToPipeline(self)

    def start(self):
        self.pipeline.set_state(Gst.State.PLAYING)
        self.mainloop.run()


class VideoElement:
    def __init__(self, file):
        self.source = Gst.ElementFactory.make("filesrc", "source")
        self.source.set_property('location', file)
        self.decode = Gst.ElementFactory.make("decodebin", "decode")
        self.sink = None
        self.kmssink_fd = None
        if platform.machine == "aarch64":
            self.sink = Gst.ElementFactory.make("kmssink", "sink")
            self.kmssink_fd = self.sink.get_property("fd")
            self.kmssink_plane_id = self.sink.get_property("plane-id")
        else:
            self.sink = Gst.ElementFactory.make("autovideosink", "sink")

    def on_new_decoded_pad(self, dbin, pad):
        print("New Pad")
        decode = pad.get_parent()
        pipeline = decode.get_parent()
        sink = pipeline.get_by_name('sink')
        decode.link(sink)

    def addToPipeline(self, container):
        container.pipeline.add(self.source)
        container.pipeline.add(self.decode)
        container.pipeline.add(self.sink)
        if not self.source.link(self.decode):
            logger.error("Link Error: source -> decode")

        self.decode.connect("pad-added", self.on_new_decoded_pad)

def main():
    vmOne = VMOneContainer()
    videoElement1 = VideoElement("./videos/BlenderReel_1080p.mp4")
    videoElement2 = VideoElement("./videos/BlenderReel_1080p.mp4")
    vmOne.addVideoElement(videoElement1)
    vmOne.addVideoElement(videoElement2)
    vmOne.start()

def on_new_decoded_pad(dbin, pad):
    decode = pad.get_parent()
    pipeline = decode.get_parent()
    sink = pipeline.get_by_name('sink')
    decode.link(sink)
    pipeline.set_state(Gst.State.PLAYING)
    

def main_old():
    # get and print the current working directory
    current_working_directory = os.getcwd()
    print(current_working_directory)

    # initialize GStreamer
    Gst.init(sys.argv[1:])

    # create pipeline elements
    source = Gst.ElementFactory.make("filesrc", "source")
    source.set_property('location', './videos/BlenderReel_1080p.mp4')
    decode = Gst.ElementFactory.make("decodebin", "decode")
    sink = None

    # create kmssink only on Raspberry Pi
    kmssink_fd = None
    kmssink_plane_id = None
    if platform.machine == "aarch64":
        sink = Gst.ElementFactory.make("kmssink", "sink")
        kmssink_fd = sink.get_property("fd")
        kmssink_plane_id = sink.get_property("plane-id")
    else:
        sink = Gst.ElementFactory.make("autovideosink", "sink")

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