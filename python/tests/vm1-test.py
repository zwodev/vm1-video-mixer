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

        self._card.disable_planes()

        # Initialize Gst and create pipeline
        Gst.init(sys.argv[1:])
        print("Gst Version: %s", Gst.version())

        # All players for HDMI-A
        self._players0 = []
        # All players for HDMI-B
        self._players1 = []

        # Create players
        isFading = False
        videoPlayer = VideoPlayer(self, True)
        videoPlayer.fading = isFading
        self._addVideoPlayer0(videoPlayer)
        videoPlayer = VideoPlayer(self, True)
        videoPlayer.fading = isFading
        self._addVideoPlayer0(videoPlayer)

        videoPlayer = VideoPlayer(self, True)
        videoPlayer.fading = isFading
        self._addVideoPlayer1(videoPlayer)
        videoPlayer = VideoPlayer(self, True)
        videoPlayer.fading = isFading
        self._addVideoPlayer1(videoPlayer)
            
        self._mainloop = GLib.MainLoop()

    # Element 0 will be displayed on HDMI0
    def _addVideoPlayer0(self, player):
        plane = self._res.reserve_overlay_plane(self._crtc0)
        self._planes0.append(plane)
        player.addDrmInfo(self._fd, plane, self._conn0)
        self._players0.append(player)

    # Element 1 will be displayed on HDMI1
    def _addVideoPlayer1(self, player):
        plane = self._res.reserve_overlay_plane(self._crtc1)
        self._planes1.append(plane)
        player.addDrmInfo(self._fd, plane, self._conn1)
        self._players1.append(player)

    def start(self):
        self.update()
        self._mainloop.run()

    def playVideo0(self, fileName):
        for player in self._players0:
            if player.state == BasePlayer.State.DEACTIVATED:
                player.play(fileName)
                break

    def playVideo1(self, fileName):
        for player in self._players1:
            if player.state == BasePlayer.State.DEACTIVATED:
                player.play(fileName)
                break

    def update(self):
        dt = 0.016
        for player in self._players0:
           player.update(dt)

        for player in self._players1:
           player.update(dt)

        GLib.timeout_add(int(dt * 1000.0), self.update)

    def onPlayerActivated(self, player):
        playersToDeactivate = [BasePlayer]
        found = False
        for p in self._players0:
            if p == player:
                p.state = BasePlayer.State.ACTIVATING
                found = True
            else:
                playersToDeactivate.append(p)
        
        if not found:
            playersToDeactivate.clear()
            for p in self._players1:
                if p == player:
                    p.state = BasePlayer.State.ACTIVATING
                    found = True
                else:
                    playersToDeactivate.append(p)

        if found:
            for p in playersToDeactivate:
                p.state = BasePlayer.State.DEACTIVATING 

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
        #self.mainloop.quit()

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

    def __del__(self):
        super().__del__()
        
    def _onPadAdded(self, dbin, pad):
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
        if pad.name == "video_0":
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
    class State(enum.Enum):
        PREROLL = 0
        ACTIVATING = 1
        ACTIVE = 2
        DEACTIVATING = 3
        DEACTIVATED = 4

    def __init__(self, vmOne: VMOneContainer, useHwDecoder: bool):
        self._vmOne = vmOne
        self._useHwDecoder = useHwDecoder
        self._fd = None
        self._plane = None
        self._conn = None
        self._element = None
        self._alpha = 0.0
        
        self.state = self.State.DEACTIVATED
        self.fading = False
        self._element: BaseElement = None

    def _resetElement():
        pass         
    
    def addDrmInfo(self, fd, plane, conn):
        self._fd = fd
        self._plane = plane
        self._conn = conn

    def play(self, fileName):
        self.setAlpha(0.0)
        self._resetElement()
        self._element.addDrmInfo(self._fd, self._plane, self._conn)
        self._element.setSourceFile(fileName)
        self._element.start()
        vmOne.onPlayerActivated(self)
        
    def stop(self):
        self._element.stop()

    def setAlpha(self, alpha):
        if self._plane == None:
            return
        
        self._alpha = min(1.0, max(0.0, alpha))
        gstStruct = Gst.Structure("s")
        value = int(self._alpha * 64000.0)
        self._plane.set_props({"alpha": value})

    def setZPos(self, zPos):
        if self._plane == None:
            return
        
        self._plane.set_props({"zpos": zPos})

    def update(self, dt):
        if self.state == self.State.DEACTIVATED:
            pass
        elif self.state == self.State.DEACTIVATING:
            if self.fading:
                alpha = self._alpha - (dt * 4.0)
                if (alpha > 0.0):
                    self.setAlpha(alpha)
                else:
                    self.setZPos(1)
                    self.state = self.State.DEACTIVATED
                    #print("Deactivated!")
            else:
                self.setAlpha(0.0)
                if self._element != None:
                    self._element.stop()
                    del self._element
                    self._element = None
                self.state = self.State.DEACTIVATED
        elif self.state == self.State.ACTIVATING:
            if self.fading:
                alpha = self._alpha + (dt * 4.0)
                #print(alpha)
                if (alpha < 1.0):
                    self.setAlpha(alpha)
                else:
                    self.setZPos(2)
                    self.state = self.State.ACTIVE
            else:
                self.setAlpha(1.0)
                self.state = self.State.ACTIVE
        elif self.state == self.State.ACTIVE:
            pass

class VideoPlayer(BasePlayer):
    def __init__(self, vmOne: VMOneContainer, useHwDecoder = True):
        super().__init__(vmOne, useHwDecoder)

    def _resetElement(self):
        if self._element != None:
            self._element.stop()
            del self._element
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