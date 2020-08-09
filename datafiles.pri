unix {
	CONFIG(debug, debug|release) {
		QMAKE_CLEAN += $$DEST_DATA_DIR/datafiles_installed
		POST_TARGETDEPS += $$DEST_DATA_DIR/datafiles_installed
		QMAKE_EXTRA_TARGETS += $$DEST_DATA_DIR/datafiles_installed
		$$DEST_DATA_DIR/datafiles_installed.commands = \
			mkdir -p $$DEST_DATA_DIR \
			&& ln -sf $$SRC_DATA_DIR/* $$DEST_DATA_DIR \
			&& touch $$DEST_DATA_DIR/datafiles_installed
	}
	else {
		POST_TARGETDEPS += datafiles
		QMAKE_EXTRA_TARGETS += datafiles
		datafiles.commands = \
			mkdir -p $$DEST_DATA_DIR \
			&& rsync -r $$SRC_DATA_DIR/ $$DEST_DATA_DIR
	}
}
win32 {
	POST_TARGETDEPS += datafiles
	QMAKE_EXTRA_TARGETS += datafiles
	datafiles.commands = \
    	# mkdir not needed for windows
	    # xcopy stopped to work with error: invalid number of parameters
	    # it seams that /bin/sh is called with mingw from some version of Qt
	    #xcopy  /e /y /i \"$$shell_path($$SRC_DATA_DIR)\" \"$$shell_path($$DEST_DATA_DIR)\"
	    mkdir -p $$shell_path($$DEST_DATA_DIR) && cp -avr $$shell_path($$SRC_DATA_DIR)/* $$shell_path($$DEST_DATA_DIR)
}
