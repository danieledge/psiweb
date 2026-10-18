# Third-party components

| Component | Where | Licence |
|---|---|---|
| [Links](http://links.twibright.com/) 2.30 (Mikulas Patocka and others), PsiWeb's engine since 0.62 | fetched and checksum-checked by `links/fetch.sh`; Psion changes in `links/` (see `links/PORTING.md`) | **GPL v2** - so `psiweb.exe` and `PsiWeb.sis` are GPL v2 as a whole; `PsiWeb.sis` carries the licence text, and the corresponding source is this repository, psion-core and the pinned upstream archives |
| [IJG libjpeg](https://www.ijg.org/) 9f (Links' JPEG pictures) | fetched by `links/fetch.sh` | IJG licence: "this software is based in part on the work of the Independent JPEG Group" |
| [libpng](http://www.libpng.org/pub/png/libpng.html) 1.6.43 (Links' PNG pictures) | fetched by `links/fetch.sh` | libpng licence (PNG Reference Library License version 2) |
| [NetSurf](https://www.netsurf-browser.org/) and its libraries (libcss, libdom, hubbub, libparserutils, libwapcaplet, libnsfb, libnsutils, libnslog, libnsgif, libnsbmp, libnspsl), [utf8proc](https://github.com/JuliaStrings/utf8proc) - PsiWeb's engine up to 0.61, still buildable with `PSIWEB_ENGINE=netsurf` (not in the released `PsiWeb.sis`) | fetched by `netsurf.sh`, Psion changes in `patches/` | NetSurf GPL v2; libraries MIT-style - so that `psiweb.exe` is GPL v2 too |
| [DejaVu fonts](https://dejavu-fonts.github.io/) (Sans, Sans Bold, Sans Mono), pre-rendered by FreeType into PsiWeb's sharp text | rendered at build time by `links/mkstrike.py` from the build container's `fonts-dejavu-core`; the glyph bitmaps are in `psiweb.exe` | Bitstream Vera licence (DejaVu changes public domain) - the text is added to PsiWeb's `COPYING.txt` |
| [zlib](https://zlib.net/), LibTomCrypt, the TLS 1.3 client and Dropbear's X25519 | `core/` (psion-core): see [its THIRD-PARTY.md](https://github.com/danieledge/psion-core/blob/main/THIRD-PARTY.md) | zlib licence; public domain; MIT |
| EPOC C Standard Library (`stdlib.sis`, ESTLIB.DLL) from the Psion EPOC R5 C++ SDK | embedded in `PsiWeb.sis` (not in this repository) | Psion's redistributable runtime, shipped with apps that use it as the SDK directs |

## Licences together

PsiWeb's own code is MIT (`LICENSE`), and MIT code may be combined with GPL
v2 code. Where it is - in `psiweb.exe`, which links Links 2 - the result is
distributed under GPL v2, as GPL v2 requires: `PsiWeb.sis` includes the GPL
v2 text, and the complete corresponding source (PsiWeb's own code, the Psion
changes to Links, the build scripts, which fetch the exact upstream versions
by checksum, and psion-core) is in these repositories. The MIT licence of
PsiWeb's own files and of psion-core is unchanged.
