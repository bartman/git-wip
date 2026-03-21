#!/usr/bin/env python3

import json
import argparse
import subprocess
import sys
import re

YELLOW = '\033[93m'
RED = '\033[91m'
BLUE = '\033[94m'
RESET = '\033[0m'
if not sys.stdout.isatty():
    YELLOW = ''
    RED = ''
    BLUE = ''
    RESET = ''

def warn(message):
    print(f"{YELLOW}Warning: {message}{RESET}")

def die(message):
    print(f"{RED}Error: {message}{RESET}")
    sys.exit(1)

def parse_args():
    parser = argparse.ArgumentParser(description="Run commands from JSON with variable substitution.", allow_abbrev=False)
    parser.add_argument('--list', action='store_true', help="List all variables in the JSON file")
    parser.add_argument('--dry-run', action='store_true', help="Print commands without executing")
    parser.add_argument('--run', action='store_true', help="Execute commands")
    parser.add_argument('--blind-run', action='store_true', help="Execute commands, ignoring failures")
    parser.add_argument('--input', dest='json_file', required=True, help="Path to the JSON file")
    parser.add_argument('remainder', nargs=argparse.REMAINDER, help="Variable substitutions after '--'")
    return parser.parse_args()

def load_json(file_path):
    with open(file_path, 'r') as file:
        return json.load(file)

def list_variables(json_data):
    variables = set()
    pattern = re.compile(r"\$\{(\w+)(?:=[^}]+)?\}")
    for command in json_data:
        for part in (command if isinstance(command, list) else [command]):
            variables.update(pattern.findall(part))
    return sorted(variables)

def substitute_variables(command, user_vars):
    pattern = re.compile(r"\$\{(\w+)(?:=([^}]+))?\}")
    result = []
    for part in command:
        def repl(match):
            key, default = match.group(1), match.group(2)
            if key in user_vars:
                return user_vars[key]
            elif default is not None:
                return default
            else:
                die(f"Missing required variable: {key}")
        substituted = pattern.sub(repl, part)
        result.append(substituted)
    return result

def execute_command(command, ignore_errors=False):
    try:
        sys.stdout.flush()
        sys.stderr.flush()
        subprocess.run(command, check=True)
        sys.stdout.flush()
        sys.stderr.flush()
        return True

    except subprocess.CalledProcessError as e:
        msg = f"Command failed: {' '.join(command)}\n{e}"
        if ignore_errors:
            warn(msg)
            return False
        die(msg)

    except Exception as e:
        msg = f"An unexpected error occurred: {e}"
        if ignore_errors:
            warn(msg)
            return False
        die(msg)

def run_commands(json_data, variables, dry_run, blind_run):
    import shlex
    ok = True
    for raw_command in json_data:
        if isinstance(raw_command, str):
            try:
                command = shlex.split(raw_command)
            except ValueError as e:
                die(f"Failed to parse string command: {raw_command}\n{e}")
        elif isinstance(raw_command, list):
            command = raw_command
        else:
            die(f"Invalid command format: {raw_command}")
        substituted = substitute_variables(command, variables)
        print(f"{BLUE}Executing: {' '.join(substituted)}{RESET}")
        if not dry_run:
            ok = ok and execute_command(substituted, ignore_errors=blind_run)
    return ok

def main():
    args = parse_args()

    try:
        json_data = load_json(args.json_file)
    except json.JSONDecodeError:
        die("Invalid JSON file")

    user_vars = {}
    for v in args.remainder:
        if '=' not in v:
            die(f"Invalid variable assignment: {v}")
        key, val = v.split('=', 1)
        user_vars[key] = val

    if args.list:
        print("Variables found in JSON:")
        for var in list_variables(json_data):
            print(var)
        return

    ok = run_commands(json_data, user_vars, dry_run=args.dry_run, blind_run=args.blind_run)
    if not ok:
        sys.exit(1)

if __name__ == "__main__":
    main()
