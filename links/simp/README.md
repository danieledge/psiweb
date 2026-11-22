# The page simplifier (0.86)

`htmlsimp.cpp` and `htmlsimp.h` are the PsiConnect Atom's page simplifier,
copied unchanged from [psiconnect](https://github.com/danieledge/psiconnect)
`src/` at commit e240f09 (dev, with its speed-up of 17 Nov 2026) (MIT, the same author). PsiWeb runs it on the
Psion for View > Page style (Lighter, Reader, Text only), before Links
parses the page: `psi_simp.cpp` is the C entry point, `../psi_style.c` the
engine's side.

Keep the copy in step with psiconnect's: change it there (with its host
tests), then copy it here. `../../test/simp/check.sh` compares the two and
runs psiconnect's own simplifier tests (`hosttest/test_proxy`) when the
psiconnect checkout is beside this one.

**PsiWeb 1.03 carries a change of its own** (marked "(PsiWeb)" in the
code; `psiweb.diff` here is the change against psiconnect's copy, which
`check.sh` applies before comparing), for Dan's report on BBC News of 21
November 2026:

- a class that hides an element from the eye ("visually-hidden",
  "sr-only", the BBC's "...-VisuallyHidden") hides it here too: that is
  where "Attribution", "Posted", "9 hours ago" and the second copy of each
  video's headline came from;
- in text-only and reader mode a picture's description stands in for it
  only when the picture is a link (a news photo's description is not text
  to read);
- lighter mode puts the site's menus (`<nav>`, `<footer>`) in one line of
  links with " | " between them, each address once, and leaves out pictures
  marked as decoration (`alt=""`), so Lighter is visibly lighter than Full.

The Atom's simplifier would gain from the same change; until psiconnect has
it, the two differ by `psiweb.diff`. psiconnect's own `test_proxy` run on
this copy passes all but four of its 224 checks, and those four are the
change itself (a lighter page's footer in its one line; a picture's
description dropped in text-only and reader mode when it is not a link).
