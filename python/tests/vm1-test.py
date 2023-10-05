#!/usr/bin/env python3
import sys
import gi   
import logging
import os
import platform
import pykms

gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gst', '1.0')

from gi.repository import Gst, GObject, GLib

logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

class VMOneContainer:
    def __init__(self):        
        self.card = pykms.Card()
        self.res = pykms.ResourceManager(self.card)

        self.fd = self.card.fd
        print("File Descriptor: ", self.fd)
        
        # Create plane for first screen
        self.conn0 = self.res.reserve_connector("HDMI-A-1")
        self.crtc0 = self.res.reserve_crtc(self.conn0)
        self.plane0 = self.res.reserve_overlay_plane(self.crtc0)
        print("HDMI-A-1 Connector ID: ", self.conn0.id)

        # Create plane for second screen
        self.conn1 = self.res.reserve_connector("HDMI-A-2")
        self.crtc1 = self.res.reserve_crtc(self.conn1)
        self.plane1 = self.res.reserve_overlay_plane(self.crtc1)
        print("HDMI-A-1 Connector ID: ", self.conn1.id)

        # Initialize Gst and create pipeline
        Gst.init(sys.argv[1:])
        print("Gst Version: %s", Gst.version())
        self.pipeline = Gst.Pipeline.new("main-pipeline")

        self.mainloop = GLib.MainLoop()

        # Create bus and connect several handlers
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message::eos', self.onEos)
        #self.bus.connect('message::tag', self.onTag)
        self.bus.connect('message::error', self.onError)
    
    def onEos(self, bus, msg):
        print ('on_eos')
        self.mainloop.quit()
        self.pipeline.set_state(Gst.State.NULL)
			
    def onTag(self, bus, msg):
        taglist = msg.parse_tag()
        print ('on_tag:')
        for key in taglist.keys():
            print ('\t%s = %s' % (key, taglist[key]))
					
    def onError(self, bus, msg):
        error = msg.parse_error()
        print ('onError:', error[1])
        self.mainloop.quit()
        self.pipeline.set_state(Gst.State.NULL)

    # Element 0 will be displayed on HDMI0
    def addElement0(self, element):
        element.addDrmInfo(self.fd, self.plane0, self.conn0)
        element.addToPipeline(self)

    # Element 1 will be displayed on HDMI1
    def addElement1(self, element):
        element.addDrmInfo(self.fd, self.plane1, self.conn1)
        element.addToPipeline(self)

    def start(self):
        self.pipeline.set_state(Gst.State.PLAYING)
        self.mainloop.run()

class BaseElement:
    def __init__(self):
        self.sink = None
        self.kmssink_fd = None
        if platform.machine() == "aarch64":
            self.sink = Gst.ElementFactory.make("kmssink")
            self.sink.set_property("skip-vsync", "true")
        else:
            self.sink = Gst.ElementFactory.make("autovideosink")

    def addDrmInfo(self, fd, plane, conn):
        self.sink.set_property("fd", fd)
        self.sink.set_property("connector-id", conn.id)
        self.sink.set_property("plane-id", plane.id)

    def addToPipeline(self, container):
        pass

class HdmiElement(BaseElement):
    def __init__(self):
        super().__init__()
        self.source = Gst.ElementFactory.make("v4l2src")
        
        caps = Gst.Caps("video/x-raw,framerate=30/1,colorimetry=bt601")
        self.filter = Gst.ElementFactory.make("capsfilter")
        self.filter.set_property("caps", caps)
        
        self.parse = Gst.ElementFactory.make("rawvideoparse")
        self.parse.set_property("format", "uyvy")
        self.parse.set_property("width", 1920)
        self.parse.set_property("height", 1080)
        self.parse.set_property("framerate", 30/1)
        
        self.convert = Gst.ElementFactory.make("v4l2convert")
        self.queue = Gst.ElementFactory.make("queue")

    def addToPipeline(self, container):
        container.pipeline.add(self.source)
        container.pipeline.add(self.filter)
        container.pipeline.add(self.parse)
        container.pipeline.add(self.convert)
        container.pipeline.add(self.queue)
        container.pipeline.add(self.sink)

        if not self.source.source(self.filter):
            logger.error("Link Error: source -> filter")
            return
        
        if not self.source.filter(self.parse):
            logger.error("Link Error: filter -> parse")
            return
        
        if not self.source.parse(self.convert):
            logger.error("Link Error: parse -> convert")
            return
        
        if not self.source.convert(self.queue):
            logger.error("Link Error: convert -> queue")
            return
        
        if not self.source.queue(self.sink):
            logger.error("Link Error: queue -> sink")
            return
        
class VideoElement(BaseElement):
    def __init__(self, file):
        super().__init__()
        self.source = Gst.ElementFactory.make("filesrc")
        self.source.set_property('location', file)
        self.decode = Gst.ElementFactory.make("decodebin")
        self.sink = None
        self.kmssink_fd = None
        if platform.machine() == "aarch64":
            self.sink = Gst.ElementFactory.make("kmssink")
            self.sink.set_property("skip-vsync", "true")
        else:
            self.sink = Gst.ElementFactory.make("autovideosink")

    def onPadAdded(self, dbin, pad):
        decode = pad.get_parent()
        pipeline = decode.get_parent()
        decode.link(self.sink)
        
    def addToPipeline(self, container):
        container.pipeline.add(self.source)
        container.pipeline.add(self.decode)
        container.pipeline.add(self.sink)

        if not self.source.link(self.decode) :
            logger.error("Link Error: source -> decode")

        self.decode.connect("pad-added", self.onPadAdded)

def main():
    vmOne = VMOneContainer()
    videoElement0 = VideoElement("./videos/BlenderReel_1080p.mp4")
    #videoElement1 = VideoElement("./videos/BlenderReel_1080p.mp4")
    hdmiElement = HdmiElement()
    vmOne.addElement0(videoElement0)
    #vmOne.addElement1(videoElement1)
    vmOne.addElement1(hdmiElement)
    vmOne.start()

main()