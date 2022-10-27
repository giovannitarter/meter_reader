from flask import Flask, request, send_file, abort

import time
import os
import json


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

            now = time.time()
            filename = f"{now}-{photo.filename}"
            print(filename)

            self.last_photo = filename

            fd = open(filename, "wb")
            fd.write(photo.stream.read())
            fd.close()
            os.chown(filename, 1000, 1000)

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
        fd = open("manifest", "r")
        data = fd.read()
        fd.close()
        return data

        #res = {
        #    "type": "esp32-fota-http",
        #    "version": 4,
        #    "host": "espcamsrv1",
        #    "port": 8085,
        #    "bin": "/fota/firmware"
        #}
        #return json.dumps(res)


    #@self.app.route('/fota/firmware', methods=["GET"])
    def get_firmware(self):
        fd = open("firmware.bin", "rb")
        res = fd.read()
        fd.close()
        return res



if __name__ == '__main__':

    app = Flask(__name__)

    fwup = FirmwareUpdater(app)
    phrec = PhotoReceiver(app)

    app.run(debug=False, host="0.0.0.0", port=8085)
