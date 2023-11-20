#!/usr/bin/env python3

import sys
import os
import re
import datetime
import shutil
#import webdav3.client


def all_files(path):

    res = set()
    matcher = re.compile("([\d\-T:]*)_([\dABCDEF]*).png")

    for dirp, dirs, files in os.walk(path):
        for f in files:
            match = matcher.match(f)
            if match:
                res.add(f)

    return res




#def create_movelist(src, dst):
#    
#    res = []
#    matcher = re.compile("([\d\-T:]*)_([\dABCDEF]*).png")
#
#    for dirp, dirs, files in os.walk(path):
#        for f in files:
#            match = matcher.match(f)
#            if match:
#                date, espid = match.groups()
#                date = datetime.datetime.fromisoformat(date)
#
#                dst = os.path.join(
#                    path, 
#                    f"{date.year:04d}-{date.month:02d}", 
#                    f"{date.day:02d}",
#                    f
#                    )
#                
#                src = os.path.join(dirp, f)
#                res.append((src, dst))
#                
#    return res


def execute_movelist(ml):
    for src, dst in ml:
        dirname, basename = os.path.split(dst)
        os.makedirs(dirname, exist_ok=True)
        shutil.move(src, dst)
    return




if __name__ == "__main__":

    #env = parse_env()
    #wcfg = parse_config(env)
    #print(wcfg)

    #ml = create_movelist(sys.argv[1])
    #execute_movelist(ml)

    #check_wdav(wcfg)

    src_files = all_files(sys.argv[1])
    dst_files = all_files(sys.argv[2])

    diff = src_files.difference(dst_files)

    print(len(src_files))
    print(len(dst_files))
    print(len(diff))



