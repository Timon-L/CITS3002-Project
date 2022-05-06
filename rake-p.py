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

def line_split(line):
    splited_list = []
    for i in line:
        splited_list.append(i.split(" "))
    return splited_list

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


def main(file):
    ll = readfile(file)
    print(list2dic(line_split(ll[0])))

main("Raketest")