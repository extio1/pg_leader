import dash
from dash import html
from dash import dcc
from dash import dash_table
from dash.dependencies import Input, Output

import dash_cytoscape as cyto

import time
import threading

import sys
import os

pipe_cluster_state = "../pipe/cluster_state_pipe"
pipe_leader = "../pipe/leader_pipe"
cluster_pipe_lock = threading.Lock()

def remove_items(from_list, item):
    return list(filter((item).__ne__, from_list))

def handle_request(req_str):
    global graph_elements
    global cluster_pipe_lock

    req_list = req_str.split()
    if(req_list[0] == "disconnect"):
        for elements in graph_elements:
            for element in elements:
                if(element['data'] == {'source': str(req_list[1]), 'target': str(req_list[2])}):
                    print("++++++++++++\n", elements)
                    elements.remove(element)
                    print("-----------\n", elements)

def updater_routine():
    global pipe_cluster_state
    try:
        os.mkfifo(pipe_cluster_state)
        print("Named pipe created successfully!")
    except FileExistsError:
        print("Named pipe already exists!")
    except OSError as e:
        print(f"Named pipe creation failed: {e}")

    while True:
        fifo = open(pipe_cluster_state, "r") # block until data comes
        for entry in fifo:
            handle_request(entry)

app = dash.Dash(__name__, update_title=None)

updater = threading.Thread(target=updater_routine)

if(len(sys.argv) < 2):
    print("Please enter num of nodes.")
    exit()
else:
    n_nodes = sys.argv[1]

n_nodes = int(n_nodes)

callback_output=[]
for i in range(0, n_nodes):
    callback_output.append(Output('graph'+str(i), 'elements'))

graph_elements = []
for i in range(0, n_nodes):
    graph=[]
    for j in range(0, n_nodes):
        node = str(j)
        if(j == i):
            node = {
                'data': {'id': node, 'label': node},
                'classes': 'follower current_node'
            }
        else:
            node = {
                'data': {'id': node, 'label': node},
                'classes': 'follower'
            } 
        graph.append(node)

    for j in range(0, n_nodes):
        for k in range(0, n_nodes):
            if (j!=k):
                node1 = str(j)
                node2 = str(k)
                edge = {
                    'data': {'source': node1, 'target': node2},
                    'classes': 'edge_arrow'
                }
                graph.append(edge)

    graph_elements.append(graph)

graph_stylesheet=[
    {
        'selector': 'node',
        'style': {
            'content': 'data(label)',
            'text-halign': 'center',    
            'text-valign': 'center',
        }
    },
    {
        'selector': 'edge',
        'style': {
            'curve-style': 'bezier'
        }
    },
    {
        'selector': '.edge_arrow',
        'style': {
            'target-arrow-color': 'red',
            'target-arrow-shape': 'triangle',
        }
    },
    {
        'selector': '.follower',
        'style': {
            'background-color': '#047e79'
        }
    },
    {
        'selector': '.candidate',
        'style': {
            'background-color': '#f2ce4d'
        }
    },
    {
        'selector': '.leader',
        'style': {
            'background-color': '#f5800d'
        }
    },
    {
        'selector': '.current_node',
        'style': {
            'padding': '10px',
        }
    }
]

follower_color = '#047e79'
candidate_color = '#f2ce4d'
leader_color = '#f5800d'

layout = [
    dcc.Interval(
        id='watch-interval',
        interval=1*1000, # in milliseconds
        n_intervals=0
    ),

    html.Div(
        className="ctl ctl:hover",
        children=[
            dcc.Markdown(
                children=
                '''
                - **start** \[ID|all\]     
                - **stop** \[ID/all\]     
                - **disconnect** ONE ANOTHER \[both\]    
                - **reconnect** ONE ANOTHER \[both\]  
                - **brainsplit** FIRST_GROUP_IDs; SECOND_GROUP_IDs  
                - **brainjoin** FIRST_GROUP_IDs; SECOND_GROUP_IDs   
                - **flush** 
                ''',
            ),
            dcc.Input(
                id="command_input",
                type='text',
                placeholder="write a command",
                style={'height': '70px', 'width': '300px'}
            )
        ],
        style={'width': '305px', 'height': '300px', 'display': 'absolute'}
    )
]

for i in range(0, n_nodes):
    layout.append(
        cyto.Cytoscape(
            stylesheet=graph_stylesheet,
            id='graph'+str(i),
            elements=graph_elements[i],
            layout={'name': 'circle'},
            style={'width': '400px', 'height': '400px'}
        )
    )

app.layout = html.Div(layout)

updater.start()

@app.callback(callback_output, 
              Input('watch-interval', 'n_intervals'),
              background=True,
            manager=background_callback_manager,
)
def update_data(n_intervals):
    global graph_elements



    return graph_elements

if __name__ == '__main__':
    app.run_server(debug=True)
