# ByteShell
A simple Linux shell written in C.  

## How to run
1. Pull or download this repository
```bash
git pull https://github.com/brahatesh/Byteshell.git
```
2. cd into the directory
```bash
cd ./Byteshell/
```
3. Run the makefile by using the following command:
```bash
make -f makefile
```
4. Run the executable
```bash
./byteshell
```

## Implemented features
- Basic commands (fully implemented): `cd`, `pwd`, `exit`
- Partially implemented commands: `builtin`, `command`, `echo`, `enable`
- Piping implemented (with multiple pipes)
- Support for multiple commands in one line using ';' delimiter
- `-c` command line option to execute commands without launching the shell

## Video demonstration

[![Video Demo](https://github.com/brahatesh/Byteshell/assets/76239328/99374138-e38e-421e-b076-2dea78999f18)](https://github.com/brahatesh/Byteshell/assets/76239328/0efed324-5d6c-44f8-8ef1-7971bcbdf0cf)

