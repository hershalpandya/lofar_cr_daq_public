# Dashboard for monitoring LORA DAQ
# -------
# Author: Hershal Pandya, VUB.
# -------

import numpy as np
import glob,sys

from dash import dcc, html, Input, Output, callback
from dash.exceptions import PreventUpdate

from apps import daq_globals as globvars
from apps import daq_return_figures as get_figs
from apps import standalone_functions as sf

#---------------------------------------------------------#
#FUNCTIONS UPDATING GLOBAL DATA HOLDING VARS
#---------------------------------------------------------#

def get_file_list():
    k=sorted(glob.glob(logfile_path + "/20*array.log")+glob.glob(logfile_path + "/*/20*array.log") )[-50:]
    k=k[::-1]
    j=[m.split('_array')[0] for m in k]
    globvars.drop_down_value=j[0]
    dictlist=[]
    dictlist.append({'label':'Auto','value':'Auto'})
    for i in j:
        dictlist.append({'label':str(i),'value':str(i)})
    return dictlist

def load_station_file_current():
    globvars.sta_info_current=sf.load_sta_info_file(globvars.drop_down_value+"_detectors.log")
    return

def load_array_file_current():
    globvars.arr_info_current=sf.load_array_info_file(globvars.drop_down_value+"_array.log")
    return

#---------------------------------------------------------#
#---------------------------------------------------------#

logfile_path = "../logfiles/"

globvars.drop_down_value=sorted(glob.glob(logfile_path + "/20*_array.log")+glob.glob(logfile_path + "/*/20*_array.log"))[-1].split("_array")[0]
globvars.radio_button_val=15e3

#initiate data.
load_station_file_current()
load_array_file_current()
currently_active_detectors = np.sort(np.unique(globvars.sta_info_current['id']))
globvars.remember_det_selection= currently_active_detectors[0]


layout = html.Div([

    html.H4('Default Shows DAQ Live. Select a File To See Past Days.',
            style={'textAlign':'center',
            'font-size':'200%',
            'display': 'inline-block'}),

    html.Div(
        id='dropdown_parent',
        children=[
            dcc.Dropdown(
                id='date_dropdown',
                searchable=False,
                # options=get_file_list(),
                placeholder="Select a Date",
                # value=globvars.drop_down_value,
                clearable=False,
                style={'font-size': "130%",'width': '60%'}
            )
        ]
    ),

    html.H4(id='reload_data',style={'color':'#1f77b4'}),
    html.H4(id='run_start_time',style={'color':'#1f77b4','display':'inline-block'}),
    html.H4(id='last_update_time',style={'display':'inline-block','color':'#ff7f0e'}),

    dcc.RadioItems(
        id='live_update_options',
        options=[
            {'label': 'Update Every 10s', 'value': 5*1e3},
            {'label': 'Update Every 15s', 'value': 15*1e3},
            {'label': 'Update Every 30s', 'value': 30*1e3},
            {'label': 'Static', 'value': 1e9},
        ],
        value=globvars.radio_button_val,
        style={'font-size': "120%"}
    ),

    html.Hr(),
    html.H5(id='list_radio_trigs',
            ),
    html.Hr(),

    dcc.Interval(
            id='interval-component',
            n_intervals=0
        ),

    html.Div([
        dcc.Graph(
        id='ndet_vs_time',
        style={'width':'100%','padding': globvars.myparams['paddingsize']}),],),

    html.Hr(),

    html.Div([
        html.Span(id='hspace0',
                  style={'display':'inline-block',
                          'width': '15%'}
                ),
        dcc.Graph(
            id='core_hist2d',
            style={'width':550, 'height':550,'display': 'inline-block'}
            ),

        html.Span(id='hspace',
                  style={'display':'inline-block',
                          'width': '10%'}
                ),
        dcc.Graph(
            id='charge_spectrum',
            style={'width':550,'height':550,'display': 'inline-block'}
            ),
        ],style={'height':550,'padding': globvars.myparams['paddingsize']}),

    html.Hr(),

    dcc.Graph(
        id='det_rate_in_last_five_mins',
        style={'width':'100%'}
    ),

    html.Hr(),

    html.Div([
        html.Label('Select Detector'),
        dcc.RadioItems(id='select_detector'),
    ]),

    html.Div([
        dcc.Graph(id='time_plot_2'),],
        style={'padding': globvars.myparams['paddingsize']}),

    html.Div([
        html.Span(id='hspace05',
                  style={'display':'inline-block',
                          'width': 250}
                ),
        dcc.Graph(
            id='charge_spectrum_det',
            style={'width':600,'display': 'inline-block'}
            ),

        html.Span(id='hspace06',
                  style={'display':'inline-block',
                          'width': 50}
                ),
        dcc.Graph(
            id='peak_spectrum_det',
            style={'width':600,'display': 'inline-block'}
            ),
        ],style={'padding': globvars.myparams['paddingsize']}),

    ],style={'height':'60%'})

@callback(Output('date_dropdown', 'options'),
              [Input('dropdown_parent', 'n_clicks')])
def load_dropdown_options(n_clicks):
    if n_clicks is None:
        raise PreventUpdate
    return get_file_list()

@callback(
    Output('reload_data', 'children'),
    [Input('date_dropdown', 'value'),Input('interval-component', 'n_intervals')])
def update_output(value,n):
    print ('update_output',value, n)

    if (value is None) or (value=='Auto'):
        #means its simply n_intervals driven update.
        #find latest file and update page.
        latest_drop_down_value=sorted(glob.glob(logfile_path + "/20*_array.log")
                                      +glob.glob(logfile_path + "/*/20*_array.log"))[-1].split("_array")[0]
        globvars.drop_down_value=latest_drop_down_value
        load_station_file_current()
        load_array_file_current()
        error_msg="Page Updated With Latest File."
    elif (value==globvars.drop_down_value):
        error_msg="File selection not Auto. Page not updated."
    else:
        #value has been picked.
        #make page static and plot this selected file.
        globvars.drop_down_value=value
        load_station_file_current()
        load_array_file_current()
        error_msg="Page Updated as File Changed."
    return error_msg

@callback(Output('interval-component','interval'),
              [Input('live_update_options','value')])
def radio_button_liveupdate(value):
    globvars.radio_button_val = value
    return globvars.radio_button_val

@callback([
        Output('time_plot_2', 'figure'),
        Output('charge_spectrum_det', 'figure'),
        Output('peak_spectrum_det', 'figure'),
        ],
        [Input('select_detector', 'value')])
def radio_button_update(det):
    fig2=get_figs.return_time_figure(det,globvars.sta_info_current)
    fig3=get_figs.return_charge_spectrum_det(globvars.arr_info_current,det)
    fig4=get_figs.return_peak_spectrum_det(globvars.arr_info_current,det)
    globvars.remember_det_selection=det
    return fig2,fig3,fig4

@callback([Output('select_detector','options')],
               [Input('interval-component','n_intervals')])
def update_det_list(n):
    currently_active_detectors = np.sort(np.unique(globvars.sta_info_current['id']))
    radio_currently_active_detectors = []
    for ixy in range(1,41):
        if ixy in currently_active_detectors:
            radio_currently_active_detectors.append({'label':'%i'%ixy, 'value':ixy})
        else:
            radio_currently_active_detectors.append({'label':sf.strikethrough('%i'%ixy), 'value':ixy,'disabled':True})
    return [radio_currently_active_detectors]

@callback([Output('det_rate_in_last_five_mins', 'figure'),
               Output('select_detector', 'value'),
               Output('last_update_time', 'children'),
               Output('ndet_vs_time', 'figure'),
               Output('core_hist2d', 'figure'),
               Output('charge_spectrum', 'figure'),
               Output('list_radio_trigs', 'children'),
               Output('run_start_time', 'children'),
               #Output('evt_display', 'figure'),
               ],
               [Input('reload_data', 'children')])
def live_update(n):
    if n=="File selection not Auto. Page not updated.":
        PreventUpdate

    fig1=get_figs.return_det_id_figure(globvars.sta_info_current, 'n_trigs', 'Blues', 'N Trigs per ~5 mins')
    value2=globvars.remember_det_selection
    value3=get_figs.get_last_load_time()
    fig4=get_figs.return_time_figure_2(globvars.arr_info_current)
    fig5=get_figs.return_core_histogram(globvars.arr_info_current)
    fig6=get_figs.return_charge_spectrum(globvars.arr_info_current)
    value7=get_figs.return_radio_trig_text(globvars.arr_info_current,'list')
    value8=get_figs.return_run_start_time(globvars.arr_info_current)
    #fig9=get_figs.return_evt_display_histogram(globvars.arr_info_current)

    return fig1, value2, value3, fig4, fig5, fig6, value7, value8
