#!/usr/bin/env python3
import sys
import gi   
import logging
import os
import platform
import pykms
import threading
import tween
import enum

gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gst', '1.0')

from gi.repository import Gst, GObject, GLib

logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

class VMOneContainer:
    def __init__(self):        
        self._card = pykms.Card()
        self._res = pykms.ResourceManager(self._card)

        self._fd = self._card.fd
        print("File Descriptor: ", self._fd)
        
        # Create plane for first screen
        self._conn0 = self._res.reserve_connector("HDMI-A-1")
        self._crtc0 = self._res.reserve_crtc(self._conn0)
        self._planes0 = []
        print("HDMI-A-1 Connector ID: ", self._conn0.id)

        # Create plane for second screen
        self._conn1 = self._res.reserve_connector("HDMI-A-2")
        self._crtc1 = self._res.reserve_crtc(self._conn1)
        self._planes1 = []
        print("HDMI-A-1 Connector ID: ", self._conn1.id)

        mode0 = self._conn0.get_default_mode()
        w0 = mode0.hdisplay
        h0 = mode0.vdisplay

        mode1 = self._conn1.get_default_mode()
        w1 = mode0.hdisplay
        h1 = mode0.vdisplay

        fb0 = pykms.DumbFramebuffer(self._card, w0, h0, "AR24")
        pykms.draw_rect(fb0, 0, 0, w0, h0, pykms.RGB(128, 128, 128, 128))
        bg0 = self._res.reserve_generic_plane(self._crtc0)

        fb1 = pykms.DumbFramebuffer(self._card, w0, h0, "AR24")
        pykms.draw_rect(fb1, 0, 0, w0, h0, pykms.RGB(128, 128, 128, 128))
        bg1 = self._res.reserve_generic_plane(self._crtc1)

        self._card.disable_planes()

        bg0.set_props({
            "FB_ID": fb0.id,
            "CRTC_ID": self._crtc0.id,
            "SRC_W": fb0.width << 16,
            "SRC_H": fb0.height << 16,
            "CRTC_W": fb0.width,
            "CRTC_H": fb0.height,
            "zpos": 0,
        })

        bg1.set_props({
            "FB_ID": fb1.id,
            "CRTC_ID": self._crtc1.id,
            "SRC_W": fb1.width << 16,
            "SRC_H": fb1.height << 16,
            "CRTC_W": fb1.width,
            "CRTC_H": fb1.height,
            "zpos": 0,
        })

        # Initialize Gst and create pipeline
        Gst.init(sys.argv[1:])
        print("Gst Version: %s", Gst.version())

        # All players for HDMI-A
        self._player0 = None
        self._player1 = None

        self._addVideoPlayer0(VideoPlayer(self, True))
        self._addVideoPlayer1(VideoPlayer(self, True))

        self._mainloop = GLib.MainLoop()

    # Element 0 will be displayed on HDMI0
    def _addVideoPlayer0(self, player):
        plane = self._res.reserve_overlay_plane(self._crtc0)
        plane.set_props({"zpos": 2})
        self._planes0.append(plane)
        player.addDrmInfo(self._fd, plane, self._conn0)
        self._player0 = player

    # Element 1 will be displayed on HDMI1
    def _addVideoPlayer1(self, player):
        plane = self._res.reserve_overlay_plane(self._crtc1)
        plane.set_props({"zpos": 2})
        self._planes1.append(plane)
        player.addDrmInfo(self._fd, plane, self._conn1)
        self._player1 = player

    def start(self):
        self._mainloop.run()

    def playVideo0(self, fileName):
        if self._player0 != None:
            self._player0.play(fileName)

    def playVideo1(self, fileName):
        if self._player1 != None:
            self._player1.play(fileName)

class BaseElement:
    def __init__(self):
        self._plane = None
        self._pipeline = Gst.Pipeline.new()       
        self._sink = None
        self._kmssink_fd = None
        if platform.machine() == "aarch64":
            self._sink = Gst.ElementFactory.make("kmssink")
            self._sink.set_property("skip-vsync", "true")
        else:
            self._sink = Gst.ElementFactory.make("autovideosink")

        # Create bus and connect several handlers
        self._bus = self._pipeline.get_bus()
        self._bus.add_signal_watch()
        self._bus.connect('message::state-changed', self._onStateChanged)
        self._bus.connect('message::eos', self._onEos)
        self._bus.connect('message::error', self._onError)

    def _onEos(self, bus, msg):
        print ('on_eos')
        self._pipeline.set_state(Gst.State.NULL)
        #self.mainloop.quit()
			
    def _onError(self, bus, msg):
        error = msg.parse_error()
        print ('onError:', error[1])
        self._pipeline.set_state(Gst.State.NULL)

    def _onStateChanged(self, bus, msg):
        oldState, newState, pendingState = msg.parse_state_changed()
        # print((
        #     f"Bus call: Pipeline state changed from {oldState.value_nick} to {newState.value_nick} "
        #     f"(pending {pendingState.value_nick})"
        # ))
        #if newState == Gst.State.NULL:
        #    print("State is NULL")
        #    #self.pipeline.set_state(Gst.State.READY)
        
    def _addToPipeline(self):
        pass

    def _setPlaying(self):
        self._pipeline.set_state(Gst.State.PLAYING)

    def start(self):
        self._pipeline.set_state(Gst.State.READY)
        GLib.timeout_add(100, self._setPlaying)

    def stop(self):
        self._pipeline.set_state(Gst.State.NULL)
    
    def addDrmInfo(self, fd, plane, conn):
        self._plane = plane
        self._sink.set_property("fd", fd)
        self._sink.set_property("connector-id", conn.id)
        self._sink.set_property("plane-id", plane.id)

class HdmiElement(BaseElement):
    def __init__(self):
        super().__init__()
        self._source = Gst.ElementFactory.make("v4l2src")
        
        caps = Gst.Caps("video/x-raw,framerate=30/1,colorimetry=bt601")
        self._filter = Gst.ElementFactory.make("capsfilter")
        self._filter.set_property("caps", caps)
        
        self._parse = Gst.ElementFactory.make("rawvideoparse")
        self._parse.set_property("format", "uyvy")
        self._parse.set_property("width", 1920)
        self._parse.set_property("height", 1080)
        self._parse.set_property("framerate", Gst.Fraction(30/1))
        
        self._convert = Gst.ElementFactory.make("v4l2convert")
        self._queue = Gst.ElementFactory.make("queue")

    def _addToPipeline(self, container):
        self._pipeline.add(self._source)
        self._pipeline.add(self._filter)
        self._pipeline.add(self._parse)
        self._pipeline.add(self._convert)
        self._pipeline.add(self._queue)
        self._pipeline.add(self._sink)

        if not self._source.link(self._filter):
            logger.error("Link Error: source -> filter")
            return
        
        if not self._filter.link(self._parse):
            logger.error("Link Error: filter -> parse")
            return
        
        if not self._parse.link(self._convert):
            logger.error("Link Error: parse -> convert")
            return
        
        if not self._convert.link(self._queue):
            logger.error("Link Error: convert -> queue")
            return
        
        if not self._queue.link(self._sink):
            logger.error("Link Error: queue -> sink")
            return
        
class VideoElement(BaseElement):
    def __init__(self):
        super().__init__()
        self._source = Gst.ElementFactory.make("filesrc")
        self._decode = Gst.ElementFactory.make("decodebin")
        self._sink = None
        self._kmssink_fd = None
        if platform.machine() == "aarch64":
            self._sink = Gst.ElementFactory.make("kmssink")
            self._sink.set_property("skip-vsync", "true")
        else:
            self._sink = Gst.ElementFactory.make("autovideosink")

        self._addToPipeline()

    def _onPadAdded(self, dbin, pad):
        print("Pad added!")
        if pad.is_linked():
            print("Is linked!")
            peer = None
            peer = pad.get_peer()
            if (peer != None):
                print("Unlink!")
                pad.unlink(peer, pad)
        self._decode.link(self._sink)

    def _addToPipeline(self):
        self._pipeline.add(self._source)
        self._pipeline.add(self._decode)
        self._pipeline.add(self._sink)

        if not self._source.link(self._decode) :
            logger.error("Link Error: source -> decode")

        self._decode.connect("pad-added", self._onPadAdded)
        
    def setSourceFile(self, srcFileName):
        self._source.set_property('location', srcFileName)

class VideoElement2(BaseElement):
    def __init__(self):
        super().__init__()
        self._source = Gst.ElementFactory.make("filesrc")
        self._demux = Gst.ElementFactory.make("qtdemux")
        self._parse = Gst.ElementFactory.make("h264parse")
        self._decode = Gst.ElementFactory.make("avdec_h264")
        self._sink = None
        self._kmssink_fd = None
        if platform.machine() == "aarch64":
            self._sink = Gst.ElementFactory.make("kmssink")
            self._sink.set_property("skip-vsync", "true")
        else:
            self._sink = Gst.ElementFactory.make("autovideosink")

        self._addToPipeline()
        
    def _onDemuxPadAdded(self, dbin, pad):
        if pad.is_linked():
            print("Is linked!")
            peer = None
            peer = pad.get_peer()
            if (peer != None):
                print("Unlink!")
                pad.unlink(peer, pad)
                
        self._decode.link(self._sink)
        if pad.name == "video_0":
            print("video pad added")
            self._demux.link(self._decode)

    def _addToPipeline(self):
        self._pipeline.add(self._source)
        self._pipeline.add(self._demux)
        self._pipeline.add(self._parse)
        self._pipeline.add(self._decode)
        self._pipeline.add(self._sink)

        if not self._source.link(self._demux) :
            logger.error("Link Error: source -> demux")

        if not self._decode.link(self._sink) :
            logger.error("Link Error: source -> sink")

        self._demux.connect("pad-added", self._onDemuxPadAdded)
        
    def setSourceFile(self, srcFileName):
        self._source.set_property('location', srcFileName)

        
class BasePlayer():
    def __init__(self, vmOne: VMOneContainer, useHwDecoder: bool):
        self._vmOne = vmOne
        self._useHwDecoder = useHwDecoder
        self._fd = None
        self._plane = None
        self._conn = None
        self._element: BaseElement = None

    def _resetElement():
        pass         
    
    def addDrmInfo(self, fd, plane, conn):
        self._fd = fd
        self._plane = plane
        self._conn = conn

    def play(self, fileName):
        self._resetElement();
        self._element.addDrmInfo(self._fd, self._plane, self._conn)
        self._element.setSourceFile(fileName)
        self._element.start()
        
    def stop(self):
        self._element.stop()

    def setZPos(self, zPos):
        if self._plane == None:
            return
        
        self._plane.set_props({"zpos": zPos})

class VideoPlayer(BasePlayer):
    def __init__(self, vmOne: VMOneContainer, useHwDecoder = True):
        super().__init__(vmOne, useHwDecoder)

    def _resetElement(self):
        if self._element != None:
            self.stop()
        if (self._useHwDecoder):
            self._element = VideoElement()
        else:
            self._element = VideoElement2()

index = 0
videoElements = []
vmOne = VMOneContainer()
fileNames0 = ["videos/BlenderReel_1080p.mp4", "videos/BlenderReel2_1080p.mp4"]
fileNames1 = ["videos/BlenderReel2_1080p.mp4", "videos/BlenderReel_1080p.mp4"]

def switchVideos():
    global index

    fileName0 = fileNames0[index]   
    vmOne.playVideo0(fileName0)
    fileName1 = fileNames1[index]   
    vmOne.playVideo1(fileName1)
    
    GLib.timeout_add_seconds(3, switchVideos)
    index = index + 1
    index = index % len(fileNames0)

def main():
    switchVideos()
    vmOne.start()

main()