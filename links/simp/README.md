# The page simplifier (0.86)

`htmlsimp.cpp` and `htmlsimp.h` are the PsiConnect Atom's page simplifier,
copied unchanged from [psiconnect](https://github.com/danieledge/psiconnect)
`src/` at commit e240f09 (dev, with its speed-up of 17 Nov 2026) (MIT, the same author). PsiWeb runs it on the
Psion for View > Page style (Lighter, Reader, Text only), before Links
parses the page: `psi_simp.cpp` is the C entry point, `../psi_style.c` the
engine's side.

Keep the copy identical to psiconnect's: change it there (with its host
tests), then copy it here. `../../test/simp/check.sh` compares the two and
runs psiconnect's own simplifier tests (`hosttest/test_proxy`) when the
psiconnect checkout is beside this one.
