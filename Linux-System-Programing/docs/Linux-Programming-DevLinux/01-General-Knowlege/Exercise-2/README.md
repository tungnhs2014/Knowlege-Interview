## Exercise 2: Creating a Shared Library (.so)

## Shared Library Build Guide

### 1. Project Structure
```
├── bin/                    
│   └── exam                  # Final executable binary
├── inc/                   
│   ├── calc_advance.h        # Header defining advanced calculation functions
│   └── calc_basic.h          # Header defining basic calculation functions
├── lib/                  
│   └── shared_lib/         
│       └── libcalc.so        # Shared library containing calculator functions
├── obj/                    
│   ├── calc_advance.o        # Object file compiled from calc_advance.c with -fPIC
│   ├── calc_basic.o          # Object file compiled from calc_basic.c with -fPIC
│   └── main.o                # Object file compiled from main.c
├── src/                   
│   ├── calc_advance.c        # Source code for advanced calculation functions
│   ├── calc_basic.c          # Source code for basic calculation functions
│   └── main.c                # Main program source code
├── Makefile                  # Build automation script
```

### 2. Build Instructions

#### 2.1 Compile & Create Shared Library
```
make all
```
This will:
- Compile `.c` files in src/ into **object files** `(.o)` in `obj/` using the `-fPIC` flag to generate position-independent code.
- Create a **shared library** `(libcalc.so)` inside `lib/shared_lib/`
- Generate the executable `(exam)` in `bin/`, linking it with the shared library

#### 2.2 Clean Build Files
```
make clean
```
This deletes `obj/`, `bin/`, and `lib/shared_lib/`.

---

### 3. Running the Executable
```
export LD_LIBRARY_PATH=$PWD/lib/shared_lib:$LD_LIBRARY_PATH
./bin/exam
```

Since exam depends on `libcalc.so`, you need to set the environment variable:
`export LD_LIBRARY_PATH=$PWD/lib/shared_lib:$LD_LIBRARY_PATH`