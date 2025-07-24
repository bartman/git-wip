#!/usr/bin/env bash
set -e

PROJECT=git-wip
TARGET=

HOSTNAME="$(hostname)"
USERNAME="$(id -u -n)"
USER_UID="$(id -u)"
USER_GID="$(id -g)"

# terminal colors
BLACK='\033[0;30m' RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[0;33m' BLUE='\033[0;34m' MAGENTA='\033[0;35m' CYAN='\033[0;36m' WHITE='\033[0;37m'
BRIGHT_BLACK='\033[1;30m' BRIGHT_RED='\033[1;31m' BRIGHT_GREEN='\033[1;32m' BRIGHT_YELLOW='\033[1;33m' BRIGHT_BLUE='\033[1;34m' BRIGHT_MAGENTA='\033[1;35m' BRIGHT_CYAN='\033[1;36m' BRIGHT_WHITE='\033[1;37m'
RESET='\033[0m'

function die {
    echo >&2 "ERROR: $*"
    exit 1
}

function available_targets {
    ls Dockerfile-* 2>/dev/null | sed -n 's/^Dockerfile-\([A-Za-z0-9]\+\)$/\1/p'
}

declare TARGET= DOCKERFILE= NAME=

function set_target {
    TARGET="$1"
    [ -z "$TARGET" ] && die "Target not specified"
    DOCKERFILE="Dockerfile-$TARGET"
    [ -f "$DOCKERFILE" ] || die "$DOCKERFILE: no such file"
    [ -r "$DOCKERFILE" ] || die "$DOCKERFILE: cannot be read"
    NAME="$PROJECT-builder-$USERNAME-$TARGET"
}

declare -a BUILD_ARGS=()

function set_build_args {
    local value

    # transfer host user.email to docker
    value="$(git config user.email)"
    [ -z "$value" ] && value="$USERNAME@$HOSTNAME"
    [ -n "$value" ] && BUILD_ARGS+=( "--build-arg=GIT_EMAIL=$value" )

    # transfer host user.name to docker
    value="$(git config user.name)"
    [ -z "$value" ] && value="$USERNAME"
    [ -n "$value" ] && BUILD_ARGS+=( "--build-arg=GIT_NAME=$value" )

    # Add UID, GID, and USERNAME as build arguments to match host user
    BUILD_ARGS+=( "--build-arg=UID=$USER_UID" )
    BUILD_ARGS+=( "--build-arg=GID=$USER_GID" )
    BUILD_ARGS+=( "--build-arg=USERNAME=$USERNAME" )
}

function show_status {
    local images=( $( docker images | awk "\$1 ~ /\<${NAME}\>/ { print \$3 }" ) )
    if [ "${#images[@]}" -gt 0 ] ; then
        echo -e "${NAME}: image is ${BRIGHT_GREEN}built${RESET}:    ${BRIGHT_BLUE}${images[*]}${RESET}"
    else
        echo -e "${NAME}: image is ${BRIGHT_RED}not built${RESET}."
    fi

    # Check if container is running
    local running=( $( docker ps | awk "\$2 ~ /\<${NAME}\>/ { print \$1 }" ) )
    if [ "${#running[@]}" -gt 0 ] ; then
        echo -e "${NAME}: cntnr is ${BRIGHT_GREEN}running${RESET}:  ${BRIGHT_BLUE}${running[*]}${RESET}"
    fi

    local stopped=( $( docker ps -a | awk "\$2 ~ /\<${NAME}\>/ && /Exited / { print \$1 }" ) )
    if [ "${#stopped[@]}" -gt 0 ] ; then
        echo -e "${NAME}: cntnr is ${BRIGHT_YELLOW}stopped${RESET}:  ${BRIGHT_BLUE}${stopped[*]}${RESET}"
    fi

    if [ "${#running[@]}" -eq 0 ] && [ "${#stopped[@]}" -eq 0 ] ; then
        echo -e "${NAME}: cntnr does ${BRIGHT_RED}not exist${RESET}."
    fi
}

# Function to display help
show_help() {
    cat <<END
${0##*/} <target> <command>[,<command>,...] [...]

    Available commands:

        build          - create a dev image
        remove         - remove a dev image
        start          - start the container
        stop           - stop the container
        status         - check if built/running
        connect        - get a shell in the container
        run     <cmd>  - run a command in the container

    Available targets:

END
    for t in $(available_targets); do
        echo "        $t"
    done
    echo
}

target="$1" ; shift
case "$target" in
    help|--help|-h)
        show_help
        exit 0
        ;;
    status|--status|-s)
        for t in $(available_targets) ; do
            set_target "$t"
            show_status
            echo
        done
        exit 0
        ;;
esac

set_target "$target"
cmd="$1" ; shift

case "$cmd" in
    help|--help|-h)
        show_help
        exit 0
        ;;
    *,*)
        re="^[a-z,]+$"
        [[ "$cmd" =~ $re ]] || die "bad"
        for x in ${cmd//,/ } ; do
            echo -e >&2 "# ${BRIGHT_YELLOW}$0 $x $*${RESET}"
            if ! $0 "$target" "$x" "$@" ; then
                die "failed: $0 $x $*"
            fi
        done
        exit 0
        ;;
    build)
        set_build_args
        ( set -x
        docker build "${@}" "${BUILD_ARGS[@]}" -t "${NAME}" -f "Dockerfile-$TARGET" .
        )
        exit $?
        ;;
    rm|remove)
        ( set -x
        docker image rm -f "${NAME}"
        )
        exit $?
        ;;
    up|start)
        ( set -x
        docker run -d --init -v "$(pwd):/home/$PROJECT" --user "$USER_UID:$USER_GID" --name "${NAME}" "${NAME}"
        )
        exit $?
        ;;
    down|stop)
        container_ids=( $(docker ps -a --filter "name=${NAME}" --format='{{.ID}}') )
        if [ "${#container_ids[@]}" = 0 ]; then
            echo "Container ${NAME} is not running."
        else
            for container_id in "${container_ids[@]}" ; do
                ( set -x
                docker stop "$container_id"
                )
            done
            for container_id in "${container_ids[@]}" ; do
                ( set -x
                docker rm "$container_id"
                )
            done
        fi
        exit 0
        ;;
    status)
        show_status
        exit $?
        ;;
    connect|enter)
        container_id="$(docker ps -a --filter "name=${NAME}" --format='{{.ID}}')"
        if [ -z "$container_id" ]; then
            echo "Container ${NAME} is not running."
        else
            ( set -x
            docker exec -it "$container_id" bash
            )
        fi
        ;;
    run|exec)
        container_id="$(docker ps -a --filter "name=${NAME}" --format='{{.ID}}')"
        if [ -z "$container_id" ]; then
            echo "Container ${NAME} is not running."
            exit 1
        elif [ $# -eq 0 ]; then
            echo "No command provided to run in the container."
            exit 1
        else
            ( set -x
            #docker exec -w /home/${PROJECT} "$container_id" "$@"
            docker exec -i -w /home/${PROJECT} "$container_id" /bin/bash -l -c "$*"
            )
        fi
        ;;
    *)
        die "${0##*/} [ build | remove | start | stop | status | connect ] <target>"
        ;;
esac
