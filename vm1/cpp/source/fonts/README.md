
We convert fonts to C-structs using this tool:

https://github.com/hzeller/bdfont.data?tab=readme-ov-file

# command to create the font:

    ./bdfont-data-gen <filename.bdf> <output-fontname> -c "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_# " -d <directory> -s

# convert ttf/otf to bdf:

    git clone https://github.com/jirutka/otf2bdf.git
    sudo apt update
    sudo apt install libfreetype6 libfreetype6-dev
    cd otf2bdf
    ./configure
    make install    
    otf2bdf <filename.ttf> -p 16 -o <filename.bdf>