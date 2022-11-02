#!/usr/bin/env python3


import os
import json
import io
import datetime

from PIL import Image, ImageFont, ImageDraw
from flask import Flask, request, send_file, abort


PHOTO_PATH = "photo_readings"
FIRMWARE_PATH = "firmware"


def process_image(img_data, ctime):

    img = Image.open(io.BytesIO(img_data))
    img = img.rotate(180)

    font = ImageFont.truetype("DejaVuSansMono.ttf", 35)
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

        photo = request.files.get("photo")

        if photo is not None:

            now = datetime.datetime.now()
            img_data = photo.stream.read()
            img_data = process_image(img_data, now)
            filename = f"{now.isoformat()}.png"

            f_path = os.path.join(PHOTO_PATH, filename)
            self.last_photo = f_path

            print(f"saving image on: {f_path}")
            os.makedirs(PHOTO_PATH, exist_ok=True)
            fd = open(f_path, "wb")
            fd.write(img_data)
            fd.close()
            os.chown(f_path, 1000, 1000)
        else:
            print("photo is None")

        return "Ok"


    def get_lastphoto(self):

        if self.last_photo is None or not os.path.exists(self.last_photo):
            abort(404)

        print(f"serving {self.last_photo}")
        return send_file(self.last_photo, mimetype='image/jpg')


class FirmwareUpdater():

    def __init__(self, app):
        self.app = app
        app.add_url_rule("/fota/manifest", "get_manifest", self.get_manifest, methods=["GET"])
        app.add_url_rule("/fota/firmware", "get_firmware", self.get_firmware, methods=["GET"])
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



if __name__ == '__main__':

    app = Flask(__name__)

    fwup = FirmwareUpdater(app)
    phrec = PhotoReceiver(app)

    app.run(debug=False, host="0.0.0.0", port=8085)
