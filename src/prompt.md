This is a C++23 project. I want to use clipp.h header only library.
Use c++23 best practices.  Use CamelCase for classes, use snake_case for method names, variable names, etc.  Use #pragma once in headers.  Use m_ prefix for member variables.
Use the latest clipp APIs.

The project is already setup.
clipp.cpp version 1.2.3 is part of the build already.
Just run `make` in the current directory to build.
You can create and modify files in src/
You cannot modify the build scripts.

generate:
src/main.cpp                # main()
src/command.hpp             # Command base class, pure virtual
src/cmd_config.hpp          # ConfigCmd :: public Command
src/cmd_delete.hpp          # DeleteCmd :: public Command
src/cmd_log.hpp             # LogCmd :: public Command
src/cmd_save.hpp            # SaveCmd :: public Command
src/cmd_status.hpp          # StatusCmd :: public Command

- Create a Command class.
  - Command::name() -> std::string   -- name of command
  - Command::desc() -> std::string   -- one line description
  - Command::run(int argc, char *argv[]) -- run this command with arguments, skipping argv[0]
- main just looks as argv[1] and dispatches to one of the Command subclasses.
- main.cpp builds a vector of available commands
- argv[1] == "help", "--help", or "-h" triggers printing help, which calls Command::name() and ::desc() for vector of commands

- Each subclassed Command implementation will use clipp to parse its options.

I want roughly the following:

1) `git-wip --help` and `git-wip help` should be equivalent...

```
$ git-wip help         # OR
$ git-wip --help

Manage Work In Progress

git-wip <command> [ --help | command options ]

    git-wip help             # this help
    git-wip status           # inspect changes
    git-wip config           # configure git-wip
    git-wip save             # save current work
    git-wip log              # look at WIP history
    git-wip delete           # cleanup

Use git-wip <command> --help to see command options.
```

It is important that each command is separate from the others and at no point should the user see all options from all commands.

3) Status doesn't do anything yet, and has no options.

```
git-wip status
```

4) Config allows listing, getting and setting options.

```
git-wip config                // list all config
git-wip config var            // get a value
git-wip config var=val        // set a variable
```

5) Save takes some options and an optional list of files...

```
git-wip save [ --editor | --untracked ] --message=<text> [ <file>... ]
```

6) Log takes optional files.

```
git-wip log [ --pretty ]  [ <file>... ]
```

7) Delete...

```
git-wip delete [ <file>... ]
```


--------------------------------------------------------------------------------------------

this is a c++ rewrite of a shell-script project git-wip.

the README currently describes how the shell script operated.
the main idea will remain; user will invoke a command like...

   git wip save "description"

which will maintain a "wip" branch for the "work in progress".

get familiar with @README.md
get familar with the old script @Attic/git-wip
note that the old script was incomplete (some features were not coded)

you will manage AGENTS.md where you will record all you learn from reviewing the old code and the current direction.

if needed, you will use @dependencies.sh to install missing packages.

you will build using `make`
you will test using `make test` and `make smoke`

for any standalone helper functions you will create unit tests in
test/unit/test_<category>.cpp and update test/unit/CMakeLists.txt

like I said this is a rewrite to c++23.  best practices.
the original work was started using @src/prompt.md
I'd say that is done now.

we need to move to the request.  I'd like you to work on:

- go through Attic/git-wip (the old implementation)
- document precisely how it works (in AGENTS.md)
- we need to remain backward compatible (at least for the commands that are invoked by the plugins: vim/ sublime/ emacs/ subdirs)

no further changes beyond this for now.

--------------------------------------------------------------------------------------------



