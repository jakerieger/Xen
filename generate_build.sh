#!/usr/bin/env bash
# generate_build.sh - Generate Ninja build files for all platforms

set -e

PROJECT_NAME="xen"
SRC_DIR="src"

PLATFORMS=(
    "linux:gcc:"
    "windows:x86_64-w64-mingw32-gcc:.exe"
    "macos:x86_64-apple-darwin25.2-cc:"
    "macos-arm:arm64-apple-darwin25.2-cc:"
)

CFLAGS_COMMON="-w -std=c11 -D_DEFAULT_SOURCE"
LDFLAGS="-lm"

SOURCES=$(find "$SRC_DIR" -name "*.c" | sort)

generate_ninja() {
    local platform=$1
    local compiler=$2
    local exe_suffix=$3
    local config=$4 # debug or release

    local build_dir="build/${platform}-${config}"
    local obj_dir="obj" # Relative to build_dir
    local bin_dir="bin" # Relative to build_dir
    local exe_name="${PROJECT_NAME}${exe_suffix}"
    local ninja_file="${build_dir}/build.ninja"

    local config_flags=""
    if [ "$config" = "debug" ]; then
        config_flags="-O0 -g -DDEBUG"
    else
        config_flags="-O2 -DNDEBUG"
    fi

    mkdir -p "$build_dir"

    cat >"$ninja_file" <<EOF
# Auto-generated Ninja build file for ${platform} (${config})
# Regenerate with: ./generate_build.sh

ninja_required_version = 1.5

# Variables
cc = ${compiler}
cflags = ${CFLAGS_COMMON} ${config_flags}
ldflags = ${LDFLAGS}
obj_dir = ${obj_dir}
bin_dir = ${bin_dir}

# Rules
rule compile
  command = \$cc \$cflags -c \$in -o \$out
  description = Compiling \$in

rule link
  command = \$cc \$in -o \$out \$ldflags
  description = Linking \$out

# Build statements
EOF

    local objects=""
    for src in $SOURCES; do
        local rel_src="${src#${SRC_DIR}/}"
        local obj="${obj_dir}/${rel_src%.c}.o"
        # Make source path relative to build_dir (go up two levels)
        local src_path="../../${src}"
        echo "build ${obj}: compile ${src_path}" >>"$ninja_file"
        objects="${objects} ${obj}"
    done

    echo "" >>"$ninja_file"

    echo "build ${bin_dir}/${exe_name}: link${objects}" >>"$ninja_file"
    echo "" >>"$ninja_file"

    echo "default ${bin_dir}/${exe_name}" >>"$ninja_file"
    echo "" >>"$ninja_file"

    cat >>"$ninja_file" <<EOF
# Phony targets
build run: phony ${bin_dir}/${exe_name}
  pool = console

build clean: phony
  pool = console
EOF

    echo "Generated: ${ninja_file}"
}

if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build/
    exit 0
fi

echo "Generating Ninja build files..."
echo ""

for platform_config in "${PLATFORMS[@]}"; do
    IFS=':' read -r platform compiler exe_suffix <<<"$platform_config"

    generate_ninja "$platform" "$compiler" "$exe_suffix" "debug"
    generate_ninja "$platform" "$compiler" "$exe_suffix" "release"
done

cat >build.sh <<'EOF'
#!/usr/bin/env bash
# Convenience build script

set -e

PLATFORM="${1:-linux}"
CONFIG="${2:-debug}"
BUILD_DIR="build/${PLATFORM}-${CONFIG}"

if [ ! -f "${BUILD_DIR}/build.ninja" ]; then
    echo "Error: Build files not found for ${PLATFORM}-${CONFIG}"
    echo "Run ./generate_build.sh first"
    exit 1
fi

cd "$BUILD_DIR"
ninja "$@"
EOF

chmod +x build.sh

cat >run.sh <<'EOF'
#!/usr/bin/env bash
# Run the built executable

PLATFORM="${1:-linux}"
CONFIG="${2:-debug}"
BUILD_DIR="build/${PLATFORM}-${CONFIG}"

shift 2 2>/dev/null || shift $# 2>/dev/null

EXE_NAME="xen"
if [ "$PLATFORM" = "windows" ]; then
    EXE_NAME="xen.exe"
fi

exec "${BUILD_DIR}/bin/${EXE_NAME}" "$@"
EOF

chmod +x run.sh

cat >build_all.sh <<'EOF'
#!/usr/bin/env bash
# Build all platforms and configurations

set -e

PLATFORMS=(linux windows macos macos-arm)
CONFIGS=(debug release)

echo "Building all platforms and configurations..."
echo ""

FAILED=()

for platform in "${PLATFORMS[@]}"; do
    for config in "${CONFIGS[@]}"; do
        BUILD_DIR="build/${platform}-${config}"
        
        if [ ! -f "${BUILD_DIR}/build.ninja" ]; then
            echo "[!] Skipping ${platform}-${config} (build files not found)"
            continue
        fi
        
        echo "Building ${platform}-${config}..."
        if (cd "$BUILD_DIR" && ninja); then
            echo "✓ ${platform}-${config} succeeded"
        else
            echo "✗ ${platform}-${config} failed"
            FAILED+=("${platform}-${config}")
        fi
        echo ""
    done
done

if [ ${#FAILED[@]} -eq 0 ]; then
    echo "✓ All builds succeeded!"
    exit 0
else
    echo "✗ Failed builds:"
    printf '  %s\n' "${FAILED[@]}"
    exit 1
fi
EOF

chmod +x build_all.sh

echo ""
echo "Done! Build structure created:"
echo "  build/"
echo "    ├── linux-debug/"
echo "    ├── linux-release/"
echo "    ├── windows-debug/"
echo "    ├── windows-release/"
echo "    ├── macos-debug/"
echo "    └── macos-release/"
echo ""
echo "Usage:"
echo "  ./build_all.sh                     # Build everything"
echo "  ./build.sh [platform] [config]     # Build (default: linux debug)"
echo "  ./run.sh [platform] [config]       # Run executable"
echo ""
echo "Examples:"
echo "  ./build.sh linux debug"
echo "  ./build.sh windows release"
echo "  ./build.sh macos debug"
echo "  ./run.sh linux debug"
echo ""
echo "Or use ninja directly:"
echo "  cd build/linux-debug && ninja"
echo "  cd build/windows-release && ninja"
