#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import json
import numpy as np


def convert_to_plotly(histobj):
    """! Converts a "FASER type" histogram to a Plotly compatible histogram."""
    timestamp = float(histobj[: histobj.find(":")])
    histobj = json.loads(histobj[histobj.find("{") :])

    xaxis = dict(title=dict(text=histobj["xlabel"]))
    yaxis = dict(title=dict(text=histobj["ylabel"]))

    layout = dict(
        width= 500,
        height= 350,
        autosize=False,
        uirevision=True,
        xaxis=xaxis,
        yaxis=yaxis,
        margin=dict(l=50, r=50, b=50, t=30, pad=4),
    )
    data = []
    hist_type = histobj["type"]
    if "categories" in hist_type:
        xarray = histobj["categories"]
        if "hitpattern" in histobj["name"]:
          xarray = ['b'+str(x) for x in histobj["categories"]] 
        yarray = histobj["yvalues"]
        ysum=0
        for y in yarray:
          ysum+=y
        yarray = [float(y)/float(ysum) if y else 0 for y in yarray]
        data = [dict(x=xarray, y=yarray, type="bar")]

    elif "2d" in hist_type:
        zarray = np.array(histobj["zvalues"])
        xmin = float(histobj["xmin"])
        xmax = float(histobj["xmax"])
        xbins = int(histobj["xbins"])
        xstep = (xmax - xmin) / float(xbins)
        ymin = float(histobj["ymin"])
        ymax = float(histobj["ymax"])
        ybins = int(histobj["ybins"])
        ystep = (ymax - ymin) / float(ybins)
        xmin -= xstep
        ymin -= ystep
        xbins += 2
        ybins += 2
        xarray = [xmin + xbin * xstep for xbin in range(xbins)]
        yarray = [ymin + ybin * ystep for ybin in range(ybins)]
        zmatrix = zarray.reshape(len(yarray), len(xarray))
        zmaxvalue = float(np.max(zarray))
        #data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(), type="heatmap", colorscale= "Bluered")]

        # Di
        #data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(),zmin=1,zmax=zmaxvalue, type="heatmap",colorscale = [[0.0, "rgb(255, 255, 255)"],[0.2,"rgb(0,0,139)"],[0.4,"rgb(0,0,225)"],[0.6,"rgb(0,128,0)"],[0.8,"rgb(173,255,47)"],[1.0,"rgb(255,215,0)"]])]
        #data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(),zmin=1,zmax=zmaxvalue, type="heatmap",colorscale = [[0.0, "rgb(255, 255, 255)"],[0.2,"rgb(0,0,139)"],[0.4,"rgb(0,0,225)"],[0.6,"rgb(0,129,0)"],[0.8,"rgb(173,255,47)"],[1.0,"rgb(255,0,0)"]])]
#        data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(),zmin=1,zmax=zmaxvalue, type="heatmap",colorscale = [[0.0, "rgb(255, 255, 255)"],[0.2,"rgb(0,0,139)"],[0.6,"rgb(149,255,108)"], [1.0,"rgb(255,0,0)"]])]
        data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(),zmin=1,zmax=zmaxvalue, type="heatmap",colorscale = [[0.0, "rgb(255, 255, 255)"],[0.0,"rgb(0,0,139)"],[0.05,"rgb(0,0,139)"], [0.3,"rgb(69,119,237)"],[0.6,"rgb(149,255,0)"], [1.0,"rgb(255,0,0)"]])]
    else:

        if "_ext" in hist_type:
            xmin = float(histobj["xmin"])
            xmax = float(histobj["xmax"])
            xbins = int(histobj["xbins"])
            step = (xmax - xmin) / float(xbins)
            xarray = [xmin + step * xbin for xbin in range(xbins)]
            yarray = histobj["yvalues"]
            data = [dict(x=xarray, y=yarray, type="bar")]

        else:  # histogram with underflow and overflow
            xmin = float(histobj["xmin"])
            xmax = float(histobj["xmax"])
            xbins = int(histobj["xbins"])
            step = (xmax - xmin) / float(xbins)
            xmin -= step
            xbins += 2

            xarray = [xmin + step * xbin for xbin in range(xbins)]
            yarray = histobj["yvalues"]

            data = [dict(x=xarray, y=yarray, type="bar")]
    return data, layout, timestamp
