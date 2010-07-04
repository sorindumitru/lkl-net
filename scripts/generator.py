#!/bin/bash

import sys;

port = 51001
switchport = 55000

class Switch:
        def __init__(self, name, interfaces, x, y):
                self.hostname = name;
                self.interfaces = interfaces;
                self.x = x;
                self.y = y;
                self.neigh = []

        def getType(self):
                return "switch";

        def addNeighbour(self, neigh):
                self.neigh.append(neigh);

        def generateMac(self, index):
                mac = "00:00:";
                mac += self.hostname[6:8];
                mac += ":0"+str(index)+":00:22";
                return mac;

        def write(self, file, folder):
                global switchport;
                switch = open(folder +"/" +self.hostname, "w");
                file.write('\tdevice {\n\t\ttype switch;\n\t\thostname ');
                file.write(self.hostname+';\n');
                file.write('\t\tconfig '+folder+"/"+self.hostname+ ';\n');
                file.write('\t\tposx '+str(self.x)+';\n\t\tposy '+str(self.y)+';\n');
                file.write('\t}\n');
                switch.write('hostname '+self.hostname+';\n');
                switch.write('ipaddress 127.0.0.1;\n');
                switch.write('port '+str(switchport) + ';\n');
                for i in range(self.interfaces):
                        switch.write('interface {\n');
                        switch.write('\tdev eth'+str(i)+';\n');
                        switch.write('\tipaddress 0.0.0.0/32;\n');
                        switch.write('\tmac '+self.generateMac(i)+';\n');
                        switch.write('\tgatewway 127.0.0.1;\n');
                        switch.write('\tport '+str(self.neigh[i].getPort())+';\n');
                        switch.write('\tlink '+self.neigh[i].getName()+';\n');
                        switch.write('\ttype hub;\n');
                        if len(self.neigh) != self.interfaces:
                                print "alfa";
                        switch.write('}\n\n');

                switch.write('switch {\n');
                for i in range(self.interfaces):
                        switch.write('\tinterface eth'+str(i)+';\n');
                switch.write('}\n');
                switchport += 1;
                switch.close();

        def __str__(self):
                return self.hostname + " " + str(self.interfaces);

class Hub:
        def __init__(self, name, x, y):
                global port;
                self.hostname = name;
                self.x = x;
                self.y = y;
                self.port = port;
                port += 1;

        def getType(self):
                if self.hostname != "":
                        return "hub";
                else:
                        return "null";

        def getPort(self):
                return self.port;

        def getName(self):
                return self.hostname;

        def addNeighbour(self, a):
                i = 0;

        def write(self, file, folder):
                if self.hostname == "":
                        return;
                file.write('\tdevice {\n\t\ttype hub;\n\t\thostname ');
                file.write(self.hostname+';\n');
                file.write('\t\tport '+str(self.port) + ';\n');
                file.write('\t\tposx '+str(self.x)+';\n\t\tposy '+str(self.y)+';\n');
                file.write('\t}\n');

        def __str__(self):
                return self.hostname;

class Topology:
        def __init__(self, rows, columns):
                self.rows = int(rows);
                self.columns = int(columns);
                self.topology = [];

        def nrInterfaces(self, i, j):
                nr = 0;
                if j == 0 or j == self.columns*2-2:
                        nr += 1;
                else:
                        nr += 2;
                if i == 0 or i == self.rows - 1:
                        nr += 1;
                else:
                        nr += 2;

                if self.rows == 1:
                        nr -= 1;
                if self.columns == 1:
                        nr -= 1;

                return nr;

        def generate(self):
                nr = 0;
                for i in range(self.rows):
                        row = [];
                        for j in range(self.columns*2 - 1):
                                nr+=1;
                                if j % 2 == 0:
                                        row.append(Switch("Switch"+str(i)+str(j), self.nrInterfaces(i,j), (i+1)*100, (j+1)*100));
                                else:
                                        row.append(Hub("Hub"+str(i)+str(j), (i+1)*100, (j+1)*100));
                        self.topology.append(row);
                        if self.rows > 1 and i != self.rows -1  :
                                row = []
                                for j in range(self.columns*2 - 1):
                                        nr += 1;
                                        if j % 2 == 0:
                                                row.append(Hub("Hub"+str(i)+str(j), (i+1)*100, (j+1)*100));
                                        else:
                                                row.append(Hub("", (i+1)*100, (j+1)*100));
                                self.topology.append(row);

                # Add neighbours
                i = 0;
                for row in self.topology:
                        j = 0;
                        for dev in row:
                                if dev.getType() == "hub":
                                        if j > 0:
                                                row[j-1].addNeighbour(dev);
                                        if j < self.columns*2 - 2:
                                                row[j+1].addNeighbour(dev);
                                        if i > 0 :
                                                self.topology[i-1][j].addNeighbour(dev);
                                        print i, j;
                                        if i < self.rows*2 - 2:
                                                self.topology[i+1][j].addNeighbour(dev);

                                j += 1;
                        i += 1;


        def show(self):
                for i in range(len(self.topology)):
                        for j in range(len(self.topology[i])):
                                print self.topology[i][j],
                        print "";

        def write(self, folder):
                hyper = open(folder+"/hypervisor", "w");
                hyper.write('hypervisor {\n');
                for row in self.topology:
                        for dev in row:
                                dev.write(hyper, folder);
                hyper.write('}\n');
                hyper.close();

if __name__ == "__main__":
        topology = Topology(sys.argv[1], sys.argv[2]);
        topology.generate();
        topology.write(sys.argv[3]);

