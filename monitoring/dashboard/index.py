
from dash import html , dcc
from dash.dependencies import Input, Output, State
import dash_bootstrap_components as dbc
import base64

from app import app
from app import server
# import all pages in the app
from apps import daq_dashboard, lora_summary, home


vub_logo = 'logos/vub_logo.jpg'
encoded_vub_logo = base64.b64encode(open(vub_logo, 'rb').read())

navbar = dbc.NavbarSimple(
    children=[
        dbc.Row([
            dbc.Col(html.Img(src='data:image/png;base64,{}'.format(encoded_vub_logo.decode()),
            style={'width': '30%','vertical-align':'middle'})),
            dbc.Col(dbc.NavbarBrand("LOFAR CosmicRay Dashboard", style={'vertical-align':'middle','font-size':'200%'})),
            dbc.Col(
        dbc.DropdownMenu(
            children=[
                dbc.DropdownMenuItem("More Pages", header=True, style={'vertical-align':'middle','font-size':'200%'}),
                dbc.DropdownMenuItem("Home", href="/home", style={'vertical-align':'middle','font-size':'200%'}),
                dbc.DropdownMenuItem("DAQ", href="/daq_dashboard", style={'vertical-align':'middle','font-size':'200%'}),
                dbc.DropdownMenuItem("LORA Summary", href="/lora_summary", style={'vertical-align':'middle','font-size':'200%'}),
            ],
            nav=True,
            in_navbar=True,
            label="More Pages",
            style={'vertical-align':'middle','font-size':'200%'}
        )),
        ]),
    ],
    # brand="LOFAR CosmicRay DashBoard",
    # brand_href="#",
    color="light",
    light=True,
)

# embedding the navigation bar
app.layout = html.Div([
    dcc.Location(id='url', refresh=False),
    navbar,
    html.Div(id='page-content')
])


@app.callback(Output('page-content', 'children'),
              [Input('url', 'pathname')])
def display_page(pathname):
    if pathname == '/daq_dashboard':
        return daq_dashboard.layout
    elif pathname == '/lora_summary':
        return lora_summary.layout
    elif pathname == '/home':
        return home.layout
    else:
        return home.layout

# if __name__ == '__main__':
    # app.run_server(host='127.0.0.1', debug=True)
    # app.run_server(debug=True,port=8010,threaded=True)
