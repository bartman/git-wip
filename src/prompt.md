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



