#!/usr/bin/python

import subprocess, sys, os
import multiprocessing,time
import time
import os
import glob

def intro():
    print '      ************************'
    print '      *** MUON STAND DAQ *****'
    print '      ************************'
    
def read_n_events():
    default = 10000
    try :
      n_evts = int(raw_input('How many ms? (default = %d , -1 for continuous): ' %default))
      return n_evts

    except ValueError :
        return default
        
def continue_with_next_command(command) :
    print ' '
    choice = raw_input('**** I will do: %s. CONTINUE? [Y/n] ****: ' % command)
    if choice in ('Y','y','') :
        print 'Executing ' + command
        return
    else:
        print 'Exiting'
        sys.exit(0)
    ##time.sleep(2)
    
def get_boards():
    f = open('data_params.txt')
    boards = []
    cnt = 0;
    for line in f:
        if(cnt != 0):
            boards.append(line)
        cnt += 1
        
    return boards

def ask_output_flag(output_flag):   
    print ' '
    choice = raw_input('output flag? default = %s ' %output_flag)
    if choice :
        output_flag = choice

    return output_flag


def execute_receive_one_from_db(board,events):
    #subprocess.call("sudo ./receive_one %s %d &" % (str(board),int(events)), shell=True)
    #things aren't looking good im sry, subprocess usually works
    time.sleep(0.01)
    os.system("sudo ./receive_one %s %d &" % (str(board),int(events)))
    
def main():
    intro()

    output_flag =  time.strftime("%Y-%d-%m-%H-%M")
    output_flag = ask_output_flag(output_flag)
    print 'output will be ' + output_flag

    events = read_n_events() ##uncomment to enable user entry for run duration
    ##events = 21600000
    boards = []
    jobs=[]
    commands = ['setup_daq', 'trigger_init.sh', 'receive_one', 'select', 'convert_data_to_root','viewer','analyze_data']

    for command in commands:

        continue_with_next_command(command)
        
        if command == 'setup_daq'  : 
            subprocess.call(['sudo','./%s' % command])
           ## os.system("sudo ./%s" % command)
            boards = get_boards()

        elif command == 'trigger_init.sh'  : 
            subprocess.call(['sudo','./%s' % command])
            ##os.system("sudo ./%s" % command)
            
        elif command == 'receive_one': 
            for board in boards:
                execute_receive_one_from_db(board[0:4],events)
                
                #seriously what is wrong with multiprocessing?
                #p = multiprocessing.Process(target=execute_receive_one_from_db, args=(board[0:4],events))
                #jobs.append(p)
            #for i in jobs:
            #    i.start()
                        
        else:
            subprocess.call(['./%s' % command])
            ##os.system('./%s' % command) 

            
    print 'End of %d packages' % events


    # create output directory
    dirname = "output."  + output_flag
    fulldirname = "output/"  + dirname
    print ' dirname ' + fulldirname
    if not os.path.isdir(fulldirname):
        os.mkdir(fulldirname)
    fulldatadirname = fulldirname + "/data"
    if not os.path.isdir(fulldatadirname):
        os.mkdir(fulldatadirname)

    # save output files
    rootfiles = glob.glob("coincidence_data_*.root")
    for x in rootfiles:
        newname = fulldatadirname + "/" + output_flag + "-" + x
        print ' rename x %s newname %s ' %(x, newname)
        os.rename(x, newname)

    
    pictures = glob.glob("events/*jpg")
    picdirname = fulldirname + "/events"
    if not os.path.isdir(picdirname):
        os.mkdir(picdirname)
    for x in pictures:
        newname = fulldirname + "/" + x
        print ' rename x %s newname %s ' %(x, newname)
        os.rename(x, newname)

    collection = ['data.root',  'result.root', 'data_params.txt', 'data_limits.txt']##removed detector.eps from list 
    for x in collection:
        newname = fulldirname + "/" + output_flag + "-" + x
        print ' rename x %s newname %s ' %(x, newname)
        os.rename(x, newname)
    newname = fulldirname + "/" + output_flag + "-detector.inp"
    os.system('cp '+ ' detector.inp ' + newname )


    print 'done, output is in %s ' % fulldirname


if __name__ == '__main__' :
    main()
