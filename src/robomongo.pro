TEMPLATE = subdirs

SUBDIRS = core \
          gui  \
          app


# Print OS and CPU architecture
contains(QMAKE_HOST.arch, x86_64) {
    win32:message("Windows x64/x86_64 (64bit) build")
    linux:message("Linux x64/x86_64 (64bit) build")
} else {
    win32:message("Windows x86 (32bit) build.")
    linux:message("Linux x86 (32bit) build.")
}
