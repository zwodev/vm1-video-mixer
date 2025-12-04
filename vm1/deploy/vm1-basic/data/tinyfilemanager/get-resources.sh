#!/bin/bash

# Base folder for local resources
base_folder="tinyfilemanager"
css_folder="$base_folder/css"
js_folder="$base_folder/js"
fa_webfonts_folder="$base_folder/webfonts"

mkdir -p "$css_folder" "$js_folder" "$fa_webfonts_folder"

# URLs of the CSS files
css_urls=(
"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css"
"https://cdnjs.cloudflare.com/ajax/libs/dropzone/5.9.3/min/dropzone.min.css"
"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css"
"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/styles/default.min.css"
)

# URLs of the JS files
js_urls=(
"https://cdnjs.cloudflare.com/ajax/libs/ace/1.32.2/ace.js"
"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"
"https://cdnjs.cloudflare.com/ajax/libs/dropzone/5.9.3/min/dropzone.min.js"
"https://code.jquery.com/jquery-3.6.1.min.js"
"https://cdn.datatables.net/1.13.1/js/jquery.dataTables.min.js"
"https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/highlight.min.js"
)

# Download CSS files into css_folder
for url in "${css_urls[@]}"; do
  echo "Downloading CSS: $url"
  wget -q -P "$css_folder" "$url"
done

# Download JS files into js_folder
for url in "${js_urls[@]}"; do
  echo "Downloading JS: $url"
  wget -q -P "$js_folder" "$url"
done

# Download font-awesome webfonts files (common font file extensions)
fa_base_url="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/fonts/"
fa_files=(
"fontawesome-webfont.eot"
"fontawesome-webfont.svg"
"fontawesome-webfont.ttf"
"fontawesome-webfont.woff"
"fontawesome-webfont.woff2"
"FontAwesome.otf"
)

for file in "${fa_files[@]}"; do
  echo "Downloading Font Awesome webfont: $file"
  wget -q -P "$fa_webfonts_folder" "${fa_base_url}${file}"
done

echo "Download complete. Resources saved under $base_folder."
echo "Verify font-awesome.min.css font paths point to '../webfonts/' relative to css folder."
