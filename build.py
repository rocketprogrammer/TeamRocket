import argparse
import io
import json
import os
import shutil
import subprocess
import urllib.request
import zipfile

from terminut import log

ROOT = os.path.dirname(os.path.abspath(__file__))
IMGUI_DIR = os.path.join(ROOT, "imgui")
MINHOOK_DIR = os.path.join(ROOT, "minhook")

USER_AGENT = (
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/149.0.0.0 Safari/537.36"
)


def latest_release_tag(repo):
    url = "https://api.github.com/repos/%s/releases/latest" % repo
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": USER_AGENT,
        },
    )
    with urllib.request.urlopen(request, timeout=10) as response:
        tag = json.load(response).get("tag_name")
    if not tag:
        log.fatal("Could not read latest release tag for %s." % repo)
        raise SystemExit(1)
    log.info("Latest %s release: %s" % (repo, tag))
    return tag


IMGUI_BACKENDS = ["opengl2", "dx9", "win32"]


def download_zip(url):
    log.info("Downloading %s" % url)
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request) as response:
        return zipfile.ZipFile(io.BytesIO(response.read()))


def extract_member(archive, member, dest):
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    with archive.open(member) as src, open(dest, "wb") as out:
        shutil.copyfileobj(src, out)


def fetch_imgui():
    shutil.rmtree(IMGUI_DIR, ignore_errors=True)

    tag = latest_release_tag("ocornut/imgui")
    archive = download_zip(
        "https://github.com/ocornut/imgui/archive/refs/tags/%s.zip" % tag
    )
    root = archive.namelist()[0].split("/")[0] + "/"

    wanted_backends = set()
    for name in IMGUI_BACKENDS:
        wanted_backends.add("imgui_impl_%s.cpp" % name)
        wanted_backends.add("imgui_impl_%s.h" % name)

    for member in archive.namelist():
        if member.endswith("/") or not member.startswith(root):
            continue
        rel = member[len(root) :]
        parts = rel.split("/")
        base = parts[-1]

        if len(parts) == 1 and (
            base.startswith(("imgui", "imstb_")) or base == "imconfig.h"
        ):
            extract_member(archive, member, os.path.join(IMGUI_DIR, base))
        elif parts[0] == "backends" and base in wanted_backends:
            extract_member(archive, member, os.path.join(IMGUI_DIR, "backends", base))

    log.success("imgui ready (%s)." % tag)


def fetch_minhook():
    shutil.rmtree(MINHOOK_DIR, ignore_errors=True)

    tag = latest_release_tag("TsudaKageyu/minhook")
    archive = download_zip(
        "https://github.com/TsudaKageyu/minhook/archive/refs/tags/%s.zip" % tag
    )
    root = archive.namelist()[0].split("/")[0] + "/"

    for member in archive.namelist():
        if member.endswith("/") or not member.startswith(root):
            continue
        rel = member[len(root) :]
        if rel.startswith("include/") or rel.startswith("src/"):
            extract_member(archive, member, os.path.join(MINHOOK_DIR, rel))
    log.success("minhook ready (%s)." % tag)


def find_msbuild():
    vswhere = os.path.join(
        os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"),
        "Microsoft Visual Studio",
        "Installer",
        "vswhere.exe",
    )
    if os.path.exists(vswhere):
        out = (
            subprocess.check_output(
                [
                    vswhere,
                    "-latest",
                    "-requires",
                    "Microsoft.Component.MSBuild",
                    "-find",
                    "MSBuild\\**\\Bin\\MSBuild.exe",
                ],
                text=True,
            )
            .strip()
            .splitlines()
        )
        if out:
            return out[0]
    found = shutil.which("msbuild")
    if found:
        return found
    log.fatal("MSBuild not found. Install Visual Studio Build Tools.")
    raise SystemExit(1)


def cleanup_deps():
    log.info("Removing fetched dependencies.")
    shutil.rmtree(IMGUI_DIR, ignore_errors=True)
    shutil.rmtree(MINHOOK_DIR, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description="Build the injector DLL.")
    parser.add_argument("-c", "--config", default="Release")
    args = parser.parse_args()

    fetch_imgui()
    fetch_minhook()

    msbuild = find_msbuild()
    log.info("MSBuild: %s" % msbuild)

    cmd = [
        msbuild,
        os.path.join(ROOT, "injector.sln"),
        "/m",
        "/p:Configuration=%s" % args.config,
        "/p:Platform=Win32",
        "/v:minimal",
    ]
    log.info(" ".join(cmd))
    rc = subprocess.call(cmd)

    cleanup_deps()
    if rc == 0:
        log.success("Build complete.")
    else:
        log.error("Build failed (exit %d)." % rc)
    raise SystemExit(rc)


if __name__ == "__main__":
    main()
