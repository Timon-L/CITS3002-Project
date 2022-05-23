from ctypes import sizeof
from mmap import PROT_READ, PROT_WRITE
import os
import socket
import sys


PORT = []
HOSTS = []


"""
Read file line by line
Ignore line starting with # and empty lines
"""
def readfile(file):
    rake_list = []
    line_list = []

    f = open(file,"r")
    lines = f.readlines()
    for line in lines:
        if(line[0] == "#" or line == "\n"):
            continue
        elif(line[:6] == "action"):
            rake_list.append(line_list)
            line_list = []
        else:
            line_list.append(line.strip("\n"))
    rake_list.append(line_list)
    f.close()
    return rake_list


"""
Split line by spaces.
"""
def line_split(line):
    splited_list = []
    for i in line:
        splited_list.append(i.split(" "))
    return splited_list


"""
Takes splited line and build dictionary
Key value = first string
Ignore "="
"""
def list2dic(list):
    dic = {}
    value_list = []
    dic_list = []

    for i in list:
        for j in i[1:]:
            if(j == "="):
                continue
            else:
                value_list.append(j)
        dic[i[0]] = value_list
        dic_list.append(dic)
        dic = {}
        value_list = []
    return dic_list


"""
Go through actionsets and group actions that require files 
"""
def make_sets(actionsets):
    numsets = len(actionsets)
    a_sets = [ [] for _ in range(numsets) ]
    for i in range(numsets):
        numactions = len(actionsets[i])
        numsend = 0
        for j in range(numactions):
            actionsets[i][j] = actionsets[i][j].split('\t')
            # If it only has one tab space
            if len(actionsets[i][j]) == 2:
                a_sets[i].append([actionsets[i][j][1]])
            # Else it has 2 tabs, so append it to previous action (creating an array)
            else:
                a_sets[i][j-1 - numsend].append(actionsets[i][j][2])
                numsend += 1
    return a_sets


"""
Execute commands in each actionset
"""
def execute(actionsets):
    global PORT
    setnum = 1
    for actionset in actionsets:
        vprint("STARTING ACTIONSET " + str(setnum) +  " ACTIONS:" + '\n')
        events.write("STARTING ACTIONSET " + str(setnum) +  " ACTIONS:" + '\n\n')
        setnum += 1
        for actions in actionset:
            out=""
            # If actions are for remote host, send quote to hosts
            if actions[0][:6] == "remote":
                try:
                    vprint("REQUESTING QUOTES.\n")
                    events.write("REQUESTING QUOTES.\n\n")
                    HOST = cheapest_quote(HOSTS)
                    index = HOST.find(':')
                    if(index != -1):
                        PORT = int(HOST[index + 1:])
                        HOST = HOST[:index]

                    # If there are required files, send them to server
                    if len(actions) == 2:
                        for file in actions[1][9:].split(" "):
                            vprint("SENDING FILES: " + '\n' + file + '\n')
                            events.write("SENDING FILES: " + '\n' + file + '\n\n')
                            send_file(HOST, PORT, file)

                    vprint("REMOTE ACTION: " + '\n' + actions[0][7:] + '\n')
                    events.write("REMOTE ACTION: " + '\n' + actions[0][7:] + '\n\n')
                    out = send_command(HOST, PORT, actions[0][7:].encode())
                except Exception as e:
                    vprint("ERROR REMOTE ACTION: " + '\n' + str(e) + '\n\n')
                    events.write("ERROR REMOTE ACTION: " + '\n' + str(e) + '\n\n\n')
                    sys.exit(1)
                else:
                    vprint("OUTPUT: " + '\n' + out + '\n')
                    events.write("OUTPUT: " + '\n' + out + '\n\n')
            
            # Else, execute on localhost via server
            else:
                try:
                    HOST = "localhost"
                    
                    # If there are required files, send them to server
                    if len(actions) == 2:
                        for file in actions[1][9:].split(" "):
                            vprint("SENDING FILES: " + '\n' + file + '\n')
                            events.write("SENDING FILES: " + '\n' + file + '\n\n')
                            send_file(HOST, PORT, file)
                    
                    vprint("ACTION: " + '\n' + actions[0] + '\n')
                    events.write("ACTION: " + '\n' + actions[0] + '\n\n')
                    out = send_command(HOST, PORT, actions[0].encode())
                except Exception as e:
                    vprint("ERROR LOCAL ACTION: " + '\n' + str(e) + '\n\n')
                    events.write("ERROR LOCAL ACTION: " + '\n' + str(e) + '\n\n\n')
                    sys.exit(1)
                else:
                    vprint("OUTPUT: " + '\n' + out + '\n')
                    events.write("OUTPUT: " + '\n' + out + '\n\n')


"""
Create file to capture events (outputs and errors)
"""
def create_event_file(path):
    filename, extension = os.path.splitext(path)
    counter = 1

    while os.path.exists(path):
        path = filename + str(counter) + extension
        counter += 1
    return open(path, 'w')


"""
Send data to server. Reference: https://stackoverflow.com/questions/1908878/netcat-implementation-in-python
"""   
def send_command(hostname, port, command):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    vprint("OPENING CONNECTION.")
    events.write("OPENING CONNECTION.\n")
    sock.connect((hostname, port))
    sock.sendall(command)
    sock.shutdown(socket.SHUT_WR)
    output = ""
    while 1:
        data = sock.recv(1024)

        if len(data) == 0:
            break
        output += data.decode("utf-8")
        vprint("RECEIVED DATA.")
        events.write("RECEIVED DATA.\n")
    vprint("CLOSING CONNECTION.\n")
    events.write("CLOSING CONNECTION.\n\n")
    sock.close()
    return output


"""
Send file to server.
"""   
def send_file(hostname, port, file):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    vprint("OPENING CONNECTION.")
    events.write("OPENING CONNECTION.\n")
    sock.connect((hostname, port))

    # Open and read file
    f = open(file, "r")
    d = f.read()
    filename = file[file.rfind('/') + 1:]
    sock.send(f"requires,{filename},{d}".encode())

    sock.shutdown(socket.SHUT_WR)
    while 1:
        data = sock.recv(1024)
        
        if len(data) == 0:
            break
        vprint("FILE RECEIVED.")
        events.write("FILE RECEIVED.\n")
    f.close()
    vprint("CLOSING CONNECTION.\n")
    events.write("CLOSING CONNECTION.\n\n")
    sock.close()


"""
Get quote from all servers and return hostname of cheapest server
"""
def cheapest_quote(hostnames):
    global PORT
    quotes = []
    for host in hostnames:
        index = host.find(':')
        if(index != -1):
            PORT = int(host[index + 1:])
            host = host[:index]
        quote = send_command(host, PORT, "REQUEST QUOTE".encode())
        vprint("RECEIVED " + host + ":" + str(PORT) + " QUOTE: " + quote + '\n')
        events.write("RECEIVED " + host + ":" + str(PORT) + " QUOTE: " + quote + '\n\n')
        quotes.append(quote)
    vprint("ACCEPTING: " + host + ":" + str(PORT) + '\n')
    events.write("ACCEPTING: " + host + ":" + str(PORT) + '\n\n')
    return hostnames[quote.index(max(quote))]


"""
Print function to work when verbose is enabled
"""
def vprint(arg):
    lambda *a: None


"""
Main function to start client
"""
def main(argv):
    file = argv[0]
    if "-v" in argv:
        global vprint
        # Make function print
        vprint = print
        file = argv[1]
    ll = readfile(file)
    ph = list2dic(line_split(ll[0]))
    global PORT
    PORT = int(ph[0]["PORT"][0])
    global HOSTS
    HOSTS = ph[1]["HOSTS"]
    actionsets = make_sets(ll[1:])
    execute(actionsets)
    events.close()

# Create file to record output and errors of actions
events = create_event_file("pyevent.txt")
# Command line to use should be: python3 rake-p.py <-v> <file>
main(sys.argv[1:])