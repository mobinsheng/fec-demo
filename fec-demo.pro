TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += \
    -L/usr/local/lib \
    -L/usr/lib \

LIBS += \
    -lpthread \


SOURCES += main.cpp \
    udp_server.cpp \
    udp_client.cpp \
    marshall.cpp \
    fec.c \
    test_fec.cpp

HEADERS += \
    udp_server.h \
    udp_client.h \
    fec.h \
    marshall.h \
    packet.h \
    fec_codec.h \
    random_generator.h \
    packet_queue.h
