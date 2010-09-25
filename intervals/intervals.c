// Waiting for prime
//   all bits high
// Waiting for prime pending
//   parent bit low
//   pending child bit high
//   present child bit low
//   not yet tried child bit high
//   absent child bit high
// Waiting for data request
//   parent bit high
//   present child bit low
//   absent child bit high


// Send prime algorithm:
//   send 0 to child
//   wait
//   send 1 to child (so we can read)
//   read bit from child. If 0
//     set flag for child present
//     send 0 to child

// Receive prime algorithm:
//   wait for 0
//



data[0] = length & 1;
data[1] = (length >> 1) & 1;
data[2] = (length >> 2) & 1;

while (true) {
    // Prime phase
    while (A == 1);       // prime
    uint8_t bits = IIIO;
    set(bits & IIOI);     // primePending->A, prime->B
    wait();
    set(bits);            // primePending->A
    if (B == 0) {         // primePending
        bits &= IIOI;
        gotB = true;
        while (B == 0);   // primePending
    }
    set(bits & IOII);     // primePending->A, prime->C
    wait();
    set(bits);            // primePending->A
    if (C == 0) {         // primePending
        bits &= IOII;
        gotC = true;
        while (C == 0);   // primePending
    }
    set(bits & OIII);     // primePending->A, prime->D
    wait();
    set(bits);            // primePending->A
    if (D == 0) {         // primePending
        bits &= OIII;
        gotD = true;
        while (D == 0);   // primePending
    }
    bits |= OOOI;
    set(bits);            //
    data[3] = parent;
    data[4] = switch;
    data[5] = gotB;
    data[6] = gotC;
    data[7] = gotD;
    afterC = gotD ? doD : doA;
    afterB = gotC ? doC : afterC;
    afterA = gotB ? doB : afterB;

    // Data phase
    while (A == 0);       // dataRequest
    goto afterA;
doB
    bits |= OOIO;
    do {
        set(bits);        // dataRequest/synch->B
        for (int i = 0; i < 8; ++i)
            if (data[i])
                set(bits);      //
            else
                set(bits & IIIO);      // data->A
        set(bits & IIIO); // more->A
        for (int i = 0; i < 8; ++i)
            data[i] = B;
        if (B == 1)       // more
            goto afterB;
        set(bits);        //
        if (A == 1)       // synch
            wait;
    } while (true);
doC
    bits |= OIOO;
    do {
        set(bits);        // dataRequest/synch->C
        for (int i = 0; i < 8; ++i)
            if (data[i])
                set(bits);//
            else
                set(bits & IIIO);  // data->A
        set(bits & IIIO);              // more->A
        for (int i = 0; i < 8; ++i)
            data[i] = C;
        if (C == 1)
            goto afterC;
        set(bits);        //
        if (A == 1)       // synch
            wait;
    } while (true);
doD
    bits |= IOOO;
    do {
        set(bits);        // dataRequest/synch->D
        for (int i = 0; i < 8; ++i)
            if (data[i])
                set(bits);//
            else
                set(bits & IIIO);
        set(bits & IIIO); // more->A
        for (int i = 0; i < 8; ++i)
            data[i] = D;
        if (D == 1)       // more
            goto doA;
        set(bits);        //
        if (A == 1)       // synch
            wait;
    } while (true);
doA
    // bits should now be IIII
    for (int i = 0; i < 8; ++i)
        if (data[i])
            set(bits);    //
        else
            set(bits & IIIO);          // data->A
    set(bits);            //
    gotB = false;
    gotC = false;
    gotD = false;
}
