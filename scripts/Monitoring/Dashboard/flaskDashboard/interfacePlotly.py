#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import json
import numpy as np
import plotly.graph_objects as go


def convert_to_plotly(histobj):
    """! Converts a "FASER type" histogram to a Plotly compatible histogram."""
    timestamp = float(histobj[: histobj.find(":")])
    histobj = json.loads(histobj[histobj.find("{") :])

    xaxis = dict(title=dict(text=histobj["xlabel"]))
    yaxis = dict(title=dict(text=histobj["ylabel"]))

    layout = dict(
        width= 485,
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
        layout["hist_type"] = "categories"
        xarray = histobj["categories"]
        if "hitpattern" in histobj["name"]:
          xarray = ['b'+str(x) for x in histobj["categories"]] 
        yarray = histobj["yvalues"]
        data = [dict(x=xarray, y=yarray, type="bar")]

    elif "2d" in hist_type:
        layout["hist_type"] = "2d"
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

        data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(),zmin=1,zmax=zmaxvalue, type="heatmap",colorscale = [[0.0, "rgb(255, 255, 255)"],[0.05,"rgb(0,0,139)"], [0.3,"rgb(69,119,237)"],[0.6,"rgb(149,255,0)"], [1.0,"rgb(255,0,0)"]])]
    else:

        if "_ext" in hist_type:
            layout["hist_type"] = "_ext"
            xmin = float(histobj["xmin"])
            xmax = float(histobj["xmax"])
            xbins = int(histobj["xbins"])
            step = (xmax - xmin) / float(xbins)
            xarray = [xmin + step * xbin for xbin in range(xbins)]
            yarray = histobj["yvalues"]
            data = [dict(x=xarray, y=yarray, type="bar")]

        else:  # histogram with underflow and overflow
            layout["hist_type"] = "uoflow"
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


def plot_js_to_plot_py(ID, data,layout):
    trace = None
    if data[0]["type"] == "bar":
        trace = go.Bar(
            data[0]
            )
    elif data[0]["type"] == "heatmap":
        trace = go.Heatmap(
            data[0]
        ) 
    fig = go.Figure(data=[trace],
                layout=go.Layout(
                title=dict(
                    text=ID,
                    xanchor="center",
                    x=0.5),
                titlefont_size=15,
                showlegend=False,
                margin=dict(l=60, r=40, b=60, t=50, pad=4),
                xaxis=layout["xaxis"],
                yaxis=layout["yaxis"],
                height=layout["height"],
                width=layout["width"],
                autosize=False,
                )
        )
    return fig