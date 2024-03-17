
import plotly.graph_objs as go
import numpy as np
from apps import standalone_functions as sf
from apps import daq_globals as pp
from  matplotlib import cm
import datetime


def return_radio_trig_text(xdata,key):
    a= np.array(xdata['lofar_trig'])
    d = np.array([sf.convert_to_time_str(i) for i in xdata['time']])

    d=d[a==1]
    d= np.unique(d)


    if len(d)==0:
        return ''
    else:
        if key=='last':
            text="Last Radio Trigger @ "
            return text+d[-1]
        elif key=='list':
            text="Radio Triggers (%i): "%(len(d))
            text2=', '.join(d)
            return text+text2
        else:
            return ''

def return_time_figure(det,xdata):
    a= np.array(xdata['id'])
    b= np.array(xdata['n_trigs'])
    c= np.array(xdata['time'])
    d = np.array([sf.convert_to_time_str(i) for i in xdata['time']])

    e= np.array(xdata['baseline'])
    f= np.array(xdata['sigma'])
    g= np.array(xdata['thresh'])

    select = a==det
    traces=[]
    x_ticks=[]
    x_ticklabels=[]
    if len(select[select])!=0:
        traces.append(go.Scattergl(x=c[select], y=b[select], text=d[select],
        mode='markers',
        name='N Trigs/past 5 mins'))
        traces.append(go.Scattergl(x=c[select], y=e[select], text=d[select],
        mode='markers',
        name='Avg Baseline/past 5 mins'))#,yaxis='y2'))
        traces.append(go.Scattergl(x=c[select], y=f[select], text=d[select],
        mode='markers',
        name='Avg Sigma/past 5 mins'))#,yaxis='y3'))
        traces.append(go.Scattergl(x=c[select], y=g[select], text=d[select],
        mode='markers',
        name='Threshold@GivenTime'))#,yaxis='y3'))

        x_ticks=np.linspace(np.amin(c[select]), np.amax(c[select]),20,dtype=np.int)
        x_ticklabels=[]
        prev_date = sf.strip_time(sf.convert_to_time_str(x_ticks[0]))
        x_ticklabels.append(prev_date)
        for i in range(1,len(x_ticks)):
            t_str=sf.convert_to_time_str(x_ticks[i])
            date_new = sf.strip_time(t_str)
            if date_new!=prev_date:
                x_ticklabels.append(date_new)
            else:
                x_ticklabels.append(sf.strip_date(t_str))
            prev_date = date_new

    return {
        'data':traces,
        'layout': {
            'title': 'Det %s'%det,
            'legend':{'font':{'size':pp.myparams['legendsize']}},
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'tickfont':{'size':pp.myparams['ticksize']},
                     'tickmode':'array',
                     'tickvals':x_ticks,
                     'ticktext':x_ticklabels
                     },
            'yaxis':{'title':'',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'tickfont':{'size':pp.myparams['ticksize']}},
        }
    }

def return_time_figure_2(xdata):
    b= np.array(xdata['ndet'])
    e= np.array(xdata['nsta'])
    c= np.array(xdata['time'])
    d = np.array([sf.convert_to_time_str(i) for i in xdata['time']])
    hover_txt=np.array([d[i] + "/ logQ:%.3f"%np.log10(xdata['tot_charge'][i]) for i in range(len(d))])


    traces=[]
    traces.append(go.Scattergl(x=c, y=b, text=hover_txt,
    mode='markers',
    name='N Dets'))
    traces.append(go.Scattergl(x=c, y=e, text=hover_txt,
    mode='markers',
    name='N Statns'))

    x_ticks=np.linspace(np.amin(c), np.amax(c),20,dtype=np.int)
    x_ticklabels=[]
    prev_date = sf.strip_time(sf.convert_to_time_str(x_ticks[0]))
    x_ticklabels.append(prev_date)
    for i in range(1,len(x_ticks)):
        t_str=sf.convert_to_time_str(x_ticks[i])
        date_new = sf.strip_time(t_str)
        if date_new!=prev_date:
            x_ticklabels.append(date_new)
        else:
            x_ticklabels.append(sf.strip_date(t_str))
        prev_date = date_new

    return {
        'data':traces,
        'layout': {
            'legend':{'font':{'size':pp.myparams['legendsize']}},
            'hovermode': 'closest',
            'title': '',
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'tickfont':{'size':pp.myparams['ticksize']},
                     'tickmode':'array',
                     'tickvals':x_ticks,
                     'ticktext':x_ticklabels
                     },
            'yaxis':{'title':'',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'tickfont':{'size':pp.myparams['ticksize']}},
                     'dtick':1,
        }
    }

def return_det_id_figure(d,key,cmap,title):
    b= np.array(d[key])
    a= np.array(d['id'])
    c= np.array(d['time'])
    d = np.array([sf.convert_to_time_str(i) for i in d['time']])

    c,a,b,d=np.array([[x1,x2,x3,x4] for x1,x2,x3,x4 in sorted(zip(c,a,b,d))]).T

    list_of_times=np.unique(c)

    cmap = cm.get_cmap(cmap)

    traces=[]#list of dictionaries
    n=6
    counter=0
    for time in list_of_times[-1*n*5::][::5]:
        counter+=1
        select = c==time
        temp={}
        temp['x']= a[select]
        temp['y']= b[select]
        temp['type']='Scattergl'
        temp['name']=d[select][0]
        temp['mode']='markers'
        #temp['colorscale']='Viridis'
        temp['marker']={'color':"rgb(%s,%s,%s)"%cmap(1.0*counter/n)[:3],
                        'size':12}
        #https://plot.ly/python/v3/figure-labels/#dash-example
        traces.append(temp)


    return {
        'data': traces,
        'layout': {
            'legend':{'font':{'size':pp.myparams['legendsize']}},
            'title': title,
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'Detector Id',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     'dtick':1,
                     'tickfont':{'size':pp.myparams['ticksize']}},
            'yaxis':{'title':'',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     'tickfont':{'size':pp.myparams['ticksize']}},
        }
    }

def return_run_start_time(xdata):
    d = np.array([sf.convert_to_time_str(i) for i in xdata['time']])
    if len(d)==0:
        d=['']
    return "Run Start UTC: "+d[0] +" . "

def get_last_load_time():
    a=str(datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S"))
    b=" Page Updated UTC: "
    c=" To plot older files, move time to static option first."
    return b+a+" ."

def return_evt_display_histogram(xdata):
    x=pp.geo_x
    xrange=[-240,240]
    xbinsize=5

    y=pp.geo_y
    yrange=[-240,240]
    ybinsize=5

    assert len(x)==len(y), "problem with geometry file. see det coords"
    assert len(x)==len(z), "mismatch in no. dets betwn array.log and det coords files"

    z=np.array(xdata['charges'][-1])

    traces=[]
    traces.append(
        go.Scattergl(x=pp.geo_x[z!=0], y=pp.geo_y[z!=0],text=pp.geo_id[z!=0],
                   mode='markers',
                   marker={'color':np.log10(z[z!=0]),
                            'colorscale':pp.mycmap[1:], 'size':10,
                            'colorbar':{'thickness':15,'title':'log charge'}
                          }
                    )
        )
    traces.append(
        go.Scattergl(x=pp.geo_x, y=pp.geo_y,#text=pp.geo_id,
                   mode='markers',
                   marker={'color':'rgba(0, 0, 0, 0.0)',#transparent circle.
                           'line':{'width':1,'color':'black'},'size':8,
                           })
        )


    return {
        'data': traces,
        'layout': {
            'showlegend':False,
            'hovermode': 'closest',
            'title': 'Most Recent Event',
            'titlefont':{'size':26},
            'xaxis':{'title':'X (m)',
                     'titlefont':{'size':24},
                     'zeroline':False,
                     'dtick':80,
                     'tickfont':{'size':12},
                     'showgrid':False,
                     'range':[xrange[0], xrange[1]]},
            'yaxis':{'title':'Y (m)',
                     'titlefont':{'size':24},
                     'zeroline':False,
                     'dtick':80,
                     'tickfont':{'size':12},
                     'showgrid':False,
                     'range':[yrange[0], yrange[1]]},
        }
        }

def return_core_histogram(xdata):
    x=xdata['core_x']
    xrange=[-760,760]
    xbinsize=20

    y=xdata['core_y']
    yrange=[-760,760]
    ybinsize=20

    traces=[]
    traces.append(
        go.Histogram2d(x=x, y=y,
        autobinx=False,
        xbins=dict(start=xrange[0], end=xrange[1], size=xbinsize),
        autobiny=False,
        ybins=dict(start=yrange[0], end=yrange[1], size=ybinsize),
        colorscale=pp.mycmap,
        colorbar = {'thickness':15,'title':'counts'},
        zmin=0,
        ))
    traces.append(
        go.Scattergl(x=pp.geo_x, y=pp.geo_y,text=pp.geo_id,
                   mode='markers',
                   name='Detectors',
                   marker={'color':'rgba(0, 0, 0, 0.0)',#transparent circle.
                           'line':{'width':1,'color':'black'},'size':8,
                           })
        )

    return {
        'data': traces,
        'layout': {
            'legend':{'font':{'size':pp.myparams['legendsize']}},
            'hovermode': 'closest',
            'title': 'Approx. Core Histogram',
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'X (m)',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':False,
                     'dtick':80,
                     'showgrid':False,
                     'tickfont':{'size':pp.myparams['ticksize']}},
            'yaxis':{'title':'Y (m)',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':False,
                     'showgrid':False,
                     'dtick':80,
                     'tickfont':{'size':pp.myparams['ticksize']}},
        }
        }

def return_charge_spectrum(xdata):
    q=np.log10(xdata['tot_charge'])
    # qrange=[-200,200]
    qbinsize=0.05

    traces=[]
    traces.append(
        go.Histogram( x=q, xbins=dict(size=qbinsize) )
        )

    return {
        'data': traces,
        'layout': {
            'title': 'Charge Spectrum',
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'log10(Total Charge (ADC))',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     #'dtick':40,
                     'tickfont':{'size':pp.myparams['ticksize']}},
            'yaxis':{'title':'N',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     #'dtick':40,
                     'tickfont':{'size':pp.myparams['ticksize']}},
        }
        }

def return_charge_spectrum_det(xdata,det):
    if det>24:
        det-=8
    q=xdata['charges_detwise'][det-1]
    q=q
    qbinsize=2.5
    xrangef=np.amax([int(np.median(q)),1400])

    traces=[]
    if not (q==0).all():
        traces.append(
            go.Histogram( x=q, xbins=dict(size=qbinsize) )
            )

    return {
        'data': traces,
        'layout': {
            'title': 'Det %s Charge Spectrum'%det,
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'Charge (ADC)',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     #'dtick':40,
                     'range':[-150,xrangef],
                     'tickfont':{'size':pp.myparams['ticksize']}},
            'yaxis':{'title':'N',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     'type':'log',
                     #'dtick':40,
                     'tickfont':{'size':pp.myparams['ticksize']}},
        }
        }

def return_peak_spectrum_det(xdata,det):
    if det>24:
        det-=8
    q=xdata['peaks_detwise'][det-1]
    q=q
    qbinsize=2
    xrangef=np.amax([int(np.median(q)),600])

    traces=[]
    if not (q==0).all():
        traces.append(
            go.Histogram( x=q, xbins=dict(size=qbinsize) )
            )

    return {
        'data': traces,
        'layout': {
            'title': 'Det %s Peak Spectrum'%det,
            'titlefont':{'size':pp.myparams['titlesize']},
            'xaxis':{'title':'Charge (ADC)',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     #'dtick':40,
                     'range':[-20,xrangef],
                     'tickfont':{'size':pp.myparams['ticksize']}},
            'yaxis':{'title':'N',
                     'titlefont':{'size':pp.myparams['axeslabelsize']},
                     'zeroline':True,
                     'type':'log',
                     #'dtick':40,
                     'tickfont':{'size':pp.myparams['ticksize']}},
        }
        }
