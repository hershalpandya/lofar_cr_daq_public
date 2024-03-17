
from dash import dash_table, html
import pandas as pd

df_2022 = pd.read_csv('apps/RadioTrigMonthlySummary_2022.csv')
df_2021 = pd.read_csv('apps/RadioTrigMonthlySummary_2021.csv')
df_2020 = pd.read_csv('apps/RadioTrigMonthlySummary_2020.csv')


layout = html.Div([
            html.Div([
                dash_table.DataTable(df_2022.to_dict('records'), [{"name": i, "id": i} for i in df_2022.columns],
                                    style_table={'overflowX': 'scroll','padding': 10},
                                    style_header={'backgroundColor': '#25597f', 'color': 'white'},
                                    style_cell={
                                        'backgroundColor': 'white',
                                        'color': 'black',
                                        'fontSize': 13,
                                        'font-family': 'Nunito Sans'})], 
            style={'width': '70%'}
            ),
    
            html.Hr(),
            html.Div([
                dash_table.DataTable(df_2021.to_dict('records'), [{"name": i, "id": i} for i in df_2021.columns],
                                     style_table={'overflowX': 'scroll','padding': 10},
                                     style_header={'backgroundColor': '#25597f', 'color': 'white'},
                                    style_cell={
                                        'backgroundColor': 'white',
                                        'color': 'black',
                                        'fontSize': 13,
                                        'font-family': 'Nunito Sans'})],
            style={'width': '70%'}
            ),
    html.Hr(),
    html.Div([
                dash_table.DataTable(df_2020.to_dict('records'), [{"name": i, "id": i} for i in df_2020.columns],
                                     style_table={'overflowX': 'scroll','padding': 10},
                                     style_header={'backgroundColor': '#25597f', 'color': 'white'},
                                    style_cell={
                                        'backgroundColor': 'white',
                                        'color': 'black',
                                        'fontSize': 13,
                                        'font-family': 'Nunito Sans'})],
            style={'width': '70%'}
            ),
])
