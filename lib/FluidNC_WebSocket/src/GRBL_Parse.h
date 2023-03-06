void _convertPos(bool isM, SafeString& s) {
  int x_idx, y_idx, z_idx;
  float x, y, z;
  
  cSF(sx,8);
  x_idx = s.indexOf(',');
  s.substring(sx, 0, x_idx);
//  DEBUG_SERIAL.println(sx); 
  sx.toFloat(x);
  s.remove(0, x_idx+1);

  cSF(sy,8);
  y_idx = s.indexOf(',', x_idx);
  s.substring(sy, 0, y_idx);
//  DEBUG_SERIAL.println(sy); 
  sy.toFloat(y);
  s.remove(0, y_idx+1);

  cSF(sz,8);
  z_idx = s.indexOf('|');
  if (z_idx < 0) {z_idx = s.indexOf('>');}
  s.substring(sz, 0, z_idx);
  sz.toFloat(z);
  if (isM) {
    _mX = x;
    _mY = y;
    _mZ = z;
  } else {
    _wX = x;
    _wY = y;
    _wZ = z;
  }
}

void _convertOverride(SafeString& s) {
  int feed_idx, spd_idx, rpd_idx;
  int feed, spd, rpd;

  cSF(s_feed,8);
  feed_idx = s.indexOf(',');
  s.substring(s_feed, 0, feed_idx);
  s_feed.toInt(feed);
  s.remove(0, feed_idx + 1);

  cSF(s_rpd,8);
  rpd_idx = s.indexOf(',');
  s.substring(s_rpd, 0, rpd_idx);
  s_rpd.toInt(rpd);
  s.remove(0, rpd_idx + 1);

  cSF(s_spd,8);
  spd_idx = s.indexOf('|');
  if (spd_idx < 0) {
    spd_idx = s.indexOf('>');
  }
  s.substring(s_spd, 0, spd_idx);
  s_spd.toInt(spd);

  _ovRapid = rpd;
  _ovFeed = feed;
  _ovSpeed = spd;
}

void _getGrblState(bool full, const void *mem, uint32_t len)
{
    //DEBUG_SERIAL.printf("[_FluidNC_WS::_getGrblState] Reply length: %d\n", len );
 
    uint32_t max_len = len;
    if (max_len>MAX_LEN-1) max_len=MAX_LEN-1;
    createSafeString(grblState, max_len);
    get_payload(mem, len, grblState);

    //DEBUG_SERIAL.println(grblState);
    //DEBUG_SERIAL.printf("charAt(1): %c\n", grblState.charAt(1));

    if (grblState.charAt(1) == 'J') _mState = Jog;
        else if (grblState.charAt(3) == 'm') _mState = Home;
        else if (grblState.charAt(4) == 'd') _mState = Hold;
        else if (grblState.charAt(1) == 'D') _mState = Door;
        else if (grblState.charAt(1) == 'R') _mState = Run;
        else if (grblState.charAt(1) == 'A') _mState = Alarm;
        else if (grblState.charAt(1) == 'I') _mState = Idle;
        else _mState = Unknwn;

        if (full) {
          if (grblState.indexOf("MPos:") >= 0) {
            grblState.remove(0, grblState.indexOf("MPos:") + 5);
            _convertPos(true, grblState);
          }
          if (grblState.indexOf("|FS:") >= 0) {
            grblState.remove(0, grblState.indexOf("|FS:") + 4);
            grblState.remove(0, grblState.indexOf(",") + 1);
            grblState.toInt(_reportedSpindleSpeed);
          }
          if (grblState.indexOf("WCO:") >= 0) {
            grblState.remove(0, grblState.indexOf("WCO:") + 4);
            _convertPos(false, grblState);
          }
          else if (grblState.indexOf("|Ov:") >= 0) {
            grblState.remove(0, grblState.indexOf("|Ov:") + 4);
            _convertOverride(grblState);
            if (grblState.indexOf("|A:") >= 0) {
              grblState.remove(0, grblState.indexOf("|A:") + 3);
              //            Serial.println(grblState);
              if (grblState.indexOf("S") >= 0 && grblState.indexOf("S") < 3) _spindleOn = true;
              else _spindleOn = false;
            }
            else _spindleOn = false;
          }
        }  
    //DEBUG_SERIAL.printf("[_FluidNC_WS::_getGrblState] State: %d\n", _mState);
}
