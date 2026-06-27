## Exercise 1: Creating a Static Library (.a)

### Static Library Build Guide

### 1. Project Structure
```
├── bin
│   └── exam                # Final executable binary
├── inc
│   ├── calc_advance.h      # Header defining advanced calculation functions
│   └── calc_basic.h        # Header defining basic calculation functions
├── lib
│   └── static_lib
│       └── libcalc.a       # Static library containing all calculator functions
├── Makefile                # Build automation script
├── obj
│   ├── calc_advance.o      # Object file compiled from calc_advance.c
│   ├── calc_basic.o        # Object file compiled from calc_basic.c
│   └── main.o              # Object file compiled from main.c
└── src
    ├── calc_advance.c      # Source code for advanced calculation functions
    ├── calc_basic.c        # Source code for basic calculation functions
    └── main.c              # Main program source code
```

### 2. Build Instructions

#### 2.1 Compile & Create Static Library
```
make all
```
This will:
- Compile `.c` files in src/ into **object files** `(.o)` inside `obj/`
- Create a **static library** `(libcalc.a)` inside `lib/static_lib/`
- Build the executable `(exam)` in `bin/`

#### 2.2 Clean Build Files
```
make clean
```
This deletes `obj/`, `bin/`, and `lib/static_lib/`.

---

### 3. Running the Executable
```
./bin/exam
```