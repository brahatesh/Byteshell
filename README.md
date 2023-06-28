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
<div style="position: relative; padding-bottom: 55.55555555555556%; height: 0;"><iframe src="https://www.loom.com/embed/a17feb63b64c4994aee26e6b52f2f9b0?sid=9eefb4af-a8cd-4e51-ba8e-facd0f91a1a7" frameborder="0" webkitallowfullscreen mozallowfullscreen allowfullscreen style="position: absolute; top: 0; left: 0; width: 100%; height: 100%;"></iframe></div>