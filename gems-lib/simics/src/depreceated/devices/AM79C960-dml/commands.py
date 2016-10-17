from cli import *
from string import *
import nic_common
import sim_commands


def checkbit(a, bit):
    if a & (1 << bit):
        return 1
    else:
        return 0

def get_info(obj):
    return [ (None,
              [ ("IRQ device", obj.irq_dev),
                ("IRQ number", obj.irq_level) ]),
             ] # + nic_common.get_nic_info(obj)

def get_status(obj):
    csr0 = obj.csr_csr0
    csr0a = "INIT=%d STRT=%d STOP=%d TDMD=%d TXON=%d RXON=%d INEA=%d INTR=%d" % (
        checkbit(csr0, 0), checkbit(csr0, 1), checkbit(csr0, 2), checkbit(csr0, 3),
        checkbit(csr0, 4), checkbit(csr0, 5), checkbit(csr0, 6), checkbit(csr0, 7))
    csr0b = "IDON=%d TINT=%d RINT=%d MERR=%d MISS=%d CERR=%d BABL=%d ERR=%d" % (
        checkbit(csr0, 8), checkbit(csr0, 9), checkbit(csr0, 10), checkbit(csr0, 11),
        checkbit(csr0, 12), checkbit(csr0, 13), checkbit(csr0, 14), checkbit(csr0, 15))
    #    log = obj.log_addr
    #    log_addr = "%x.%x.%x.%x.%x.%x.%x.%x" % (
    #        log[0], log[1], log[2], log[3], log[4], log[5], log[6], log[7])
    #    if obj.connected == 1:
    #        connected = "yes"
    #    else:
    #        connected = "no"
    return ([ (None,
               [ ("CSR0", csr0a),
                 (None,  csr0b),
                 ("CSR1", "0x%x" % obj.csr_csr1),
                 ("CSR2", "0x%x" % obj.csr_csr2),
                 ("CSR3", "BCON=%d ACON=%d BSWP=%d" % (
        (checkbit(obj.csr_csr3, 0), checkbit(obj.csr_csr3, 1), checkbit(obj.csr_csr3, 2)))),
                 ("CSR15", "0x%x" % obj.csr_csr15),
                 ("RAP", obj.ioreg_rap) ]),
              #              (None,
              #               [("Packets sent", obj.pkt_snt),
              #                ("Packets received", obj.pkt_rec)])
              ] + nic_common.get_nic_status(obj))


nic_common.new_nic_commands('AM79C960-dml')

sim_commands.new_info_command('AM79C960-dml', get_info)
sim_commands.new_status_command('AM79C960-dml', get_status)
