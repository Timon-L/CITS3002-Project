#  CITS3002 Project 2022
#  Name(s):             Joe Lao , Ching Chun Liu
#  Student number(s):   22982055 , 22660829

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
    # For each actionset
    for i in range(numsets):
        numactions = len(actionsets[i])
        numsend = 0
        # For each action in the actionset
        for j in range(numactions):
            actionsets[i][j] = actionsets[i][j].split('\t')
            # If action only has one tab space, it is a command, add it to its actionset's array
            if len(actionsets[i][j]) == 2:
                a_sets[i].append([actionsets[i][j][1]])
            # Else it has 2 tabs, so it is a file, append it to previous action (creating an array)
            else:
                a_sets[i][j-1 - numsend].append(actionsets[i][j][2])
                numsend += 1

    # Results in an array of actionsets, in each actionset array, there are actions, if the action requires files,
    # then instead of an action in the actionset array, it is an array containing the action and the required files
    # eg. [[actionset1 action1, [actionset1 action2, required files for action2]], [actionset2 action1]]
    return a_sets


"""
Execute commands in each actionset
"""
def execute(actionsets):
    global PORT
    setnum = 1
    # For each actionset
    for actionset in actionsets:
        vprint("STARTING ACTIONSET " + str(setnum) +  " ACTIONS:" + '\n')
        events.write("STARTING ACTIONSET " + str(setnum) +  " ACTIONS:" + '\n\n')

        setnum += 1
        # For each action in the actionset
        for actions in actionset:
            out=""
            # If actions are for remote host, request quotes from hosts
            if actions[0][:6] == "remote":
                try:
                    vprint("REQUESTING QUOTES.\n")
                    events.write("REQUESTING QUOTES.\n\n")

                    HOST = cheapest_quote(HOSTS)
                    # If host is in the form HOST:PORT, assign HOST and PORT values separately
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
                # If no errors occured, record output
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
                # If no errors occured, record output
                else:
                    vprint("OUTPUT: " + '\n' + out + '\n')
                    events.write("OUTPUT: " + '\n' + out + '\n\n')


"""
Create file to capture events (outputs and errors). Reference: https://stackoverflow.com/questions/13852700/create-file-but-if-name-exists-add-number
"""
def create_event_file(path):
    filename, extension = os.path.splitext(path)
    counter = 1

    # If the file exists, create a file with a number attached that isn't used
    while os.path.exists(path):
        path = filename + str(counter) + extension
        counter += 1
    return open(path, 'w')


"""
Send command to server. Reference: https://stackoverflow.com/questions/1908878/netcat-implementation-in-python
"""   
def send_command(hostname, port, command):
    # Create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    vprint("OPENING CONNECTION.")
    events.write("OPENING CONNECTION.\n")

    # Connect to server
    sock.connect((hostname, port))
    # Send server command
    sock.sendall(command)
    #sock.shutdown(socket.SHUT_WR)
    
    vprint("RECEIVING DATA:")
    events.write("RECEIVING DATA:\n")

    # If server is sending file, ask for filename
    output = sock.recv(1024).decode("utf-8")
    if(output == "SENDING FILE."):
        vprint("COMMAND RESULTED IN FILE CREATED.")
        events.write("COMMAND RESULTED IN FILE CREATED.\n")

        vprint("REQUESTING FILENAME.")
        events.write("REQUESTING FILENAME.\n")
        sock.send("SEND FILENAME.".encode())

        # Create file of filename received
        output = sock.recv(1024).decode("utf-8")
        print(output)
        f = open(output, "w")
        vprint("RECEIVED FILENAME.")
        events.write("RECEIVED FILENAME.\n")
        # Tell server we received filename
        sock.send("RECEIVED FILENAME.".encode())

        # Server will then send data for file
        vprint("RECEIVING FILE DATA:")
        events.write("RECEIVING FILE DATA.\n")
        while 1:
            data = sock.recv(1024).decode("utf-8")
            
            # If server sends "END OF FILE" there is no more data to be sent
            if data == "END OF FILE":
                break
            f.write(data)
            vprint(data)
            events.write(data + "\n")

    # Else get output of server (server sends output of command)
    else:
        while 1:
            data = sock.recv(1024)

            if len(data) == 0:
                break
            data = data.decode("utf-8")
            output += data
            vprint(data)
            events.write(data + "\n")
    
    # Close connection
    vprint("CLOSING CONNECTION.\n")
    events.write("CLOSING CONNECTION.\n\n")
    sock.close()
    return output


"""
Send file to server.
"""   
def send_file(hostname, port, file):
    # Create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    vprint("OPENING CONNECTION.")
    events.write("OPENING CONNECTION.\n")

    # Connect to server
    sock.connect((hostname, port))
    output = ""

    # Open file
    f = open(file, "r")

    # If filename had path through folders, just get the file's name without folder path
    # eg. for /project/code/helloworld.c, just get helloworld.c
    filename = file[file.rfind('/') + 1:]

    # Tell server we are going to sent a file
    sock.send("SENDING FILE".encode())

    # If server tells us to send filename, send it
    output = sock.recv(1024).decode("utf-8")
    if output == "SEND FILENAME.":
        vprint("SENDING FILENAME.")
        events.write("SENDING FILENAME.\n")
        sock.send(filename.encode())

    # If server tells us it received the filename, send the file's data
    output = sock.recv(1024).decode("utf-8")
    if output == "RECEIVED FILENAME.":
        vprint("FILENAME RECEIVED.")
        events.write("FILENAME RECEIVED.\n")

        vprint("SENDING FILE DATA.")
        events.write("SENDING FILE DATA.\n")
        
        # Send lines from file to server
        while 1:
            d = f.readline()
            if not d:
                break
            sock.send(d.encode())
        # No more lines to send, tell server to stop listening for data
        sock.send("END OF FILE".encode())
    sock.shutdown(socket.SHUT_WR)
    
    # If server tells us it received the data, close file and connection
    output = sock.recv(1024).decode("utf-8")
    if output == "RECEIVED FILE DATA.":
        vprint("FILE DATA RECEIVED.")
        events.write("FILE DATA RECEIVED.\n")
        
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
    minimum_quote = 1;
    try:
        # For each host
        for host in hostnames:
            # If host is in the form HOST:PORT, assign the values separately
            index = host.find(':')
            if(index != -1):
                PORT = int(host[index + 1:])
                host = host[:index]
            
            # Ask server for a quote, get the value they send back
            quote = send_command(host, PORT, "REQUEST QUOTE".encode())
            vprint("RECEIVED " + host + ":" + str(PORT) + " QUOTE: " + quote + '\n')
            events.write("RECEIVED " + host + ":" + str(PORT) + " QUOTE: " + quote + '\n\n')
            
            # If quote is the minimum quote we can get, accept the host, no need to quote other servers
            if quote == minimum_quote:
                vprint("ACCEPTING: " + host + ":" + str(PORT) + '\n')
                events.write("ACCEPTING: " + host + ":" + str(PORT) + '\n\n')
                return host + ":" + str(PORT)
                
            # Add quote to list of quotes
            quotes.append(quote)
    except Exception as e:
        vprint("ERROR GETTING QUOTES: " + '\n' + str(e) + '\n\n')
        events.write("ERROR GETTING QUOTES: " + '\n' + str(e) + '\n\n\n')
     
    # Return host with lowest quote
    cheapest = hostnames[quotes.index(min(quotes))]
    vprint("ACCEPTING: " + cheapest + '\n')
    events.write("ACCEPTING: " + cheapest + '\n\n')
    return cheapest


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
    # If verbose option is enables, make verbose print active
    if "-v" in argv:
        global vprint
        # Make function print
        vprint = print
        file = argv[1]
    try:
        # Read file in
        ll = readfile(file)
        # Store port and hosts in dictionaries
        ph = list2dic(line_split(ll[0]))
        # Assign port the read port value
        global PORT
        PORT = int(ph[0]["PORT"][0])
        # Assign values to array of hosts
        global HOSTS
        HOSTS = ph[1]["HOSTS"]
        # Sort actionsets for efficient execution later
        actionsets = make_sets(ll[1:])
    except Exception as e:
        vprint("ERROR ASSIGNING RAKEFILE VALUES: " + '\n' + str(e) + '\n\n')
        events.write("ERROR ASSIGNING RAKEFILE VALUES: " + '\n' + str(e) + '\n\n\n')
        sys.exit(1)
    # Execute actions in actionset
    execute(actionsets)
    # Close file the recorded events
    events.close()

# Create file to record output and errors of actions
events = create_event_file("pyevent.txt")

# Command line to use should be: python3 rake-p.py <-v> <file>
main(sys.argv[1:])