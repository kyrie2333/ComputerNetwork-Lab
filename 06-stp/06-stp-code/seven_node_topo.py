#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.cli import CLI

def clearIP(n):
    for iface in n.intfList():
        n.cmd('ifconfig %s 0.0.0.0' % (iface))

class RingTopo(Topo):
    def build(self):
        b = [0] * 8
        for i in range(1, 8):
            b[i] = self.addHost('b%d'%i)

        edge = [ (1,2), (2,3), (3,4), (4,5), (5,6), (6,7),\
                 (7,1), (1,4), (1,5), (2,7), (2,5), (4,7) ]
        
        for (i,j) in edge:
            self.addLink(b[i], b[j])

if __name__ == '__main__':
    topo = RingTopo()
    net = Mininet(topo = topo, controller = None) 

    nports = [4, 4, 2, 4, 4, 2, 4]

    for idx in range(len(nports)):
        name = 'b' + str(idx+1)
        node = net.get(name)
        clearIP(node)
        node.cmd('./disable_offloading.sh')
        node.cmd('./disable_ipv6.sh')

        # set mac address for each interface
        for port in range(nports[idx]):
            intf = '%s-eth%d' % (name, port)
            mac = '00:00:00:00:0%d:0%d' % (idx+1, port+1)

            node.setMAC(mac, intf = intf)

        node.cmd('./stp > %s-output.txt 2>&1 &' % name)

    net.start()
    CLI(net)
    net.stop()
