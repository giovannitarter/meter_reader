#!/usr/bin/env python3


import os
import json
import io
import datetime
from zoneinfo import ZoneInfo
import time
import asyncio
from tinyflux import TinyFlux, Point, TimeQuery, TagQuery

from PIL import Image, ImageFont, ImageDraw
from quart import Quart, request, send_file, abort, jsonify, make_response

import webdav3.client
import requests

import corrections

import logging
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    level=logging.INFO
)
logging.getLogger('httpx').setLevel(logging.ERROR)



PHOTO_PATH = "./photo_readings"
FIRMWARE_PATH = "./firmware"
FONT_PATH = "DejaVuSansMono.ttf"
DB_PATH = os.path.join(PHOTO_PATH, "db.csv")

def process_image(img_data, ctime):

    img = Image.open(io.BytesIO(img_data))
    img = img.rotate(180)

    font = ImageFont.truetype(FONT_PATH, 35)
    width, height = img.size
    text = str(ctime)
    img_edit = ImageDraw.Draw(img)
    img_edit.text((0, height-40), text, (237, 230, 211), font=font)

    res = io.BytesIO()
    img.save(res, format="PNG")
    res = res.getvalue()
    return res


def all_intermediate_paths(path, sep="/"):

    res = []

    if path:
        comp = path.split(sep)
        for i in range(1, len(comp)+1, 1):
            tmp = sep.join(comp[:i])
            res.append(tmp)

    return res



class PhotoReceiver():

    def __init__(self, app):
        self.app = app
        self.last_photo = None
        self.queue = asyncio.Queue()

        self.db = TinyFlux(DB_PATH)

        #self.last_sleeptime = {}

        photo_list = []
        for dir_path, _, files in os.walk(PHOTO_PATH):
            for f in files:
                if f.endswith(".png"):
                    photo_list.append((f, dir_path))

        photo_list.sort(reverse=True, key=lambda x: x[0])

        if photo_list:
            last_photo_path = os.path.join(photo_list[0][1], photo_list[0][0])
            logging.info(f"last_photo_path: {last_photo_path}")
            self.last_photo = last_photo_path
            #with open(last_photo_path, "rb") as fd:
            #    self.last_photo = fd.read()

        app.add_url_rule("/sendphoto", "sendphoto", self.post_sendphoto, methods=["POST"])
        app.add_url_rule("/lastphoto", "lastphoto", self.get_lastphoto, methods=["GET"])
        return


    def wdav_upload(self, cfg, timestamp, filename, img_data):

        wdav_success = False
        wdir = cfg.get("webdav_dir")

        subdir = f"{timestamp.year:04d}-{timestamp.month:02d}"
        daydir = f"{timestamp.day:02d}"
        wdir = f"{wdir}/{subdir}/{daydir}"
        wfile = f"{wdir}/{filename}"

        if cfg.get("webdav_hostname") != "":
            logging.info(
                    f"wdav upload to: {cfg['webdav_hostname']}{wdir}"
            )
            try:
                wd_client = webdav3.client.Client(cfg)

                for p in all_intermediate_paths(wdir):
                    wd_client.mkdir(p)

                wd_client.upload_to(
                    img_data,
                    wfile,
                    )
                wdav_success = True
            except Exception as e:
                logging.info("Error uploading to webdav")
                logging.info(e)

            logging.info(f"image upload end")
        else:
            logging.info("wdav upload not configured, skipping")

        if cfg.get("iamalive_url") != "":
            requests.get(cfg["iamalive_url"])
        else:
            logging.info("iamalive_url not configured, skipping")

        logging.info("\n")

        return


    async def manage_photo(self):

        while True:

            timestamp, img_data, espid = await self.queue.get()

            text_date = timestamp.isoformat(timespec="seconds")
            img_data = process_image(img_data, text_date)
            filename = f"{text_date}_{espid}.png"

            subdir = f"{timestamp.year:04d}-{timestamp.month:02d}"
            daydir = f"{timestamp.day:02d}"

            f_dir = os.path.join(PHOTO_PATH, subdir, daydir)
            f_path = os.path.join(f_dir, filename)

            logging.info(f"saving image on: {f_path}")
            try:
                os.makedirs(f_dir, exist_ok=True)
                fd = open(f_path, "wb")
                fd.write(img_data)
                fd.close()
                os.chown(f_path, 1000, 1000)
                os.chmod(f_path, 0o777)
                self.last_photo = f_path

            except Exception as e:
                logging.info("Error saving image")
                logging.info(e)

            loop = asyncio.get_running_loop()
            await loop.run_in_executor(None, self.wdav_upload, cfg, timestamp, filename, img_data)
            self.queue.task_done()

        return


    async def post_sendphoto(self):

        logging.info("post_sendphoto")

        espid = (await request.form).get("espid", "XXXXXXXXXXXX")
        espid = espid.strip()

        temp = (await request.form).get("temp", "XXX.XX")
        temp = temp.strip().strip('\x00')

        wkreason = (await request.form).get("wkreason", "")
        wkreason = wkreason.strip().strip('\x00')

        rawtemp = (
            (await request.form)
            .get("rawtemp", "")
            .strip()
            .strip('\x00')
            .strip(',')
            )

        photo = (await request.files).get("photo")

        if photo:
            now = datetime.datetime.now()
            img_data = photo.stream.read()
            self.queue.put_nowait((now, img_data, espid))
        else:
            logging.error("photo is None")

        ctime = int(time.time())
        wakeup_period = cfg["wakeup_period"]
        sleep_time = wakeup_period - (ctime % wakeup_period)

        correction = corrections.yesterday_correction(
                self.db, wakeup_period, espid
                )

        max_corr = 0.2
        if correction > 1 + max_corr:
            correction = 1 + max_corr
        elif correction < 1 - max_corr:
            correction = 1 - max_corr

        logging.info(f"correction: {correction}")

        if sleep_time < wakeup_period * 0.05:
            sleep_time = sleep_time + wakeup_period

        esp_boot_time = 3

        logging.info(f"sleep_time before correction: {sleep_time}")
        corr_sleep_time = int((sleep_time - esp_boot_time) * correction)
        logging.info(
                f"sleep_time correction: {sleep_time} -> {corr_sleep_time} "
                )

        res = {
            "res": "Ok",
            "ctime": ctime,
            "sleeptime": corr_sleep_time,
        }
        logging.info(json.dumps(res, indent=4))

        try:
            temp = float(temp)
        except ValueError:
            temp = float('nan')

        logging.info(f"temp: {temp}")
        logging.info(f"rawtemp: {rawtemp}")

        p = Point(
            time=datetime.datetime.now(),
            tags={
                "espid": espid,
                "wkreason": wkreason,
                },
            fields={
                "sltime": corr_sleep_time,
                "temp": temp,
                },
        )
        self.db.insert(p, compact_key_prefixes=True)

        return await make_response(jsonify(res), 200)


    async def get_lastphoto(self):

        if self.last_photo is None or not os.path.exists(self.last_photo):
            abort(404)

        logging.info(f"serving {self.last_photo}")
        return await send_file(self.last_photo, mimetype='image/jpg')


class FirmwareUpdater():

    def __init__(self, app):
        self.app = app
        app.add_url_rule("/fota/manifest", "get_manifest", self.get_manifest, methods=["GET"])
        app.add_url_rule("/fota/firmware", "get_firmware", self.get_firmware, methods=["GET"])
        app.add_url_rule(
            "/firmware",
            "post_firmware",
            self.post_firmware,
            methods=["POST"]
            )
        return


    #@self.app.route('/fota/manifest', methods=["GET"])
    async def get_manifest(self):

        f_path = os.path.join(FIRMWARE_PATH, "manifest")
        if not os.path.exists(f_path):
            abort(404)
        fd = open(f_path, "r")
        data = fd.read()
        fd.close()
        return data


    #@self.app.route('/fota/firmware', methods=["GET"])
    async def get_firmware(self):

        f_path = os.path.join(FIRMWARE_PATH, "firmware.bin")
        if not os.path.exists(f_path):
            abort(404)
        fd = open(f_path, "rb")
        res = fd.read()
        fd.close()
        return res


    async def post_firmware(self):

        files = await request.files

        firmware = files.get("firmware")
        manifest = files.get("manifest")

        if firmware and manifest:

            f_path = os.path.join(FIRMWARE_PATH, "firmware.bin")
            with open(f_path, "wb") as fd:
                fd.write(firmware.stream.read())

            m_path = os.path.join(FIRMWARE_PATH, "manifest")
            with open(m_path, "wb") as fd:
                manifest_data = manifest.stream.read()
                fd.write(manifest_data)
                manifest_data = json.loads(manifest_data.decode("utf-8"))
                logging.info(json.dumps(manifest_data, indent=4))

        else:
            abort(400)

        return {"res": "Ok"}


if __name__ == '__main__':

    logging.basicConfig(level=logging.INFO)

    app = Quart(__name__)

    fwup = FirmwareUpdater(app)
    phrec = PhotoReceiver(app)
    env = os.environ

    cfg = {
        'webdav_hostname': env.get("WEBDAV_HOST", ""),
        'webdav_login': env.get("WEBDAV_USER", ""),
        'webdav_password': env.get("WEBDAV_PASSWORD", ""),
        'webdav_dir': env.get("WEBDAV_DIR", ""),
        'iamalive_url': env.get("IAMALIVE_URL", ""),
        'wakeup_period': int(env.get("WAKEUP_PERIOD", "3600")),
    }
    logging.info(json.dumps(cfg, indent=4))

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.create_task(phrec.manage_photo())
    app.run(debug=False, host="0.0.0.0", port=8085, loop=loop)
