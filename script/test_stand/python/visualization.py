import dash
from dash import html
from dash import dcc
from dash import dash_table
from dash.dependencies import Input, Output

import dash_cytoscape as cyto

import time
import threading

import sys
import fcntl
import os
import copy

pipe_cluster_state = "../pipe/cluster_state_pipe"
pipe_leader = "../pipe/leader_pipe"
cluster_pipe_lock = threading.Lock()
init_graph_state=[]
graph_elements = []

def remove_items(from_list, item):
    return list(filter((item).__ne__, from_list))

leader_id=-1
def handle_request(req_str):
    global graph_elements 
    global init_graph_state
    req_list = req_str.split()

    print(req_list)
    if(req_list[0] == "disconnect"):
        for elements in graph_elements:
            for element in elements:
                if(element['data'] == {'source': str(req_list[1]), 'target': str(req_list[2])}):
                    elements.remove(element)
    elif(req_list[0] == "reconnect"):
        edge = {
                    'data': {'source': str(req_list[1]), 'target': str(req_list[2])},
                    'classes': 'edge_arrow'
        }
        add=False
        try:
            pos = graph_elements[0].index(edge)
        except ValueError:
            add=True

        for i in range(0, n_nodes):
            pos = init_graph_state[i].index(edge)
            graph_elements[i].append(init_graph_state[i][pos])
    elif(req_list[0] == "flush"):
        graph_elements = copy.deepcopy(init_graph_state)
    elif(req_list[0].find("leader") != -1):
        current_graph = int(req_list[1])
        aim_node_id = int(req_list[2])

        if(aim_node_id != -1):
            for i in range(0, n_nodes):
                classes_value=''
                if(i == aim_node_id):
                    classes_value = "leader"
                    leader_id = i
                else: 
                    classes_value = "follower"

                if(current_graph == i):
                    classes_value+=" current_node"

                graph_elements[current_graph][i]['classes'] = classes_value
        else:
            if(leader_id != -1):
                graph_elements[current_graph][leader_id]['classes'] = "follower"



def updater_routine():
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
        interval=1000, # in milliseconds
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

init_graph_state = copy.deepcopy(graph_elements)

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

app.layout = html.Div(layout, style={'display': 'flex', 'flex-wrap': 'wrap'})

try:
    os.mkfifo(pipe_cluster_state)
    print("Named pipe created successfully!")
except FileExistsError:
    print("Named pipe already exists!")
except OSError as e:
    print(f"Named pipe creation failed: {e}")

f = open(pipe_cluster_state, "r", encoding='utf-8', errors='ignore')
fd = f.fileno()
flag = fcntl.fcntl(fd, fcntl.F_GETFL)
fcntl.fcntl(fd, fcntl.F_SETFL, flag | os.O_NONBLOCK)

@app.callback(callback_output, Input('watch-interval', 'n_intervals'))
def update_data(n_intervals):
    global graph_elements
    global f

    while True:
        try:
            request = f.readline(100)
            if(len(request) > 0):
                handle_request(request)
            else:
                break
        except OSError:
            print("OSError")

    return graph_elements

if __name__ == '__main__':
    app.run_server(debug=True)
