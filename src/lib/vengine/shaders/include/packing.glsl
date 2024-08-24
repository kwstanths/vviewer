uint pack8bitTo16bit(uint a, uint b)
{
    return (a << 8) | b;
}

void unpack16bitTo8Bit(in uint p, out uint a, out uint b)
{
    b = p & 0xFF;
    a = (p >> 8) & 0xFF;
    return;
}

void unpack16bitTo8Bit(in uint p, out float a, out float b)
{
    uint a_, b_;
    unpack16bitTo8Bit(p, a_, b_);
    a = float(a_) / 255.0;
    b = float(b_) / 255.0;
}


