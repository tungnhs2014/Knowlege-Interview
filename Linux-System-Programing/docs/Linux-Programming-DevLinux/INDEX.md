# Linux Programming DevLinux - Module Index

Source goc: *Linux Programming for DevLinux*.

Day la **entry point** cho AI khi su dung source:

```
docs/Linux-Programming-DevLinux/
```

Muc tieu cua source nay:

- bo sung goc nhin thuc hanh cho Linux System Programming;
- cung cap workflow build/run, code examples, exercises, mini-projects;
- lam cau noi giua concept he thong va cach ap dung trong project thuc te.

Source nay **khong thay the hoan toan** cho TLPI.  
TLPI van la nguon uu tien cho semantics, API behavior, design reasoning.  
DevLinux nen duoc dung de bo sung:

- practical intuition;
- build and tool usage;
- example organization;
- exercise/project context.

---

## Cach Doc Source Nay

Khi learning map chi dinh DevLinux cho mot topic, AI phai doc theo thu tu:

1. `docs/Linux-Programming-DevLinux/INDEX.md`
2. `docs/Linux-Programming-DevLinux/README.md`
3. module README tuong ung
4. exercise/project files lien quan neu can vi du thuc hanh

Neu mot topic duoc map ca TLPI va DevLinux:

- doc ca hai nguon;
- uu tien correctness va semantics tu TLPI;
- dung DevLinux de bo sung intuition, build flow, examples, project context.

---

## Module Index

| Module | Folder | Main Coverage | Typical Use |
|---|---|---|---|
| 01 | `01-General-Knowlege/` | Makefile, compile pipeline, static/shared libraries | build basics, library workflow, toolchain context |
| 02 | `02-Linux-File-System/` | file system basics, file operations, internals, locking | practical filesystem intuition |
| 03 | `03-Linux-Process/` | programs vs processes, memory layout, creation/execution, management | process examples and lifecycle review |
| 04 | `04-IPC-Signal/` | signals, handling, management/control | signal practice and examples |
| 05 | `05-Thread/` | thread basics, lifecycle, synchronization | pthread practical learning |
| 06 | `06-IPC-Socket/` | socket concepts, communication flow, internet/unix domain | networking/socket practice |
| 07 | `07-Mini-Project-Chat-Application/` | chat application project | integrated socket/thread practice |
| 08 | `08-IPC-Pipes-FIFOs/` | pipes, FIFOs, client-server with FIFOs | IPC practice for pipes/FIFOs |
| 09 | `09-IPC-Message-Queues/` | IPC overview, System V/POSIX message queues | message queue practice |
| 10 | `10-IPC-Shared-Memory/` | shared memory intro, System V, POSIX | shared memory practice |
| 11 | `11-IPC-Semaphore/` | semaphore intro, core ops, System V/POSIX | synchronization practice |
| 12 | `12-Final-Project-Sensor-Monitoring-System/` | multi-thread/multi-process sensor monitoring system | integrated final project reference |

---

## Mapping Hints

Source nay duoc su dung thong qua:

```
LINUX_SYSTEM_LEARNING_MAP.md
```

AI phai lay mapping tu learning map truoc, khong duoc doan module theo cam tinh.

Vi du:

- Topic `10.1 Shared Library Fundamentals` -> DevLinux `01`
  - Doc `01-General-Knowlege/README.md`
  - Tap trung vao section `1.3 Static and Shared Libraries in Linux`

- Topic `6.x Threads` -> DevLinux `05`
  - Doc `05-Thread/README.md`
  - Mo them `Exercise-*` neu can vi du practical

- Topic `8.x Sockets` -> DevLinux `06`
  - Doc `06-IPC-Socket/README.md`
  - Can co the tham khao project `07-Mini-Project-Chat-Application/`

- Topic `7.2 Pipes & FIFOs` -> DevLinux `08`
- Topic `7.8 POSIX Message Queues` / `7.4 System V Message Queues` -> DevLinux `09`
- Topic `7.10 POSIX Shared Memory` / `7.6 System V Shared Memory` -> DevLinux `10`
- Topic `7.9 POSIX Semaphores` / `7.5 System V Semaphores` -> DevLinux `11`

---

## Reading Notes

- Cac module README thuong la training note tong hop, giai thich de hieu va practical.
- Cac `Exercise-*` thuong chua code va Makefile, phu hop de lay vi du build/run structure.
- Cac module project (`07`, `12`) nen duoc doc khi can integration context, khong phai luc nao cung can.

---

## Limits of This Source

DevLinux khong bao phu day du moi topic trong roadmap Linux System.

Nhung topic duoi day thuong can TLPI lam source chinh:

- terminal internals / PTY semantics;
- many advanced process, security, and linker semantics;
- detailed standards-level behavior and portability rules.

Do do, khi DevLinux co noi dung lien quan, AI nen xem no la:

- source thuc hanh bo sung;
- khong phai phep thay the cho semantic detail tu TLPI.

---

## Recommended Workflow

Cho moi topic:

1. Doc `CODEX.md`
2. Doc `LINUX_SYSTEM_LEARNING_MAP.md`
3. Doc `INDEX.md` cua source can dung
4. Doc training docs/module docs lien quan
5. Giai thich va hoi dap
6. Distill thanh knowledge doc

Neu topic co DevLinux mapping, AI khong duoc bo qua source nay ma khong neu ro ly do.
