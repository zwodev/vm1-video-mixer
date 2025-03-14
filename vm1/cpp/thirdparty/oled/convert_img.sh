#!/bin/bash

# Create the "png" folder if it doesn't exist
mkdir -p img/png

# Loop through all PNG files in "img/"
for f in img/*.png; do
    # Convert PNG to BMP with RGB565 format
    ffmpeg -y -i "$f" -pix_fmt rgb565le "${f%.png}.bmp"
    
    # Move the original PNG to the "png" folder
    mv "$f" img/png/
done

echo "Conversion completed. Original PNGs moved to img/png/"
