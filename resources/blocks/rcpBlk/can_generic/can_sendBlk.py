from supsisim.RCPblk import RCPblk
from numpy import size

def can_sendBlk(pout):
    blk = RCPblk('can_send',[],pout,[0,0],0,[],[])
    return blk
