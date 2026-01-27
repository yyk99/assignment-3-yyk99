#!/bin/bash

# Generate TAGS file for a C/C++ project

echo "Generating TAGS file..."

# Remove old TAGS
rm -f TAGS

# Find all C/C++ source files and generate tags
find . /usr/src/linux-headers-6.8.0-90-generic ../../ldd3 -type f \
     \( -name "*.[ch]" -o -name "*.cpp" -o -name "*.hpp" \) \
     -not -path "./.git/*" \
     -not -path "*/build/*" \
     -not -path "*/output/*" \
     -not -path "*/node_modules/*" \
     -print0 | xargs -0 etags -a

echo "TAGS file generated successfully"
echo "Files tagged: $(wc -l < TAGS)"
