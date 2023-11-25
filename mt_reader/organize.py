#!/usr/bin/env python3

import sys
import os
import re
import datetime
import shutil
#import webdav3.client


def create_movelist(path):

    res = []
    matcher = re.compile("([\d\-T:]*)_([\dABCDEF]*).png")

    for dirp, dirs, files in os.walk(path):
        for f in files:
            match = matcher.match(f)
            if match:
                date, espid = match.groups()
                date = datetime.datetime.fromisoformat(date)

                dst = os.path.join(
                    path,
                    f"{date.year:04d}-{date.month:02d}",
                    f"{date.day:02d}",
                    f
                    )

                src = os.path.join(dirp, f)
                res.append((src, dst))

    return res


def execute_movelist(ml):
    for src, dst in ml:
        dirname, basename = os.path.split(dst)
        os.makedirs(dirname, exist_ok=True)
        shutil.move(src, dst)
    return


def parse_config(env):

    cfg = {
        'webdav_hostname': env.get("WEBDAV_HOST", ""),
        'webdav_login': env.get("WEBDAV_USER", ""),
        'webdav_password': env.get("WEBDAV_PASSWORD", ""),
        'webdav_dir' : env.get("WEBDAV_DIR", ""),
    }
    return cfg


def parse_env():
    env = {}
    with open("environment") as fd:
        lines = fd.readlines()
        lines = [l.strip() for l in lines]
        for l in lines:
            fields = l.split("=")
            if len(fields) == 2:
                key, val = fields
                env[key] = val
    return env


def check_wdav(cfg):
    wd_client = webdav3.client.Client(cfg)
    print(wd_client.list(cfg["webdav_dir"]))
    return


if __name__ == "__main__":

    #env = parse_env()
    #wcfg = parse_config(env)
    #print(wcfg)

    ml = create_movelist(sys.argv[1])
    execute_movelist(ml)

    #check_wdav(wcfg)


