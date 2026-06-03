#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_DIR="${ROOT_DIR}/.run"
LOG_DIR="${ROOT_DIR}/logs"
VENV_PY="${ROOT_DIR}/.venv/bin/python"

OLLAMA_HOST="${OLLAMA_HOST:-127.0.0.1}"
OLLAMA_PORT="${OLLAMA_PORT:-11434}"
GRADIO_HOST="${GRADIO_HOST:-127.0.0.1}"
GRADIO_PORT="${GRADIO_PORT:-7870}"
GRADIO_LIVE=0
MODEL_ARG=()

mkdir -p "${RUN_DIR}" "${LOG_DIR}"

usage() {
    cat <<'EOF'
Usage:
  ./copilot_services.sh start|stop|restart|status [all|ollama|gradio] [options]

Services:
  all       Manage Ollama and the Gradio GUI. Default service.
  ollama    Manage only `ollama serve`.
  gradio    Manage only microscope_gradio_gui.py.

Options:
  --live               Start Gradio in LIVE GXSM4 mode.
  --dry-run            Start Gradio in dry-run mode. Default.
  --host HOST          Gradio host. Default: 127.0.0.1
  --port PORT          Gradio port. Default: 7870
  --ollama-host HOST   Ollama host. Default: 127.0.0.1
  --ollama-port PORT   Ollama port. Default: 11434
  --model MODEL        Pass model override to Gradio.

Examples:
  ./copilot_services.sh start all --live
  ./copilot_services.sh restart gradio --live --port 7870
  ./copilot_services.sh status
  ./copilot_services.sh stop all

This script stops only processes it started and recorded in .run/*.pid.
EOF
}

pid_running() {
    local pid="${1:-}"
    [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null
}

read_pid() {
    local file="$1"
    [[ -f "${file}" ]] && cat "${file}" || true
}

http_ok() {
    local url="$1"
    curl -fsS --max-time 2 "${url}" >/dev/null 2>&1
}

start_ollama() {
    local pid_file="${RUN_DIR}/ollama.pid"
    local pid
    pid="$(read_pid "${pid_file}")"
    if pid_running "${pid}"; then
        echo "Ollama already running from pidfile: ${pid}"
        return
    fi
    if http_ok "http://${OLLAMA_HOST}:${OLLAMA_PORT}/api/tags"; then
        echo "Ollama is already reachable at http://${OLLAMA_HOST}:${OLLAMA_PORT}"
        echo "Not recording a pid because this script did not start it."
        return
    fi
    if ! command -v ollama >/dev/null 2>&1; then
        echo "ERROR: ollama command not found. Install Ollama first." >&2
        return 1
    fi
    echo "Starting Ollama on ${OLLAMA_HOST}:${OLLAMA_PORT}"
    OLLAMA_HOST="${OLLAMA_HOST}:${OLLAMA_PORT}" nohup ollama serve \
        >"${LOG_DIR}/ollama.log" 2>&1 &
    echo "$!" > "${pid_file}"
    echo "Ollama pid: $(cat "${pid_file}")"
}

stop_ollama() {
    local pid_file="${RUN_DIR}/ollama.pid"
    local pid
    pid="$(read_pid "${pid_file}")"
    if pid_running "${pid}"; then
        echo "Stopping Ollama pid ${pid}"
        kill "${pid}"
        rm -f "${pid_file}"
    else
        echo "No Ollama pid recorded by this script."
        rm -f "${pid_file}"
    fi
}

start_gradio() {
    local pid_file="${RUN_DIR}/gradio.pid"
    local pid live_arg=() mode_label="dry-run"
    pid="$(read_pid "${pid_file}")"
    if pid_running "${pid}"; then
        echo "Gradio already running from pidfile: ${pid}"
        return
    fi
    if [[ ! -x "${VENV_PY}" ]]; then
        echo "ERROR: ${VENV_PY} not found. Create/install the GUI venv first:" >&2
        echo "  python3 -m venv .venv" >&2
        echo "  .venv/bin/python -m pip install -r requirements-gui.txt" >&2
        return 1
    fi
    if [[ "${GRADIO_LIVE}" == "1" ]]; then
        live_arg=(--live)
        mode_label="LIVE GXSM4"
    fi
    echo "Starting Gradio on http://${GRADIO_HOST}:${GRADIO_PORT} (${mode_label})"
    nohup "${VENV_PY}" "${ROOT_DIR}/microscope_gradio_gui.py" \
        "${live_arg[@]}" \
        --host "${GRADIO_HOST}" \
        --port "${GRADIO_PORT}" \
        "${MODEL_ARG[@]}" \
        >"${LOG_DIR}/gradio.log" 2>&1 &
    echo "$!" > "${pid_file}"
    echo "Gradio pid: $(cat "${pid_file}")"
}

stop_gradio() {
    local pid_file="${RUN_DIR}/gradio.pid"
    local pid
    pid="$(read_pid "${pid_file}")"
    if pid_running "${pid}"; then
        echo "Stopping Gradio pid ${pid}"
        kill "${pid}"
        rm -f "${pid_file}"
    else
        echo "No Gradio pid recorded by this script."
        rm -f "${pid_file}"
    fi
}

status_one() {
    local name="$1" pid_file="$2" url="$3"
    local pid
    pid="$(read_pid "${pid_file}")"
    if pid_running "${pid}"; then
        echo "${name}: running, pid ${pid}"
    else
        echo "${name}: no running pid recorded"
    fi
    if [[ -n "${url}" ]] && http_ok "${url}"; then
        echo "${name}: reachable at ${url}"
    elif [[ -n "${url}" ]]; then
        echo "${name}: not reachable at ${url}"
    fi
}

status_services() {
    status_one "Ollama" "${RUN_DIR}/ollama.pid" "http://${OLLAMA_HOST}:${OLLAMA_PORT}/api/tags"
    status_one "Gradio" "${RUN_DIR}/gradio.pid" "http://${GRADIO_HOST}:${GRADIO_PORT}"
}

cmd="${1:-status}"
service="all"
if [[ "${cmd}" == "-h" || "${cmd}" == "--help" ]]; then
    usage
    exit 0
fi
shift || true
if [[ $# -gt 0 && "${1}" != --* ]]; then
    service="$1"
    shift || true
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --live)
            GRADIO_LIVE=1
            shift
            ;;
        --dry-run)
            GRADIO_LIVE=0
            shift
            ;;
        --host)
            GRADIO_HOST="$2"
            shift 2
            ;;
        --port)
            GRADIO_PORT="$2"
            shift 2
            ;;
        --ollama-host)
            OLLAMA_HOST="$2"
            shift 2
            ;;
        --ollama-port)
            OLLAMA_PORT="$2"
            shift 2
            ;;
        --model)
            MODEL_ARG=(--model "$2")
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage
            exit 2
            ;;
    esac
done

case "${cmd}:${service}" in
    start:all)
        start_ollama
        start_gradio
        ;;
    start:ollama)
        start_ollama
        ;;
    start:gradio)
        start_gradio
        ;;
    stop:all)
        stop_gradio
        stop_ollama
        ;;
    stop:ollama)
        stop_ollama
        ;;
    stop:gradio)
        stop_gradio
        ;;
    restart:all)
        stop_gradio
        stop_ollama
        start_ollama
        start_gradio
        ;;
    restart:ollama)
        stop_ollama
        start_ollama
        ;;
    restart:gradio)
        stop_gradio
        start_gradio
        ;;
    status:all|status:ollama|status:gradio)
        if [[ "${service}" == "ollama" ]]; then
            status_one "Ollama" "${RUN_DIR}/ollama.pid" "http://${OLLAMA_HOST}:${OLLAMA_PORT}/api/tags"
        elif [[ "${service}" == "gradio" ]]; then
            status_one "Gradio" "${RUN_DIR}/gradio.pid" "http://${GRADIO_HOST}:${GRADIO_PORT}"
        else
            status_services
        fi
        ;;
    *)
        echo "Unknown command/service: ${cmd} ${service}" >&2
        usage
        exit 2
        ;;
esac
