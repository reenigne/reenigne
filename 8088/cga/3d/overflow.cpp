// Code to run in interrupt 0 if we get an overflow during the perspective
// calculation. This happens if X or Y is 0x10000 or more times Z, which
// can occur if the point is very far offscreen. We transform such points
// using a different method which gives another point also offscreen and
// in the same direction so that the resulting image looks the same.

// What does CPU do with e.g. 0x0001'0000 / 0x0002 == 0x8000  Overflow triggered because of carry from CORD to POSTIDIV?
// What does CPU do with e.g. 0x0001'0000 / 0xfffe == 0x8000  Either both or neither of these should overflow - it's both: 8088/8086 only allows -0x7fff to 0x7fff as quotient

// 168   CD FG  J L  OPQRSTU       DE    -> tmpa                            11111011?.00   iDIV rmw      tmpa = DX
// 169  BCD FG     M O Q           XA    -> tmpc      1   LRCY  tmpa                                     tmpc = AX    sigma = tmpa << 1
// 16a A CD F   J LMN PQR          M     -> tmpb      7   X0    PREIDIV                                  tmpb = rmw   preidiv()
// 16b ABC  F HI  LMN     TU                          7   UNC   CORD                                     cord()
// 16c ABC  F HI   MNO Q S                            1   COM1  tmpc        11111011?.01                 sigma = -tmpc
// 16d A CD FG  J LMN PQR  U       DE    -> tmpb      7   X0    POSTIDIV                                 tmpb = DX    postidiv()
// 16e    DEF  I  L  OPQRS U       SIGMA -> XA        4   none  NX                                       AX = sigma
// 16f  B DE G I  L  OPQR          tmpa  -> DE        4   none  RNI                                      DX = tmpa
//
// 1b4 ABC  F  I  L  OPQRSTU       SIGMA -> no dest                         100100011.00  PREIDIV
// 1b5 ABC  F HI    NO   STU                          0   NCY      7                                     if ((tmpa & 0x8000) == 0) goto 7
// 1b6 ABC  F HI   MNO QRS                            1   NEG   tmpc                      NEGATE
// 1b7  BCD F  I K MNO Q           SIGMA -> tmpc      1   COM1  tmpa     F                               tmpc = -tmpc sigma = ~tmpa
// 1b8 ABC  F HI     OPQ ST                           0   CY       6        100100011.01                 if ((tmpc & 0x8000) != 0) goto 6
// 1b9 ABC  F HI   MNO QR                             1   NEG   tmpa
// 1ba   CD F  I  L    Q STU       SIGMA -> tmpa      4   CF1   none                                   6:tmpa = -tmpa f1 = !f1
// 1bb ABC  F HI   M O Q  T                           1   LRCY  tmpb                                   7:             sigma = tmpb << 1
// 1bc ABC  F  I   MNO QR T        SIGMA -> no dest   1   NEG   tmpb        100100011.10                              sigma = -tmpb
// 1bd ABC  F HI    NO  R TU                          0   NCY     11
// 1be A CD F  I  L    Q S         SIGMA -> tmpb      4   CF1   RTN
// 1bf ABC  F HI  L  OPQRS                            4   none  RTN
//
// 1c4 ABC  F HI  L NO   STU                          5   NCY   INT0        100100100.00  POSTIDIV
// 1c5 ABC  F HI   M O Q  T                           1   LRCY  tmpb
// 1c6 ABC  F  I   MNO QR          SIGMA -> no dest   1   NEG   tmpa
// 1c7 ABC  F HI    NO   S U                          0   NCY      5
// 1c8   CD F  I  L  OPQRSTU       SIGMA -> tmpa                            100100100.01
// 1c9 ABC  F HI   MNO   S                            1   INC   tmpc
// 1ca ABC  F HI    NO QR                             0   F1       8
// 1cb ABC  F HI   MNO Q S                            1   COM1  tmpc
// 1cc ABC  F HI  L   PQ S                            4   CCOF  RTN         100100100.10
//
// 188 ABC  F HI   M  P R                             1   SUBT  tmpa        100100010.00  CORD
// 189 ABC  F  I KL      STU       SIGMA -> no dest   4   MAXC  none     F
// 18a ABC  F HI  L NO   STU                          5   NCY   INT0
// 18b ABC  F HI   M O Q S                            1   LRCY  tmpc
// 18c  BCD F  I   M O Q           SIGMA -> tmpc      1   LRCY  tmpa        100100010.01
// 18d   CD F  I   M  P R          SIGMA -> tmpa      1   SUBT  tmpa
// 18e ABC  F HI     OPQRS U                          0   CY      13
// 18f ABC  F  I KL  OPQRSTU       SIGMA -> no dest                      F
// 190 ABC  F HI    NO  RST                           0   NCY     14        100100010.10
// 191 ABC  F HI     O    TU                          0   NCZ      3
// 192 ABC  F HI   M O Q S                            1   LRCY  tmpc
// 193  BCD F  I  L  OPQRSTU       SIGMA -> tmpc
// 194 ABC  F  I  L  OPQRS         SIGMA -> no dest   4   none  RTN         100100010.11
// 195 ABC  F HI  L   P  STU                          4   RCY   none
// 196   CD F  I     O    TU       SIGMA -> tmpa      0   NCZ      3
// 197 ABC  F HI    N   R T                           0   UNC     10




// Y/Z <= 0x7fff => Z = Y >> 15  (or 14 if 15 gives artifacts due to result having wrong sign)   x = X/Z = X/(Y >> 15)
if (Z >= 0) {  // => interior/exterior code should treat Z=0 as positive
  if (X >= 0) {
    if (Y >= 0) {
      if (X > Y) {   // 0 <= Y < X
        x = 0x4000;
        y = Y*0x4000/X;
      }
      else {         // 0 <= X <= Y
        y = 0x4000;
        x = X*0x4000/(Y == 0 ? 1 : Y);
      }
    }
    else { // Y < 0
      if (X > -Y) {  // 0 < -Y < X
        x = 0x4000;
        y = Y*0x4000/X;
      }
      else {         // 0 < X <= -Y
        y = -0x4000;
        x = -X*0x4000/Y;
      }
    }
  }
  else {
    if (Y >= 0) {
      if (-X > Y) {  // 0 <= Y < -X
        x = -0x4000;
        y = -Y*0x4000/X;
      }
      else {         // 0 < -X <= Y
        y = 0x4000;
        x = X*0x4000/Y;
      }
    }
    else { // Y < 0
      if (-X > -Y) { // 0 < -Y < -X
        x = -0x4000;
        y = -Y*0x4000/X;
      }
      else {         // 0 < -X <= -Y
        y = -0x4000;
        x = -X*0x4000/Y;
      }
    }
  }
}
else {
  if (X >= 0) {
    if (Y >= 0) {
      if (X > Y) {   // 0 <= Y < X
        x = -0x4000;
        y = -Y*0x4000/X;
      }
      else {         // 0 <= X <= Y
        y = -0x4000;
        x = -X*0x4000/(Y == 0 ? 1 : Y);
      }
    }
    else { // Y < 0
      if (X > -Y) {  // 0 < -Y < X
        x = -0x4000;
        y = -Y*0x4000/X;
      }
      else {         // 0 < X <= -Y
        y = 0x4000;
        x = X*0x4000/Y;
      }
    }
  }
  else {
    if (Y >= 0) {
      if (-X > Y) {  // 0 <= Y < -X
        x = 0x4000;
        y = Y*0x4000/X;
      }
      else {         // 0 < -X <= Y
        y = -0x4000;
        x = -X*0x4000/Y;
      }
    }
    else { // Y < 0
      if (-X > -Y) { // 0 < -Y < -X
        x = 0x4000;
        y = Y*0x4000/X;
      }
      else {         // 0 < -X <= -Y
        y = 0x4000;
        x = X*0x4000/Y;
      }
    }
  }
}
// Need a 48/32->16 divide
//   Suppose we're trying to find Y*0x4000/X (we know X >= Y so the result will be in the range 0..0x4000)
//   Shift Y and X left until 0x4000'0000 <= X <= 0x7fff'ffff and then shift X right by 15  (X is then between 0x8000 and 0xffff inclusive) and Y right by 1

if (X_high > 0) {               // 0x0001'0000 <= X < 0x8000'0000
  if (X_high >= 0x100) {        // 0x0100'0000 <= X < 0x8000'0000
    if (X_high >= 0x1000) {     // 0x1000'0000 <= X < 0x8000'0000
      if (X_high >= 0x2000) {   // 0x2000'0000 <= X < 0x8000'0000
        if (X_high > = 0x4000 { // 0x4000'0000 <= X < 0x8000'0000
          X >>= 15;
          Y >>= 1;
        }
        else                    // 0x2000'0000 <= X < 0x4000'0000
          X >>= 14;
      }
      else {                    // 0x1000'0000 <= X < 0x2000'0000
        X >>= 13;
        Y <<= 1;
      }
    }
    else {                      // 0x0100'0000 <= X < 0x1000'0000
      if (X_high >= 0x400) {    // 0x0400'0000 <= X < 0x1000'0000
        if (X_high >= 0x800) {  // 0x0800'0000 <= X < 0x1000'0000
          X >>= 12;
          Y <<= 2;
        }
        else {                  // 0x0400'0000 <= X < 0x0800'0000
          X >>= 11;
          Y <<= 3;
        }
      }
      else {                    // 0x0100'0000 <= X < 0x0400'0000
        if (X_high >= 0x200) {  // 0x0200'0000 <= X < 0x0400'0000
          X >>= 10;
          Y <<= 4;
        }
        else {                  // 0x0100'0000 <= X < 0x0200'0000
          X >>= 9;
          Y <<= 5;
        }
      }
    }
  }
  else {                        // 0x0001'0000 <= X < 0x0100'0000
    if (X_high >= 0x10) {       // 0x0010'0000 <= X < 0x0100'0000
      if (X_high >= 0x40) {     // 0x0040'0000 <= X < 0x0100'0000
        if (X_high >= 0x80) {   // 0x0080'0000 <= X < 0x0100'0000
          X >>= 8;
          Y <<= 6;
        }
        else {                  // 0x0040'0000 <= X < 0x0080'0000
          X >>= 7;
          Y <<= 7;
        }
      }
      else {                    // 0x0010'0000 <= X < 0x0040'0000
        if (X_high >= 0x20) {   // 0x0020'0000 <= X < 0x0040'0000
          X >>= 6;
          Y <<= 8;
        }
        else {                  // 0x0010'0000 <= X < 0x0020'0000
          X >>= 5;
          Y <<= 9;
        }
      }
    }
    else {                      // 0x0001'0000 <= X < 0x0010'0000
      if (X_high >= 0x4) {      // 0x0004'0000 <= X < 0x0010'0000
        if (X_high >= 0x8) {    // 0x0008'0000 <= X < 0x0010'0000
          X >>= 4;
          Y <<= 10;
        }
        else {                  // 0x0004'0000 <= X < 0x0008'0000
          X >>= 3;
          Y <<= 11;
        }
      }
      else {                    // 0x0001'0000 <= X < 0x0004'0000
        if (X_high >= 0x2) {    // 0x0002'0000 <= X < 0x0004'0000
          X >>= 2;
          Y <<= 12;
        }
        else                    // 0x0001'0000 <= X < 0x0002'0000
          X >>= 1;
          Y <<= 13;
      }
    }
  }
  return Y/((short)X);
}
                                // 0x0000'0000 <= X < 0x0001'0000
if (X_low == 0)
  return 0x4000;
else
  return (Y << 14)/((short)X); // No point shifting both X and Y left - it won't change the result!

