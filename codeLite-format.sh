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
  CodeLite/entry.h

_apply_cmake_format \
  CodeLite/CMakeLists.txt

#_apply_format \
# Plugin/clMenuBar.cpp \
# sdk/codelite_indexer/codelite_index_main.cpp \
# sdk/codelite_indexer/equeue_win_impl.h \
# sdk/codelite_indexer/network/named_pipe.h \
# sdk/codelite_indexer/network/named_pipe_server.h

# Tlhelp32.h is lower case in mingw GCC
