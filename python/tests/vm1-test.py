#!/usr/bin/env python3
import sys
import gi   
import logging
import os
import platform
import pykms
import threading

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
        
        self.mainloop = GLib.MainLoop()

    # Element 0 will be displayed on HDMI0
    def addPlayer0(self, player):
        player.addDrmInfo(self.fd, self.plane0, self.conn0)

    # Element 1 will be displayed on HDMI1
    def addPlayer1(self, element):
        element.addDrmInfo(self.fd, self.plane1, self.conn1)

    def start(self):
        self.mainloop.run()

class BaseElement:
    def __init__(self):
        self.pipeline = Gst.Pipeline.new()       
        self.sink = None
        self.kmssink_fd = None
        if platform.machine() == "aarch64":
            self.sink = Gst.ElementFactory.make("kmssink")
            self.sink.set_property("skip-vsync", "true")
        else:
            self.sink = Gst.ElementFactory.make("autovideosink")

        # Create bus and connect several handlers
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message::state-changed', self.onStateChanged)
        self.bus.connect('message::eos', self.onEos)
        self.bus.connect('message::error', self.onError)

    def __del__(self):
        self.pipeline.set_state(Gst.State.NULL)
    
    def onEos(self, bus, msg):
        print ('on_eos')
        #self.mainloop.quit()
        #self.pipeline.set_state(Gst.State.NULL)
			
    def onError(self, bus, msg):
        error = msg.parse_error()
        print ('onError:', error[1])
        #self.mainloop.quit()
        #self.pipeline.set_state(Gst.State.NULL)

    def onStateChanged(self, bus, msg):
        oldState, newState, pendingState = msg.parse_state_changed()
        # print((
        #     f"Bus call: Pipeline state changed from {oldState.value_nick} to {newState.value_nick} "
        #     f"(pending {pendingState.value_nick})"
        # ))
        #if newState == Gst.State.NULL:
        #    print("State is NULL")
        #    #self.pipeline.set_state(Gst.State.READY)
        

    def start(self):
        self.addToPipeline()
        self.pipeline.set_state(Gst.State.READY)
        self.pipeline.set_state(Gst.State.PLAYING)

    def stop(self):
        self.pipeline.set_state(Gst.State.NULL)
        #self.pipeline.set_state(Gst.State.PAUSED)
        #self.pipeline.set_state(Gst.State.READY)
    
    def addDrmInfo(self, fd, plane, conn):
        self.sink.set_property("fd", fd)
        self.sink.set_property("connector-id", conn.id)
        self.sink.set_property("plane-id", plane.id)

    def addToPipeline(self):
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
        self.parse.set_property("framerate", Gst.Fraction(30/1))
        
        self.convert = Gst.ElementFactory.make("v4l2convert")
        self.queue = Gst.ElementFactory.make("queue")

    def addToPipeline(self, container):
        self.pipeline.add(self.source)
        self.pipeline.add(self.filter)
        self.pipeline.add(self.parse)
        self.pipeline.add(self.convert)
        self.pipeline.add(self.queue)
        self.pipeline.add(self.sink)

        if not self.source.link(self.filter):
            logger.error("Link Error: source -> filter")
            return
        
        if not self.filter.link(self.parse):
            logger.error("Link Error: filter -> parse")
            return
        
        if not self.parse.link(self.convert):
            logger.error("Link Error: parse -> convert")
            return
        
        if not self.convert.link(self.queue):
            logger.error("Link Error: convert -> queue")
            return
        
        if not self.queue.link(self.sink):
            logger.error("Link Error: queue -> sink")
            return
        
class VideoElement(BaseElement):
    def __init__(self):
        super().__init__()
        self.source = Gst.ElementFactory.make("filesrc")
        self.decode = Gst.ElementFactory.make("decodebin")
        self.sink = None
        self.kmssink_fd = None
        if platform.machine() == "aarch64":
            self.sink = Gst.ElementFactory.make("kmssink")
            self.sink.set_property("skip-vsync", "true")
        else:
            self.sink = Gst.ElementFactory.make("autovideosink")
        
        #self.addToPipeline()

    def onPadAdded(self, dbin, pad):
        decode = pad.get_parent()
        pipeline = decode.get_parent()
        decode.link(self.sink)

    def setSourceFile(self, srcFileName):
        #self.stop()
        self.source.set_property('location', srcFileName)

    def setAlpha(self, alpha):
        self.sink.set_property("plane-properties", Gst.Structure("s,alpha=0xf000"))
        
    def addToPipeline(self):
        self.pipeline.add(self.source)
        self.pipeline.add(self.decode)
        self.pipeline.add(self.sink)

        if not self.source.link(self.decode) :
            logger.error("Link Error: source -> decode")

        self.decode.connect("pad-added", self.onPadAdded)

class BasePlayer():
    def __init__(self):
        self.fd = None
        self.plane = None
        self.conn = None
        self.element = VideoElement()

    def addDrmInfo(self, fd, plane, conn):
        self.fd = fd
        self.plane = plane
        self.conn = conn
        
class VideoPlayer(BasePlayer):
    def __init__(self):
        super().__init__()
        element = VideoElement()

    def play(self, fileName):
        self.element.stop()
        self.element = VideoElement()
        self.element.addDrmInfo(self.fd, self.plane, self.conn)
        self.element.setSourceFile(fileName)
        self.element.start()
        
    def stop(self):
        self.element.stop()

index = 0
videoElements = []
vmOne = VMOneContainer()
videoPlayer = VideoPlayer()
fileNames = ["videos/BlenderReel_1080p.mp4", "videos/BlenderReel2_1080p.mp4"]


def test():
    global index
    fileName = fileNames[index]   
    videoPlayer.play(fileName)
    
    GLib.timeout_add_seconds(2, test)
    index = index + 1
    index = index % len(fileNames) 

def main():
    # show video on first display
    vmOne.addPlayer0(videoPlayer)
    test()

    # videoElements.append(VideoElement())
    # hdmiElement = HdmiElement()
    
    # show HDMI input on first display
    #vmOne.addElement0(hdmiElement)

    # show video on second display
    #vmOne.addElement1(videoElement1)

    # show HDMI-input on second display
    #vmOne.addElement1(hdmiElement)
    
    vmOne.start()

main()