#!/bin/bash
sudo apt update
sudo apt install ccache bc flex -y
# === SETUP VARIABLE ===
kernel_dir="${PWD}"
objdir="${kernel_dir}/out"
builddir="${kernel_dir}/build"
anykernel="/tmp/src/android/AnyKernel3"
CLANG_DIR="/tmp/src/android/neutron-clang"
GCC64_DIR="/tmp/src/android/gcc64/aarch64--glibc--stable-2025.08-1"
GCC32_DIR="/tmp/src/android/gcc32"
TC_DIR="/tmp/src/android/"
CONFIG_FILE="blossom_defconfig"
mkdir tmp
export CCACHE_DIR=tmp
export USE_CCACHE=1
# === ENVIRONMENT EXPORT ===
export ARCH="arm64"
export KBUILD_BUILD_USER="t.me"
export KBUILD_BUILD_HOST="AnymoreProject"
export PATH="$CLANG_DIR/bin:$GCC64_DIR/bin:$GCC32_DIR/bin:$PATH"
export CCACHE=$(command -v ccache)

# === COLOR CODES ===
NC='\033[0m'
RED='\033[0;31m'
LRD='\033[1;31m'
LGR='\033[1;32m'

# === BUILD TIMER ===
BUILD_START=0
BUILD_END=0

# === TOOLCHAIN CHECK ===
if ! [ -d "$CLANG_DIR/bin" ]; then
    echo -e "${LGR}Neutron Clang not found! Installing using AntMan...${NC}"
    mkdir -p "$CLANG_DIR"
    cd "$CLANG_DIR"
    
    # Download and run AntMan
    if ! bash <(curl -s "https://raw.githubusercontent.com/Neutron-Toolchains/antman/main/antman") -S; then
        echo -e "${LRD}AntMan installation failed! Aborting...${NC}"
        exit 1
    fi
    
    cd "$kernel_dir"
fi

# === CLEAN BUILD ===
echo -e "${LGR}Cleaning previous build...${NC}"
rm -rf "$objdir"
mkdir -p "$objdir"

# === CLEAN SOURCE TREE ===
echo -e "${LGR}Cleaning source tree with make mrproper...${NC}"
make mrproper
if [ $? -ne 0 ]; then
    echo -e "${LRD}Warning: make mrproper had issues, continuing...${NC}"
fi

# === REGENERATE DEFCONFIG IF NEEDED ===
if [ "$VAYU_CONFIG_REGEN" = "true" ]; then
    echo -e "${LGR}Regenerating defconfig...${NC}"
    make ARCH=$ARCH O=$objdir vendor/sm8150-perf_defconfig \
        vendor/debugfs.config \
        vendor/xiaomi/sm8150-common.config \
        vendor/xiaomi/vayu.config
    cp "$objdir/.config" "arch/arm64/configs/${CONFIG_FILE}"
else
    echo -e "${RED}Not regenerating config${NC}"
fi

# === FUNCTION: Generate defconfig ===
make_defconfig() {
    echo -e "${LGR}Generating defconfig...${NC}"
    make -s ARCH=$ARCH O=$objdir $CONFIG_FILE
    if [ $? -ne 0 ]; then
        echo -e "${LRD}Failed to load base defconfig${NC}"
        exit 1
    fi
    make -s ARCH=$ARCH O=$objdir LLVM=1 CC=clang olddefconfig
}

# === FUNCTION: Compile kernel ===
compile() {
    BUILD_START=$(date +%s)
    
    # Generate answers for interactive kernel config prompts
    # 2 = LTO_CLANG choice, then 6x y for remaining options

    echo -e "${LGR}######### Compiling kernel #########${NC}"
    make -j$(nproc --all) \
        O="$objdir" \
        ARCH="arm64" \
        SUBARCH="arm64" \
        DTC_EXT="dtc" \
        CLANG_TRIPLE="aarch64-linux-gnu-" \
        CROSS_COMPILE="aarch64-linux-gnu-" \
        CROSS_COMPILE_ARM32="arm-linux-gnueabi-" \
        CROSS_COMPILE_COMPAT="arm-linux-gnueabi-" \
        LD="ld.lld" \
        AR="llvm-ar" \
        NM="llvm-nm" \
        STRIP="llvm-strip" \
        OBJCOPY="llvm-objcopy" \
        OBJDUMP="llvm-objdump" \
        READELF="llvm-readelf" \
        HOSTCC="clang" \
        HOSTCXX="clang++" \
        HOSTAR="llvm-ar" \
        HOSTLD="ld.lld" \
        LLVM=1 \
        LLVM_IAS=1 \
        CC="ccache clang" \
        ${1:-}

    if [ $? -ne 0 ]; then
        echo -e "${LRD}Kernel compilation failed!${NC}"
       # exit 1
    fi

    BUILD_END=$(date +%s)
}

# === FUNCTION: Check output ===
completion() {
    local image="${objdir}/arch/arm64/boot/Image"
    local dtb_img="${objdir}/arch/arm64/boot/dtb.img"
    local output_dir="${kernel_dir}/newkernel"
    local output_kernel="${output_dir}/kernel"
    local output_dtb="${output_dir}/dtb.img"
    local root_kernel="${kernel_dir}/kernel"
    local root_dtb="${kernel_dir}/dtb.img"

    # Create output directory if it doesn't exist
    mkdir -p "$output_dir"

    if [[ -f "$image" ]]; then
        echo -e "${LGR}############################################"
        echo -e "${LGR}############# Build Successful!  ##########"
        echo -e "${LGR}############################################${NC}"
        echo -e "${LGR}Kernel Image: $image${NC}"
        
        # Copy kernel to newkernel folder
        echo -e "${LGR}Copying kernel to: $output_kernel${NC}"
        cp "$image" "$output_kernel"
        
        # Compress with gzip (maximum compression)
        echo -e "${LGR}Compressing kernel with gzip...${NC}"
        gzip -9 -f "$output_kernel"
        
        # Rename from kernel.gz back to kernel (bootloader detects compression by magic bytes)
        mv "${output_kernel}.gz" "$output_kernel"
        
        # Also copy to root for Android build system
        echo -e "${LGR}Copying kernel to root: $root_kernel${NC}"
        cp "$output_kernel" "$root_kernel"
        
        # Copy dtb.img if it exists, otherwise try to find and copy available dtb files
        if [[ -f "$dtb_img" ]]; then
            echo -e "${LGR}Copying dtb.img to: $output_dtb${NC}"
            cp "$dtb_img" "$output_dtb"
            echo -e "${LGR}Copying dtb.img to root: $root_dtb${NC}"
            cp "$dtb_img" "$root_dtb"
        else
            # Try to find mtk.dtb or other dtb files (MediaTek kernel)
            if [[ -f "${objdir}/arch/arm64/boot/mtk.dtb" ]]; then
                echo -e "${LGR}Found mtk.dtb, copying as dtb.img...${NC}"
                cp "${objdir}/arch/arm64/boot/mtk.dtb" "$output_dtb"
                cp "${objdir}/arch/arm64/boot/mtk.dtb" "$root_dtb"
            else
                # For MediaTek kernels with individual DTB files, copy the main ones
                local primary_dtb="${objdir}/arch/arm64/boot/dts/mediatek/mt6765.dtb"
                if [[ -f "$primary_dtb" ]]; then
                    echo -e "${LGR}Copying mt6765.dtb as dtb.img...${NC}"
                    cp "$primary_dtb" "$output_dtb"
                    cp "$primary_dtb" "$root_dtb"
                else
                    echo -e "${RED}Warning: No suitable DTB files found${NC}"
                    echo -e "${RED}Available DTB files:${NC}"
                    find "${objdir}/arch/arm64/boot" -name "*.dtb" 2>/dev/null | head -20
                fi
            fi
        fi
        
        echo -e "${LGR}Output locations:${NC}"
        echo -e "${LGR}  - $output_kernel${NC}"
        echo -e "${LGR}  - $root_kernel${NC}"
        if [[ -f "$output_dtb" ]]; then
            echo -e "${LGR}  - $output_dtb${NC}"
            echo -e "${LGR}  - $root_dtb${NC}"
        fi
        ls -lh "$output_kernel" "$root_kernel" ${output_dtb:+"$output_dtb"} ${root_dtb:+"$root_dtb"}
    else
        echo -e "${LRD}############################################"
        echo -e "${LRD}##      Kernel build failed :'(           ##"
        echo -e "${LRD}############################################${NC}"
       # exit 1
    fi
}

# === FUNCTION: Show build time ===
show_build_time() {
    local duration=$((BUILD_END - BUILD_START))
    local hours=$((duration / 3600))
    local minutes=$(((duration % 3600) / 60))
    local seconds=$((duration % 60))

    echo -e "${LGR}############################################${NC}"
    echo -e "${LGR} Build Time : ${hours}h ${minutes}m ${seconds}s${NC}"
    echo -e "${LGR}############################################${NC}"
}

# === PACK ANYKERNEL3 ZIP ===
pack_anykernel() {
    echo -e "${LGR}Packing AnyKernel3 ZIP...${NC}"

    if [ ! -d "$anykernel/.git" ] && [ ! -f "$anykernel/tools/ak3-core.sh" ]; then
        mkdir -p "$(dirname "$anykernel")"
        git clone --depth=1 https://github.com/osm0sis/AnyKernel3 "$anykernel" || {
            echo -e "${LRD}Failed to fetch AnyKernel3${NC}"; exit 1; }
    fi

    local image="${objdir}/arch/arm64/boot/Image"
    local dtb_src="${kernel_dir}/dtb.img"

    cd "$anykernel" || exit 1
    rm -f Image Image.gz Image.gz-dtb dtb *.zip

    # Flashable kernel payload: AK3 repacks the stock boot.img with these
    gzip -9 -c "$image" > Image.gz
    if [[ -f "$dtb_src" ]]; then
        cp "$dtb_src" dtb
    else
        echo -e "${RED}Warning: no dtb found, stock dtb stays in boot.img${NC}"
    fi

    # AK3 installer script for blossom
    cat > anykernel.sh << 'AKEOF'
### AnyKernel3 Ramdisk Mod Script
properties() { '
kernel.string=Blossom kernel
do.devicecheck=1
do.modules=0
do.systemless=0
do.cleanup=1
do.cleanuponabort=0
device.name1=blossom
device.name2=angelica
device.name3=angelican
device.name4=
device.name5=
supported.versions=
supported.patchlevels=
supported.vendorpatchlevels=
'; }

BLOCK=/dev/block/by-name/boot;
IS_SLOT_DEVICE=0;
RAMDISK_COMPRESSION=auto;
PATCH_VBMETA_FLAG=auto;

. tools/ak3-core.sh;

split_boot;
flash_boot;
AKEOF

    local ZIPNAME="Blossom-Kernel-$(date +%Y%m%d-%H%M).zip"
    zip -r9 "$ZIPNAME" . -x ".git*" "README*" "*.md" "*.zip" || exit 1
    mkdir -p "${kernel_dir}/newkernel"
    cp "$ZIPNAME" "${kernel_dir}/newkernel/"
    echo -e "${LGR}Flashable zip: ${kernel_dir}/newkernel/${ZIPNAME}${NC}"
    cd "$kernel_dir"
}

# === BUILD EXECUTION ===
make_defconfig
compile
completion
pack_anykernel
show_build_time

cd "$kernel_dir"