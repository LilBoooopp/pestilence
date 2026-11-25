# Pestilence

A 42 project evolving from [Famine](https://github.com/LilBoooopp/famine): an ELF64 infector with anti-debugging, process detection, and code obfuscation. `Pestilence` injects a self-replicating stub into x86_64 executables using the **PT_NOTE → PT_LOAD** technique, **without changing the host program's behaviour**. Confined to `/tmp/test` and `/tmp/test2` — this is a controlled, educational exercise in ELF internals.

## How it works

### Infection

1. Validate the file: 64-bit, x86_64, `ET_EXEC` or `ET_DYN`.
2. Skip if already infected (signature check via `memmem`).
3. Copy the binary into a buffer padded to a page boundary, append the stub.
4. Compute the stub's virtual address from the binary's layout — top of the highest `PT_LOAD`, page-rounded — so it never collides with existing mappings.
5. Repurpose the `PT_NOTE` program header into a `PT_LOAD` segment (`R+X`) mapping the stub, redirect `e_entry`.
6. Patch the original `e_entry` and `PT_PHDR` vaddr into the stub (for load-base recovery).
7. Rewrite the target file in place, preserving original permission bits.

At runtime the stub runs first, walks `auxv` (`AT_PHDR`) to recover the load base (PIE-safe), performs its own infection pass on `/tmp/test` and `/tmp/test2`, then jumps to the original entry point.

### Anti-debugging

Before any infection logic, both the host binary and the injected stub check:

1. **TracerPid** — reads `/proc/self/status`, checks if `TracerPid` is non-zero (catches strace, gdb, ltrace).
2. **ptrace(PTRACE_TRACEME)** — if it fails, a debugger is already attached. If it succeeds, blocks future attachment for the rest of execution.

Checks run in that order — TracerPid first (reads clean kernel state), ptrace second (locks out future attachers). Reversed order would false-positive because `PTRACE_TRACEME` itself sets `TracerPid`.

### Process detection

Iterates `/proc/*/comm` to check if a process named `test` is running. If found, skips execution entirely. Works in both NOSTD (raw syscalls) and libc builds using the shared `my_opendir`/`my_readdir64` infrastructure.

## Layout

```
include/famine.h    types, tunables, macros, prototypes
include/m_std.h     NOSTD libc replacements + syscall macros
include/m_syscall.h syscall dispatch macros (variadic)
src/main.c          entry points: host binary + stub _start/my_start
src/elf.c           ELF validation, PT_NOTE/PT_PHDR discovery
src/inject.c        stub copy, sentinel patching, PT_NOTE hijack
src/infect.c        per-file infection pipeline
src/walk.c          directory traversal (recursive with -r flag)
src/antidebug.c     is_traced() + process_active() (dual NOSTD/libc)
src/std.c           NOSTD syscall wrappers + string primitives
src/x64-syscall.c   raw x86_64 syscall0–6 (inline asm)
src/stub_data.c     xxd-generated stub blob (stub_bin[] / stub_bin_len)
```

The stub is compiled as a freestanding PIE (`-nostdlib -ffreestanding`), linked with a custom `linker.ld` that packs everything into a single section, then extracted to a raw binary via `objcopy` and turned into a C header via `xxd -i`.

## Build

```
make            # release build -> ./Pestilence (silent)
make DEBUG=1 re # verbose: stderr logging
make re         # clean rebuild
make fclean     # remove everything
```

## Test

```
./test.sh           # full regression suite
./test.sh --rebuild  # force make re first
```

The test suite covers: infection correctness, idempotency, self-propagation, permission preservation, rejection of non-targets (non-ELF, 32-bit, truncated), containment, argument parsing (`-r`, explicit file/dir targets), anti-debug (strace/gdb/ltrace), process detection, and false-positive guards.

## Status

### Mandatory
- [x] Infect ELF64 binaries in `/tmp/test` and `/tmp/test2`
- [x] PT_NOTE → PT_LOAD injection with entry-point redirection
- [x] Stub vaddr computed from binary layout (no fixed constant)
- [x] PIE-safe load-base recovery via `auxv`
- [x] In-place infection, preserving permissions
- [x] Signature guard — skip already-infected binaries
- [x] Self-propagation — infected binaries infect on execution
- [x] Total silence — no stdout/stderr output
- [x] Anti-debugging — ptrace + TracerPid checks
- [x] Process detection — skip if specific process is running
- [ ] Code obfuscation — infection routine resistant to static analysis

### Bonus
- [x] Recursive infection from OS root
