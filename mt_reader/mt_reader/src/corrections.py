#!/usr/bin/env python3

import tinyflux
import datetime
from zoneinfo import ZoneInfo


import logging
#logging.basicConfig(
#    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
#    level=logging.ERROR,
#)



def last_hour_correction(db, wk_period, tag, tz=ZoneInfo("Europe/Rome")):

    curr_time = datetime.datetime.now(tz)
    time_ref = (
            datetime.datetime.now(tz=tz)
            - datetime.timedelta(seconds=wk_period * 2)
            )
    timeq = tinyflux.TimeQuery()
    tagq = tinyflux.TagQuery()

    c1 = timeq > time_ref
    c2 = tagq.espid == tag

    qres = db.search(c1 & c2)
    correction = 1.0
    if qres:
        last_sleeptime = qres[-1].fields["sltime"]
        l_ctime = qres[-1].time.timestamp()

        real_sleeptime = curr_time - l_ctime

        #logging.info(f"last slp: {last_sleeptime} vs real: {real_sleeptime}")
        correction = last_sleeptime / real_sleeptime

    else:
        #logging.info("first wakeup")
        pass

    return correction


def yesterday_correction(db, wk_period, tag, tz=ZoneInfo("Europe/Rome")):

    tagq = tinyflux.TagQuery()

    timeq = tinyflux.TimeQuery()
    curr_time = datetime.datetime.now(tz)
    yesterday = curr_time - datetime.timedelta(days=1)
    ref_max = yesterday + datetime.timedelta(seconds=wk_period)
    ref_min = yesterday - datetime.timedelta(seconds=wk_period)

    #logging.info(f"curr_time: {curr_time.astimezone(tz)}")
    #logging.info(f"ref_min: {ref_min.astimezone(tz)}")
    #logging.info(f"ref_max: {ref_max.astimezone(tz)}")

    res = [(r.time, r.fields.get("sltime"))
           for r in db.search(
                (timeq > ref_min)
                & (timeq < ref_max)
                & (tagq.espid == tag)
            )
           ]

    if not res:
        return 1.0

    #logging.info("res")
    for t, sltime in res:
        #logging.info(t.astimezone(tz))
        diff = abs((yesterday - t).total_seconds())
        #logging.info(f"diff: {diff}")
    #logging.info("")

    res.sort(key=lambda x: abs((yesterday - x[0]).total_seconds()))

    prev = res[0]
    res2 = db.search(
              (timeq > prev[0])
            & (timeq < (prev[0] + datetime.timedelta(seconds=wk_period * 2)))
            & (tagq.espid == tag)
        )

    if not res2:
        return 1.0

    aft = res2[0]
    diff = (aft.time - prev[0]).total_seconds()
    correction = prev[1] / diff

    aft_time_localized = aft.time.astimezone(tz)

    #logging.info(f"nearest sample: {prev[0].astimezone(tz)}")
    #logging.info(f"following wk  : {aft_time_localized}")
    #logging.info(f"diff: {diff}, sleep: {prev[1]}")
    #logging.info(f"correction: {correction:.3f}")

    if diff < wk_period * 0.8 or diff > wk_period * 1.2:
        return 1.0

    return correction


if __name__ == "__main__":

    db = tinyflux.TinyFlux("photo_readings/db.csv")
    c = calc_correction(db, 3600, "55623A334FC4")
    print(c)

