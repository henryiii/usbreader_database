#!/usr/bin/env python

from __future__ import print_function

from plumbum import local, cli, FG, BG
from plumbum import colors
import time
from contextlib import nested
import sys

local.env.path.insert(0, '.')

from plumbum.cmd import sudo

def wrap_command(command):
    def func():
        #if raw_input(colors.green | "About to run {}, press enter:".format(command)) == '':
        print(colors.green | "About to run {}".format(command))
        time.sleep(.5)
        command & FG
        #else:
        #    raise RuntimeError("Operation canceled by user")
    return func

select = local['./select']
setup_daq = wrap_command(sudo[local['./setup_daq']])
trigger_init = wrap_command(local['./trigger_init.sh'])
convert_data_to_root = wrap_command(local['./convert_data_to_root'])
viewer = wrap_command(local['./viewer'])
analyze_data = wrap_command(local['./analyze_data'])

def get_boards():
    with open('data_params.txt') as f:
        boards = f.readlines()
    return reversed(sorted(boards[1:]))

def execute_receive_one_from_db(board, events, tout = .03):
    receive_one = sudo[local['./receive_one']]
    command = receive_one[board, events]

    time.sleep(tout)
    print('Running', command, 'in background.')

    return command & BG(stderr=sys.stderr, stdout=sys.stdout)

class Board(cli.Application):
    '''This is documentation.'''

    time_ = cli.SwitchAttr(['t','time'], float, default=10000,
           help='Time in ms')
    output_flag = cli.SwitchAttr(['o','output_flag'],
                                 str,
                                 default=time.strftime("%Y-%d-%m-%H-%M"),
                                 help='type in "-o string" without quotes after ./newboard.py to rename output file' )

    def main(self):

        events = self.time_
        output_flag = self.output_flag

        setup_daq()
        boards = get_boards()

        # Run receiving in the background
        tout = .03
        outputs = [execute_receive_one_from_db(board[0:4], events, tout) for board in boards)

        # Run select in foreground
        select & FG

        # Make sure receiving is finalized
        for out in outputs:
            out.join()

        convert_data_to_root()
        viewer()
        analyze_data()

        fulldirname = local.cwd / 'output' / ('output.' + output_flag)
        if not fulldirname.exists():
            fulldirname.mkdir()

        fulldatadirname = fulldirname / "data"
        if not fulldatadirname.exists():
             fulldatadirname.mkdir()

        rootfiles = local.cwd // "coincidence_data_*.root"
        for f in rootfiles:
            newname = fulldatadirname / (output_flag + "-" + f.name)
            print(' rename f', f, 'newname', newname)
            f.move(newname)

        pictures = local.cwd / 'events' // '*jpg'
        picdirname = fulldirname / 'events'

        if not picdirname.exists():
            picdirname.mkdir()

        for x in pictures:
            newname = fulldirname / x.name
            print(' rename x', x, 'newname', newname)
            x.move(newname)

        collection = ['data.root',  'result.root', 'data_params.txt', 'data_limits.txt']
        for a in collection:
            newname = fulldirname / (output_flag + '-' + a)
            print(' rename a', a, 'newname', newname)
            (local.cwd / a).move(newname)

        (local.cwd / 'detector.inp').copy(fulldirname / (output_flag+'-detector.inp'))

        print('done, output is in', fulldirname)

if __name__ == '__main__':
    Board.run()
