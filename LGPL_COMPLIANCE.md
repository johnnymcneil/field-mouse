# LGPL Compliance Notes

This repository is set up for an open-source Field Mouse build that uses Qt 6
under LGPLv3 by dynamically linking against the Qt shared libraries.

## Current Compliance Strategy

1. Field Mouse is licensed under LGPL-3.0-or-later.
2. Qt is expected to be deployed as shared libraries, not a static Qt build.
3. Binary packages must ship the LGPL and GPL license texts together with a
   prominent notice that Qt is used.
4. Binary distributors must preserve the user's ability to replace the shipped
   Qt libraries with a compatible build.

## Binary Release Checklist

1. Build against a shared Qt 6 installation.
2. Deploy Qt DLLs and plugins with `windeployqt` or an equivalent process that
   preserves the normal DLL and plugin layout.
3. Include these files in the release bundle:
   - `LICENSE`
   - `THIRD_PARTY_NOTICES.md`
   - `LGPL_COMPLIANCE.md`
   - `LICENSES/LGPL-3.0.txt`
   - `LICENSES/GPL-3.0.txt`
4. Record the exact Qt version bundled with the release in the release notes or
   installer metadata.
5. Provide the corresponding source code for that exact Qt version, including
   any local Qt patches, or provide a written offer and durable instructions for
   obtaining that source.
6. Do not add EULA, DRM, installer, or store terms that prohibit reverse
   engineering for debugging Qt modifications or prevent users from replacing
   the Qt libraries.

## Practical Release Guidance

- Prefer shipping the application executable with the Qt DLLs in the same
  directory tree and Qt plugins in their standard subdirectories.
- If a release uses modified Qt binaries, publish those Qt modifications under
  the applicable open-source terms together with build instructions.
- If you ever switch to static linking, this compliance approach is no longer
  sufficient and the release process must be revisited before distribution.

## Non-Legal Note

This document is an engineering checklist intended to keep the repository and
binary outputs aligned with the Qt LGPL distribution model. It is not legal
advice.