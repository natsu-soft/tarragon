#!/usr/bin/python

# NOTE:
# for async process
# https://docs.python.org/3/library/subprocess.html#popen-constructor
# for json stuff
# https://docs.python.org/3/library/json.html

# import os
# import subprocess
# import sys
import os
import subprocess
import json
import time

request_id = 1

def encode(json_str):
    # return "Content-Length: {}\r\n\r\n{}".format(len(json_str), json_str)
    temp = "Content-Length: {}\r\n\r\n{}".format(len(json_str), json_str)
    print(temp)
    return temp

def request(method, params=None):
    global request_id
    if params is not None:
        req = encode('{{"jsonrpc":"2.0","id":{},"method":"{}","params":{}}}'.format(request_id, method, params))
    else:
        req = encode('{{"jsonrpc":"2.0","id":{},"method":"{}"}}'.format(request_id, method))
    request_id += 1
    return req

def notice(method, params=None):
    if params is not None:
        notice = encode('{{"jsonrpc":"2.0","method":"{}","params":{}}}'.format(method, params))
    else:
        notice = encode('{{"jsonrpc":"2.0","method":"{}"}}'.format(method))
    return notice

def main():
    # p = subprocess.Popen(["./tarragon", "-h"], stdout=subprocess.PIPE, text=True);
    p = subprocess.Popen(["./tarragon", "-l", "./bruh.log"], stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE, text=True);
    # out, err = p.communicate(request("initialize", '{"processId": 420}'))
    p.stdin.write((request("initialize", '{"processId": 420}')))
    p.stdin.flush()
    time.sleep(0.1)
    # print(p.stdout.read())
    p.stdin.write(notice("initialized", "{}"))
    p.stdin.flush()
    time.sleep(1)
    p.stdin.write(request("shutdown", "null"))
    p.stdin.flush()
    time.sleep(1)
    p.stdin.write(notice("exit"))
    p.stdin.flush()
    time.sleep(1)
    if p.poll():
        print("BRUH: its still schmoovin")

    # p.stdin.flush()
    # print(p.stdout.read()[:-1])
    # print(out)
    # out, err = p.communicate(notice("initialized", "{}"))
    # print(out)
    # out, err = p.communicate(request("shutdown", "null"))
    # print(out)
    # out, err = p.communicate(notice("exit"))
    # print(out)

    # out, err = p.communicate(request("shutdown", 'null'))
    # out, err = p.communicate(notice("exit"))
    # print("{}".format(out.readline()))
    # p.communicate(new_request("shutdown", 'null'))
    # p.communicate(new_request("shutdown", 'null'))
    # p.terminate()
    # print("{}".format(p.stdout.read()))


    # print(subprocess.check_output("./tarragon -v", cwd=os.path.dirname("__file__")))


if __name__=="__main__":
    main()
