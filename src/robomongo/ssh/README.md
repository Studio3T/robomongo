SSH tunnel submodule
====================

Implemented in vanilla C99 and uses LIBSSH2 library.

Notes
-----

1. Users of this module should include *only* `ssh.h`. Do not pollute it 
with unnecessary includes or implementation specific declarations.
1. Implementation files include `private.h`, which is also includes 
`ssh.h` automatically. 
1. The preferred use of `*` is adjacent to the data name or function name 
and not adjacent to the type name (i.e. `char *data`, not `char* data`)
1. Public functions and types always should be prefixed with `rbm_ssh_`. 
It means that all declarations in `ssh.h` should use this prefix.
1. Private functions and types always should be prefixed with just 
`rbm_`. You may declare file-scoped objects without this prefix, but still
prefer to always use `rbm_` prefix.

