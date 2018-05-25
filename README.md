# ntdedupe

Software-based packet deduplication using Napatech hardware acceleration.

*Note: Presently only works with the latest version of napatech's software/firmware package.*

```
dedupe -h 
This application is a forwarding daemon that performs deduplication
on network traffic that is received by the application. This
deduplication utilizes Napatech DYN4 packet descriptors and correlation
keys.

Syntax:

   dedupe <rx_stream> <tx_port> <lru_size>

       rx_stream       : the stream to capture on.
       tx_port         : the port to send traffic from.
       lru_size        : the size of the packet cache.
```


The following command will listen on stream 11, send out port 1, and perform basic deduplication with a 100 frame lru cache.

`dedupe 11 1 100`

## ntservice.ini

The ntservice settings are largely unimportant for this application, however, it is important to set the card's profile setting to *None*. This will greatly improve transmit performance.

## Example NTPL

```
## Delete all previously defined configuration
Delete = All

## Define Descriptor for use with software deduplication. This is necessary because
## Napatech does not make available a field for deduplication out of the box
## This will make the dedupe crc value available in the color1 field of the DYN4
## packet descriptor
Define desc = Descriptor(DYN4, ColorBits=8)

## Define the mask for how the hash will be calculated.
Define ckDefault = CorrelationKey(Begin=Layer3Header[0], End=EndOfFrame[-4], StaticMask=(IPTOSDSCP, IPTOSECN, IPID, IPTTL, IPFLAGS, IPCHK, IPFLOW, IPHOP, IPTC, IPNXTHDR, TCPCHK, UDPCHK))

Assign[StreamId=(11..12); CorrelationKey=ckDefault; Descriptor=desc] = All
# Set Hash Mode to a Symmetric Hash with the fewest fields possible for simplicity
HashMode[priority=0; InnerLayer4Type=UDP, TCP, SCTP] = HashInner5TupleSorted
HashMode[priority=1; InnerLayer3Type=IPV4] = HashInner2TupleSorted
HashMode[priority=2; Layer4Type=UDP, TCP, SCTP] = Hash5TupleSorted
HashMode[priority=3; Layer3Type=IPV4] = Hash2TupleSorted
HashMode[priority=4; Layer3Type=IPV6] = Hash2TupleSorted
HashMode[priority=5]=HashRoundRobin
```
