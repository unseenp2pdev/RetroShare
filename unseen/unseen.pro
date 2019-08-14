!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
DESTDIR = lib
QT += network xml
################################# Linux ##########################################
linux-* {

}

unix {
        #target.path = "$${BIN_DIR}"
        #INSTALLS += target
}

linux-g++ {
        #OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
        #OBJECTS_DIR = temp/linux-g++-64/obj
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
        ####
}

#################################### Windows #####################################

win32-g++ {
       ###
}

##################################### MacOS ######################################

macx {
      ###
}

##################################### FreeBSD ######################################

freebsd-* {
        ###
}

##################################### OpenBSD  ######################################

openbsd-* {
        ###
}

##################################### Haiku ######################################

haiku-* {
        ###
	
}

############################## Common stuff ######################################


# Input
HEADERS +=  httpclient/cert_exchange.h
SOURCES +=  httpclient/cert_exchange.cpp
