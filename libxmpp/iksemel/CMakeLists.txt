
add_definitions(-DSTDC_HEADERS -DHAVE_ERRNO_H -DUSE_DEFAULT_IO)

if(UNIX)
    add_definitions(-DHAVE_UNISTD_H)
endif()

add_library(iksemel STATIC
    src/iks.c src/md5.c src/stream.c src/utility.c src/jabber.c src/ikstack.c src/sha.c
    src/dom.c src/base64.c src/sax.c src/io-posix.c)
