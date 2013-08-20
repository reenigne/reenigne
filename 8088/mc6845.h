class Motorola6845CRTC : public Component
{
public:
    Motorola6845CRTC() : _xcounter(0), _ycounter(0), ma(0), ra(0)
    {
    }
    void simulateCycle()
    {
      ma++;
      ma &= 0x3fff;
      _xcounter++;
      if(_xcounter >= _crtcdata[1] || _ycounter >= _crtcdata[6]) _displayenable = false;
      if(_xcounter >= _crtcdata[2] && _xcounter < (_crtcdata[2] + _crtcdata[3])) _hsync = true;
      else _hsync = false;
      if(_xcounter == (_crtcdata[0] - 1))
      {
          ma = _crtcdata[0x1] * _ycounter;
          ma &= 0x3fff;
          _xcounter = 0;
          ra++;
          ra &= 0x1f;
          _ycounter++;
          if(_ycounter >= _crtcdata[7] && _ycounter < (_crtcdata[7] + 16)) _vsync = true;
          else _vsync = false;
          if(_ycounter == (_crtcdata[4] + 1))
          {
              ma = _crtcdata[0xd] | (_crtcdata[0xc] << 8);
              ma &= 0x1fff;
              ra = 0;
              _ycounter = 0;
          }
      }
    }
    bool _displayenable;
    bool _vsync;
    bool _hsync;
    UInt16 ma;
    UInt8 ra;
    unsigned int _ycounter;
    unsigned int _xcounter;
    UInt8 _charwidth;
    UInt8 _crtcdata[0x10];
};