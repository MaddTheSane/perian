PREFIX?=
BUILD_DIR?=$(shell defaults read com.apple.Xcode PBXProductDirectory 2> /dev/null)

ifeq ($(strip $(BUILD_DIR)),)
	BUILD_DIR=build
endif

DEFAULT_BUILDCONFIGURATION=Deployment
DEFAULT_TARGET=PerianPane
BUILDCONFIGURATION?=$(DEFAULT_BUILDCONFIGURATION)

CP=ditto --rsrc
RM=rm

.PHONY: all perian clean latest

perian:
	xcodebuild -project Perian.xcodeproj -target $(DEFAULT_TARGET) -configuration $(BUILDCONFIGURATION) build


#install:
#	    cp -R build/Adium.app ~/Applications/
#	    cp -R build/AIUtilities.framework ~/Library/Frameworks/

clean:
	xcodebuild -project Perian.xcodeproj -target $(DEFAULT_TARGET) -configuration $(BUILDCONFIGURATION)  clean



latest:
	svn up
	make perian

