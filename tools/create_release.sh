#!/usr/bin/env bash
set -euo pipefail

# Piper release script

VERSION="1.0.0"
PROJECT_NAME="piper"
TOPDIR="${PWD}/rpmbuild"

echo "🔵 Starting release build for $PROJECT_NAME version $VERSION"

# Clean previous builds
rm -rf "$TOPDIR"
mkdir -p "$TOPDIR/SOURCES" "$TOPDIR/SPECS" "$TOPDIR/BUILD" "$TOPDIR/BUILDROOT" "$TOPDIR/RPMS" "$TOPDIR/SRPMS"

# Generate tarball
echo "📦 Creating source tarball..."
git archive --format=tar.gz --prefix=${PROJECT_NAME}-${VERSION}/ -o ${TOPDIR}/SOURCES/${PROJECT_NAME}-${VERSION}.tar.gz HEAD

# Copy the .spec file
echo "📝 Copying .spec file..."
cp piper.spec "$TOPDIR/SPECS/"

# Build RPM
echo "🛠️  Building RPM..."
rpmbuild --define "_topdir $TOPDIR" -ba "$TOPDIR/SPECS/piper.spec"

# Output paths
echo ""
echo "✅ Release artifacts created!"
echo "Tarball: $TOPDIR/SOURCES/${PROJECT_NAME}-${VERSION}.tar.gz"
echo "RPM: $TOPDIR/RPMS/x86_64/${PROJECT_NAME}-${VERSION}-1.x86_64.rpm"
echo ""
