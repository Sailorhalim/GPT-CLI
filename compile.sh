#!/usr/bin/sh

if [ -z "$1" ] || [ -z "$2" ]; then
  echo "Cara penggunaan:\n  sh compile.sh script.cpp script"
else
  clang++ "$1" -o "$2" -std=c++17 -lcurl -lcpr -lssl -lcrypto
fi
