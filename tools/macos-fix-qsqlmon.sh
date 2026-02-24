#!/bin/bash
# Fix qsqlmon Qt framework paths to use bundled frameworks
# Run this after macdeployqt
# Usage: ./tools/macos-fix-qsqlmon.sh [bundle-path]

set -e

BUNDLE_PATH="${1:-install/quickevent.app}"
QSQLMON="$BUNDLE_PATH/Contents/MacOS/qsqlmon"

if [ ! -f "$QSQLMON" ]; then
    echo "qsqlmon not found at $QSQLMON"
    exit 1
fi

echo "Fixing Qt framework paths in qsqlmon..."

for fw in QtSvg QtPrintSupport QtWidgets QtSql QtQml QtNetwork QtXml QtGui QtCore; do
    old_path=$(otool -L "$QSQLMON" | grep "/${fw}.framework" | awk '{print $1}')
    if [ -n "$old_path" ]; then
        install_name_tool -change "$old_path" "@executable_path/../Frameworks/${fw}.framework/Versions/A/${fw}" "$QSQLMON"
        echo "  Fixed $fw"
    fi
done

echo "qsqlmon Qt framework paths fixed!"
