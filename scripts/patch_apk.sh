#!/usr/bin/env bash
# scripts/patch_apk.sh — Non-root APK patcher for Cryptonit internal injection
# Usage: ./patch_apk.sh <path/to/standoff2.apk> <path/to/cryptonit_internal.so>
# Deps: apktool, apksigner (build-tools), zipalign, python3, keytool (JDK)
set -euo pipefail

APK_IN="${1:-}"
SO_PATH="${2:-}"
WORK_DIR="$(pwd)/_patch_work"
OUT_APK="$(pwd)/cryptonit_patched.apk"

if [[ -z "$APK_IN" || -z "$SO_PATH" ]]; then
    echo "Usage: $0 <standoff2.apk> <cryptonit_internal.so>"
    exit 1
fi

if [[ ! -f "$APK_IN" ]]; then echo "APK not found: $APK_IN"; exit 1; fi
if [[ ! -f "$SO_PATH" ]]; then echo "SO not found: $SO_PATH"; exit 1; fi

for tool in apktool zipalign apksigner python3 keytool; do
    if ! command -v "$tool" &>/dev/null; then
        echo "Missing: $tool — install Android build-tools + apktool + JDK"
        exit 1
    fi
done

echo "[1/6] Decompiling APK..."
rm -rf "$WORK_DIR"
apktool d -f -o "$WORK_DIR" "$APK_IN"

echo "[2/6] Injecting cryptonit_internal.so..."
LIB_DIR="$WORK_DIR/lib/arm64-v8a"
mkdir -p "$LIB_DIR"
cp "$SO_PATH" "$LIB_DIR/libcryptonit_internal.so"

echo "[3/6] Locating Application class in smali..."
APP_SMALI=$(python3 - "$WORK_DIR" <<'PYEOF'
import os, sys, re

work = sys.argv[1]
candidates = []
for root, dirs, files in os.walk(work):
    if "smali" not in root.split(os.sep):
        continue
    for f in files:
        if not f.endswith(".smali"):
            continue
        path = os.path.join(root, f)
        with open(path, "r", errors="ignore") as fh:
            content = fh.read()
        if re.search(r"extends\s+Landroid/app/Application;", content):
            candidates.append(path)

for c in candidates:
    with open(c) as fh:
        if ".method public onCreate()V" in fh.read():
            print(c)
            break
else:
    if candidates:
        print(candidates[0])
    else:
        print("")
PYEOF
)

if [[ -z "$APP_SMALI" ]]; then
    echo "[!] No Application subclass found — injecting into MainActivity instead"
    APP_SMALI=$(python3 - "$WORK_DIR" <<'PYEOF'
import os, sys

work = sys.argv[1]
for root, dirs, files in os.walk(work):
    if "smali" not in root.split(os.sep):
        continue
    for f in files:
        if "MainActivity" in f and f.endswith(".smali"):
            print(os.path.join(root, f))
            break
    else:
        continue
    break
PYEOF
)
fi

if [[ -z "$APP_SMALI" ]]; then
    echo "[!] Could not find injection target. Patch manually."
    exit 1
fi

echo "    Target: $APP_SMALI"

echo "[4/6] Patching smali to call System.loadLibrary..."
python3 - "$APP_SMALI" <<'PYEOF'
import sys, re

path = sys.argv[1]
with open(path, "r") as fh:
    content = fh.read()

LOAD_SNIPPET = """\
    # >>> cryptonit injection <<<
    const-string v0, "cryptonit_internal"
    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
    # <<< end cryptonit >>>
"""

if "cryptonit injection" in content:
    print("    Already patched, skipping.")
    sys.exit(0)

target_method = ".method public onCreate()V"
if target_method in content:
    pattern = re.compile(
        r"(" + re.escape(target_method) + r"[^\n]*\n(?:\s+\.locals[^\n]*\n)?)",
        re.MULTILINE
    )
    new_content = pattern.sub(r"\1" + LOAD_SNIPPET, content, count=1)
else:
    STATIC_INIT = """
.method static constructor <clinit>()V
    .locals 1

    # >>> cryptonit injection <<<
    const-string v0, "cryptonit_internal"
    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
    # <<< end cryptonit >>>

    return-void
.end method

"""
    first_method = content.find(".method")
    if first_method == -1:
        print("ERROR: No .method found in target smali")
        sys.exit(1)
    new_content = content[:first_method] + STATIC_INIT + content[first_method:]

with open(path, "w") as fh:
    fh.write(new_content)

print("    Patched OK.")
PYEOF

echo "[5/6] Rebuilding APK..."
apktool b -f "$WORK_DIR" -o "${WORK_DIR}/unaligned.apk"

echo "    Aligning..."
zipalign -v -p 4 "${WORK_DIR}/unaligned.apk" "${WORK_DIR}/aligned.apk"

echo "[6/6] Signing APK..."
KEYSTORE="${HOME}/.cryptonit_debug.keystore"
if [[ ! -f "$KEYSTORE" ]]; then
    echo "    Generating debug keystore..."
    keytool -genkeypair -v \
        -keystore "$KEYSTORE" \
        -alias cryptonit \
        -keyalg RSA \
        -keysize 2048 \
        -validity 9999 \
        -storepass cryptonit123 \
        -keypass cryptonit123 \
        -dname "CN=Cryptonit,OU=Dev,O=Dev,L=Dev,ST=Dev,C=US" 2>/dev/null
fi

apksigner sign \
    --ks "$KEYSTORE" \
    --ks-key-alias cryptonit \
    --ks-pass pass:cryptonit123 \
    --key-pass pass:cryptonit123 \
    --out "$OUT_APK" \
    "${WORK_DIR}/aligned.apk"

echo ""
echo "Done: $OUT_APK"
echo ""
echo "Install (uninstall original first if needed):"
echo "  adb uninstall com.axlebolt.standoff2"
echo "  adb install -r $OUT_APK"
echo ""
echo "Note: patched APK is signed with a debug key — different from Play Store key."
echo "      Uninstall the original before installing to avoid signature mismatch."
