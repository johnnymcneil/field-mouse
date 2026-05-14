# Third-Party Notices

Field Mouse source code is licensed under the GNU Lesser General Public License,
version 3 or, at your option, any later version. See the root [LICENSE](LICENSE)
file for the project license text.

This project uses Qt 6, including the Qt Core, Qt GUI, and Qt Widgets modules.
Qt is dual-licensed by The Qt Company Ltd. and Qt contributors. Field Mouse uses
Qt under the open-source GNU Lesser General Public License version 3
distribution model.

## Qt Notice

- Copyright: Qt is Copyright (C) The Qt Company Ltd. and other contributors.
- License basis used by this project: LGPL-3.0-or-later for the Qt libraries
  shipped with open-source builds.
- Linking model: official Field Mouse builds are intended to link against Qt as
  shared libraries. Static linking is not covered by this repository's release
  process.
- User rights: recipients may replace the shipped Qt DLLs with a compatible
  modified Qt build, and no additional project terms may restrict reverse
  engineering for debugging those library modifications.
- Source availability: when Field Mouse is redistributed in binary form with Qt
  DLLs, the distributor must provide the complete corresponding source code for
  the exact Qt version shipped, including any local modifications to Qt, or a
  written offer and clear instructions for obtaining that source.

## Files Included For Compliance

Binary redistributions should include at least these files alongside the app:

- LICENSE
- THIRD_PARTY_NOTICES.md
- LGPL_COMPLIANCE.md
- LICENSES/LGPL-3.0.txt
- LICENSES/GPL-3.0.txt

## Release Metadata Requirement

Each official binary release should publish the exact Qt version used and a
public location for the corresponding Qt source package for that same version.
For unmodified upstream Qt releases, the official source archives are available
from Qt's public open-source distribution channels, but the release artifact or
release notes still need to identify the exact version that was shipped.

This file is provided for software distribution hygiene and does not replace
legal review for a particular release.