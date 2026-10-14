#!/bin/bash -e
usage() {
    cat <<'EOF'
usage: build.sh [-h] [-v] [-D|-R] [other options...] [<target> ...]
-h/--help: show this message
-v/--verbose: verbose output
-a <arch>/--arch=<arch>: target architecture (x86_64/arm64, default: autodetect)
-c <arg>/--cmake=<arg>: additional argument for cmake, may specify multiple times
-f/--fast: do not explicitly reconfigure cmake (may ignore changes to build options)
-j <jobs>/--jobs=<jobs>: number of parallel build jobs (default: number of processors)
-m <arg>/--make=<arg>: additional argument for build tool (make/ninja/MSBuild), may specify multiple times
-p <platform>/--platform=<platform>: specify target platform (linux/macos/windows/android, default: autodetect)
-D/--debug: build Debug configuration
-R/--release: build Release configuration (default)
--fresh: clean build directory before building
<target>: target(s) to build (default if none specified: Samples)
EOF
}
sdkDir=$(cd "$(dirname "$0")/.." && pwd)

verbose=0
arch=''
platform=''
extraCmakeArgs=()
extraMakeArgs=()
jobs=''
config=Release
fresh=0
runCmake=1
while getopts ':hva:c:fj:m:p:DR-:' opt; do
    case $opt in
        -) case $OPTARG in
               help) usage; exit 0 ;;
               verbose) verbose=1 ;;
               arch=*) arch=${OPTARG#*=} ;;
               cmake=*) extraCmakeArgs+=("${OPTARG#*=}") ;;
               fast) runCmake=0 ;;
               jobs=*) jobs=${OPTARG#*=} ;;
               make=*) extraMakeArgs+=("${OPTARG#*=}") ;;
               platform=*) platform=${OPTARG#*=} ;;
               debug) config=Debug ;;
               release) config=Release ;;
               fresh) fresh=1 ;;
               *)
                   >&2 echo "Option not understood: $OPTARG"
                   >&2 usage
                   exit 1
           esac ;;
        h) usage; exit 0 ;;
        v) verbose=1 ;;
        a) arch=$OPTARG ;;
        c) extraCmakeArgs+=("$OPTARG") ;;
        f) runCmake=0 ;;
        j) jobs=$OPTARG ;;
        m) extraMakeArgs+=("$OPTARG") ;;
        p) platform=$OPTARG ;;
        D) config=Debug ;;
        R) config=Release ;;
        *)
            >&2 echo "Option not understood: $OPTARG"
            >&2 usage
            exit 1
    esac
done
shift $((OPTIND-1))
targets=("$@")
if ((!${#targets[@]})); then
    targets=(Samples)
fi

uname=$(uname -s)
case $uname in
    Linux) hostPlatform=linux ;;
    Darwin) hostPlatform=macos ;;
    *MINGW*) hostPlatform=windows ;;
    *)
        >&2 echo "What host platform is this? uname: $uname"
        exit 1
esac

case $hostPlatform in
    linux) jobs=${jobs:-$(nproc)} ;;
    macos) jobs=${jobs:-$(sysctl -n hw.logicalcpu)} ;;
    windows) jobs=${jobs:-$(nproc)} ;; # Assuming MinGW environment has it
esac

platform=${platform:-"$hostPlatform"}
case $platform in
    linux)
        cmake=cmake
        cmakeGenerator='Unix Makefiles'
        generatorSupportsConfigs=0
        makeArgs=(-j"$jobs")
        if ((verbose)); then
            makeArgs+=(VERBOSE=1)
        fi
        ;;
    macos)
        cmake=cmake
        cmakeGenerator='Unix Makefiles'
        generatorSupportsConfigs=0
        makeArgs=(-j"$jobs")
        if ((verbose)); then
            makeArgs+=(VERBOSE=1)
        fi
        ;;
    windows)
        cmake=cmake
        cmakeGenerator='Visual Studio 15 2017 Win64'
        generatorSupportsConfigs=1
        makeArgs=(-maxcpucount:"$jobs")
        if ((verbose)); then
            makeArgs+=(-verbosity:diagnostic)
        else
            makeArgs+=(-verbosity:minimal)
        fi
        ;;
    android)
        if [[ "$ANDROID_HOME" ]]; then
            androidSdk=$ANDROID_HOME
        else
            case $hostPlatform in
                macos) androidSdk=$HOME/Library/Android/sdk ;;
                windows) androidSdk=$HOME/AppData/Local/Android/Sdk ;;
                *)
                    >&2 echo "Don't know where to find Android SDK on host platform: $hostPlatform"
                    >&2 echo "Set ANDROID_HOME to specify SDK location"
                    exit 1
            esac
            if [[ ! -d "$androidSdk" ]]; then
                >&2 echo "Android SDK not found: $androidSdk"
                >&2 echo "Set ANDROID_HOME to specify SDK location"
                exit 1
            fi
            export ANDROID_HOME="$androidSdk"
        fi
        androidCmakeBin=''
        while IFS= read -r -d '' dir; do
            androidCmakeBin=$dir/bin
        done < <(find -P "$androidSdk/cmake" -xdev -mindepth 1 -maxdepth 1 -type d -regex '^.*/[0-9][0-9]*\(\.[0-9][0-9]*\)*$' -print0 | sort -Vz)
        if [[ ! "$androidCmakeBin" ]]; then
            >&2 echo "Could not find cmake/<version>/bin in Android SDK: $androidSdk"
            exit 1
        fi
        cmake=$androidCmakeBin/cmake
        cmakeGenerator='Ninja'
        generatorSupportsConfigs=0
        makeArgs=(-j"$jobs")
        if ((verbose)); then
            makeArgs+=(-v)
        fi
        ;;
    *)
        >&2 echo "Unexpected platform: $platform"
        exit 1
esac

if ((verbose)); then
    set -x
fi

if [[ "${cmake/'/'/}" = "$cmake" ]] && ! command -v "$cmake" >/dev/null; then
    >&2 echo "CMake is required, but is not installed or is not in your PATH."
    exit 1
fi

cmakeArgs=(
    -G "$cmakeGenerator"
)
if ((!generatorSupportsConfigs)); then
    cmakeArgs+=(-DCMAKE_BUILD_TYPE="$config")
fi
if ((verbose)); then
    cmakeArgs+=(--debug-output)
fi

arch=${arch:-$(uname -m)}
case $arch in
    x86_64) targetArch=x86_64 ;;
    arm64|aarch64) targetArch=arm64 ;;
    # Sanity check. If you really are building on something else, add a clause above
    *)
        >&2 echo "Unexpected architecture: $arch"
        exit 1
esac
cmakeArgs+=(-DSCSDK_TARGET_ARCH="$targetArch")

if [[ "$platform" = android ]]; then
    ndkPlatform=android-24
    case $targetArch in
        arm64)
            ndkPlatformArch=arch-arm64
            androidAbi=arm64-v8a
            ;;
        *)
            >&2 echo "Don't know Android ABI name for target arch: $targetArch"
            exit 1
    esac
    cmakeArgs+=(
        -DANDROID:BOOL=1
        -DCMAKE_TOOLCHAIN_FILE="$androidSdk/ndk-bundle/build/cmake/android.toolchain.cmake"
        -DANDROID_ABI="$androidAbi"
        -DANDROID_PLATFORM="$ndkPlatform"
        -DANDROID_TOOLCHAIN=clang
        -DANDROID_STL=c++_shared
        -DANDROID_CPP_FEATURES='rtti;exceptions'
        -DANDROID_PIE=ON
        -DANDROID_ALLOW_UNDEFINED_SYMBOLS=FALSE
        -DANDROID_DISABLE_NO_EXECUTE=FALSE
        -DANDROID_DISABLE_RELRO=FALSE
        -DANDROID_DISABLE_FORMAT_STRING_CHECKS=FALSE
        -DANDROID_ARM_MODE=arm
        -DANDROID_ARM_NEON=true
        -DCMAKE_MAKE_PROGRAM="$androidCmakeBin/ninja"
        -DSCSDK_LINK_TIME_RPATH="$androidSdk/ndk-bundle/platforms/$ndkPlatform/$ndkPlatformArch/usr/lib"
    )
fi

if ((generatorSupportsConfigs)); then
    buildDir=$sdkDir/Builds/$platform-$targetArch
else
    buildDir=$sdkDir/Builds/$platform-$(printf '%s' "$config" | tr A-Z a-z)-$targetArch
fi
mkdir -p "$buildDir"
if ((fresh)); then
    find -P "$buildDir" -xdev -mindepth 1 -delete
fi

(
    set -e
    cd "$buildDir"
    if ((runCmake)) || [[ ! -e did-initial-cmake ]]; then
        "$cmake" "${cmakeArgs[@]}" "${extraCmakeArgs[@]}" "$sdkDir"
        >did-initial-cmake echo 'Generated by build.sh'
    fi
    for target in "${targets[@]}"; do
        if ((generatorSupportsConfigs)); then
            "$cmake" --build . --config "$config" --target "$target" -- "${makeArgs[@]}" "${extraMakeArgs[@]}"
        else
            "$cmake" --build . --target "$target" -- "${makeArgs[@]}" "${extraMakeArgs[@]}"
        fi
    done
)
