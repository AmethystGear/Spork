import os
import sys
from io import TextIOWrapper

COMPILER = 'clang'

LINKER = 'ld'

FLAGS = '-g -O0'

LINKER_FLAGS = '-g -O0'

MAIN = 'main.c'

C_FILE_EXT = '.c'
HEADER_FILE_EXT = '.h'
OBJ_FILE_EXT = '.o'

LIBRARIES = 'lib'
SOURCE = 'src'
TEST = 'tst'

SPORK_COMPILER = 'compiler_spork'
SPORK_TESTSUITE = 'testsuite_spork'


# Iterate over every file and folder in the provided directory.
# If it's a file (e.g. example.c), create a build recipe in the makefile
# that compiles it into example.o, that depends on example.c and example.h
# If it's a directory (e.g. example), create a build recipe in the makefile
# that compiles it into example.o, that depends on every file in the directory.
# finally, create a build recipe called <dir>.o which depends on all the previous .o
# recipes we made based on the files and directories contained in the provided directory.
# return <dir>.o as the object file to depend on to compile all recipes in this directory.
def add_dir_incremental_build(makefile: TextIOWrapper, dir: str):
    makefile_build = "{}: {}\n\t{}\n\n"
    objs = []
    for fname in os.listdir(dir):
        if fname == MAIN:
            continue

        file_path = os.path.join(dir, fname)
        if os.path.isfile(file_path) and fname.endswith(C_FILE_EXT):
            obj = fname[0:len(fname) - len(C_FILE_EXT)]
            header_file_path = os.path.join(dir, obj) + HEADER_FILE_EXT
            build_command = makefile_build.format(
                obj + OBJ_FILE_EXT,
                ' '.join([header_file_path, file_path]),
                ' '.join([COMPILER, '-c', file_path, FLAGS])
            )
            makefile.write(build_command)
            objs.append(obj + OBJ_FILE_EXT)
        elif os.path.isdir(file_path):
            subdir = fname
            subdir_path = file_path
            files: list[str] = []
            for fname in os.listdir(subdir_path):
                file_path = os.path.join(subdir_path, fname)
                if os.path.isfile(file_path):
                    files.append(file_path)

            c_files = ' '.join(
                filter(lambda file: file.endswith(C_FILE_EXT), files))
            build_command = makefile_build.format(
                subdir + OBJ_FILE_EXT,
                ' '.join(files),
                ' '.join([COMPILER, '-c', c_files, FLAGS,
                          '-o', subdir + OBJ_FILE_EXT])
            )
            makefile.write(build_command)
            objs.append(subdir + OBJ_FILE_EXT)

    build_command = makefile_build.format(
        dir + OBJ_FILE_EXT,
        ' '.join(objs),
        ' '.join([LINKER, '-r'] + objs + ['-o', dir + OBJ_FILE_EXT, LINKER_FLAGS])
    )
    makefile.write(build_command)
    return dir + OBJ_FILE_EXT


# add an incremental build that consists of just one c file
def add_single_file_incremental_build(makefile: TextIOWrapper, file: str, dir: str):
    makefile_build = '{1}.o: {2}/{1}.c\n\t{0} -c {2}/{1}.c {3}\n\n'.format(
        COMPILER,
        file,
        dir,
        FLAGS,
    )
    makefile.write(makefile_build)
    return file + OBJ_FILE_EXT


# add a target
def add_linker_target(makefile, target, target_deps, target_obj_deps, exec, run_exec):
    makefile_build = '{1}: {2}\n\t{0} {3} -o {4} {5}'.format(
        COMPILER,
        target,
        "{} {}".format(target_obj_deps, target_deps),
        target_obj_deps,
        exec,
        '-rdynamic'
    )

    if run_exec:
        makefile_build += ' && ./{}'.format(exec)

    makefile_build += '\n\n'
    makefile.write(makefile_build)


# generate the makefile which actually builds our code
# do incremental builds for the lib and src dirs
with open('makefile', 'w+') as makefile:
    lib = add_dir_incremental_build(makefile, LIBRARIES)
    src = add_dir_incremental_build(makefile, SOURCE)
    test = add_single_file_incremental_build(makefile, 'testsuite', TEST)
    main = add_single_file_incremental_build(makefile, 'main', SOURCE)

    add_linker_target(makefile, 'test', '', ' '.join(
        [test, lib, src]), SPORK_TESTSUITE, True)
    add_linker_target(makefile, 'all', 'test', ' '.join(
        [main, lib, src]), SPORK_COMPILER, False)

    clean_target = 'clean:\n\trm *.o {} {}\n\n'.format(
        SPORK_COMPILER,
        SPORK_TESTSUITE
    )
    makefile.write(clean_target)


# run the makefile with whatever args this program is provided
os.system(' '.join(x for x in ['make'] + sys.argv[1:]))
