PREFIX?=
BUILD_DIR?=$(shell defaults read com.apple.Xcode PBXProductDirectory 2> /dev/null)

ifeq ($(strip $(BUILD_DIR)),)
	BUILD_DIR=build
endif

DEFAULT_BUILDCONFIGURATION=Deployment

BUILDCONFIGURATION?=$(DEFAULT_BUILDCONFIGURATION)

CP=ditto --rsrc
RM=rm

.PHONY: all perian clean latest

perian:
	xcodebuild -project Perian.xcodeproj -configuration $(BUILDCONFIGURATION) build


#install:
#	    cp -R build/Adium.app ~/Applications/
#	    cp -R build/AIUtilities.framework ~/Library/Frameworks/

clean:
	xcodebuild -project Perian.xcodeproj -configuration $(BUILDCONFIGURATION)  clean


#localizable-strings:
#	mkdir tmp || true
#	mv "Plugins/Gaim Service" tmp
#	mv "Plugins/WebKit Message View" tmp
#	mv "Plugins/joscar Service" tmp
#	genstrings -o Resources/English.lproj -s AILocalizedString Source/*.m Source/*.h Plugins/*/*.h Plugins/*/*.m Plugins/*/*/*.h Plugins/*/*/*.m
#	genstrings -o tmp/Gaim\ Service/English.lproj -s AILocalizedString tmp/Gaim\ Service/*.h tmp/Gaim\ Service/*.m
#	genstrings -o tmp/WebKit\ Message\ View/English.lproj -s AILocalizedString tmp/WebKit\ Message\ View/*.h tmp/WebKit\ Message\ View/*.m
#	genstrings -o tmp/joscar\ Service/English.lproj -s AILocalizedString tmp/joscar\ Service/*.h tmp/joscar\ Service/*.m
#	genstrings -o Frameworks/AIUtilities\ Framework/Resources/English.lproj -s AILocalizedString Frameworks/AIUtilities\ Framework/Source/*.h Frameworks/AIUtilities\ Framework/Source/*.m
#	genstrings -o Frameworks/Adium\ Framework/Resources/English.lproj -s AILocalizedString Frameworks/Adium\ Framework/Source/*.m Frameworks/Adium\ Framework/Source/*.h
#	mv "tmp/Gaim Service" Plugins
#	mv "tmp/WebKit Message View" Plugins
#	mv "tmp/joscar Service" Plugins
#	rmdir tmp || true

latest:
	svn up
	make perian
