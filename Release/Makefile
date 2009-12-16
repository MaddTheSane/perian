#Things to change for version upgrades:
# Makefiles - Change this makefile, the VERSION declaration.
# Plists - Modify all plists, they should all have version numbers.
# Check the Perian.component target and PerianPane target.

VERSION=1.2
BETA=TRUE
SRC_DIR=..
BUILD_DIR=build
COMMAND_BUILD_DIR=CommandLineComponents
RELEASE_NAME=Perian
PERIAN_DIR=$(BUILD_DIR)/Perian
ART_DIR=Artwork

ifeq ($(BETA),TRUE)
    BUILDSTYLE=Deployment+Debug
else
    BUILDSTYLE=Deployment
endif


ifeq ($(strip $(PRODUCT_DIR)),)
	PERIAN_BUILD_DIR=$(SRC_DIR)/build/$(BUILDSTYLE)
else
	TARGET_BUILD_DIR=$(PRODUCT_DIR)/$(BUILDSTYLE)
	PERIAN_BUILD_DIR=$(TARGET_BUILD_DIR)
endif


BUILDFLAGS="BUILDCONFIGURATION=$(BUILDSTYLE)"

release: compile createfolder diskimage

command: compile
	@# clean build dir
	rm -rf $(COMMAND_BUILD_DIR)
	mkdir $(COMMAND_BUILD_DIR)
	cp $(PERIAN_BUILD_DIR)/Perian.prefPane/Contents/Resources/Components/*.zip $(PERIAN_BUILD_DIR)/Perian.prefPane/Contents/Resources/Components/*/*.zip $(COMMAND_BUILD_DIR)
	cp CommandLine-Perian-Install.sh $(COMMAND_BUILD_DIR)
	makeself --nocrc --nocomp --nox11 $(COMMAND_BUILD_DIR) Perian-CommandLine-$(VERSION).sh Perian ./CommandLine-Perian-Install.sh

compile:
	$(MAKE) $(BUILDFLAGS) -C $(SRC_DIR)

createfolder:
	@# clean build directory
	rm -rf $(BUILD_DIR)
	mkdir $(BUILD_DIR)

	mkdir $(PERIAN_DIR)
	
	@# copy the app
	cp -R $(PERIAN_BUILD_DIR)/Perian.prefPane $(PERIAN_DIR)
#	strip -s $(SRC_DIR)/exportedSymbols -u $(PERIAN_DIR)/Perian.component/Contents/MacOS/Perian

	@# copy the text files
#	cp $(SRC_DIR)/Changes.txt $(PERIAN_DIR)
#	cp $(SRC_DIR)/License.txt $(PERIAN_DIR)
	cp $(SRC_DIR)/Release/Read\ Me.rtf $(PERIAN_DIR)
	cp $(SRC_DIR)/Release/Changes.txt $(PERIAN_DIR)
	
	@# symlink /Library/QuickTime
#	ln -s /Library/QuickTime $(PERIAN_DIR)/QuickTime

	@# delete svn  and backup files
	find $(BUILD_DIR) -name ".svn" -type d -exec rm -rf {} \; -prune
	find $(BUILD_DIR) \( -name "*~" -or -name .DS_Store \) -type f -delete
	
	@# delete unneeded nib files for non-beta builds
ifeq ($(BETA),FALSE)
	find $(BUILD_DIR) \( -name classes.nib -or -name info.nib \) -type f -delete
endif

diskimage:
	rm "$(BUILD_DIR)/$(RELEASE_NAME).dmg" || true
	@# make Perian disk image
	mkdir -p $(PERIAN_DIR)/.background
#.background
	cp $(ART_DIR)/periandmgbg.png $(PERIAN_DIR)/.background

	./make-diskimage.sh $(BUILD_DIR)/$(RELEASE_NAME)_$(VERSION).dmg $(PERIAN_DIR) "Perian $(VERSION)" dmg_perian.scpt
	
	@echo Build finished
