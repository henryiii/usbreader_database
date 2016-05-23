#!/usr/bin/env python

from plumbum import local, cli, FG
from plumbum.cmd import sudo

newboard = local['./newboard.py']
clear_tables = local['./clear_tables.pl']
class timedboard(cli.Application):
   
    total_time = cli.SwitchAttr(["--time","-t"], float, default=10000)
    ntimes = cli.SwitchAttr(["--num","-n"], int, default=1)
    
    def main(self):
        for i in range(self.ntimes):
            sudo[newboard]['-t', self.total_time] & FG
            clear_tables()
            
if __name__ == '__main__':
    timedboard.run()
                    
