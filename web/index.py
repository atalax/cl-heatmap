
import json
import javascript as js
from browser import window, document, ajax

TILE_URL = "https://maps.wikimedia.org/osm-intl/{z}/{x}/{y}.png"

leaflet = window.L


class Map:

    def __init__(self):
        self.lmap = leaflet.map("map").setView([50.035, 15.776], 14)

        firstbase = None
        baselaydesc = {}
        for name, url, attr in (("Wikimedia",
                           "https://maps.wikimedia.org/osm-intl/{z}/{x}/{y}.png",
                           "&copy; <a href=\"http://osm.org/copyright\">OpenStreetMap</a> contributor"),
                          ("OpenTopoMap",
                           "https://b.tile.opentopomap.org/{z}/{x}/{y}.png",
                           "TODO")):
            layer = leaflet.tileLayer(url, {"attribution": attr})
            baselaydesc[name] = layer
            if firstbase is None:
                firstbase = layer

        firstbase.addTo(self.lmap)

        self.layer_control = leaflet.control.layers(baselaydesc, []).addTo(self.lmap)

        testlay = leaflet.tileLayer("/tiles/{z}/{x}/{y}.png")
        testlay.addTo(self.lmap)

        popup = leaflet.popup()
        def onclick(ev):
            popup.setLatLng(ev.latlng)
            popup.setContent("%f %f" % (ev.latlng.lat, ev.latlng.lng))
            popup.openOn(self.lmap)
        self.lmap.on("click", onclick)


Map()
