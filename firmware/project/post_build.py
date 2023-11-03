#!/usr/bin/env python

import os
import json
import sys
import shutil


Import("env")

#print("Current CLI targets", COMMAND_LINE_TARGETS)
#print("Current Build targets", BUILD_TARGETS)


def post_program_action(source, target, env):

    out_path = os.path.dirname(target[0].get_abspath())
    write_manifest(out_path)

    increase_version()

    deploy_artifacts(out_path)

    return


env.AddPostAction("$BUILD_DIR/firmware.bin", post_program_action)


def write_manifest(path):

    cenv = parse_envfile()

    manifest = {
        "type": "esp32-fota-http",
        "version": parse_version(),
        "host": cenv.get("SRVNAME", "espcamsrv1"),
        "port": int(cenv.get("SRVPORT", 8085)),
        "bin": "/fota/firmware",
    }

    man_path = os.path.join(path, "manifest")
    with open(man_path, "w") as fd:
        json.dump(manifest, fd)

    print(f"\nmanifest_path: {man_path}")
    print(json.dumps(manifest, indent=4))
    return


def parse_envfile():

    res = {}
    with open("environment", "r") as fd:
        for l in fd.readlines():

            l = l.strip()

            if not l:
                continue

            key, value = l.split("=")
            res[key] = value

    print(f"\nenvironment file:")
    print(json.dumps(res, indent=4))
    return res


def parse_version():
    version = 0
    with open("VERSION", "r") as fd:
        version = int(fd.read().strip())
    return version


def increase_version():

    ver = parse_version()

    with open("VERSION", "w") as fd:
        fd.write(str(ver + 1))


def deploy_artifacts(path):

    deploy_dir = os.environ.get("DEPLOY_DIR")

    if deploy_dir:
        for f in ["firmware.bin", "manifest"]:
            src = os.path.join(path, f)
            shutil.copy2(src, deploy_dir)
    else:
        print("DEPLOY_DIR not defined in environment")

