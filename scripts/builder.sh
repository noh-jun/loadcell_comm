#!/usr/bin/env bash
set -euo pipefail

# Default values
build_shared="ON"           # default: shared(.so)
build_dir="build/loadcell_comm"
source_dir="source"
install_dir="./install"     # install prefix (directory)

usage() {
  cat <<EOF
Usage:
  $(basename "$0") [--shared|--static] [--build-dir DIR] [--source-dir DIR] [--install-dir DIR]

Options:
  --shared           Build shared library (.so). (default)
  --static           Build static library (.a).
  --build-dir DIR    Build directory. Default: build
  --source-dir DIR   Source directory. Default: source
  --install-dir DIR  Install directory (prefix). Default: ./install

Behavior:
  - Always removes previous build dir before configuring.
  - Removes previous install dir ONLY AFTER a successful build,
    then installs into a fresh install dir.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --shared)
      build_shared="ON"
      shift
      ;;
    --static)
      build_shared="OFF"
      shift
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --build-dir requires an argument" >&2; exit 2; }
      build_dir="$2"
      shift 2
      ;;
    --source-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --source-dir requires an argument" >&2; exit 2; }
      source_dir="$2"
      shift 2
      ;;
    --install-dir)
      [[ $# -ge 2 ]] || { echo "ERROR: --install-dir requires an argument" >&2; exit 2; }
      install_dir="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

# Always run relative to project root (scripts/..)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}"

if [[ ! -d "${source_dir}" ]]; then
  echo "ERROR: source dir not found: ${PROJECT_ROOT}/${source_dir}" >&2
  exit 1
fi

echo "[1/5] Remove old build dir: ${build_dir}"
rm -rf "${build_dir}"
mkdir -p "${build_dir}"

echo "[2/5] Configure (BUILD_SHARED_LIBS=${build_shared}, PREFIX=${install_dir})"
cmake -S "${source_dir}" -B "${build_dir}" \
  -DBUILD_SHARED_LIBS="${build_shared}" \
  -DCMAKE_INSTALL_PREFIX="${install_dir}"

echo "[3/5] Build"
cmake --build "${build_dir}"

echo "[4/5] Clean install dir AFTER successful build: ${install_dir}"
rm -rf "${install_dir}"
mkdir -p "${install_dir}"

echo "[5/5] Install"
cmake --install "${build_dir}"

echo "DONE"
