from supsisim.RCPblk import RCPblk, RcpParam

def mz_apo_oscilloscopeBlk(pin: list[int], params: RcpParam) -> RCPblk:
    """
    Call:   mz_apo_oscilloscopeBlk(pin, params)

    Parameters
    ----------
       pin : connected input ports
       params: block's parameters

    Block parameters
    ----------------
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
      Block's reprezentation RCPblk

    """

    return RCPblk('mz_apo_oscilloscope', pin, [], [0, 0], 0, params)
