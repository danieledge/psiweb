# CLAUDE.md: PsiWeb for the Psion Series 5mx

Read this before changing anything. The user-facing documentation is
`README.md`; the port of the engine is described in `links/PORTING.md`; the
shared code's notes are in `core/CLAUDE.md` and `core/docs/BUILDING.md`.

## What this is

PsiWeb is a web browser for the **Psion Series 5mx** (EPOC Release 5, ARM710T
at 36 MHz, no FPU, about 16 MB RAM, 640x240 in 16 greys, gcc 3.0 Psion 98r2).
Its engine is **Links 2.30** in graphics mode, patched. UID 0x01000A7A; the
version is `KVersion` in `app/psiweb.cpp` and `pkg/psiweb.pkg`. The quality
bar is first-party Psion engineering: it should look and behave like Psion's
own programs.

It is one of a suite (PsiTerm, PsiMail, PsiCast, PsiLinux, PsiConnect,
PsiProxy) that share [psion-core](https://github.com/danieledge/psion-core),
the git submodule `core/`: the connection layer, the shared chunk layout, the
connection settings, TLS, zlib, the C-library shims, pwnet and the build and
emulator tools. Core keeps the old relative paths (`core/ssh/psiglue.cpp`,
`core/web/engine/pwnet.c`...).

## Two processes

1. **PsiWeb.app** (`app/`, an EIKON DLL): screen, keys, pen, menus, dialogs,
   settings, favourites and history (`pwfavs.cpp`), help (`pwhelp.cpp`).
2. **psiweb.exe** (Links, plain C on ESTLIB): `links/psi_drv.c` (the graphics
   driver: RGB565 frame to 16 greys, keys, pen, commands, questions, picture
   memory), `links/epoc/psi_os.c` (the select loop and pseudo-sockets over
   pwnet), `links/epoc/psi_heap.cpp` (the 10 MB heap), `engine/pwepoc.cpp`
   (the chunk, PsiWeb.log, heartbeats), `engine/pwupdate.c` (signed
   updates), with `core/web/engine/pwnet.c` over `core/ssh/psiglue.cpp` and
   `core/ssh/tls13.c` (`tls/` holds the LibTomCrypt configuration;
   `core/web/tls` the arithmetic).

They talk through the global chunk "PsiWebShared" (`psiweb.h`, which embeds
`core/ssh/psishared.h`): events, commands (`psiweb_cmds.h`), questions, and a
640x240 4bpp screen. If either header changes, every object that touches the
chunk must be rebuilt, or app and engine disagree and the engine crashes.

The NetSurf engine (up to 0.61) is still in the tree (`Makefile`,
`engine/fetch_psi.c`, `fb/`, `patches/`, `emu/`), buildable with
`PSIWEB_ENGINE=netsurf`.

## Building and testing

```
git clone --recurse-submodules https://github.com/danieledge/psiweb.git
core/tools/docker/psibuild setup                 # once
core/tools/docker/psibuild "bash build.sh"       # dist/PsiWeb.sis
core/tools/docker/psibuild "make -f links/epoc.mk -j8 exe"   # psiweb.exe only
```

The first build fetches Links, libjpeg and libpng at pinned checksums
(`links/fetch.sh`) into `build/links`. `links/epoc.mk` and `build.sh` take
`CORE` (default `core/`). The app's `.mmp` finds the shared sources through
the SDK link `ptproj/psitermssh`, which `build.sh` points at `core/ssh`.

- **ARM harness:** `links/emu/run_links.py`, `links/emu/run_pages.sh` run
  the exact ARM `psiweb.exe` under unicorn, with a 10 MB heap; the fixed
  page set is the gate for engine changes (time, heap, screenshots).
- **Emulator** (`core/tools/emu`, see its README): `node
  --experimental-strip-types core/tools/emu/mkcard.ts psiweb`, then
  `core/tools/emu/run.sh NAME ...`; `python3 core/tools/emu/net.py web` needs
  the psiconnect checkout beside this one. Look at the screenshots with the
  Read tool. About one run in five stalls at "Starting the browser engine..."
  or ends in KERN-EXEC 3 as an emulator artefact: try another card layout
  (`EMU_PAD`) before suspecting the code.
- **Stale objects:** the SDK keeps every program's objects in one folder
  (`$EPOCROOT/epoc32/build/.../ptproj/marmd/rel`); delete the `*.o` there
  after switching checkouts (USER 19 panics otherwise).

## Releasing

Installed PsiWebs read `dist/PsiWeb-version.txt`, `PsiWeb.sis` and
`PsiWeb.sis.sig` from the **psiterm** repository's branch `dist/`
(`main`, or `dev` for test builds); the updater's address in the code must
not change. So:

1. Bump `KVersion` and `pkg/psiweb.pkg` (two digits, 0.70 = `0,70,0`: the
   EPOC installer only compares major.minor) and build.
2. `bash release.sh <version>`: signs with `core/tools/release/sign.py
   --product PsiWeb` (the key is `--key`, `$PSITERM_RELEASE_KEY` or
   `~/.psiterm-signing/release.key`, never in a repository), writes the
   version file, and copies the three files into `../psiterm/dist/`
   (`$PSITERM`) when that checkout is there.
3. Commit and push them in psiterm's `dev`; the owner promotes to `main`.

`dist/` here is git-ignored. Never commit keys, the SDK, the emulator or
its ROMs, `PsiApps.sis`, LAN addresses, home paths or local headers.

## UI rules (the EIKON Application Style Guide)

Use the `epoc-eikon-ui-guide` skill if it is available.

- Menu bar: File, View, Go, Favourites, Tools. At most 8 items a pane; "..."
  only on commands that open a dialog; tick boxes rather than flip-flop
  names; dim rather than hide. In `DynInitMenuPaneL`, only dim or tick items
  that exist in that pane (EIKON 8 otherwise).
- Standard shortcuts: Ctrl+E Close, Ctrl+K Preferences, Ctrl+M / Shift+Ctrl+M
  zoom (cycling), Ctrl+T toolbar, Shift+Ctrl+A About, Shift+Ctrl+H Help.
- Dialogs fit 640x240; queries state the fact, then ask a question ending
  in "?", names in double quotes. Busy messages bottom-left, outcomes as
  infoprints (no "error", no "please", no full stops).
- Toolbar on the right: title, four 24x20 pictures (`tools/mkicons.py`),
  the clock. Black on white, grey for secondary text, clipped with "...".
- Don't change the connection settings (the dialog, its fields, formats or
  defaults) unless the owner asks. `TPsiLink` is shared by every program.

## Other rules

- All network input is untrusted: bounded copies everywhere (`snprintf`,
  the shims). The engine must not use ESTLIB's `sprintf` and stdio
  (`core/web/compat/nsprintf.c` instead).
- Long work in the engine must not starve the app: keep heartbeats going.
- British English, plain short sentences, the EIKON glossary terms, no em
  dashes. Commit messages are plain British English.
