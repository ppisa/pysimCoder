
from supsisim.RCPblk import RCPblk
from numpy import size

def mz_apo_LEDBlk(pin, offset, len, min_val, max_val):
    """

    Call:   mz_apo_LEDBlk(pin, offset, len, min_val, max_val)

    Progressbar-like showing of input value.

    Parameters
    ----------
       offset: Index of first LED for which this block is repsonsible. [-32, 32]
       len: Number of LEDs for which this block is repsonsible. Negative value will be interpreted as abs(len) length, but progress grows from right instead. [-32, 32]
       min_val: Value of input corresponding to 0 lit LED's.
       max_val: Value of input corresponding to 'len' LED's lit.

    Returns
    -------
       blk: RCPblk

    """

    blk = RCPblk('mz_apo_LED', pin, [], [0, 0], 0, [min_val, max_val], [offset, len])
    return blk
