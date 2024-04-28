from supsisim.RCPblk import RCPblk
from numpy import size

def can_recvBlk(pout):
    blk = RCPblk('can_recv', [], pout, [0,0], 1, [], [])
    return blk
