# https://wiki.codelite.org/pmwiki.php/Developers/CodingGuidelines

#   git push --force-with-lease

_apply_cmake_format() {
  for _file in "$@"
  do
    dos2unix ${_file}
    sed -i 's/[[:space:]]*$//' ${_file}
  done
}

_apply_format() {
  for _file in "$@"
  do
    dos2unix ${_file}
    clang-format --style=file -i ${_file}
    sed -i 's/[[:space:]]*$//' ${_file}
  done
}

_apply_format \
  LiteEditor/app.cpp \
  wxcrafter/src/wxgui_bitmaploader.cpp \

_apply_cmake_format \
  CMakeLists.txt                  \
  CodeLite/CMakeLists.txt         \
  DatabaseExplorer/CMakeLists.txt \
  Plugin/CMakeLists.txt           \
  sdk/wxsqlite3/CMakeLists.txt


# Tlhelp32.h is lower case in mingw GCC
