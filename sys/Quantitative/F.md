# Interconnection Networks

## F.1 Introduction
> skip this chapter because alread read it months ago, if have time, then reread it.

Interconnection Network Domains

Approach and Organization of This Appendix

## F.2 Interconnecting Two Devices
#### F6
This includes concepts that
deal with situations in which the **receiver may not be ready** to process incoming
data from the sender and situations in which **transport errors** may occur

 We first describe the basic functions that must be performed at the **end
nodes** to commence and complete communication, and then we discuss network
media and the basic functions that must be performed by the **network** to carry
out communication

*The address and data information is typically referred to as the message payload*

#### F7
Depending on the network domain and design specifications for the network, the network interface hardware varys.

network interfaces can include software or firmware to
perform the needed operations
#### F8
some definition including *maximum transfer unit*, *datagram*, *message ID field*, *communication protocal*, *checksum*

Some network interfaces include extra hardware to offload protocol processing
from the host computer, such as TCP offload engines for LANs and WANs

a very concise description about steps needed to send a message and receive a message at end node devices over a network.
#### F9
The type of media and form factor depends largely on the interconnect
distances over which certain signaling rates (e.g., transmission speed) should be
sustainable

#### F10
explain two layer of network : *packet transport* *flow control*

introduce two flow control method: Xon/Xoff and credit-based

#### F11
Let’s compare the buffering requirements of the two flow control techniques in
a simple example covering the various interconnection network domains.

#### F12
For instance, most
networks delivering packets over relatively short distances (e.g., OCNs and SANs)
tend to implement flow control; on the other hand, networks delivering packets
over relatively long distances (e.g., LANs and WANs) tend to be designed to drop
packets

The communication protocol across the network and network end nodes must
handle many more issues other than packet transport, flow control, and reliability

#### F13
*bandwidth* *effective bandwidth* and *transmission speed*

#### F14
> skip some pages, because we have already learn in computer network

## F.3 Connecting More than Two Devices

#### F20
n this section, we also classify networks into two broad categoriesbased on their connection structure shared-media versus switched-media networks and we compare them

Finally, expanded expressions for characterizing
network performance are given, followed by an example.

#### F21
Additional Network Structure and Functions: Topology, Routing, Arbitration, and Switching

#### F22
define Topology, Routing, Arbitration, and Switching
#### F23
introduce shared-media networks
#### F24
carrier sensing, collision detection, back off
#### F25
> skip the rest of pages, because related already know, if we have time, we can reread it

## Network Topology
#### F30
When the number of required network ports exceeds the number of ports supported by a single switch, a fabric of
interconnected switches is needed

The interconnection structure across all the components—including switches,
links, and end node devices—is referred to as the network topology

researchers struggled to propose
new topologies that could reduce the number of switches through which packets
must traverse, referred to as the hop count
#### F31
> skip following pages because we have taken the examnation about the content, samely, if have time, review this part

## Network Routing, Arbitration, and Switching

#### F45
In this section,
we focus on describing a representative set of approaches used in commercial systems for the more commonly used network topologies.
#### F46
The routing algorithm defines which network path, or paths, are allowed for each
packet.

Paths that have an unbounded number of allowed nonminimal hops from
packet sources, for instance, may result in packets never reaching their destinations. This situation is referred to as *livelock.*

Likewise, paths that cause a set of
packets to block in the network forever waiting only for network resources (i.e.,
links or associated buffers) held by other packets in the set also prevent packets
from reaching their destinations. This situation is referred to as *deadlock*
