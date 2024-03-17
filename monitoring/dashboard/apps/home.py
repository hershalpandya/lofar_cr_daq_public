from dash import dcc, html, Input, Output, callback
import base64


inv_sand = 'logos/inverted_Sandwich.jpg' # replace with your own image
encoded_inv_sand = base64.b64encode(open(inv_sand, 'rb').read())

layout = html.Div([
    html.H3('Welcome to LOFAR-CR Dashboard'),
    html.Hr(),
    html.Img(src='data:image/png;base64,{}'.format(encoded_inv_sand.decode()),
            style={'width': '20%','vertical-align':'middle','display': 'inline-block'}),
    html.H4("Your lack of experience with html shows.\n I appreciate your efforts,\n but stop bragging about websites that aren't even functional."),
    html.H5("- Baba Stijn")

])
