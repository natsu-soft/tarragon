# Tarragon

A standalone GDScript LSP

MAKE SURE YOU `git clone --recursive` all them SUBMODULES, breh

### PLS TEST

tarragon on windows OUTSIDE of emacs/eglot.  Eglot is not sending `\r\n` on Windows.  Is this an eglot specific probelm?  I would test with `nvim` but I truly am a lazy bastard.

### NOTA BENE

You might want to use brotli for binary-ized json string compression.

_BUT_ you really should bake the API trie--then you could get rid of all zip nonsense.  It'll be worth it (wow I love things living in the stack).  THEN you could stop statically linking libarchive (wow slightly smaller binary wowowow).
 
