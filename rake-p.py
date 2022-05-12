"""
Read file line by line
Ignore line starting with # and empty lines

"""
from ctypes import sizeof
import os
import subprocess

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
Execute commands in each actionset
"""
def execute(actionsets):
    setnum = 1
    for actionset in actionsets:
        events.write("ACTIONSET " + str(setnum) +  " ACTIONS:" + '\n')
        setnum += 1
        for action in actionset:
            action = action.split('\t')
            # If it only had one tab space
            if len(action) == 2:
                # If action is for remote host, send to remote host
                if action[1][:6] == "remote":
                    ## Code to send to remote host
                    print("Sending to remote host...")
                
                # Else, execute on localhost
                else:
                    events.write("ACTION: " + '\n' + action[1] + '\n')
                    try:
                        os.system(action[1])
                    except Exception as e:
                        events.write("ERROR: " + '\n' + str(e) + '\n\n')
                    else:
                        out = subprocess.check_output(action[1].split(" "))
                        events.write("OUTPUT: " + '\n' + out.decode('utf-8') + '\n')

            # Else it as 2 tab spaces
            else:
                ## Code to send required files
                print("Sending required files...")

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

def main(file):
    ll = readfile(file)
    print(list2dic(line_split(ll[0])))
    execute(ll[1:])

# Create file to record output and errors of actions
events = create_event_file("event.txt")

main("Raketest")

events.close()