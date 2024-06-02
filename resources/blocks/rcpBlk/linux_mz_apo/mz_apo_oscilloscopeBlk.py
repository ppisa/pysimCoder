
from supsisim.RCPblk import RCPblk
from numpy import size

def mz_apo_oscilloscopeBlk(pin, x, y, width, height, x_min_val, x_max_val, y_min_val, y_max_val, strength_min_val, strength_max_val, decay_rate):
    """

    Call:   mz_apo_oscilloscopeBlk(pin, x, y, width, height, x_min_val, x_max_val, y_min_val, y_max_val, strength_min_val, strength_max_val)

    Draws path specified by x and y inputs with intesity specified by strength input.

    Parameters
    ----------
       x: x-coordinate of upper left corner of the oscilloscope window. (0,0) is top left.
       y: y-coordinate of upper left corner of the oscilloscope window. (0,0) is top left.
       width: Width of oscilloscope window.
       height: Height of oscilloscope window.
       x_min_val: Lower bound of allowed range of 'x' input. Values less than that will be clipped to this value.
       x_max_val: Upper bound of allowed range of 'x' input. Values more than that will be clipped to this value.
       y_min_val: Lower bound of allowed range of 'y' input. Values less than that will be clipped to this value.
       y_max_val: Upper bound of allowed range of 'y' input. Values more than that will be clipped to this value.
       strength_min_val: Lower bound of allowed range of 'strength' input. Values less than that will be clipped to this value.
       strength_max_val: Upper bound of allowed range of 'strength' input. Values more than that will be clipped to this value.
       decay_rate: Rate at which old values on screen disappear.

    Returns
    -------
       blk: RCPblk

    """

    blk = RCPblk('mz_apo_oscilloscope', pin, [], [0, 0], 0, [x_min_val, x_max_val, y_min_val, y_max_val, strength_min_val, strength_max_val, decay_rate], [x, y, width, height])
    return blk
