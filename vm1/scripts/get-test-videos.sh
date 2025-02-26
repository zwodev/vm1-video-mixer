#!/bin/bash

SCRIPT_DIR=$(dirname "$0")

# fetch h.264 test videos
wget https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_30MB.mp4 -O $SCRIPT_DIR/../videos/bigbuckbunny_1080_10s_h264.mp4
wget https://test-videos.co.uk/vids/jellyfish/mp4/h264/1080/Jellyfish_1080_10s_30MB.mp4 -O $SCRIPT_DIR/../videos/jellyfish_1080_10s_h264.mp4

# fetch h.265 test videos
wget https://test-videos.co.uk/vids/bigbuckbunny/mp4/h265/1080/Big_Buck_Bunny_1080_10s_30MB.mp4 -O $SCRIPT_DIR/../videos/bigbuckbunny_1080_10s_h265.mp4
wget https://test-videos.co.uk/vids/jellyfish/mp4/h265/1080/Jellyfish_1080_10s_30MB.mp4 -O $SCRIPT_DIR/../videos/jellyfish_1080_10s_h265.mp4
