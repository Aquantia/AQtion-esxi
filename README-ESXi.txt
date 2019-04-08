
Aquantia's AQtion PCIe(Atlantic) VmWare ESXi driver
===============================================

This is a VmWare vmklinux layer adptation of standard linux.

ESXi versions supported: 6.0 and higher

Binary installation
--------------------
Preparation:
 - change acceptance level: esxcli software acceptance set -level=CommunitySupported
Installation:
  - esxcli software vib install -f -v <full_path_to_vib>/<vib_name>
    e.g esxcli software vib install -f -v /tmp/net670-atlantic-v2.2.3.0-esxi1.4.x86_x64.vib
    e.g esxcli software vib install -f -v $(pwd)/net670-atlantic-v2.2.3.0-esxi1.4.x86_x64.vib
  - reboot

Binary removal
--------------------
  - esxcli software vib remove -f -n net-atlantic
  - reboot

Download latest VIB binary from release area:
https://github.com/Aquantia/AQtion-esxi/releases

Install as a standalone VIB or integrate offline bundle into your ESXi image.

Source build
--------------------
Please use `esxi/build-atlantic.sh` to rebuild the driver.
Notice you have to download and install VmWare Open Source disclosure and toolchain packages

Command line tools
--------------------
 - `esxcfg-nics -l` - show status
    Name    PCI          Driver      Link Speed      Duplex MAC Address       MTU    Description
    vmnic0  0000:01:00.0 atlantic    Up   1000Mbps   Full   xx:xx:xx:xx:xx:xx 1500   Aquantia Corp. AQC107 NBase-T/IEEE 802.3bz Ethernet Controller [AQtion]

 - `ethtool -i vmnicX` - show driver information
    driver: atlantic
    version: 2.2.3.0-esxi1.4
    firmware-version: x.x.x
    bus-info: 0000:01:00.0

 - `ethtool vmnicX` - show settings
    Settings for vmnicX:
         Supported ports: [ TP ]
         Supported link modes:   100baseT/Full
                                 1000baseT/Full
         Supports auto-negotiation: Yes
         Advertised link modes:  100baseT/Full
                                 1000baseT/Full
         Advertised auto-negotiation: Yes
         Speed: 1000Mb/s
         Duplex: Full
         Port: Twisted Pair
         PHYAD: 0
         Transceiver: external
         Auto-negotiation: on
         Supports Wake-on: g
         Wake-on: g
         Current message level: 0x00000005 (5)
         Link detected: yes

  - `ethtool -S vmnicX` - show statistics
     NIC statistics:
        InPackets: 14933
        InUCast: 61
        InMCast: 9404
        InBCast: 5468
        InErrors: 0
        OutPackets: 0
        OutUCast: 0
        OutMCast: 0
        OutBCast: 0
        InUCastOctets: 5106
        OutUCastOctets: 0
        InMCastOctets: 1253572
        OutMCastOctets: 0
        InBCastOctets: 654283
        OutBCastOctets: 0
        InOctets: 1912961
        OutOctets: 0
        InPacketsDma: 14933
        OutPacketsDma: 0
        InOctetsDma: 1853223
        OutOctetsDma: 0
        InDroppedDma: 0
        Queue[0] InPackets: 5405
        Queue[0] OutPackets: 0
        Queue[0] Restarts: 0
        Queue[0] InJumboPackets: 0
        Queue[0] InLroPackets: 0
        Queue[0] InErrors: 0
        Queue[1] InPackets: 3379
        Queue[1] OutPackets: 0
        Queue[1] Restarts: 0
        Queue[1] InJumboPackets: 0
        Queue[1] InLroPackets: 0
        Queue[1] InErrors: 0
        Queue[2] InPackets: 3294
        Queue[2] OutPackets: 0
        Queue[2] Restarts: 0
        Queue[2] InJumboPackets: 0
        Queue[2] InLroPackets: 0
        Queue[2] InErrors: 0
        Queue[3] InPackets: 2855
        Queue[3] OutPackets: 0
        Queue[3] Restarts: 0
        Queue[3] InJumboPackets: 0
        Queue[3] InLroPackets: 0
        Queue[3] InErrors: 0   

  - `ethtool -k vmnicX` - get offload parameters
     Offload parameters for vmnic1:
     rx-checksumming: on
     tx-checksumming: on
     scatter-gather: on
     tcp segmentation offload: on
     udp fragmentation offload: off
     generic segmentation offload: off

  - `ethtool -K vmnicX <offload> on/off` - enable/disable offload
     e.g: `ethtool -K vmnic0 rx off` - disable rx checksumming
     Note: only RX checksumming on/off is supported
  - `ethtool -s vmnicX speed/autoneg/wol` - set/change speed/autoneg/wake-on-lan
     e.g: `ethtool -s vmnic0 autoneg off` - disable autonegotiation 
