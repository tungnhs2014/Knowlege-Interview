# Chapter 10 Interview - System & Security

> Scope: shared-library deployment, advanced dynamic loading, Linux capabilities, and secure privileged programs.
> Interview style: scenario-first, production/debug oriented, with keywords kept as drill-down prompts.

---

## Review Basis

Chapter 10 maps to these repository sources:

- `knowledge/ch10_shared_library_fundamentals.md`
- `knowledge/ch10_advanced_shared_libraries.md`
- `knowledge/ch10_linux_capabilities.md`
- `knowledge/ch10_writing_secure_programs.md`
- TLPI-derived docs: `ch41_fundamentals_of_shared_libraries.md`, `ch42_advanced_features_of_shared_libraries.md`, `ch39_capabilities.md`, `ch38_writing_secure_privileged_programs.md`
- DevLinux Module 01: `INDEX.md`, top-level `README.md`, `01-General-Knowlege/README.md`, plus shared/static library exercises 1 and 2.

Technical semantics were cross-checked against Linux man-pages and official docs:

- `ld.so(8)`, `ldconfig(8)`, `ldd(1)`, `dlopen(3)`, `capabilities(7)`, and Linux kernel `no_new_privs` documentation.
- Related API families: `setuid()`, `seteuid()`, `setresuid()`, `setgid()`, `setgroups()`, `open()`, `openat()`, `stat()`, `lstat()`, `fstat()`, `mkstemp()`, `umask()`, `chroot()`, and `prctl()`.

Interview calibration used reputable external signals only for prioritization, not as technical authority:

- Amazon official SDE prep lists operating systems and system design as common technical areas and emphasizes applying fundamentals over memorization.
- Microsoft official technical interviewing guidance emphasizes design, testing, security implications, boundaries, and error conditions.
- Google technical interview guidance emphasizes clarifying assumptions, communicating trade-offs, and verifying solutions.
- Renesas embedded Linux job requirements emphasize Linux applications/BSP, low-level debugging, GDB, Docker/CI, and embedded production constraints.

---

## Priority Map

### A - Project and production scenarios

Study these deeply. A strong answer should describe the mechanism, risk, and debug workflow:

- Shipping a binary that cannot find a `.so` on the target.
- Replacing a shared library and breaking old applications because the ABI changed.
- Using `LD_LIBRARY_PATH` as a permanent deployment fix.
- Debugging plugin load and symbol lookup failures with `dlopen()` and `dlsym()`.
- Fixing C++ plugin symbol failures caused by name mangling or hidden visibility.
- Using `LD_PRELOAD` or interposition for observability, and reviewing the risk.
- Running a low-port service without full root.
- Reviewing an embedded service that was granted `CAP_SYS_ADMIN`.
- Debugging `EPERM` despite root, file capabilities, containers, or service-manager settings.
- Reviewing a setuid/setgid helper that runs shell commands or trusts the environment.
- Fixing TOCTOU, symlink, hardlink, and unsafe temporary-file bugs.
- Preventing sensitive file descriptor leaks across `exec`.
- Dropping privileges permanently and verifying UID/GID/capability state.
- Handling production incidents involving loader paths, capabilities, or privileged execution.

### B - Design comparisons and senior follow-ups

Know the trade-offs and when to use the tool:

- Static vs dynamic linking; `.a` vs `.so`; static linking as an embedded fallback.
- ELF interview basics: executable, shared object, symbol table, relocation, `DT_NEEDED`.
- PIC vs PIE at practical level.
- Soname, real name, linker name, `ldconfig`, `/etc/ld.so.conf`, `/etc/ld.so.cache`.
- `RPATH` vs `RUNPATH`, `$ORIGIN`, installed libraries, and package layout.
- `RTLD_NOW` vs `RTLD_LAZY`; `RTLD_LOCAL` vs `RTLD_GLOBAL`.
- Symbol visibility, version scripts, symbol versioning, and namespace collisions.
- `LD_PRELOAD`, `RTLD_NEXT`, and interposition.
- Capability sets: permitted, effective, inheritable, bounding, ambient.
- File capabilities, `setcap`, `getcap`, `capsh --print`, `/proc/<pid>/status`.
- setuid/setgid, real/effective/saved UID/GID, supplementary groups.
- Environment sanitization, `PATH`, `LD_LIBRARY_PATH`, `LD_PRELOAD`, and privileged execution.
- Descriptor-based file validation with `open()`/`openat()` plus `fstat()`.
- Defense in depth: capabilities, seccomp, AppArmor, SELinux, namespaces, containers.

### C - Lower-priority / know enough to recognize

Recognize these and know where to look them up:

- Every individual `CAP_*` value beyond common examples.
- Raw `capget()` and `capset()` when `libcap` is unavailable or being debugged.
- `securebits`, `PR_SET_KEEPCAPS`, ambient capabilities, and `PR_SET_NO_NEW_PRIVS`.
- `rtld-audit(7)`, `LD_AUDIT`, `/etc/ld.so.preload`, and system-wide preload behavior.
- `dlmopen()`, `dladdr()`, `dlvsym()`, `RTLD_NODELETE`, `RTLD_NOLOAD`, `RTLD_DEEPBIND`.
- Constructors/destructors beyond recognizing load/unload side effects.
- `chroot()` limitations, setuid script portability, and old `_init()`/`_fini()` hooks.

---

## Final Interview List

### Priority A

1. A binary works on the build machine but fails on the target with "cannot open shared object file". How do you debug and fix it?
2. A production update replaces `libfoo.so.1.0.1` with `libfoo.so.1.0.2`, and an old application starts crashing. How do you review the ABI and rollout?
3. A team fixes startup failures by exporting `LD_LIBRARY_PATH` in production. What can go wrong, and what would you replace it with?
4. A plugin-based service fails at runtime: sometimes `dlopen()` fails, sometimes `dlsym()` fails. How do you design the load path and diagnostics?
5. A C++ plugin loads, but the host cannot find `plugin_create`. What are the likely causes?
6. An observability team wants to use `LD_PRELOAD` to intercept `open()` and `connect()`. How do you evaluate usefulness and risk?
7. A service must bind to port 80 but should not run as root. What design would you propose?
8. An embedded service was granted `CAP_SYS_ADMIN` to "make it work". How do you review and narrow the privilege?
9. A production service still gets `EPERM` even though it appears to run as root or has file capabilities. How do you investigate?
10. A setuid helper runs `system("tool " + user_input)`. Why is this dangerous, and how should it be rewritten?
11. A privileged program calls `access()` or `stat()` and later opens the path. How can this become exploitable?
12. A daemon opens a secret key and then `exec`s a helper. How can the key leak, and how do you prevent it?
13. A helper creates `/tmp/app.log.<pid>` and sometimes overwrites another file. What attack is likely, and what is the safe pattern?
14. A daemon drops from root to a service user, but a child can still regain privilege. What went wrong?
15. An incident only reproduces under systemd or in a container, not from your shell. How do you debug loader paths, capabilities, and privilege state together?

### Priority B

16. Compare static linking, dynamic linking, `.a`, and `.so` for an embedded product.
17. Explain ELF basics needed for shared-library debugging: `DT_NEEDED`, symbols, relocation, PIC, PIE.
18. Compare real name, soname, and linker name. Where does `ldconfig` fit?
19. Compare `RPATH`, `RUNPATH`, `$ORIGIN`, `LD_LIBRARY_PATH`, and installed library paths.
20. When should a plugin loader use `RTLD_NOW` vs `RTLD_LAZY`, and `RTLD_LOCAL` vs `RTLD_GLOBAL`?
21. Why should a shared library hide internal symbols?
22. How do version scripts and symbol versioning help long-lived libraries?
23. What does `dlclose()` guarantee, and what does it not guarantee?
24. Compare setuid root, file capabilities, service-manager privileges, and containers.
25. Explain permitted, effective, inheritable, bounding, and ambient capabilities.
26. How do UID/GID changes interact with capabilities?
27. Why must privileged programs sanitize environment variables and close file descriptors before `exec`?
28. Why is `chroot()` not a complete sandbox?

### Priority C

29. What are `rtld-audit`, `LD_AUDIT`, and `/etc/ld.so.preload` used for?
30. What are `dlmopen()`, `dladdr()`, and `dlvsym()`?
31. What are `securebits`, `PR_SET_KEEPCAPS`, and `PR_SET_NO_NEW_PRIVS`?
32. What are `RTLD_NODELETE`, `RTLD_NOLOAD`, and `RTLD_DEEPBIND`?
33. Why can using `ldd` on an untrusted executable be unsafe?

---

## High-Value Comparisons

| Comparison | Strong interview answer |
|------------|-------------------------|
| Static vs shared library | Static libraries copy selected object code into the executable. Shared libraries keep code in a separate `.so` that the dynamic linker maps at runtime. |
| Static linking vs dynamic linking | Static linking is the build-time link step. Dynamic linking is runtime library search, mapping, relocation, and symbol binding. |
| `.a` vs `.so` | `.a` is an archive of object files. `.so` is an ELF shared object with dynamic symbols, relocation data, and dependency metadata. |
| PIC vs PIE | PIC is position-independent code for shared libraries. PIE is a position-independent executable, useful for ASLR and hardening. |
| Real name vs soname vs linker name | The real name is the full versioned file, the soname is the ABI-level runtime name, and the linker name is the development-time `libname.so` used by `-l`. |
| `RPATH` vs `RUNPATH` | `RPATH` is older and has higher precedence if no `RUNPATH` exists. `RUNPATH` is newer, lower precedence than `LD_LIBRARY_PATH`, and applies to direct dependencies. |
| `LD_LIBRARY_PATH` vs `$ORIGIN` `RUNPATH` | `LD_LIBRARY_PATH` is mutable environment state. `$ORIGIN` `RUNPATH` makes app-local dependency layout explicit and relocatable. |
| `RTLD_NOW` vs `RTLD_LAZY` | `RTLD_NOW` fails during `dlopen()` if symbols cannot resolve. `RTLD_LAZY` defers function binding until first call. |
| `RTLD_LOCAL` vs `RTLD_GLOBAL` | `RTLD_LOCAL` keeps plugin symbols out of later global lookup. `RTLD_GLOBAL` exposes them for later-loaded objects. |
| Hidden visibility vs version script | Hidden visibility controls exports from source/compile settings. A version script controls the public ABI at link time and can hide everything else. |
| UID 0 vs capability | UID 0 is broad traditional privilege. A capability is a narrower kernel-checked privilege unit. |
| Permitted vs effective capability | Permitted means the thread may enable it. Effective means the kernel currently counts it for checks. |
| setuid root vs file capability | setuid root grants broad root-compatible privilege. File capabilities can grant only the needed `CAP_*`. |
| Temporary vs permanent privilege drop | Temporary drop can regain privilege via saved IDs or permitted caps. Permanent drop removes that path. |
| `stat()` then `open()` vs `open()` then `fstat()` | The first can check one object and use another after a race. The second validates the object actually opened. |
| `FD_CLOEXEC` vs `O_CLOEXEC` | `FD_CLOEXEC` marks an existing descriptor. `O_CLOEXEC` sets close-on-exec atomically during descriptor creation. |

---

## Common Project Failure Patterns

- Shipping a binary that links on the build host but cannot find a `.so` on the target root filesystem.
- Forgetting the soname symlink or failing to run `ldconfig` after installing a system library.
- Using `LD_LIBRARY_PATH` as a permanent service fix, then loading the wrong library under a different user, unit file, container, or shell.
- Replacing a shared library without changing the major soname after an ABI break.
- Mixing target architectures, such as an x86 host `.so` in an ARM root filesystem.
- Building a shared library from objects that were not compiled with `-fPIC`.
- Using `ldd` casually on untrusted binaries instead of safer metadata inspection.
- Letting plugins export too many symbols and collide with host or other plugin symbols.
- Calling `dlsym()` without the `dlerror()` clear-call-check pattern.
- Looking up C++ plugin entry points without `extern "C"` or with hidden visibility.
- Granting `CAP_SYS_ADMIN` instead of finding the narrow capability actually required.
- Assuming file capabilities survive copy, packaging, filesystem transfer, container layering, or deployment tools.
- Running setuid root when a dedicated group, file capability, or service-manager setup would be enough.
- Dropping only effective UID while leaving saved UID, supplementary groups, or capabilities able to regain privilege.
- Trusting `PATH`, `LD_LIBRARY_PATH`, `LD_PRELOAD`, locale, current directory, or standard descriptors in privileged code.
- Using `system()` or path-searched `exec` with user-controlled input.
- Creating predictable files in `/tmp` or checking paths before opening them.
- Leaking privileged file descriptors into a child process after `exec`.
- Treating `chroot()` as a complete sandbox.

---

## Detailed Answers - Priority A

### 1. A binary works on the build machine but fails on the target with "cannot open shared object file". How do you debug and fix it?

**What the interviewer is testing**

They want to see whether you understand build-time linking vs runtime loading, ELF metadata, soname, loader search paths, and target deployment.

**Strong answer**

I would first identify the exact missing soname, then compare what the executable records with what exists on the target. I would not immediately set `LD_LIBRARY_PATH` as a permanent fix. I would inspect `DT_NEEDED`, `RPATH`/`RUNPATH`, target architecture, `ldconfig` cache, and whether the package installed the real file and soname symlink correctly.

**Mechanism**

The executable records dependencies such as `DT_NEEDED = libfoo.so.1`. At `execve()`, the kernel starts the ELF interpreter, usually `ld-linux.so`, which searches according to loader rules: path dependencies with slashes, then `RPATH` if applicable, `LD_LIBRARY_PATH` for ordinary execution, `RUNPATH`, `/etc/ld.so.cache`, and default directories such as `/lib` and `/usr/lib` with architecture variants.

**Pitfalls**

The build machine may have a development library that the target image lacks. The target may have the real file but not the soname symlink. `/etc/ld.so.cache` may be stale. A container or chroot may not contain the same paths. The file may be the wrong architecture.

**Debug angle**

Use `readelf -d ./app`, `objdump -p ./app`, `ldd ./app` on trusted binaries, `file ./app ./libfoo.so*`, `ldconfig -p | grep foo`, and `LD_DEBUG=libs ./app`. On embedded targets, also inspect the root filesystem, package install scripts, and cross-compile sysroot.

**Follow-up keywords**

`DT_NEEDED`, `DT_SONAME`, `ld.so`, `ldconfig`, `/etc/ld.so.conf`, `/etc/ld.so.cache`, `RPATH`, `RUNPATH`, `$ORIGIN`, `file`, `readelf`, `objdump`, `ldd`, `LD_DEBUG=libs`.

### 2. A production update replaces `libfoo.so.1.0.1` with `libfoo.so.1.0.2`, and an old application starts crashing. How do you review the ABI and rollout?

**What the interviewer is testing**

They are testing whether you distinguish source compatibility from binary compatibility and understand soname discipline.

**Strong answer**

I would treat this as a suspected ABI break under the same soname. I would compare exported symbols, public struct layouts, function signatures, calling conventions, symbol versions, and behavior guarantees. If the change is incompatible, the fix is to restore compatibility or release a new major soname, not silently replace `libfoo.so.1`.

**Mechanism**

An executable linked against `libfoo.so.1` expects the ABI represented by that soname. Compatible minor updates can keep the same soname. Removing symbols, changing signatures, changing public data layout, or changing semantics that old binaries depend on can crash or corrupt old applications even if recompilation of new code works.

**Pitfalls**

Changing only headers can mislead teams into thinking the change is safe. Old binaries do not re-read headers; they bind to exported symbols and binary layouts. Running processes may keep the old mapped file until restart, which can make rollout behavior inconsistent.

**Debug angle**

Compare `nm -D`, `objdump -T`, `readelf --dyn-syms`, `readelf --version-info`, package changelogs, and crash backtraces. Reproduce with the old binary and both old/new libraries. Verify symlink chain: `libfoo.so -> libfoo.so.1 -> libfoo.so.1.0.x`.

**Follow-up keywords**

ABI, API, soname, real name, linker name, symbol versioning, version script, `nm -D`, `objdump -T`, `readelf`, major/minor version, compatible upgrade.

### 3. A team fixes startup failures by exporting `LD_LIBRARY_PATH` in production. What can go wrong, and what would you replace it with?

**What the interviewer is testing**

They want production judgment around loader behavior, security, operability, and deployment reproducibility.

**Strong answer**

`LD_LIBRARY_PATH` is fine for local testing but fragile as a production contract. It can make unrelated programs load the wrong `.so`, depends on environment inheritance, differs between shells and service managers, and is restricted for privileged execution. I would replace it with proper installation plus `ldconfig`, or controlled `RUNPATH` with `$ORIGIN` for app-local libraries.

**Mechanism**

For ordinary programs, the dynamic linker consults `LD_LIBRARY_PATH` before `RUNPATH` and before cache/default directories. In secure-execution mode, loader environment variables such as `LD_LIBRARY_PATH` and `LD_PRELOAD` are ignored or sanitized.

**Pitfalls**

One service's environment can shadow another library version. A test passes from an interactive shell but fails under systemd, cron, sudo, setuid, container entrypoint, or an embedded init script. Environment-driven loading also complicates incident response because the binary metadata no longer tells the whole story.

**Debug angle**

Compare `env`, service unit files, container entrypoints, `readelf -d`, `objdump -p`, `LD_DEBUG=libs`, and `/proc/<pid>/environ`. Prefer explicit deployment layout and inspectable ELF metadata.

**Follow-up keywords**

`LD_LIBRARY_PATH`, secure-execution mode, `$ORIGIN`, `RUNPATH`, `RPATH`, systemd environment, container image, `/proc/<pid>/environ`, `ld.so`.

### 4. A plugin-based service fails at runtime: sometimes `dlopen()` fails, sometimes `dlsym()` fails. How do you design the load path and diagnostics?

**What the interviewer is testing**

They are testing dynamic loading, plugin ABI design, error handling, and debug workflow.

**Strong answer**

I would make plugin paths explicit, load with `RTLD_NOW | RTLD_LOCAL` by default, log `dlerror()` diagnostics with the plugin path and symbol name, validate a small ABI version or function table after lookup, and fail the plugin cleanly instead of crashing the host.

**Mechanism**

`dlopen()` loads a shared object and its dependency tree. If the filename contains `/`, it is treated as a path; otherwise normal loader search rules apply. `dlsym()` searches the handle and dependencies for an exported symbol. `RTLD_NOW` resolves undefined symbols before `dlopen()` returns; `RTLD_LAZY` can defer function failures until first use.

**Pitfalls**

`dlopen()` can fail because the plugin or a dependency is missing, wrong architecture, unreadable, or has unresolved symbols. `dlsym()` can fail because the symbol is hidden, misspelled, mangled, versioned unexpectedly, or not exported. Treating `NULL` from `dlsym()` alone as failure is also wrong because `NULL` can be a valid symbol value.

**Debug angle**

Use `dlerror()` with the clear-call-check pattern, `ldd` on trusted plugin files, `readelf --dyn-syms`, `objdump -T`, `LD_DEBUG=libs,bindings`, and `strace -e openat,execve`. For plugins, log ABI version, plugin path, dependency resolution, and host build version.

**Follow-up keywords**

`dlopen()`, `dlsym()`, `dlclose()`, `dlerror()`, `RTLD_NOW`, `RTLD_LAZY`, `RTLD_LOCAL`, `RTLD_GLOBAL`, `-ldl`, plugin ABI, function table.

### 5. A C++ plugin loads, but the host cannot find `plugin_create`. What are the likely causes?

**What the interviewer is testing**

They want practical knowledge of C++ ABI boundaries, symbol names, and visibility.

**Strong answer**

I would check whether the entry point was exported with C linkage and default visibility. In C++, a function name is usually mangled, so looking up `"plugin_create"` with `dlsym()` will fail unless the function is declared `extern "C"`. It may also be hidden by `-fvisibility=hidden` or a version script.

**Mechanism**

`dlsym()` looks up a symbol name in the dynamic symbol table. C++ compilers encode function names, namespaces, overloads, and types into mangled symbol names. Visibility controls decide whether a symbol appears in the dynamic symbol table at all.

**Pitfalls**

Exporting C++ classes directly across plugin boundaries can couple compiler versions, standard library ABI, exception behavior, RTTI, and allocation ownership. A stable plugin ABI is usually a C-style factory returning a function table or opaque handle.

**Debug angle**

Use `nm -D --defined-only plugin.so`, `readelf --dyn-syms plugin.so`, and `c++filt` to inspect symbol names. Confirm build flags, version scripts, and visibility attributes.

**Follow-up keywords**

`extern "C"`, name mangling, `c++filt`, `-fvisibility=hidden`, version script, `nm -D`, `readelf --dyn-syms`, plugin ABI.

### 6. An observability team wants to use `LD_PRELOAD` to intercept `open()` and `connect()`. How do you evaluate usefulness and risk?

**What the interviewer is testing**

They are testing whether you understand interposition and security side effects.

**Strong answer**

`LD_PRELOAD` can be useful for tracing, testing, compatibility shims, and experiments, but I would avoid relying on it as a hidden production dependency. It changes process-wide symbol binding, can conflict with other wrappers, can break assumptions, and is ignored or restricted for privileged execution. For production observability I would prefer explicit instrumentation, eBPF, audit logs, or controlled preload rollout with strong tests.

**Mechanism**

The dynamic linker loads preloaded objects before normal dependencies. Their symbols can be found first during symbol resolution. Wrappers often call the real function using `dlsym(RTLD_NEXT, "open")`.

**Pitfalls**

Interposing common libc calls can recurse accidentally, break async-signal-safety, interfere with allocators or thread initialization, or hide the true source of a failure. `/etc/ld.so.preload` is system-wide and high blast radius.

**Debug angle**

Check `LD_PRELOAD`, `/etc/ld.so.preload`, `LD_DEBUG=libs,bindings`, `/proc/<pid>/maps`, and `readelf --dyn-syms` on the wrapper. Test under the exact service manager and privilege mode.

**Follow-up keywords**

`LD_PRELOAD`, `RTLD_NEXT`, symbol interposition, `/etc/ld.so.preload`, `LD_DEBUG=bindings`, secure-execution mode, `rtld-audit`.

### 7. A service must bind to port 80 but should not run as root. What design would you propose?

**What the interviewer is testing**

They want least-privilege design using Linux capabilities or privilege separation.

**Strong answer**

The narrow Linux capability is `CAP_NET_BIND_SERVICE`. I would either grant it to the executable with file capabilities, configure it in the service manager, or have a privileged supervisor bind the socket and pass it to an unprivileged service. After binding, the long-running service should drop the capability if it no longer needs it.

**Mechanism**

Binding ports below 1024 requires the relevant network-bind capability. File capabilities can grant selected capabilities at `execve()`. Service managers such as systemd can also set capability bounding and ambient/effective capability state for the service.

**Pitfalls**

Running the full server as root just to bind one port expands the blast radius. Granting broader capabilities such as `CAP_NET_ADMIN` or `CAP_SYS_ADMIN` is overkill. File capabilities can be lost during copy, packaging, filesystem transfer, or container image processing.

**Debug angle**

Check `getcap ./server`, `capsh --print`, `grep '^Cap' /proc/<pid>/status`, the service unit, and `strace -e bind`. Confirm the capability is effective at the time of `bind()`.

**Follow-up keywords**

`CAP_NET_BIND_SERVICE`, `setcap`, `getcap`, file effective bit, permitted set, effective set, systemd `CapabilityBoundingSet`, `AmbientCapabilities`, socket activation.

### 8. An embedded service was granted `CAP_SYS_ADMIN` to "make it work". How do you review and narrow the privilege?

**What the interviewer is testing**

They want capability threat modeling and embedded production judgment.

**Strong answer**

I would treat `CAP_SYS_ADMIN` as a red flag because it covers many unrelated privileged operations. I would reproduce the failing operation, trace the exact syscall returning `EPERM`, map that syscall to the narrow required capability, and remove everything else from the effective and bounding sets.

**Mechanism**

Linux capabilities split root privilege, but some capabilities are still broad. `CAP_SYS_ADMIN` is especially dangerous because many kernel subsystems use it as a catch-all administrative gate.

**Pitfalls**

In embedded systems, teams often "fix" permission issues by running as root or granting broad caps. That can turn a parsing bug, network bug, or plugin bug into device compromise, persistent configuration damage, or container escape risk.

**Debug angle**

Use `strace -f`, audit logs if available, `/proc/<pid>/status`, `capsh --print`, container runtime capability settings, and the relevant syscall man page. On target devices, also check init scripts, read-only rootfs behavior, and packaging that may strip xattrs.

**Follow-up keywords**

`CAP_SYS_ADMIN`, least privilege, `EPERM`, bounding set, container capabilities, file xattrs, `security.capability`, `capabilities(7)`.

### 9. A production service still gets `EPERM` even though it appears to run as root or has file capabilities. How do you investigate?

**What the interviewer is testing**

They are testing capability mechanics, namespaces, service runtime differences, and systematic debugging.

**Strong answer**

I would find the failing syscall first, then inspect the real runtime credential and capability state of the exact process or thread. "Looks like root" is not enough: namespaces, bounding sets, effective vs permitted sets, `no_new_privs`, service manager settings, and LSM policy can all affect the result.

**Mechanism**

The kernel checks credentials and usually the thread's effective capability set. File capabilities are transformed during `execve()` and masked by the bounding set. `no_new_privs` prevents `execve()` from granting new privilege. Containers and user namespaces can make UID 0 inside the namespace different from global root.

**Pitfalls**

Only checking `id` in a shell can be misleading. Capabilities are per-thread on Linux. A capability may be permitted but not effective. A container runtime may drop it from the bounding set. SELinux/AppArmor/seccomp may deny an operation even when capabilities look correct.

**Debug angle**

Use `strace -f`, `getcap`, `getpcap`, `capsh --print`, `grep '^Uid:\\|^Gid:\\|^Cap\\|^NoNewPrivs' /proc/<pid>/status`, `/proc/<pid>/task/<tid>/status`, service-unit configuration, container runtime config, and LSM audit logs.

**Follow-up keywords**

`EPERM`, `CapPrm`, `CapEff`, `CapBnd`, `CapAmb`, `NoNewPrivs`, `PR_SET_NO_NEW_PRIVS`, user namespace, seccomp, AppArmor, SELinux.

### 10. A setuid helper runs `system("tool " + user_input)`. Why is this dangerous, and how should it be rewritten?

**What the interviewer is testing**

They want secure privileged-program design, shell risk, and environment sanitization.

**Strong answer**

This is dangerous because `system()` invokes a shell with privileged context. User input can become shell syntax, and the behavior can be influenced by environment, file descriptors, current directory, and `PATH`. I would avoid the shell, validate input as data, drop privilege before executing if possible, and call a fixed absolute executable with explicit `argv` and a sanitized environment.

**Mechanism**

setuid changes effective credentials on `exec`. Shells interpret metacharacters, expand variables, and perform path lookup. Privileged programs must assume the user controls argv, env, file system timing, and sometimes descriptors.

**Pitfalls**

Quoting is often incomplete. `PATH`, `IFS`, locale, `LD_*`, closed standard descriptors, and inherited file descriptors can all affect behavior. If the child does not need privilege, failing to permanently drop before `exec` can preserve or regain privilege.

**Debug angle**

Inspect `execve()` arguments and environment with `strace -f -e execve,setuid,setresuid,setgid,setresgid`. Review UID/GID state before `exec`, open descriptors in `/proc/<pid>/fd`, and whether environment is cleared and rebuilt.

**Follow-up keywords**

`system()`, `popen()`, `execlp()`, `execvp()`, `execve()`, absolute path, sanitized environment, `PATH`, `IFS`, `LD_PRELOAD`, setuid, setgid.

### 11. A privileged program calls `access()` or `stat()` and later opens the path. How can this become exploitable?

**What the interviewer is testing**

They want to see TOCTOU reasoning and descriptor-based file safety.

**Strong answer**

This is a time-of-check/time-of-use bug. The program checks pathname state, but an attacker can replace the path before the later open. The safer pattern is to open the file with safe flags, then validate the already-open descriptor using `fstat()`, and operate on the descriptor with `fchmod()`, `fchown()`, or similar APIs.

**Mechanism**

Pathnames are names that can be rebound by rename, symlink changes, mount changes, or directory replacement. A file descriptor refers to the object opened at that time. Descriptor validation removes the gap between checking and using different objects.

**Pitfalls**

Public writable directories amplify the attack. Symlinks and hardlinks can redirect privileged writes. Signals or repeated execution can widen the race window. `access()` is especially suspicious in privileged programs because it checks using real IDs, not necessarily the credentials used later for opening.

**Debug angle**

Use `strace -e access,stat,lstat,open,openat,fstat,rename,symlink`. Review whether checks happen on paths or fds. Consider `openat()` with trusted directory fds, `O_NOFOLLOW` where appropriate, and avoiding public writable directories.

**Follow-up keywords**

TOCTOU, `access()`, `stat()`, `lstat()`, `open()`, `openat()`, `fstat()`, symlink attack, hardlink attack, `O_NOFOLLOW`, trusted directory fd.

### 12. A daemon opens a secret key and then `exec`s a helper. How can the key leak, and how do you prevent it?

**What the interviewer is testing**

They want process execution, file descriptor inheritance, and secure daemon hygiene.

**Strong answer**

File descriptors are inherited across `execve()` by default. If the daemon opens a secret key and then executes a helper, the helper may keep a readable descriptor even if it could not open the file itself. Use `O_CLOEXEC` when opening, set `FD_CLOEXEC` on existing descriptors, and close all unneeded descriptors before `exec`.

**Mechanism**

`execve()` replaces the program image but preserves open file descriptors unless close-on-exec is set. `O_CLOEXEC` sets the flag atomically at descriptor creation, avoiding a race with another thread that might fork and exec.

**Pitfalls**

Setting `FD_CLOEXEC` with a separate `fcntl()` after `open()` can race in multithreaded programs. Sockets, directory fds, memfds, device fds, and log fds can all leak sensitive authority, not just regular files.

**Debug angle**

Inspect `/proc/<pid>/fd` and `readlink /proc/<pid>/fd/<n>`. Use `strace -f -e openat,fcntl,close,execve`. Prefer `open(..., O_CLOEXEC)`, `pipe2(O_CLOEXEC)`, `accept4(..., SOCK_CLOEXEC)`, and `dup3(..., O_CLOEXEC)`.

**Follow-up keywords**

`FD_CLOEXEC`, `O_CLOEXEC`, `fcntl(F_SETFD)`, `execve()`, `/proc/<pid>/fd`, `pipe2`, `accept4`, `dup3`.

### 13. A helper creates `/tmp/app.log.<pid>` and sometimes overwrites another file. What attack is likely, and what is the safe pattern?

**What the interviewer is testing**

They want safe temporary-file handling and filesystem attack awareness.

**Strong answer**

This looks like a predictable temporary-file attack, possibly through symlink or precreation in a public writable directory. The helper should avoid public writable directories when possible. If it must use one, use `mkstemp()` or safe `open()` flags that create a new file atomically, set a restrictive `umask`, and validate/operate through the descriptor.

**Mechanism**

In `/tmp`, an attacker can often predict a name and create a symlink or file before the privileged program does. If the program opens it unsafely, it may write to an attacker-chosen target or change ownership/mode of the wrong object.

**Pitfalls**

Using PID, username, timestamp, or a counter is not enough uniqueness. Creating a file permissively and fixing permissions later creates a window. Path-based `chmod()` or `chown()` after creation can race.

**Debug angle**

Trace `openat`, `symlink`, `rename`, `fchmod`, `fchown`, and `umask`. Review directory permissions and sticky bit. Prefer `mkstemp()` and descriptor operations, then unlink if the file should be anonymous after opening.

**Follow-up keywords**

`mkstemp()`, `umask(077)`, `O_CREAT`, `O_EXCL`, `O_NOFOLLOW`, `/tmp`, sticky bit, symlink race, `fchmod()`, `fchown()`.

### 14. A daemon drops from root to a service user, but a child can still regain privilege. What went wrong?

**What the interviewer is testing**

They are testing real/effective/saved IDs, groups, capabilities, and verification.

**Strong answer**

The daemon probably performed only a temporary or partial privilege drop. A permanent drop must remove privileged real, effective, and saved UIDs/GIDs, drop supplementary groups, and clear unneeded capabilities. On Linux, `setresuid(uid, uid, uid)` and `setresgid(gid, gid, gid)` make the intent explicit, but the program must still verify the result.

**Mechanism**

setuid/setgid programs have real, effective, and saved IDs. The saved ID can allow privilege reacquisition. Capabilities can also permit privileged operations after UID changes, depending on securebits and keep-caps behavior. Supplementary groups may grant file access even after UID changes.

**Pitfalls**

Dropping UID before dropping supplementary groups can make group cleanup fail. Dropping only effective UID can leave saved root. Dropping permitted capabilities too early can break setup, but keeping them too long expands risk. Failed credential syscalls must not be ignored.

**Debug angle**

Use `strace -f -e setuid,seteuid,setresuid,setgid,setresgid,setgroups,prctl,capset,execve`, then verify with `getresuid()`, `getresgid()`, `id`, and `/proc/<pid>/status`.

**Follow-up keywords**

real UID, effective UID, saved UID, `seteuid()`, `setresuid()`, `setresgid()`, `setgroups()`, `PR_SET_KEEPCAPS`, securebits, permitted capabilities.

### 15. An incident only reproduces under systemd or in a container, not from your shell. How do you debug loader paths, capabilities, and privilege state together?

**What the interviewer is testing**

They want production debugging discipline across environment, filesystem, loader, and security state.

**Strong answer**

I would compare the exact runtime context, not just the binary. For loader issues, inspect ELF metadata and actual search results. For privilege issues, inspect UID/GID, capabilities, `NoNewPrivs`, bounding set, LSM/seccomp policy, and container namespace configuration. Then reproduce with the same service unit or container image.

**Mechanism**

systemd, containers, setuid, file capabilities, and `no_new_privs` can change environment variables, loader behavior, capabilities, file views, namespaces, and allowed syscalls. A program launched from a shell may have different `LD_LIBRARY_PATH`, different working directory, different root filesystem, and broader privileges.

**Pitfalls**

Assuming a shell reproduction is equivalent wastes time. `LD_DEBUG` and `LD_PRELOAD` may not work in secure-execution mode. File capabilities can be stripped by packaging or not honored if `no_new_privs` is set. Containers can run as UID 0 while missing host capabilities.

**Debug angle**

For libraries: `readelf -d`, `objdump -p`, `ldd` on trusted binaries, `LD_DEBUG=libs`, `strace -e openat,execve`, and `/etc/ld.so.cache`. For privilege: `grep '^Uid:\\|^Gid:\\|^Cap\\|^NoNewPrivs' /proc/<pid>/status`, `capsh --print`, service unit settings, container runtime caps, seccomp and LSM logs.

**Follow-up keywords**

systemd unit, container runtime, `CapabilityBoundingSet`, `AmbientCapabilities`, `NoNewPrivs`, `/proc/<pid>/status`, `LD_DEBUG`, `strace -e openat,execve`, namespaces, seccomp.

---

## Short Answers - Priority B

### 16. Static linking, dynamic linking, `.a`, and `.so` for an embedded product

Static linking gives a self-contained binary, useful for rescue tools, tiny root filesystems, or targets where runtime dependencies are hard to manage. Shared libraries reduce storage and memory footprint when many programs reuse code, and allow central updates, but require careful target packaging, ABI discipline, and loader path control.

### 17. ELF basics for shared-library debugging

At interview level, know that an executable and a `.so` are ELF objects. The dynamic section records dependencies such as `DT_NEEDED` and paths such as `RPATH`/`RUNPATH`. Dynamic symbols are used for runtime binding. Relocations adjust addresses after mapping. PIC makes shared object code safe to map at different virtual addresses.

### 18. Real name, soname, linker name, and `ldconfig`

The linker name is used by `-lfoo`, usually `libfoo.so`. The soname is the ABI runtime name, such as `libfoo.so.1`. The real name is the versioned file, such as `libfoo.so.1.2.3`. `ldconfig` updates cache entries and soname symlinks for installed libraries.

### 19. `RPATH`, `RUNPATH`, `$ORIGIN`, `LD_LIBRARY_PATH`, and installed paths

Installed library paths plus `ldconfig` are best for system libraries. `$ORIGIN` with `RUNPATH` is useful for relocatable application bundles. `LD_LIBRARY_PATH` is good for testing but fragile for production. `RPATH` is older and has different precedence; recognize it during debugging.

### 20. `RTLD_NOW`/`RTLD_LAZY` and `RTLD_LOCAL`/`RTLD_GLOBAL`

Use `RTLD_NOW` when you want plugin load to fail early if symbols are missing. Use `RTLD_LAZY` only when startup cost matters and late failure is acceptable. Use `RTLD_LOCAL` by default to avoid polluting global symbol lookup. Use `RTLD_GLOBAL` only for intentional plugin dependency chains.

### 21. Hiding internal symbols

A shared library should export only its documented ABI. Internal exports can become accidental compatibility promises, slow or complicate symbol lookup, and collide with symbols from the main program or other libraries. Use `static`, hidden visibility, and version scripts.

### 22. Version scripts and symbol versioning

A version script can hide every symbol except the declared public API. Symbol versioning can provide old and new ABI versions of the same symbol in one `.so`, allowing old binaries to keep binding to the old implementation while new binaries use the new one.

### 23. `dlclose()` guarantees

`dlclose()` decrements the dynamic linker's reference count for a handle. It does not guarantee immediate removal of code from the address space if other references, dependencies, or flags keep the object loaded. Destructors run only when the object is actually unloaded or at process exit.

### 24. setuid root, file capabilities, service-manager privileges, and containers

setuid root is broad and risky. File capabilities narrow the privilege on the executable. A service manager can grant capabilities, drop bounding sets, set users/groups, and use socket activation. Containers add namespaces and policy, but root inside a container is still risky if dangerous capabilities remain.

### 25. Capability sets

Permitted is the ceiling of capabilities the thread may make effective. Effective is what the kernel checks right now. Inheritable participates in capability transfer across `execve()`. Bounding limits capabilities that can be gained in future `execve()`. Ambient helps preserve selected capabilities across `execve()` for non-setuid programs, but should be used carefully.

### 26. UID/GID changes and capabilities

Linux keeps compatibility with traditional root behavior. Moving from UID 0 to nonzero can clear effective/permitted capabilities unless keep-caps or securebits change the rules. Dropping privilege correctly requires ordering: drop supplementary groups before dropping root, and verify final UID/GID/capability state.

### 27. Environment and file descriptors before `exec`

Privileged programs should rebuild a small trusted environment and avoid `PATH` search. They should close or mark unneeded descriptors close-on-exec before executing another program. Otherwise the child may inherit authority through environment-controlled behavior or open descriptors.

### 28. `chroot()` limitations

`chroot()` changes pathname resolution root, but it is not a complete sandbox. It does not remove capabilities, close descriptors, isolate processes, block syscalls, or enforce MAC policy. A privileged process may escape or still affect the system through inherited resources.

---

## Recognition Notes - Priority C

- `rtld-audit(7)` and `LD_AUDIT` hook dynamic linker events for auditing and tracing. Recognize them when investigating advanced loader behavior.
- `/etc/ld.so.preload` is a system-wide preload file. It can break the whole system if misused.
- `dlmopen()` can load objects into separate link-map namespaces. It is advanced and rarely needed in normal plugin designs.
- `dladdr()` maps an address back to shared object/symbol information. Useful for diagnostics and profilers.
- `dlvsym()` looks up a specific versioned symbol. Recognize it when working with symbol-versioned libraries.
- `RTLD_NODELETE` keeps a library loaded after `dlclose()`. `RTLD_NOLOAD` checks or promotes an already-loaded object. `RTLD_DEEPBIND` changes symbol preference toward the loaded object.
- Constructors and destructors run on load/unload. Heavy logic in them can create surprising startup/shutdown failures.
- Raw `capget()` and `capset()` exist, but `libcap` is usually the better first API.
- `securebits` and `PR_SET_KEEPCAPS` change how UID transitions interact with capabilities. Treat them as senior-level hardening details.
- `PR_SET_NO_NEW_PRIVS` makes `execve()` unable to grant new privilege via setuid, setgid, or file capabilities, and is inherited once set.
- `ldd` can be unsafe on untrusted executables because some implementations may execute code to obtain dependency information. Use `objdump -p` or `readelf -d` for safer direct dependency inspection.
- Linux ignores setuid/setgid bits on scripts, but privileged scripts remain a bad portability and security design.

---

## Extra Questions Worth Adding

1. How would you design a plugin ABI so incompatible plugins fail clearly instead of crashing the host?
2. What CI checks would catch accidental ABI breaks or accidental symbol exports?
3. How would you package private native libraries for an embedded target with limited storage?
4. How would you migrate a setuid root helper to file capabilities or service-manager privileges?
5. How would you verify that a packaged file capability survived copy, archive extraction, and deployment?
6. How would you debug a C++ service where `LD_PRELOAD` interposition works in testing but not under systemd?
7. How would you combine least privilege with defense in depth using capabilities, seccomp, AppArmor, or SELinux?
8. How would you review a privileged updater that writes files owned by root but accepts user-controlled paths?

---

## One-Minute Review

- A shared-library bug is usually a deployment, search path, soname, architecture, or ABI problem.
- Use `readelf -d`, `objdump -p`, `nm -D`, `ldd` on trusted binaries, `ldconfig -p`, `LD_DEBUG=libs`, and `strace -e openat,execve`.
- Use soname discipline: compatible minor update keeps major soname; ABI break needs a new major soname or compatibility layer.
- Prefer installed libraries or `$ORIGIN` `RUNPATH` over production `LD_LIBRARY_PATH`.
- Use `RTLD_NOW | RTLD_LOCAL` for most plugin loaders, and check `dlsym()` with `dlerror()`.
- C++ plugin entry points need `extern "C"` and exported visibility.
- `LD_PRELOAD` is powerful but high-risk; secure execution restricts loader environment variables.
- Capabilities split root privilege, but broad caps such as `CAP_SYS_ADMIN` are still dangerous.
- Debug `EPERM` by finding the failing syscall and checking effective capabilities, bounding set, UID/GID, `NoNewPrivs`, namespace, and LSM/seccomp policy.
- Avoid setuid root when a group, file capability, socket activation, or tiny helper is enough.
- Drop privileges early, permanently when possible, and verify real/effective/saved IDs plus groups and capabilities.
- Avoid `system()` and path-searched `exec` in privileged code.
- Treat argv, environment, filesystem paths, current directory, signals, and descriptors as attacker-controlled.
- Prevent FD leaks with `O_CLOEXEC` or `FD_CLOEXEC`.
- Avoid TOCTOU by validating open file descriptors, not stale pathnames.
- Use `mkstemp()`, restrictive `umask`, safe creation flags, and descriptor operations for temporary files.
- `chroot()` is only one layer, not a complete sandbox.
