#!/bin/bash

# Default values
TTF_FILE=""
BDF_FILE=""
OUTPUT_FILE=""
FONTSIZE="12"
GLYPHS="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:<>[]/-_# "

# Usage message
usage() {
    echo "Usage: $0 [-t <ttf_file>] [-s <fontsize>] [-b <bdf_file>] [-o <output_file>]"
    exit 1
}

# Parse options
while getopts ":t:s:b:o:" opt; do
    case $opt in
        t)
            TTF_FILE="$OPTARG"
            ;;
        s)
            FONTSIZE="$OPTARG"
            ;;
        b)
            BDF_FILE="$OPTARG"
            ;;
        o)
            OUTPUT_FILE="$OPTARG"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            usage
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage
            ;;
    esac
done

# Check if required options are set
if [ -z "$TTF_FILE" ] && [ -z "$BDF_FILE" ]; then
    echo "Error: Either -ttf or -bdf must be specified." >&2
    usage
fi

if [ -z "$OUTPUT_FILE" ]; then
    echo "Error: -o <output_file> is required." >&2
    usage
fi

# Check if input files exist
if [ -n "$TTF_FILE" ] && [ ! -f "$TTF_FILE" ]; then
    echo "Error: TTF file '$TTF_FILE' not found." >&2
    exit 1
fi

if [ -n "$BDF_FILE" ] && [ ! -f "$BDF_FILE" ]; then
    echo "Error: BDF file '$BDF_FILE' not found." >&2
    exit 1
fi

# Print the options (replace with your actual logic)
echo "TTF file: ${TTF_FILE:-not set}"
echo "Fontsize: ${FONTSIZE:-not set}"
echo "BDF file: ${BDF_FILE:-not set}"
echo "Output file: $OUTPUT_FILE"

# Your conversion logic here
# Example:
if [ -n "$TTF_FILE" ]; then
    echo "##### Converting TTF to BDF File #####"
    BDF_FILE="bdf/${OUTPUT_FILE}.bdf"
    otf2bdf "${TTF_FILE}" -p $FONTSIZE -o ${BDF_FILE} -r 72
    echo "Created ${BDF_FILE}"
fi

if [ -n "$BDF_FILE" ]; then
    echo "##### Converting BDF to C-Struct #####"
    echo "BDF-File: ${BDF_FILE}"

    ./bdfont-data-gen "${BDF_FILE}" "${OUTPUT_FILE}" -c "${GLYPHS}]" -d . -s
    echo "Created ${OUTPUT_FILE}.h and ${OUTPUT_FILE}.c"
fi
