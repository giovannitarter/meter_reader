#!/usr/bin/env python3


import os
import json
import io
import datetime
import time

from PIL import Image, ImageFont, ImageDraw
from flask import Flask, request, send_file, abort, jsonify, make_response

import webdav3.client
import requests

import logging
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    level=logging.INFO
)
#logging.getLogger('httpx').setLevel(logging.ERROR)


PHOTO_PATH = "./photo_readings"
FIRMWARE_PATH = "./firmware"
FONT_PATH = "DejaVuSansMono.ttf"


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


class PhotoReceiver():

    def __init__(self, app):
        self.app = app
        self.last_photo = None
        app.add_url_rule("/sendphoto", "sendphoto", self.post_sendphoto, methods=["POST"])
        app.add_url_rule("/lastphoto", "lastphoto", self.get_lastphoto, methods=["GET"])
        return

    def post_sendphoto(self):

        wdav_success = False
        espid = request.form.get("espid", "XXXXXXXXXXXX")
        photo = request.files.get("photo")

        if photo is not None:

            now = datetime.datetime.now().isoformat(timespec="seconds")

            img_data = photo.stream.read()
            img_data = process_image(img_data, now)
            filename = f"{now}_{espid}.png"

            f_path = os.path.join(PHOTO_PATH, filename)
            self.last_photo = f_path

            logging.info(f"saving image on: {f_path}")
            try:
                os.makedirs(PHOTO_PATH, exist_ok=True)
                fd = open(f_path, "wb")
                fd.write(img_data)
                fd.close()
                os.chown(f_path, 1000, 1000)
                os.chmod(f_path, 0o777)
            except Exception as e:
                logging.info("Error saving image")
                logging.info(e)


            logging.info(f"uploading image to webdav: {cfg['webdav_hostname']}")
            if cfg.get("webdav_hostname") != "":
                try:
                    wd_client = webdav3.client.Client(cfg)
                    wd_client.mkdir("meter_photo")
                    wd_client.upload_to(
                        img_data,
                        f"meter_photo/{filename}",
                        )
                    wdav_success = True
                except Exception as e:
                    logging.info("Error uploading to webdav")
                    logging.info(e)

        else:
            logging.error("photo is None")

        logging.info(f"image upload end")

        if wdav_success:
            if cfg.get("iamalive_url") != "":
                requests.get(cfg["iamalive_url"])

        ctime = str(int(time.time()))
        res = {
            "res" : "Ok",
            "time" : ctime,
        }
        logging.info(json.dumps(res, indent=4))

        return make_response(jsonify(res), 200)


    def get_lastphoto(self):

        if self.last_photo is None or not os.path.exists(self.last_photo):
            abort(404)

        logging.info(f"serving {self.last_photo}")
        return send_file(self.last_photo, mimetype='image/jpg')


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
    def get_manifest(self):

        f_path = os.path.join(FIRMWARE_PATH, "manifest")
        if not os.path.exists(f_path):
            abort(404)
        fd = open(f_path, "r")
        data = fd.read()
        fd.close()
        return data


    #@self.app.route('/fota/firmware', methods=["GET"])
    def get_firmware(self):

        f_path = os.path.join(FIRMWARE_PATH, "firmware.bin")
        if not os.path.exists(f_path):
            abort(404)
        fd = open(f_path, "rb")
        res = fd.read()
        fd.close()
        return res


    def post_firmware(self):

        firmware = request.files.get("firmware")
        manifest = request.files.get("manifest")

        if firmware and manifest:

            f_path = os.path.join(FIRMWARE_PATH, "firmware.bin")
            with open(f_path, "wb") as fd:
                fd.write(firmware)

            m_path = os.path.join(FIRMWARE_PATH, "manifest")
            with open(m_path, "wb") as fd:
                fd.write(manifest)
        
        else:
            abort(400)

        return





if __name__ == '__main__':

    logging.basicConfig(level=logging.INFO)

    app = Flask(__name__)

    fwup = FirmwareUpdater(app)
    phrec = PhotoReceiver(app)
    env = os.environ

    cfg = {
        'webdav_hostname': env.get("WEBDAV_HOST", ""),
        'webdav_login': env.get("WEBDAV_USER", ""),
        'webdav_password': env.get("WEBDAV_PASSWORD", ""),
        'iamalive_url' : env.get("IAMALIVE_URL", ""),
    }
    logging.info(cfg)

    app.run(debug=False, host="0.0.0.0", port=8085)
