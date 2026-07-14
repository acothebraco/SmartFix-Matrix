import shutil
import subprocess
from pathlib import Path

Import("env")

PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
BUILD_DIR = Path(env.subst("$BUILD_DIR"))

APP_BIN = BUILD_DIR / "firmware.bin"
OTA_BIN = BUILD_DIR / "DIY-LED-Matrix-ota.bin"
USB_BIN = BUILD_DIR / "DIY-LED-Matrix-usb.bin"

BOOTLOADER_BIN = BUILD_DIR / "bootloader.bin"
PARTITIONS_BIN = BUILD_DIR / "partitions.bin"

FRAMEWORK_DIR = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
BOOT_APP0_CANDIDATES = [
    FRAMEWORK_DIR / "tools" / "partitions" / "boot_app0.bin",
    FRAMEWORK_DIR / "tools" / "sdk" / "esp32" / "bin" / "boot_app0.bin",
]


def find_boot_app0():
    for candidate in BOOT_APP0_CANDIDATES:
        if candidate.exists():
            return candidate
    return None


def after_build(target, source, env):
    print("\n=== DIY LED Matrix BIN Export ===")

    if not APP_BIN.exists():
        print(f"ERROR: App firmware not found: {APP_BIN}")
        return

    shutil.copyfile(APP_BIN, OTA_BIN)
    print(f"OTA BIN created: {OTA_BIN}")

    if not BOOTLOADER_BIN.exists():
        print(f"ERROR: Bootloader not found: {BOOTLOADER_BIN}")
        return

    if not PARTITIONS_BIN.exists():
        print(f"ERROR: Partitions not found: {PARTITIONS_BIN}")
        return

    cmd = [
        env.subst("$PYTHONEXE"),
        "-m",
        "esptool",
        "--chip",
        "esp32s3",
        "merge_bin",
        "-o",
        str(USB_BIN),
        "--flash_mode",
        "dio",
        "--flash_freq",
        "80m",
        "--flash_size",
        "32MB",
        "0x0",
        str(BOOTLOADER_BIN),
        "0x8000",
        str(PARTITIONS_BIN),
    ]

    boot_app0 = find_boot_app0()
    if boot_app0:
        cmd.extend([
            "0xe000",
            str(boot_app0),
        ])

    cmd.extend([
        "0x10000",
        str(APP_BIN),
    ])

    print("Creating USB full flash BIN...")
    subprocess.check_call(cmd)

    print(f"USB BIN created: {USB_BIN}")
    print("=== Export complete ===\n")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)